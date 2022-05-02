package main

import (
	"encoding/json"
	"flag"
	"fmt"
	log "gopkg.in/inconshreveable/log15.v2"
	"io/ioutil"
	"musicplatform/tcpbuf"
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
	logger log.Logger
	read int
}

var (
	cfgPath string
	cfg SrvCFG
)


func parseFlags() {
	flag.StringVar(&cfgPath, "cfg", "server.cfg",
			"path to cfg file")
	flag.Parse()
}

func parseCfg() {
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

func testStreamCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	state := userCtx.(*ServerState)
	logger := state.logger
	switch evt.Kind {
	case tcpbuf.READ:
		if len(evt.Data) < 6 {
			break
		}
		state.read += len(evt.Data)
		logger.Info(fmt.Sprintf("recv: %s", string(evt.Data)))
		logger.Info(fmt.Sprintf("total: %d", state.read))
		return tcpbuf.RetType(len(evt.Data))
	case tcpbuf.WRITE:
		if state.read > 0 {
			//state.read = 0
			//msg := "received"
			//copy(evt.Data, msg)
			//return tcpbuf.RetType(len(msg))
		}
	case tcpbuf.CLOSING:
		logger.Info(fmt.Sprintf("closing %s", evt.Addr.String()))
	}
	return tcpbuf.OK
}

func testServCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	state := userCtx.(*ServerState)
	logger := state.logger
	switch evt.Kind {
	case tcpbuf.ERROR_LISTEN:
		if evt.Addr == nil {
			logger.Error("invalid address")
		} else {
			logger.Error("error listening", "host", evt.Addr.String())
		}
	case tcpbuf.LISTENING:
		logger.Info("listening on", "host", evt.Addr.String())
	case tcpbuf.NEW_CONN:
		logger.Info("new conn", "remote", evt.Addr.String())
		evt.Instance.AcceptStream(state, testStreamCB)
	}
	return tcpbuf.OK
}

func main() {
	parseFlags()
	parseCfg()
	fmt.Println(cfg)
	srvlog := log.New("module", "server")
	lvl, err := log.LvlFromString(cfg.LogLvl)
	if err != nil {
		panic(err)
	}
	srvlog.SetHandler(log.LvlFilterHandler(lvl, log.StdoutHandler))


	state := &ServerState{srvlog, 0}

	tcpLogger := log.New("module", "tcpbuf")
	//tcpLogger.SetHandler(log.)
	instance := tcpbuf.NewTCPBuf(tcpLogger, 128)
	serv := instance.NewServerListener(cfg.Host, cfg.Port,
										testServCB, cfg.InSize, cfg.OutSize,
										state)
	serv.StartServer()
	state.logger.Info("starting", "host", cfg.Host, "port", cfg.Port,
		"loop ms", cfg.MsLoop, "inSize", cfg.InSize, "outSize", cfg.OutSize)
	msLoop := time.Duration(cfg.MsLoop) * time.Millisecond
	for {
		instance.Loop(msLoop)
	}
}
