// ============================================================
//  ESP32 — STACK FINAL (versi kantor)
//  Sensor   : SHT30 (I2C, SDA=21 SCL=22)
//  Simpan   : Supabase (HTTPS POST langsung, tiap 20 detik)
//  Alert    : Buzzer GPIO26, hysteresis 33.0/32.3°C
//  Notif    : Email Gmail ke beberapa orang, HANYA saat status berubah
//
//  Library tambahan yang perlu diinstall (Library Manager):
//  "ESP Mail Client" by Mobizt
// ============================================================

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>

// ── WiFi ─────────────────────────────────────────────────────
const char* WIFI_SSID     = "Minimal Mandi";
const char* WIFI_PASSWORD = "112233445566";

// ── Supabase ─────────────────────────────────────────────────
const char* SUPABASE_URL      = "https://vfpjclrtmtzoajsxeefe.supabase.co";
const char* SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZmcGpjbHJ0bXR6b2Fqc3hlZWZlIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODY4OTQwNzAsImV4cCI6MjEwMjQ3MDA3MH0.19kW_emlzbCGmt0MIiEKvAsVPytB6WmfJahYSUCSTGg";

// ── Gmail (pengirim) ─────────────────────────────────────────
// WAJIB pakai App Password 16-digit, BUKAN password Gmail biasa.
// Cara buat: myaccount.google.com/apppasswords (aktifkan 2-Step
// Verification dulu kalau belum).
#define SMTP_HOST      "smtp.gmail.com"
#define SMTP_PORT      465
const char* SENDER_EMAIL    = "pengirim@gmail.com";
const char* SENDER_APP_PASS = "rmprpgmouqjsjxix";

// ── Penerima notifikasi (bisa lebih dari satu) ───────────────
const char* RECIPIENTS[] = {
  "orang1@email.com",
  "orang2@email.com",
  "orang3@email.com"
};
const int RECIPIENT_COUNT = 3;

// ── Pin ──────────────────────────────────────────────────────
#define SDA_PIN        21
#define SCL_PIN        22
#define BUZZER_PIN     26

// ── SHT30 I2C ────────────────────────────────────────────────
#define SHT30_ADDR     0x44

// ── Threshold dengan hysteresis ──────────────────────────────
#define TEMP_ON        33.0
#define TEMP_OFF       32.3

// ── Interval ─────────────────────────────────────────────────
#define READ_INTERVAL     3000
#define SUPABASE_INTERVAL 20000

// ── Auto-kalibrasi ───────────────────────────────────────────
#define CALIB_SAMPLES  10
float tempOffset = 0.0;
float humiOffset = 0.0;

unsigned long lastReadTime     = 0;
unsigned long lastSupabaseTime = 0;
bool buzzerActive = false;

SMTPSession smtp;

// ============================================================
//  I2C / SHT30
// ============================================================
bool sht30SendCommand(uint16_t cmd) {
  Wire.beginTransmission(SHT30_ADDR);
  Wire.write((cmd >> 8) & 0xFF);
  Wire.write(cmd & 0xFF);
  return (Wire.endTransmission() == 0);
}

bool verifyCRC(uint8_t msb, uint8_t lsb, uint8_t crcByte) {
  uint8_t crc = 0xFF;
  uint8_t buf[2] = { msb, lsb };
  for (int i = 0; i < 2; i++) {
    crc ^= buf[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
  }
  return crc == crcByte;
}

bool sht30Read(float &temp, float &humi) {
  if (!sht30SendCommand(0x2400)) return false;
  delay(20);

  Wire.requestFrom((uint8_t)SHT30_ADDR, (uint8_t)6);
  if (Wire.available() < 6) return false;

  uint8_t d[6];
  for (int i = 0; i < 6; i++) d[i] = Wire.read();

  if (!verifyCRC(d[0], d[1], d[2]) || !verifyCRC(d[3], d[4], d[5])) {
    Serial.println("[WARN] CRC mismatch — baca ulang...");
    return false;
  }

  uint16_t rawT = ((uint16_t)d[0] << 8) | d[1];
  uint16_t rawH = ((uint16_t)d[3] << 8) | d[4];

  temp = -45.0 + 175.0 * ((float)rawT / 65535.0);
  humi = 100.0  * ((float)rawH / 65535.0);
  return true;
}

// ============================================================
//  AUTO-KALIBRASI
// ============================================================
void autoCalibrate() {
  Serial.println("[CALIB] Memulai auto-kalibrasi...");
  float sumT = 0, sumH = 0;
  int valid = 0;

  for (int i = 0; i < CALIB_SAMPLES; i++) {
    float t, h;
    delay(500);
    if (sht30Read(t, h)) {
      sumT += t; sumH += h; valid++;
      Serial.print("  Sampel " + String(i + 1) + ": ");
      Serial.print(t, 2); Serial.print(" C  |  ");
      Serial.print(h, 1); Serial.println(" %");
    }
  }

  if (valid == 0) {
    tempOffset = 0.0; humiOffset = 0.0;
    Serial.println("[CALIB] Semua sampel gagal.");
    return;
  }

  tempOffset = 0.0;
  humiOffset = 0.0;
  Serial.println("[CALIB] Selesai ✓  (offset 0.00 — tanpa referensi eksternal)");
  Serial.println("────────────────────────────────────────");
}

// ============================================================
//  WIFI
// ============================================================
void connectWiFi() {
  Serial.print("[WIFI] Menghubungkan ke " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Terhubung ✓  IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WIFI] Gagal konek.");
  }
}

