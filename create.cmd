@echo off
set PGPASSWORD=%1
dropdb MusicDB
createdb MusicDB
psql -d MusicDB -f sql/create.sql
psql -d MusicDB -f sql/values.sql