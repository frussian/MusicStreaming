insert into Musician(MusicianName, DateOfBirth, Biography) values
  ('Slash', '1965-07-23', 'GnR guitarist'), ('James Hetfield', '1963-08-03', 'Metallica frontman');

call insert_musician('Slash', 'GnR guitarist',
              '1965-07-23', '{guitar}');
call insert_musician('James Hetfield', 'Metallica frontman',
    '1963-08-03', '{vocal, guitar}');
call insert_musician('Axl Rose', 'GnR frontman', '1962-02-06', '{vocal}');

select * from musician;
select * from instrument;
