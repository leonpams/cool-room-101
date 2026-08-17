# Cool Room 101 — Cold Room Monitor (ThingSpeak + Supabase + Email)

ESP32 + SHT30 -> kirim ke ThingSpeak (MQTT) DAN Supabase (HTTPS) sekaligus, tiap 15 detik
-> Buzzer & Email (alert ke tim, edge-triggered) -> Dashboard (Vercel, dua grafik terpisah + interaktif)

## Range operasi
Didesain untuk **cold room 21–25°C**:
- **Normal**: di bawah 24.5°C
- **Waspada**: 24.5–26°C
- **Alert**: 26°C ke atas — buzzer nyala, email terkirim, dashboard update instan

Kalau range ruangan Anda beda, ubah `T_WARN`/`T_DANGER` di `dashboard/index.html` dan
`TEMP_ON`/`TEMP_OFF` di `esp32_final_stack.ino` — pastikan dua-duanya tetap sinkron.

## Logic fitur dashboard (biar tidak rancu)

**Kartu Notifikasi** — menunjukkan JAM notifikasi terakhir terkirim (dideteksi dari
titik data pertama yang menyentuh ambang alert di histori yang sudah dimuat),
BUKAN histori lengkap atau tujuan/penerima — sengaja tanpa info penerima demi privasi.

**Kartu Status Ruangan** — cuma penanda (lampu + teks), sengaja tanpa grafik.

**Tombol "Kelola Data di ThingSpeak"** — link ke halaman channel ThingSpeak Anda
(perlu login sendiri di sana). Sengaja TIDAK dibuat otomatis menghapus dari
dashboard — proses hapus channel (Clear Channel) butuh User API Key (kunci
level akun, bukan cuma satu channel), yang terlalu berbahaya untuk ditanam di
dashboard publik yang bakal diakses banyak orang di beberapa lokasi.

Tidak ada tombol refresh manual — dashboard sudah otomatis tarik data tiap 15
detik (persis sama dengan kecepatan data baru dibuat, jadi tidak ada delay
berarti untuk ditunggu manual).

## Kecepatan data — sekarang dipisah per platform

- **ESP32 → ThingSpeak**: tiap **15 detik** (batas keras paket gratis, tidak bisa lebih cepat)
- **ESP32 → Supabase**: tiap **1 detik** (Supabase tidak punya batas rate seperti ThingSpeak,
  jadi bisa jauh lebih detail)
- **Dashboard → baca ThingSpeak**: tiap 15 detik (sama persis dengan kecepatan data
  dibuat, supaya tidak ada delay yang membingungkan)

Dashboard tetap menampilkan data dari **ThingSpeak** (15 detik), BUKAN dari Supabase —
Supabase saat ini berfungsi sebagai penyimpanan cadangan resolusi tinggi untuk analisis
lebih detail di kemudian hari, bukan sumber tampilan dashboard.

## Kapasitas & kapan perlu dibersihkan

**ThingSpeak** (paket gratis): batas kirim 3 juta pesan/tahun, pemakaian saat ini
~2,1 juta pesan/tahun — aman, di 70% jatah. Data lama otomatis terhapus kalau total
tersimpan tembus 10 juta pesan — di kecepatan ini, itu baru terjadi di **~4,75 tahun**.

**Supabase** (paket gratis, 500MB) — karena sekarang kirim **tiap 1 detik** (bukan 15
detik lagi), kapasitasnya terisi jauh lebih cepat: estimasi **penuh dalam ~1,3–2 bulan**.
Ini konsekuensi yang sudah disepakati demi dapat data resolusi tinggi. **Wajib** rutin
bersih-bersih tabel `readings` di Supabase (lewat SQL Editor, hapus baris yang lebih tua
dari sekian hari) supaya insert tidak mulai gagal begitu 500MB penuh. Cek ukuran real di
Supabase Dashboard → Database secara berkala untuk pantau seberapa dekat ke batas.

## Urutan setup

### 1. ThingSpeak
Channel ID, MQTT credentials, Write API Key, dan Read API Key semua sudah terisi di kode.

### 2. Supabase (jalur kedua, paralel)
Kalau tabel `readings` belum pernah dibuat, jalankan `supabase_schema.sql` sekali di SQL Editor
(aman dijalankan ulang, pakai `if not exists`).

**Wajib untuk kirim 1 detik/data**: jalankan juga `supabase_cleanup_cron.sql` sekali di SQL
Editor — ini setup pembersihan OTOMATIS terjadwal (pakai `pg_cron`, gratis, sudah tersedia
di Supabase tanpa upgrade), hapus data lebih tua dari 20 hari tiap hari jam 03:00 UTC. Tanpa
ini, tabel `readings` akan penuh dalam ~1,3-2 bulan dan insert mulai gagal.

### 3. ESP32
1. **Extract dulu (Extract All)** zip ini ke folder biasa sebelum dibuka
2. Install library **"PubSubClient"** by Nick O'Leary lewat Library Manager
   (cuma ini SATU-SATUNYA library tambahan yang dibutuhkan — email sekarang
   dikirim manual pakai WiFiClientSecure bawaan ESP32, tanpa ESP_Mail_Client,
   supaya tidak ada lagi risiko bentrok versi library/toolchain)
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
