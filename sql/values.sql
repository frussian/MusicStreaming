call insert_musician('Slash', 'GnR guitarist',
              '1965-07-23', '{guitar}');
call insert_musician('Axl Rose', 'GnR frontman', '1962-02-06', '{vocal}');
call insert_musician('Izzy Stradlin', 'GnR rhythm guitarist', '1962-04-08', '{guitar, vocal}');
call insert_musician('Duff McKagan', 'GnR bassist', '1964-02-05', '{bass}');
call insert_musician('Steven Adler', 'GnR drummer', '1965-01-22', '{drums}');
call insert_musician('Richard Fortus', 'GnR rhythm', '1966-11-17', '{guitar}');

call insert_musician('James Hetfield', 'Metallica frontman',
    '1963-08-03', '{vocal, guitar}');
call insert_musician('Kirk Hammett', 'Metallica lead guitarist', '1962-11-18', '{guitar}');
call insert_musician('Lars Ulrich', 'Metallica drummer', '1963-12-26', '{drums}');
call insert_musician('Cliff Burton', 'Metallica bassist', '1962-02-10', '{bass}');
call insert_musician('Jason Newsted', 'Metallica bassist', '1963-03-04', '{bass}');
call insert_musician('Robert Trujillo', 'Metallica bassist', '1964-10-23', '{bass}');

call insert_musician('Anthony Kiedis', 'RHCP vocalist', '1963-03-04', '{vocal}');
call insert_musician('John Frusciante', 'RHCP guitarist', '1970-03-05', '{vocal, guitar}');
call insert_musician('Flea', 'RHCP bassist', '1962-10-16', '{bass}');
call insert_musician('Chad Smith', 'RHCP drummer', '1961-10-25', '{drums}');


call insert_band('GnR', 'rock',
    'Guns ''N Roses  is an American hard rock band from Los Angeles, California, formed in 1985' || chr(13) ||
    'When they signed to Geffen Records in 1986, the band comprised vocalist Axl Rose, lead guitarist Slash, rhythm guitarist Izzy Stradlin, bassist Duff McKagan, and drummer Steven Adler.' || chr(13) ||
    'The current lineup consists of Rose, Slash, McKagan, guitarist Richard Fortus, drummer Frank Ferrer and keyboardists Dizzy Reed and Melissa Reese.' || chr(13) ||
    'Guns N'' Roses'' debut album, Appetite for Destruction (1987), reached number one on the Billboard 200 a year after its release, on the strength of the top 10 singles "Welcome to the Jungle", "Paradise City", and "Sweet Child o'' Mine", the band''s only single to reach number one on the Billboard Hot 100.' || chr(13) ||
    'The album has sold approximately 30 million copies worldwide, including 18 million units in the United States, making it the country''s bestselling debut album and eleventh-bestselling album.',
    '1986-03-08', null, 'Slash', '1987-10-01', '1996-01-01', null);

call insert_band('Metallica', 'metal', 'Metallica is an American heavy metal band. ' || chr(13) || 'The band was formed in 1981 in Los Angeles by vocalist/guitarist James Hetfield and drummer Lars Ulrich, and has been based in San Francisco for most of its career.' || chr(13) ||
    'The band''s fast tempos, instrumentals and aggressive musicianship made them one of the founding "big four" bands of thrash metal, alongside Megadeth, Anthrax and Slayer. Metallica''s current lineup comprises founding members and primary songwriters Hetfield and Ulrich, ' || chr(13) ||
    'longtime lead guitarist Kirk Hammett and bassist Robert Trujillo. Guitarist Dave Mustaine (who went on to form Megadeth after being fired from the band) and bassists Ron McGovney, Cliff Burton and Jason Newsted are former members of the band.',
    '1981-01-01', null,
    'James Hetfield', '1981-01-01', null, null);

call insert_band('Red Hot Chili Peppers', 'rock',
    'The Red Hot Chili Peppers are an American rock band formed in Los Angeles in 1983.' || chr(13) ||
    'Their music incorporates elements of alternative rock, funk, punk rock and psychedelic rock.' || chr(13) ||
    'The band consists of co-founders Anthony Kiedis (lead vocals), Flea (bass), drummer Chad Smith and guitarist John Frusciante. With over 100 million records sold worldwide, the Red Hot Chili Peppers are one of the best-selling bands of all time.',
    '1983-01-01', null,
    'Anthony Kiedis', '1983-01-01', null, null);


call insert_membership('GnR', 'Slash', '2016-01-01', null);
call insert_membership('GnR', 'Axl Rose', '1985-01-01', null);
call insert_membership('GnR', 'Izzy Stradlin', '1985-01-01', '1991-01-01');
call insert_membership('GnR', 'Duff McKagan', '1985-01-01', '1997-01-01');
call insert_membership('GnR', 'Duff McKagan', '2016-01-01', null);
call insert_membership('GnR', 'Steven Adler', '1985-01-01', '1990-01-01');
call insert_membership('GnR', 'Richard Fortus', '2002-01-01', null);

call insert_membership('Metallica', 'Kirk Hammett', '1983-01-01', null);
call insert_membership('Metallica', 'Lars Ulrich', '1981-01-01', null);
call insert_membership('Metallica', 'Cliff Burton', '1982-01-01', '1986-01-01');
call insert_membership('Metallica', 'Jason Newsted', '1986-01-01', '2001-01-01');
call insert_membership('Metallica', 'Robert Trujillo', '2003-01-01', null);

call insert_membership('Red Hot Chili Peppers', 'Flea', '1983-01-01', null);
call insert_membership('Red Hot Chili Peppers', 'Chad Smith', '1988-01-01', null);
call insert_membership('Red Hot Chili Peppers', 'John Frusciante', '1988-01-01', '1992-01-01');
call insert_membership('Red Hot Chili Peppers', 'John Frusciante', '1998-01-01', '2009-01-01');
call insert_membership('Red Hot Chili Peppers', 'John Frusciante', '2019-01-01', null);

insert into Concert(capacity, description, concertdate, location, concerttime) values
    (25000, 'Damaged Justice Tour, North American Leg II', '1989-08-29',
     'SEATTLE, WASHINGTON, UNITED STATES, SEATTLE CENTER COLISEUM', '19:00'),
    (10000, 'Damage Inc. Tour European Leg #3', '1987-02-01',
     'ZÜRICH, SWITZERLAND, SPORTZENTRUM', '20:00'),
    (5000, 'Guns N'' Roses performs live at the Ritz in New York, NY on February 2nd 1988.',
     '1988-02-02', 'New York, New York, UNITED STATE, RITZ', '21:00'),
    (40000, 'Use Your Illusion World Tour', '1992-12-08',
     'TOKYO, JAPAN', '20:00'),
    (85000, 'The concert, which took place on August 23, 2003, was their first headlining show at Slane Castle in Ireland having previously performed there in August 2001 opening for U2. The show was one of the biggest ever for the band with 80,000 fans in attendance with tickets selling out in under two and a half hours.',
     '2003-08-23', 'SLANE CASTLE, IRELAND', '19:00');

insert into bandconcertint(bandid, concertid) values
    ((select bandid from band where bandname='Metallica'), 1),
    ((select bandid from band where bandname='Metallica'), 2),
    ((select bandid from band where bandname='GnR'), 3),
    ((select bandid from band where bandname='GnR'), 4),
    ((select bandid from band where bandname='Red Hot Chili Peppers'), 5);
