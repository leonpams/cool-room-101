-- =========================================================
-- Tabel penyimpanan data suhu & kelembaban (jalur kedua, paralel ThingSpeak)
-- Jalankan di: Supabase Dashboard > SQL Editor > New Query
-- Lewati langkah ini kalau tabel 'readings' sudah pernah dibuat sebelumnya.
-- =========================================================

create table if not exists readings (
  id bigint generated always as identity primary key,
  created_at timestamptz not null default now(),
  temperature numeric not null,
  humidity numeric not null
);

create index if not exists idx_readings_created_at on readings(created_at desc);

alter table readings enable row level security;

drop policy if exists "anon boleh insert" on readings;
create policy "anon boleh insert" on readings
  for insert to anon with check (true);

drop policy if exists "anon boleh baca" on readings;
create policy "anon boleh baca" on readings
  for select to anon using (true);
