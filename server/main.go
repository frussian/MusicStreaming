package main

import (
	"encoding/json"
	"flag"
	"github.com/jackc/pgx/v4"
	log "gopkg.in/inconshreveable/log15.v2"
	"io/ioutil"
	"musicplatform/tcpbuf"
	"github.com/gammazero/deque"
	//"musicplatform/proto"
	//protobuf "google.golang.org/protobuf/proto"
	"os"
	"time"
)

type SrvCFG struct {
	Host     string `json:"host"`
	Port     int    `json:"port"`
	MsLoop   int    `json:"ms_loop"`
	InSize   int    `json:"in_size"`
	OutSize  int    `json:"out_size"`
	DbServer string `json:"db_server"`
	DbPort   int    `json:"db_port"`
	DbName   string `json:"db_name"`
	DbUser   string `json:"db_user"`
	DbPass   string `json:"db_pass"`
	LogLvl   string `json:"log_lvl"`
}

type ServerState struct {
	cfg SrvCFG
	lib *tcpbuf.TCPBuf
	logger log.Logger
	pgxconn *pgx.Conn
	read int
	messages deque.Deque
}

var (
	cfgPath string
)


func parseFlags() {
	flag.StringVar(&cfgPath, "cfg", "server.cfg",
			"path to cfg file")
	flag.Parse()
}

func parseCfg(cfg *SrvCFG) {
	file, err := os.Open(cfgPath)
	if err != nil {
		panic(err)
	}
	data, err := ioutil.ReadAll(file)
	if err != nil {
		panic(err)
	}
	err = json.Unmarshal(data, &cfg)
	if err != nil {
		panic(err)
	}
}

func test() {
	//write
	//album := &proto.Album{}
	//album.Title = "Metallica"
	//album.BandName = "Metallica"
	//album.Songs = []*proto.Song{{SongName: "Enter Sandman"}}
	//t, _ := time.Parse("02-01-2006", "19-08-1991")
	//album.UnixReleaseDate = t.Unix()
	//bytes, err := protobuf.Marshal(album)
	//if err != nil {
	//	panic(err)
	//}
	//f, _ := os.Create("test.out")
	//f.Write(bytes)

	//read
	//bytes := make([]byte, 1024)
	//f, _ := os.Open("test.out")
	//n, _ := f.Read(bytes)
	//album := &proto.Album{}
	//protobuf.Unmarshal(bytes[:n], album)
	//fmt.Println(album)
}

func main() {
	state := &ServerState{}
	parseFlags()
	parseCfg(&state.cfg)

	srvlog := log.New("module", "server")
	lvl, err := log.LvlFromString(state.cfg.LogLvl)
	if err != nil {
		panic(err)
	}
	srvlog.SetHandler(log.LvlFilterHandler(lvl, log.StdoutHandler))

	tcpLogger := log.New("module", "tcpbuf")
	//tcpLogger.SetHandler(log.)
	instance := tcpbuf.NewTCPBuf(tcpLogger, 128)

	state.lib = instance
	state.logger = srvlog
	state.read = 0

	connected := connectToDB(state)
	if !connected {
		os.Exit(1)
	}
	serv := instance.NewServerListener(state.cfg.Host, state.cfg.Port,
		servCB, state.cfg.InSize,
		state.cfg.OutSize, state)
	serv.StartServer()

	msLoop := time.Duration(state.cfg.MsLoop) * time.Millisecond

	state.logger.Info("starting", "host", state.cfg.Host, "port", state.cfg.Port,
		"loop ms", state.cfg.MsLoop, "inSize", state.cfg.InSize, "outSize", state.cfg.OutSize)
	for {
		loop(state, msLoop)
	}
}
