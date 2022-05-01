package main

import (
	"encoding/json"
	"flag"
	"fmt"
	log "gopkg.in/inconshreveable/log15.v2"
	"io/ioutil"
	"musicplatform/tcpbuf"
	"os"
)

type SrvCFG struct {
	DbServer string `json:"db_server"`
	DbPort   int    `json:"db_port"`
	DbName   string `json:"db_name"`
	DbUser   string `json:"db_user"`
	DbPass   string `json:"db_pass"`
	LogLvl   string `json:"log_lvl"`
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
	logger := userCtx.(log.Logger)
	switch evt.Kind {
	case tcpbuf.READ:
		logger.Info(fmt.Sprintf("recv: %s", string(evt.Data)))
		return tcpbuf.RetType(len(evt.Data))
	case tcpbuf.WRITE:
		//evt.Data[1] = '1'
		//return 1
	case tcpbuf.CLOSING:
		logger.Info(fmt.Sprintf("closing %s", evt.Addr.String()))
	}
	return tcpbuf.OK
}

func testServCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	logger := userCtx.(log.Logger)
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
		evt.Instance.AcceptStream(logger, testStreamCB)
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
	srvlog.Info("test msg", "path", "test")
	srvlog.Debug("test msg", "path", "test")
	srvlog.Info(fmt.Sprintf("%d", tcpbuf.OK))

	tcpLogger := log.New("module", "tcpbuf")
	instance := tcpbuf.NewTCPBuf(tcpLogger)
	serv := instance.NewServerListener("192.168.1.30", 3018,
										testServCB, 65535, 65535,
										srvlog)
	serv.StartServer()

	for {
		instance.Loop()
	}
}
