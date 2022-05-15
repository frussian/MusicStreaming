drop table if exists Musician cascade;
create table Musician (
	musicianID int generated always as identity primary key,
	musicianName varchar(64) unique not null,
	biography text,
	dateOfBirth date,
	img oid
);

drop type if exists instr_enum cascade;
create type instr_enum as enum ('guitar', 'bass', 'vocal', 'drums');

drop table if exists Instrument cascade;
create table Instrument (
    musicianID int not null references Musician(musicianId)
        on delete cascade,
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
	description text null,
	foundingDate date,
	terminationDate date,
	img oid
);

drop table if exists Membership cascade;
create table Membership (
	musID int not null references Musician(musicianID)
        on delete cascade,
	bandID int not null references Band(bandID)
        on delete cascade,
	enterDate date not null,
	quitDate date,
	primary key (musID, bandID, enterDate)
);

--triggers and procedures for band and membership

create or replace procedure insert_band(bandName varchar(64), genre genre_enum, description text,
                                        foundingDate date, terminationDate date,
                                        musicianNameP varchar(32), enterDate date,
                                        quitDate date, img oid)
language plpgsql
as $$
declare
    id int;
begin
--     start transaction;
    insert into band(bandName, genre, description, foundingDate, terminationDate, img) values
        (bandName, genre, description, foundingDate, terminationDate, img) returning bandID into id;
    insert into Membership(musID, bandID, enterDate, quitDate) values
        ((select musicianID from musician m where m.musicianName = musicianNameP),
         id, enterDate, quitDate);
--     raise notice 'id: %', (select musicianID from musician m where m.musicianName = musicianNameP);
--     commit;
end
$$;

--insert band trigger
create or replace function band_insert_trig() returns trigger as $band_insert_trig$
begin
    if not exists(
            select 1 from Membership
            where bandID = new.bandID
        ) then
        raise exception 'cannot insert band: band % must have at least 1 participant', new.bandName;
    end if;
    return null;
end;
$band_insert_trig$ language plpgsql;

drop trigger if exists band_insert_trig on Band;
create constraint trigger band_insert_trig
    after insert on Band
    initially deferred
    for each row
execute procedure band_insert_trig();
--

--delete membership trigger
create or replace function membership_delete_trig() returns trigger as $membership_delete_trig$
begin
    if not exists (select 1 from Membership
            where bandID = old.bandID
        ) then
        raise exception 'cannot delete membership: band % must have at least 1 participant', old.bandID;
    end if;
    return null;
end;
$membership_delete_trig$ language plpgsql;

drop trigger if exists membership_delete_trig on Membership;
create constraint trigger membership_delete_trig
    after delete on Membership
    initially deferred
    for each row
execute procedure membership_delete_trig();
--

create or replace procedure insert_membership(bandNameP varchar(64), musNameP varchar(64),
                                              enterDate date, quitDate date)
language plpgsql
as $$
declare
    bandID int;
    musID int;
begin
    bandID = (select b.bandID from Band b where b.bandName = bandNameP);
    musID = (select musicianID from Musician m where m.musicianname = musNameP);
    insert into Membership(musID, bandID, enterDate, quitDate) values
         (musID, bandID, enterDate, quitDate);
end
$$;

--

drop table if exists Album cascade;
create table Album (
    albumID int generated always as identity primary key,
    releaseDate date,
    title varchar(64) not null,
    bandID int not null references Band(bandID)
        on delete cascade,
    unique(title, bandID)
);

--TODO: https://postgrespro.ru/docs/postgresql/9.5/lo
drop table if exists Song cascade;
create table Song (
    songID int generated always as identity primary key,
    length interval hour to second not null,
    index int not null,
    data oid not null,
    songName varchar(64) not null,
    albumID int not null references Album(albumID)
        on delete cascade,
    unique(albumID, songName, index)
);

--album and song triggers and procedures

--insert album trigger
create or replace function album_insert_trig() returns trigger as $album_insert_trig$
begin
    if not exists(
            select 1 from Song
            where albumID = new.albumID
        ) then
        raise exception 'cannot insert album: album % must have at least 1 song', new.title;
    end if;
    return null;
end;
$album_insert_trig$ language plpgsql;

drop trigger if exists album_insert_trig on Album;
create constraint trigger album_insert_trig
    after insert on album
    initially deferred
    for each row
execute procedure album_insert_trig();
--

--delete song trigger
create or replace function song_delete_trig() returns trigger as $song_delete_trig$
begin
--     raise notice 'song %', old.data;
    if (not exists(
            select 1 from Song
            where albumID = old.albumID
        ) and exists(select title from album a where a.albumID = old.albumID)) then
        raise exception 'cannot delete song: album with id % must have at least 1 song', old.albumID;
    end if;
    perform lo_unlink(old.data);
    return null;
end;
$song_delete_trig$ language plpgsql;

drop trigger if exists song_delete_trig on Song;
create constraint trigger song_delete_trig
    after delete on Song
    initially deferred
    for each row
execute procedure song_delete_trig();


--

drop table if exists Concert cascade;
create table Concert (
    concertID int generated always as identity primary key,
    description text null,
    capacity int null,
    concertDate date not null,
    location varchar(64) not null,
    concertTime time not null,
    unique (concertDate, location, concertTime)
);

drop table if exists BandConcertInt cascade;
create table BandConcertInt (
    bandID int references Band(bandID) on delete cascade,
    concertID int references Concert(concertID) on delete cascade,
    primary key (bandID, concertID)
);

create or replace function band_concert_int_upd_trig() returns trigger as $bandconcertint_upd_trig$
    begin
        if new.concertID <> old.concertID or new.bandid <> old.bandid then
            raise exception 'cannot update foreign key in instrument';
        end if;
    end;
$bandconcertint_upd_trig$ language plpgsql;


drop trigger if exists band_concert_int_upd_trig on BandConcertInt;
create constraint trigger band_concert_int_upd_trig
    after update on BandConcertInt
    for each row
    execute procedure band_concert_int_upd_trig();

drop view if exists AlbumTable;
create view AlbumTable as
    select title, (select bandname from band b where b.bandid = a.bandid),
			(select count(*) from song s where s.albumid = a.albumid), releaseDate
    from album a order by title;

drop view if exists SongTable;
create view SongTable as
    select songname, length, a.title, (select bandname from band b
     where b.bandid = a.bandID) from song
    join album a on a.albumID = song.albumID
    order by songname;