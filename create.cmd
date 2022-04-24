dropdb MusicDB
createdb MusicDB
psql -d MusicDB -f sql/create.sql
psql -d MusicDB -f sql/values.sql