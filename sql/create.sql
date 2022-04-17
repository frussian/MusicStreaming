drop table if exists Musician cascade;
create table Musician (
	musicianID int generated always as identity primary key,
	musicianName varchar(32) unique not null,
	biography text,
	dateOfBirth date,
	img oid
);

drop type if exists instr_enum cascade;
create type instr_enum as enum ('guitar', 'bass', 'vocal', 'drums');

drop table if exists Instrument cascade;
create table Instrument (
    musicianID int references Musician(musicianId)
        on delete cascade
        deferrable initially deferred,
	"type" instr_enum not null,
	primary key(musicianID, "type")
);

-- drop table if exists MusInstrInt;
-- create table MusInstrInt (
-- 	musicianID int references Musician(musicianId) deferrable initially deferred,
-- 	"type" instr_enum references Instrument("type"),
-- 	primary key(musicianID, "type")
-- );

--triggers and procedures for musician and instrument

--insert musician
create or replace function mus_insert_trig() returns trigger as $mus_insert_trig$
    begin
        if not exists(
            select 1 from Instrument
            where musicianID = new.musicianID
        ) then
            raise exception 'musician % must have at least 1 instrument', new.musicianName;
        end if;
--         raise notice 'name: %', new.musicianName;
        return null;
    end;
$mus_insert_trig$ language plpgsql;

drop trigger if exists mus_insert_trig on Musician;
create constraint trigger mus_insert_trig
    after insert on Musician
    initially deferred
    for each row
    execute procedure mus_insert_trig();
--

--update instrument
create or replace function instr_upd_trig() returns trigger as $instr_upd_trig$
    begin
        if new.musicianID <> old.musicianID then
            raise exception 'cannot update foreign key in instrument';
        end if;
    end;
$instr_upd_trig$ language plpgsql;

drop trigger if exists instr_upd_trig on Instrument;
create constraint trigger instr_upd_trig
    after update on Instrument
    for each row
    execute procedure instr_upd_trig();
--

--delete instrument
create or replace function instr_del_trig() returns trigger as $instr_del_trig$
    begin
        if not exists(select 1 from Instrument i
                        where old.musicianID = i.musicianID) then
            raise exception 'cannot delete instrument because musician must have at least 1 instrument';
        end if;
    end;
$instr_del_trig$ language plpgsql;

drop trigger if exists instr_del_trig on Instrument;
create constraint trigger instr_del_trig
    after delete on Instrument
    for each row
    execute procedure instr_del_trig();
--

--helper procedure for minimum cardinality
create or replace procedure insert_musician(name varchar(32), bio text,
    dateOfBirth date, instr instr_enum[]) language plpgsql
as $$
declare
    id int;
    t instr_enum;
begin
--     start transaction;
    insert into musician(musicianName, biography, dateOfBirth) values
        (name, bio, dateOfBirth) returning musicianID into id;
    foreach t in array instr
    loop
        insert into Instrument(musicianID, type) values(id, t);
    end loop;
--     commit;
end
$$;
--

--

drop type if exists genre_enum cascade;
create type genre_enum as enum('rock', 'alternative', 'indie', 'blues', 'metal');

drop table if exists Band cascade;
create table Band (
	bandID int generated always as identity primary key,
	bandName varchar(64) unique not null,
	genre genre_enum,
	foundingDate date,
	terminationDate date,
	img oid
);

drop table if exists Membership cascade;
create table Membership (
	membID int generated always as identity primary key,
	musName varchar(32) references Musician(musicianName),
	bandName varchar(32) references Band(bandName),
	enterDate date not null,
	quitDate date,
	unique (musName, bandName, enterDate)
);

drop table if exists Album cascade;
create table Album (
    albumID int generated always as identity primary key,
    releaseDate date,
    title varchar(64) not null,
    bandID int not null references Band(bandID),
    unique(title, bandID)
);

drop table if exists Song cascade;
create table Song (
    songID int generated always as identity primary key,
    length interval hour to second not null,
    index int not null,
    data oid not null,
    songName varchar(64) not null,
    albumID int not null references Album(albumID),
    unique(songName, albumID),
    unique(albumID, index)
);


