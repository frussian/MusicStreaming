package main

import (
	"encoding/json"
	"flag"
	"fmt"
	log "gopkg.in/inconshreveable/log15.v2"
	"io/ioutil"
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
	flag.StringVar(&cfgPath, "cfg", "server.cfg", "path to cfg file")
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

func main() {
	//connStr := "user=anton password=<pass> dbname=MusicDB sslmode=disable"
	parseFlags()
	parseCfg()
	fmt.Println(cfg)
	srvlog := log.New("module", "server")
	srvlog.SetHandler(log.LvlFilterHandler(log.LvlInfo, log.StdoutHandler))
	srvlog.Info("test msg", "path", "test")
	srvlog.Debug("test msg", "path", "test")
}
