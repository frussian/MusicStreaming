package main

import (
	"fmt"
	"os"
	"os/exec"
	_ "os/exec"
	"path"
	_ "path"
	"path/filepath"
	"strconv"
	_ "strconv"
	"strings"
)

//ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "Nirvana - Lithium (Alt Take).flac"
//ffmpeg -i WholeLottaRosie.mp3 -c:a libopus -b:a 48k -vbr:a off wlr.opus
//select interval '0 hours 2 minutes 54 seconds';
const (
	rawDirPrefix = "raw_tmp"
)

type SongInfo struct {
	index int
	name string
	hours, min, sec int
}

func getSongInfo(path string) (int, int, int) {
	probe := "ffprobe -v error -show_entries " +
		"format=duration -of default=noprint_wrappers=1:nokey=1"
	pathArg := fmt.Sprintf("%s", path)
	args := []string{"cmd", "/C"}
	args = append(args, strings.Split(probe, " ")...)
	args = append(args, pathArg)

	cmd := exec.Command(args[0], args[1:]...)

	lengthBytes, err := cmd.Output()
	if err != nil {
		fmt.Println(string(lengthBytes))
		panic(err)
	}
	lengthStr := string(lengthBytes)
	lengthStr = strings.ReplaceAll(lengthStr, "\n", "")
	lengthStr = strings.ReplaceAll(lengthStr, "\r", "")
	lengthStr = strings.Split(lengthStr, ".")[0]
	sec, err := strconv.ParseInt(lengthStr, 10, 32)
	if err != nil {
		panic(err)
	}
	hour := sec / 3600
	sec -= hour * 3600
	min := sec / 60
	sec -= min * 60
	return int(hour), int(min), int(sec)
}

func genOpus(name, path string) {
	args := []string{"cmd", "/C", "ffmpeg", "-i", path,
		"-c:a", "libopus", "-b:a", "48k", "-vbr:a", "off",
		rawDirPrefix + "/" + name + ".opus"}

	cmd := exec.Command(args[0], args[1:]...)
	out, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println(string(out))
		panic(err)
	}

}

func processSong(index int, name, path string) SongInfo {
	hour, min, sec := getSongInfo(path)
	genOpus(name, path)
	fmt.Println(hour, min, sec)
	return SongInfo{index, name, hour, min, sec}
}

func main() {
	dir := "C:/Users/Anton/Desktop/BMSTU/CourseDB/albums/AFD"
	//album := path.Base(dir)
	err := os.Mkdir(rawDirPrefix, 0777)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer os.RemoveAll(path.Join(dir, rawDirPrefix))

	songInfos := make([]SongInfo, 0, 8)
	filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		name := info.Name()
		if strings.Contains(name, ".mp3") ||
			strings.Contains(name, ".wav") {
			indexStr := strings.Split(name, "_")[0]
			index, err := strconv.ParseInt(indexStr, 10, 32)
			if err != nil {
				panic(name + " should be prefixed with index")
			}
			info := processSong(int(index), name[len(indexStr)+1:len(name)-4], path)
			songInfos = append(songInfos, info)
		}
		return nil
	})
	fmt.Println(songInfos)
}