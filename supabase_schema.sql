-- =========================================================
-- Tabel penyimpanan data suhu & kelembaban
-- Jalankan di: Supabase Dashboard > SQL Editor > New Query
-- =========================================================

create table readings (
  id bigint generated always as identity primary key,
  created_at timestamptz not null default now(),
  temperature numeric not null,
  humidity numeric not null
);

create index idx_readings_created_at on readings(created_at desc);

-- Row Level Security: device (pakai anon key) boleh insert,
-- dashboard (juga pakai anon key) boleh baca.
-- Tidak ada yang boleh update/delete lewat API publik.
alter table readings enable row level security;

create policy "anon boleh insert" on readings
  for insert to anon with check (true);

create policy "anon boleh baca" on readings
  for select to anon using (true);
