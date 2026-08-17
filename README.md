# Cool Room 101 — Cold Room Monitor (ThingSpeak + Supabase + Email)

ESP32 + SHT30 -> kirim ke ThingSpeak (MQTT) DAN Supabase (HTTPS) sekaligus, tiap 15 detik
-> Buzzer & Email (alert ke tim, edge-triggered) -> Dashboard (Vercel, dua grafik terpisah + interaktif)

## Range operasi
Didesain untuk **cold room 21–25°C**:
- **Normal**: di bawah 23°C
- **Waspada**: 23–25°C
- **Alert**: di atas 25°C — buzzer nyala, email terkirim, dashboard update instan

Kalau range ruangan Anda beda, ubah `T_WARN`/`T_DANGER` di `dashboard/index.html` dan
`TEMP_ON`/`TEMP_OFF` di `esp32_final_stack.ino` — pastikan dua-duanya tetap sinkron.

## Urutan setup

### 1. ThingSpeak
Channel ID, MQTT credentials, Write API Key, dan Read API Key semua sudah terisi di kode.

### 2. Supabase (jalur kedua, paralel)
Kalau tabel `readings` belum pernah dibuat, jalankan `supabase_schema.sql` sekali di SQL Editor
(aman dijalankan ulang, pakai `if not exists`).

### 3. ESP32
1. **Extract dulu (Extract All)** zip ini ke folder biasa sebelum dibuka
2. Install library **"PubSubClient"** dan **"ESP Mail Client" by Mobizt**
3. Buka folder `esp32_final_stack/`, klik `esp32_final_stack.ino` — semua kredensial sudah terisi
4. Upload, buka Serial Monitor (115200)

### 4. Dashboard
Sudah terisi lengkap. Upload folder `dashboard/` ke GitHub (Vercel auto-deploy), atau drag ke vercel.com/drop.

## Cara kerja alert (edge-triggered, anti-spam)
Email TIDAK dikirim berdasarkan waktu — hanya saat status BERUBAH:
- Suhu naik lewat 25°C (Normal → Alert): 1 email terkirim
- Suhu masih di atas 25°C terus-menerus: tidak ada email tambahan
- Suhu turun ke bawah 24.3°C (Alert → Normal): 1 email "sudah normal" terkirim

Hysteresis (25.0 naik / 24.3 turun, bukan angka yang sama) mencegah bolak-balik kirim kalau
suhu "goyang" persis di titik ambang.

**Sinkronisasi instan**: begitu status berubah, ESP32 langsung kirim data ke ThingSpeak +
Supabase saat itu juga (tidak nunggu siklus 15 detik) — supaya titik data yang memicu alert
selalu tercatat di dashboard, bukan cuma di email.

## Fitur Dashboard
- **Dua grafik terpisah** (Suhu & Kelembaban) dengan skala masing-masing — hover titik mana pun
  untuk lihat nilai presisi + jam persis
- **Lampu status** hijau/kuning/merah di kartu Status Ruangan
- **Riwayat**: tombol "Tampilkan Lebih Banyak" (buka data lebih lama), "Segarkan Tampilan"
  (reset ke tampilan awal), "Unduh CSV" (ekspor langsung dari ThingSpeak)
- Hapus data permanen **sengaja tidak disediakan di dashboard** (demi keamanan) — lakukan
  langsung di ThingSpeak: Channel Settings → Clear Channel

## Catatan keamanan
Anon key Supabase & Read Key ThingSpeak yang ada di kode dashboard memang didesain untuk
publik — batas keamanannya ada di Row Level Security (Supabase) dan sifat read-only key
(ThingSpeak), bukan di menyembunyikan angkanya.
