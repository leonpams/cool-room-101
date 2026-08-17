-- =========================================================
-- PEMBERSIHAN OTOMATIS — Supabase readings table
-- Jalankan SEKALI di Supabase SQL Editor untuk setup.
--
-- Kenapa perlu ini: kirim data tiap 1 detik ke Supabase bikin
-- kapasitas gratis (500MB) penuh dalam ~1,3-2 bulan kalau tidak
-- pernah dibersihkan. Script ini otomatis hapus data yang lebih
-- tua dari 20 hari, jalan sendiri tiap hari jam 03:00 UTC —
-- tidak perlu diingat-ingat manual lagi.
--
-- 20 hari dipilih dengan margin aman (bukan mepet ke batas
-- 40-60 hari) buat jaga-jaga kalau job ini sempat gagal jalan
-- beberapa hari, ukuran baris ternyata lebih besar dari estimasi,
-- dsb. Kalau mau retensi lebih lama/pendek, tinggal ubah angka
-- '20 days' di bagian STEP 2 dan STEP 4, lalu jalankan ulang.
-- =========================================================

-- ── STEP 1: Aktifkan ekstensi pg_cron (sekali saja) ─────────
create extension if not exists pg_cron;

-- ── STEP 2: Jadwalkan pembersihan harian ────────────────────
-- Hapus semua baris di 'readings' yang lebih tua dari 20 hari,
-- jalan otomatis tiap hari jam 03:00 UTC (= 10:00 WIB).
select cron.schedule(
  'cleanup-old-readings',                      -- nama job (unik)
  '0 3 * * *',                                 -- tiap hari jam 03:00 UTC
  $$ delete from readings where created_at < now() - interval '20 days' $$
);

-- =========================================================
-- QUERY BANTUAN — jalankan kapan saja untuk cek/kontrol
-- =========================================================

-- ── Cek job terjadwal masih aktif ───────────────────────────
-- select * from cron.job where jobname = 'cleanup-old-readings';

-- ── Cek riwayat 10 kali eksekusi terakhir (berhasil/gagal) ──
-- select jobname, status, return_message, start_time, end_time
-- from cron.job_run_details
-- order by start_time desc
-- limit 10;

-- ── Cek jumlah baris & estimasi ukuran tabel sekarang ───────
-- select
--   count(*) as jumlah_baris,
--   pg_size_pretty(pg_total_relation_size('readings')) as ukuran_tabel
-- from readings;

-- ── Hapus manual SEKARANG (kalau tidak mau nunggu jadwal) ───
-- delete from readings where created_at < now() - interval '20 days';

-- ── Ubah jadi retensi berapa hari yang beda, misal 30 hari ──
-- select cron.unschedule('cleanup-old-readings');
-- select cron.schedule(
--   'cleanup-old-readings',
--   '0 3 * * *',
--   $$ delete from readings where created_at < now() - interval '30 days' $$
-- );

-- ── Matikan job ini sepenuhnya (kalau suatu saat tidak perlu) ──
-- select cron.unschedule('cleanup-old-readings');

-- =========================================================
-- CATATAN
-- =========================================================
-- DELETE di Postgres tidak langsung mengecilkan ukuran file di
-- disk — baris yang dihapus jadi "dead tuple" dulu, baru benar-
-- benar dibebaskan lewat proses VACUUM. Supabase menjalankan
-- autovacuum otomatis di belakang layar, jadi biasanya tidak
-- perlu campur tangan manual. Kalau setelah beberapa minggu
-- ukuran tabel (lihat query "Cek jumlah baris" di atas) masih
-- terasa lebih besar dari yang diharapkan, itu tandanya
-- autovacuum belum sempat jalan — biasanya beres sendiri dalam
-- beberapa jam.
