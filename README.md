# Cool Room 101 — Room Temperature Monitor (Dual: ThingSpeak + Supabase)

ESP32 + SHT30 -> kirim ke ThingSpeak (MQTT) DAN Supabase (HTTPS) sekaligus, keduanya
tiap 15 detik -> Buzzer & Email (alert ke tim) -> Dashboard (Vercel, baca dari ThingSpeak)

## Urutan setup

### 1. ThingSpeak
Channel ID, MQTT credentials, Write API Key, dan Read API Key semua sudah terisi di kode.
Tidak ada yang perlu diambil lagi dari sisi ThingSpeak.

### 2. Supabase (jalur kedua, paralel)
1. Kalau belum pernah, buka Supabase SQL Editor, jalankan isi `supabase_schema.sql`
   (aman dijalankan ulang — pakai `if not exists`, tidak akan error kalau tabel sudah ada)
2. URL dan anon key sudah terisi di kode ESP32

### 3. ESP32
1. **Extract dulu (Extract All)** zip ini ke folder biasa sebelum dibuka
2. Install library **"PubSubClient"** dan **"ESP Mail Client" by Mobizt** lewat Library Manager
3. Buka folder `esp32_final_stack/`, klik `esp32_final_stack.ino` — semua kredensial sudah terisi
4. Upload ke ESP32, buka Serial Monitor (115200) — akan muncul dua baris konfirmasi kirim
   tiap 15 detik: `[MQTT] Tersimpan` dan `[SUPABASE] Tersimpan`

### 4. Dashboard
Sudah terisi lengkap (Channel ID + Read API Key ThingSpeak). Upload folder `dashboard/`
ke GitHub → import ke Vercel → Deploy, atau drag ke vercel.com/drop.

## Kecepatan data
Semua jalur dibuat sama: **15 detik** — ini batas tercepat paket gratis ThingSpeak
(1 pesan/15 detik), jadi Supabase disamakan biar konsisten meskipun Supabase sendiri
sebenarnya tidak punya limit seketat itu. Dashboard membaca dari ThingSpeak tiap 5 detik
(baca boleh lebih sering dari kirim, tidak kena limit yang sama).

## Cara kerja alert
- Buzzer nyala di suhu >= 33.0°C, mati di <= 32.3°C (hysteresis, anti-flapping)
- Email HANYA terkirim saat status berubah (naik ke alert, atau turun lagi ke normal)
- Data tetap terkirim ke ThingSpeak DAN Supabase tiap 15 detik terlepas dari status alert
