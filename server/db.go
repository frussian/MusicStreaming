package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/log/log15adapter"
	log "gopkg.in/inconshreveable/log15.v2"
	"io"
	"math"
	"musicplatform/proto"
	"time"
)

type EntityType int32

const (
	BAND       EntityType = 0
	ALBUM
	SONG
	CONCERT
	MUSICIAN
	MEMBERSHIP
)

var mapToGenre = map[string]proto.Genre {"rock": proto.Genre_ROCK,
	"blues": proto.Genre_BLUES, "metal": proto.Genre_METAL,
	"alternative": proto.Genre_ALTERNATIVE, "indie": proto.Genre_INDIE}

func connectToDB(state *ServerState) bool {
	url := fmt.Sprintf("postgres://%s:%s@%s:%d/%s",
		state.cfg.DbUser, state.cfg.DbPass, state.cfg.DbServer,
		state.cfg.DbPort, state.cfg.DbName)
	cfg, err := pgx.ParseConfig(url)
	if err != nil {
		panic(err)
	}

	cfg.Logger = log15adapter.NewLogger(log.New("module", "pgx"))
	cfg.LogLevel = pgx.LogLevelInfo

	conn, err := pgx.ConnectConfig(context.Background(), cfg)
	if err != nil {
		state.logger.Error(err.Error())
		return false
	}
	state.pgxconn = conn
	return true
}

//select pg_size_pretty(length(lo_get(data))::bigint) from song;

func scanBandRow(state *ServerState, row pgx.Row) *proto.Band {
	band := &proto.Band{}
	var genre string
	var found, term pgtype.Date
	var descr pgtype.Text

	err := row.Scan(&band.BandName, &genre, &found, &term, &descr)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	band.Genre = mapToGenre[genre]
	//TODO if found.Status = Null {unix = 0}
	if found.Status == pgtype.Null {
		band.UnixFoundDate = 0
	} else {
		band.UnixFoundDate = found.Time.Unix()
	}
	if term.Status == pgtype.Null {
		band.UnixTermDate = 0
	} else {
		band.UnixTermDate = term.Time.Unix()
	}
	band.Description = descr.String

	return band
}

func dbTableBandReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select bandname, genre, foundingdate, terminationdate, description from band where bandname ilike $1 order by bandname limit $2 offset $3;`
	rows, err := state.pgxconn.Query(context.Background(), sql, filter, last-first+1, first)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	bands := &proto.TableAns{Bands: make([]*proto.Band, 0, last-first+1)}
	for rows.Next() {
		band := scanBandRow(state, rows)
		if band == nil {
			continue
		}
		bands.Bands = append(bands.Bands, band)
	}
	bands.Type = proto.EntityType_BAND
	return bands
}

func dbTableAlbumReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select * from AlbumTable where title ilike $1 limit $2 offset $3;`
	rows, err := state.pgxconn.Query(context.Background(), sql, filter, last-first+1, first)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	albums := &proto.TableAns{Albums: make([]*proto.Album, 0, last-first+1)}

	for rows.Next() {
		album := &proto.Album{}
		var nsongs pgtype.Int8
		var title, bandname pgtype.Varchar
		var reldate pgtype.Date
		err = rows.Scan(&title, &bandname, &nsongs, &reldate)
		if err != nil {
			state.logger.Error(err.Error())
			continue
		}
		album.Title = title.String
		album.BandName = bandname.String
		album.UnixReleaseDate = reldate.Time.Unix()
		album.Songs = make([]*proto.Song, int(nsongs.Int))
		albums.Albums = append(albums.Albums, album)
	}

	albums.Type = proto.EntityType_ALBUM

	return albums
}

func dbTableSongReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select * from SongTable where songname ilike $1 limit $2 offset $3;`
	rows, err := state.pgxconn.Query(context.Background(), sql, filter, last-first+1, first)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	songs := &proto.TableAns{Songs: make([]*proto.Song, 0, last-first+1)}

	for rows.Next() {
		var length pgtype.Interval
		var songname, bandname, title pgtype.Varchar
		song := &proto.Song{}
		err = rows.Scan(&songname, &length, &title, &bandname)
		if err != nil {
			state.logger.Error(err.Error())
			continue
		}

		song.SongName = songname.String
		song.BandName = bandname.String
		song.AlbumName = title.String
		song.LengthSec = int32(length.Microseconds / int64(math.Pow10(6)))
		state.logger.Info("len", "len", song.LengthSec)

		songs.Songs = append(songs.Songs, song)
	}

	songs.Type = proto.EntityType_SONG

	return songs
}

func scanConcertRow(state *ServerState, row pgx.Rows) *proto.Concert {
	var date pgtype.Date
	var concertTime pgtype.Time
	var location pgtype.Varchar
	var description pgtype.Text
	var capacity pgtype.Int4
	concert := &proto.Concert{}

	err := row.Scan(&description, &capacity, &date, &location, &concertTime)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}

	concert.Description = description.String
	concert.Capacity = capacity.Int
	concert.Location = location.String
	msec := concertTime.Microseconds
	timemsec := time.Microsecond * time.Duration(msec)
	concert.UnixDateTime = date.Time.Add(timemsec).Unix()

	return concert
}

func dbTableConcertReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select description, capacity, concertDate, location, concertTime from Concert where location ilike $1 limit $2 offset $3;`
	rows, err := state.pgxconn.Query(context.Background(), sql, filter, last-first+1, first)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	concerts := &proto.TableAns{Concerts: make([]*proto.Concert, 0, last-first+1)}

	for rows.Next() {
		concert := scanConcertRow(state, rows)
		if concert == nil {
			continue
		}
		concerts.Concerts = append(concerts.Concerts, concert)
	}

	concerts.Type = proto.EntityType_CONCERT

	return concerts
}

func scanMembershipRow(state *ServerState, row pgx.Rows) *proto.Membership {
	var enter, quit pgtype.Date
	var mus, band pgtype.Varchar
	err := row.Scan(&mus, &band, &enter, &quit)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	quitUnix := int64(0)
	if quit.Status == pgtype.Null {
		quitUnix = 0
	} else {
		quitUnix = quit.Time.Unix()
	}
	return &proto.Membership{MusName: mus.String,
		BandName: band.String,
		UnixEntryDate: enter.Time.Unix(),
		UnixQuitDate: quitUnix}
}

func dbSimpleBandReq(conn *Conn, req string) *proto.SimpleAns_Band {
	sql := `select bandname, genre, foundingdate, terminationdate, description from band where bandname=$1;`
	row := conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, req)
	band := scanBandRow(conn.stateSrv, row)
	if band == nil {
		return nil
	}

	sql = `select bandid from band where bandname=$1;`
	row = conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, req)
	var id pgtype.Int4
	err := row.Scan(&id)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	sql = `select title from album where bandid=$1;`
	rows, err := conn.stateSrv.pgxconn.Query(context.Background(), sql, id)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	for rows.Next() {
		var title pgtype.Varchar
		err = rows.Scan(&title)
		if err != nil {
			conn.stateSrv.logger.Error(err.Error())
			return nil
		}
		band.AlbumNames = append(band.AlbumNames, title.String)
	}

	sql = `select (select musicianName from musician where musicianID=musID), $1,
			enterDate, quitDate	from membership where bandID=$2;`
	rows, err = conn.stateSrv.pgxconn.Query(context.Background(), sql, band.BandName, id)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	for rows.Next() {
		memb := scanMembershipRow(conn.stateSrv, rows)
		if memb == nil {
			continue
		}
		band.Participants = append(band.Participants, memb)
	}

	//description, &capacity, &date, &location, &concertTime
	sql = `select description, capacity, concertDate, location, concertTime from BandConcert
			where bandid=$1;`
	rows, err = conn.stateSrv.pgxconn.Query(context.Background(), sql, id)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	for rows.Next() {
		concert := scanConcertRow(conn.stateSrv, rows)
		if concert == nil {
			continue
		}
		band.Concerts = append(band.Concerts, concert)
	}

	ans := &proto.SimpleAns_Band{Band: band}
	return ans
}

