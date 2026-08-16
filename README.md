# Stack Final — Room Temperature Monitor (Versi Kantor)

ESP32 + SHT30 -> Supabase (data & histori) + Buzzer & Email (alert instan ke grup) -> Dashboard (Vercel, siap pakai tanpa setup untuk yang cuma lihat)

## Urutan setup

### 1. Supabase (tempat data disimpan)
1. Buat project baru di supabase.com (gratis)
2. Buka SQL Editor, jalankan isi `supabase_schema.sql`
3. Catat dari Project Settings > API: **Project URL** dan **anon public key**

### 2. Gmail (pengirim notifikasi)
1. Aktifkan **2-Step Verification** di akun Gmail yang mau dipakai kirim (myaccount.google.com/security)
2. Buka myaccount.google.com/apppasswords
3. Bikin App Password baru (App: Mail, Device: Other — kasih nama misal "Sensor Suhu")
4. Copy password 16 digit yang muncul — **ini yang dipakai di kode, BUKAN password Gmail biasa**
5. Siapkan daftar email semua orang kantor yang perlu menerima alert

### 3. ESP32
1. Install library **"ESP Mail Client" by Mobizt** lewat Library Manager Arduino IDE
2. Buka `esp32_final_stack.ino`
3. Isi semua bagian `ISI_...`, `NAMA_WIFI_KAMU`, `pengirim@gmail.com`, dan daftar `RECIPIENTS[]`
4. Upload ke ESP32, buka Serial Monitor (115200) untuk cek jalan tidaknya

### 4. Dashboard (untuk semua orang kantor)
1. Buka `dashboard/index.html`, cari bagian `KONFIGURASI SUPABASE` di dalam `<script>`
2. Ganti `supabaseUrl` dan `anonKey` dengan punya Anda (sekali saja, sebelum deploy)
3. Upload folder `dashboard/` ke GitHub
4. Import ke Vercel -> Deploy
5. Bagikan link Vercel-nya ke semua orang kantor — mereka tinggal buka, tidak perlu setup apa pun

## Cara kerja alert
- Buzzer nyala di suhu >= 33.0°C, mati di <= 32.3°C (hysteresis, anti-flapping)
- Email alert HANYA terkirim saat status berubah (naik ke alert, atau turun lagi ke normal) — bukan tiap baca sensor
- Email terkirim ke semua alamat di `RECIPIENTS[]` sekaligus
- Data tetap terkirim ke Supabase tiap 20 detik terlepas dari status alert, untuk histori dashboard

## Catatan keamanan
Anon key Supabase yang tertulis di dashboard memang didesain untuk terlihat publik di sisi
client — ini bukan celah, karena batas keamanan sesungguhnya ada di **Row Level Security (RLS)**
yang sudah diatur di `supabase_schema.sql` (anon cuma boleh insert & select tabel `readings`,
tidak bisa update/delete, dan tidak bisa akses tabel lain). Ini adalah model keamanan standar
Supabase, sama seperti API key Google Maps yang juga terlihat di banyak website publik.
