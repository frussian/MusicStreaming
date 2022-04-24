package main

import (
	"context"
	"errors"
	"fmt"
	"github.com/jackc/pgx/v4"
	"github.com/manifoldco/promptui"
	"io"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

//ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "Nirvana - Lithium (Alt Take).flac"
//ffmpeg -i WholeLottaRosie.mp3 -c:a libopus -b:a 48k -vbr:a off wlr.opus
//select interval '0 hours 2 minutes 54 seconds';

const (
	RAW_DIR_PREFIX = "raw_tmp"
)

type SongInfo struct {
	index int
	name string
	hours, min, sec int
}

func getSongInfo(path string) (int, int, int, error) {
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
		return 0, 0, 0, err
	}
	lengthStr := string(lengthBytes)
	lengthStr = strings.ReplaceAll(lengthStr, "\n", "")
	lengthStr = strings.ReplaceAll(lengthStr, "\r", "")
	lengthStr = strings.Split(lengthStr, ".")[0]
	sec, err := strconv.ParseInt(lengthStr, 10, 32)
	if err != nil {
		return 0, 0, 0, err
	}
	hour := sec / 3600
	sec -= hour * 3600
	min := sec / 60
	sec -= min * 60
	return int(hour), int(min), int(sec), nil
}

func genOpus(name, path string) error {
	args := []string{"cmd", "/C", "ffmpeg", "-i", path,
		"-c:a", "libopus", "-b:a", "48k", "-vbr:a", "off",
		RAW_DIR_PREFIX + "/" + name + ".opus"}

	cmd := exec.Command(args[0], args[1:]...)
	out, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println(string(out))
		return err
	}
	return nil
}

func processSong(index int, name, path string) (SongInfo, error) {
	hour, min, sec, err := getSongInfo(path)
	if err != nil {
		return SongInfo{}, err
	}
	err = genOpus(name, path)
	if err != nil {
		return SongInfo{}, err
	}
	return SongInfo{index, name, hour, min, sec}, nil
}

func processDir(dir string) ([]SongInfo, error) {
	songInfos := make([]SongInfo, 0, 8)
	err := filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		name := info.Name()
		if strings.Contains(name, ".mp3") ||
			strings.Contains(name, ".wav") {
			indexStr := strings.Split(name, "_")[0]
			index, err := strconv.ParseInt(indexStr, 10, 32)
			if err != nil {
				return errors.New(name + " should be prefixed with index")
			}
			info, err := processSong(int(index), name[len(indexStr)+1:len(name)-4], path)
			if err != nil {
				return err
			}
			songInfos = append(songInfos, info)
		}
		return nil
	})
	return songInfos, err
}

func insertSongToDB(tx pgx.Tx, albumID int, song SongInfo) error {
	los := tx.LargeObjects()

	objID, err := los.Create(context.Background(), 0)
	if err != nil {
		return err
	}
	lo, err := los.Open(context.Background(), objID, pgx.LargeObjectModeWrite)

	file, err := os.Open(RAW_DIR_PREFIX +"/"+song.name+".opus")
	if err != nil {
		return err
	}
	defer file.Close()

	length := 1024
	buf := make([]byte, length)
	for {
		read, err := file.Read(buf)
		if err != nil && err != io.EOF {
			return err
		}
		if read == 0 {
			break
		}

		_, err = lo.Write(buf[:read])
		if err != nil {
			return err
		}
	}

	insertSong := `insert into Song(length, index, data, songName, albumID)
values (interval '%d hours, %d minutes %d seconds', $1, $2, $3, $4);`
	insertSong = fmt.Sprintf(insertSong, song.hours, song.min, song.sec)
	_, err = tx.Exec(context.Background(), insertSong,
					 song.index, objID, song.name, albumID)

	return nil
}

func insertAlbumToDB(conn *pgx.Conn, album, band string, releaseDate time.Time, songs []SongInfo) error {
	var bandID, albumID int
	err := conn.QueryRow(context.Background(), "select bandID from Band" +
		" where BandName = $1;", band).Scan(&bandID)
	if err != nil {
		return err
	}

	insertAlbum := `insert into Album(releaseDate, title, BandID)
values ($1, $2, $3);`
	getAlbumID := `select albumID from album where title=$1;`
	err = conn.BeginFunc(context.Background(), func(tx pgx.Tx) error {
		_, err = tx.Exec(context.Background(), insertAlbum, releaseDate, album, bandID)
		if err != nil {
			return err
		}
		err = tx.QueryRow(context.Background(), getAlbumID, album).Scan(&albumID)
		if err != nil {
			return err
		}
		for _, song := range songs {
			err = insertSongToDB(tx, albumID, song)
			if err != nil {
				return err
			}
		}
		return nil
	})

	return err
}

func addAlbum(conn *pgx.Conn) error {
	prompt := promptui.Prompt{}
	prompt.Label = "Enter band name"
	band, err := prompt.Run()
	if err != nil {
		return err
	}

	prompt.Label = "Enter album path"
	dir, err := prompt.Run()
	if err != nil {
		return err
	}
	album := path.Base(dir)

	prompt.Label = "Enter release date(dd-mm-yyyy)"
	prompt.Validate = func(s string) error {
		_, err := time.Parse(TIME_FORMAT, s)
		return err
	}
	releaseDateStr, err := prompt.Run()
	if err != nil {
		return err
	}
	releaseDate, _ := time.Parse(TIME_FORMAT, releaseDateStr)

	err = os.Mkdir(RAW_DIR_PREFIX, 0777)
	if err != nil {
		return err
	}
	defer os.RemoveAll(RAW_DIR_PREFIX)
	songs, err := processDir(dir)
	if err != nil {
		return err
	}

	err = insertAlbumToDB(conn, album, band, releaseDate, songs)
	return err
}

//func main() {
//	logger := log15adapter.NewLogger(log.New("module", "pgx"))
//
//	url := "postgres://anton:<pass>@localhost:5432/MusicDB"
//	cfg, err := pgx.ParseConfig(url)
//	if err != nil {
//		panic(err)
//	}
//
//	cfg.Logger = logger
//
//	conn, err := pgx.ConnectConfig(context.Background(), cfg)
//	if err != nil {
//		panic(err)
//	}
//	err = addAlbum(conn)
//	if err != nil {
//		panic(err)
//	}
//}