func dbSimpleAlbumReq(conn *Conn, req string) *proto.SimpleAns_Album {
	sql := `select albumid, releasedate, bandid from album where title=$1;`
	row := conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, req)
	var albumId, bandId pgtype.Int4
	var date pgtype.Date
	err := row.Scan(&albumId, &date, &bandId)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	album := &proto.Album{}
	album.UnixReleaseDate = date.Time.Unix()
	album.Title = req

	sql = `select bandname from band where bandid=$1`
	row = conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, bandId)
	var bandName pgtype.Varchar
	err = row.Scan(&bandName)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	album.BandName = bandName.String

	sql = `select songname, length from song where albumid=$1 order by index asc;`
	rows, err := conn.stateSrv.pgxconn.Query(context.Background(), sql, albumId)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	for rows.Next() {
		var length pgtype.Interval
		var songname pgtype.Varchar

		song := &proto.Song{}
		err = rows.Scan(&songname, &length)
		if err != nil {
			conn.stateSrv.logger.Error(err.Error())
			continue
		}
		song.SongName = songname.String
		song.LengthSec = int32(length.Microseconds / int64(math.Pow10(6)))
		song.BandName = bandName.String
		album.Songs = append(album.Songs, song)
	}

	ans := &proto.SimpleAns_Album{Album: album}
	return ans
}

func dbSimpleMusicianReq(conn *Conn, req string) *proto.SimpleAns_Musician {
	sql := "select dateOfBirth, biography, musicianID from musician where musicianName=$1;"
	var musID pgtype.Int4
	var date pgtype.Date
	var bio pgtype.Varchar
	row := conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, req)
	err := row.Scan(&date, &bio, &musID)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	mus := &proto.Musician{}
	mus.MusName = req
	mus.UnixDateOfBirth = date.Time.Unix()
	mus.Bio = bio.String

	sql = "select $1, (select bandname from band b where b.bandid=m.bandid), " +
		"enterdate, quitdate from membership m where musID=$2;"
	rows, err := conn.stateSrv.pgxconn.Query(context.Background(), sql, req, musID.Int)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	for rows.Next() {
		memb := scanMembershipRow(conn.stateSrv, rows)
		if memb == nil {
			continue
		}
		mus.Memberships = append(mus.Memberships, memb)
	}

	ans := &proto.SimpleAns_Musician{Musician: mus}
	return ans
}

func dbGetObjId(conn *Conn, req *proto.StreamReq) int {
	sql := ""
	switch req.Type {
	case proto.EntityType_SONG: {
		sql = `select data from song where songname=$1`
	}
	default: {
		conn.stateSrv.logger.Warn("stream request not supported", "type", req.Type)
		return -1
	}
	}

	row := conn.stateSrv.pgxconn.QueryRow(context.Background(), sql, req.ReqString)
	var id pgtype.OIDValue

	err := row.Scan(&id)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return -1
	}
	if id.Status == pgtype.Null {
		return -1
	}
	return int(id.Uint)
}

func dbGetChunk(conn *Conn, stream *Stream) []byte {
	tx, err := conn.stateSrv.pgxconn.Begin(context.Background())
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	los := tx.LargeObjects()

	lo, err := los.Open(context.Background(), uint32(stream.objId), pgx.LargeObjectModeRead)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	_, err = lo.Seek(int64(stream.offset), 0)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	chunk := make([]byte, stream.size)

	n, err := lo.Read(chunk)
	if err != nil && err != io.EOF {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}
	chunk = chunk[:n]

	err = lo.Close()
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	err = tx.Commit(context.Background())
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return nil
	}

	return chunk
}