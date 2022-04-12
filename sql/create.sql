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
	"type" instr_enum unique not null
);

drop table if exists MusInstrInt;
create table MusInstrInt (
	musicianID int references Musician(musicianId),
	"type" instr_enum references Instrument("type"),
	primary key(musicianID, "type")
);


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
    length interval HOUR TO SECOND not null,
    data oid not null,
    songName varchar(64) not null,
    albumID int not null references Album(albumID),
    unique(songName, albumID)
);


