package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/log/log15adapter"
	log "gopkg.in/inconshreveable/log15.v2"
	"math"
	"musicplatform/proto"
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

func dbTableBandReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select bandname, genre, foundingdate, terminationdate, img from band where bandname ilike $1 order by bandname limit $2 offset $3;`
	rows, err := state.pgxconn.Query(context.Background(), sql, filter, last-first+1, first)
	if err != nil {
		state.logger.Error(err.Error())
		return nil
	}
	bands := &proto.TableAns{Bands: make([]*proto.Band, 0, last-first+1)}
	for rows.Next() {
		band := &proto.Band{}
		var genre string
		var found, term pgtype.Date
		err = rows.Scan(&band.BandName, &genre, &found, &term, &band.ObjId)
		if err != nil {
			state.logger.Error(err.Error())
			continue
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