// ============================================================
//  SUPABASE — simpan data untuk histori & dashboard
// ============================================================
void sendToSupabase(float temp, float humi) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SUPABASE] WiFi tidak terhubung, skip.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/readings";
  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY));
  http.addHeader("Content-Type", "application/json");

  char payload[100];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.2f,\"humidity\":%.1f}", temp, humi);

  int httpCode = http.POST(payload);

  if (httpCode == 201) {
    Serial.println("[SUPABASE] Tersimpan ✓  " + String(temp,2) + "C, " + String(humi,1) + "%");
  } else {
    Serial.println("[SUPABASE] Gagal simpan. HTTP code: " + String(httpCode));
  }
  http.end();
}

// ============================================================
//  EMAIL (Gmail) — kirim HANYA saat status berubah, ke semua penerima
// ============================================================
void smtpCallback(SMTP_Status status) {
  Serial.println("[EMAIL] " + status.info());
}

void sendEmailAlert(const String &subject, const String &body) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[EMAIL] WiFi tidak terhubung, skip.");
    return;
  }

  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = SENDER_EMAIL;
  config.login.password = SENDER_APP_PASS;
  config.login.user_domain = "";
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 7;
  config.time.day_light_offset = 0;

  SMTP_Message message;
  message.sender.name = "Sensor Suhu Ruangan";
  message.sender.email = SENDER_EMAIL;
  message.subject = subject;

  for (int i = 0; i < RECIPIENT_COUNT; i++) {
    message.addRecipient("", RECIPIENTS[i]);
  }

  message.text.content = body.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  smtp.callback(smtpCallback);

  if (!smtp.connect(&config)) {
    Serial.println("[EMAIL] Gagal konek ke server SMTP.");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("[EMAIL] Gagal kirim: " + smtp.errorReason());
  } else {
    Serial.println("[EMAIL] Notifikasi terkirim ke " + String(RECIPIENT_COUNT) + " orang ✓");
  }

  smtp.closeSession();
}

// ============================================================
//  BUZZER + NOTIFIKASI — hysteresis, edge-triggered
// ============================================================
void handleBuzzer(float temp) {
  if (!buzzerActive && temp >= TEMP_ON) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerActive = true;
    Serial.println("[BUZZ] AKTIF — Suhu " + String(temp, 1) + " °C");

    sendEmailAlert(
      "[ALERT] Suhu Ruangan Tinggi",
      "Suhu ruangan saat ini: " + String(temp, 1) + " C\n"
      "Batas alert: " + String(TEMP_ON, 1) + " C\n"
      "Buzzer aktif di lokasi. Mohon dicek.\n\n"
      "Dashboard: (isi link dashboard Vercel Anda di sini)"
    );
  }
  else if (buzzerActive && temp <= TEMP_OFF) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    Serial.println("[BUZZ] BERHENTI — Suhu " + String(temp, 1) + " °C");

    sendEmailAlert(
      "[NORMAL] Suhu Ruangan Sudah Kembali Aman",
      "Suhu ruangan saat ini: " + String(temp, 1) + " C\n"
      "Kondisi sudah kembali normal."
    );
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== ESP32 SHT30 — Stack Final (Kantor) ===");
  Serial.println("Supabase (histori) + Buzzer (lokal) + Email (alert grup)");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Serial.println("[INIT] I2C: SDA=21, SCL=22");
  Serial.println("[INIT] Buzzer di GPIO" + String(BUZZER_PIN));

  Wire.beginTransmission(SHT30_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("[INIT] SHT30 ditemukan ✓");
  } else {
    Serial.println("[ERROR] SHT30 tidak ditemukan! Cek kabel.");
  }

  sht30SendCommand(0x30A2);
  delay(100);

  autoCalibrate();
  connectWiFi();

  Serial.println("[CFG]  Nyala buzzer >= " + String(TEMP_ON, 1) + " C");
  Serial.println("[CFG]  Mati buzzer  <= " + String(TEMP_OFF, 1) + " C");
  Serial.println("[CFG]  Penerima email: " + String(RECIPIENT_COUNT) + " orang");
  Serial.println("[INFO] System ready.\n");

  digitalWrite(BUZZER_PIN, HIGH); delay(80);
  digitalWrite(BUZZER_PIN, LOW);  delay(80);
  digitalWrite(BUZZER_PIN, HIGH); delay(160);
  digitalWrite(BUZZER_PIN, LOW);
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  unsigned long now = millis();

  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    float rawTemp = 0, rawHumi = 0;
    if (!sht30Read(rawTemp, rawHumi)) {
      Serial.println("[ERROR] Gagal baca SHT30.");
      return;
    }

    float temp = rawTemp + tempOffset;
    float humi = constrain(rawHumi + humiOffset, 0.0, 100.0);

    Serial.print("[READ] Suhu: ");
    Serial.print(temp, 2);
    Serial.print(" C  |  RH: ");
    Serial.print(humi, 1);
    Serial.println(buzzerActive ? " %  |  ALERT" : " %  |  NORMAL");

    handleBuzzer(temp);

    if (now - lastSupabaseTime >= SUPABASE_INTERVAL) {
      lastSupabaseTime = now;
      sendToSupabase(temp, humi);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Terputus. Reconnect...");
    connectWiFi();
  }
}
