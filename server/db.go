package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/log/log15adapter"
	log "gopkg.in/inconshreveable/log15.v2"
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

func dbTableBandReq(state *ServerState, first, last uint32, filter string) *proto.TableAns {
	filter = "%" + filter + "%"
	sql := `select * from band where bandname like $1 order by bandname limit $2 offset $3;`
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
		err = rows.Scan(nil, &band.BandName, &genre, &found, &term, &band.ObjId)
		if err != nil {
			state.logger.Error(err.Error())
		}
		band.Genre = mapToGenre[genre]
		//TODO if found.Status = Null {unix = 0}
		band.UnixFoundDate = found.Time.Unix()
		band.UnixTermDate = term.Time.Unix()
		bands.Bands = append(bands.Bands, band)
		state.logger.Info(fmt.Sprintf("%v", band))
	}
	bands.Type = proto.EntityType_BAND
	return bands
}