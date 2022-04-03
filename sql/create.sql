-- Database: musicdb
-- DROP DATABASE IF EXISTS musicdb;

-- CREATE DATABASE musicdb
--     WITH
--     OWNER = anton
--     ENCODING = 'UTF8'
--     LC_COLLATE = 'English_United States.1252'
--     LC_CTYPE = 'English_United States.1252'
--     TABLESPACE = pg_default
--     CONNECTION LIMIT = -1;

-- GRANT ALL ON DATABASE musicdb TO anton;
--
-- GRANT ALL ON DATABASE musicdb TO postgres;
--
-- GRANT TEMPORARY, CONNECT ON DATABASE musicdb TO PUBLIC;
--
-- SELECT version();
drop table if exists Musician;

create table Musician (
	MusicianID serial primary key,
	DateOfBirth date not null,
	Biography text,
	MusicianName varchar(32) unique not null
);
