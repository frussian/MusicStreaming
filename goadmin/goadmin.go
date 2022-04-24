package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgx/v4"
	"github.com/manifoldco/promptui"
	"time"
)

const (
	TIME_FORMAT = "02-01-2006" //dd-mm-yyyy
)

func getInstruments(conn *pgx.Conn) []string {
	var instruments []string
	rows, err := conn.Query(context.Background(), "select unnest(enum_range(NULL::instr_enum));")
	if err != nil {
		panic(err)
	}
	for rows.Next() {
		instr := ""
		err = rows.Scan(&instr)
		if err != nil {
			panic(err)
		}
		instruments = append(instruments, instr)
	}
	return instruments
}

func addMusician(conn *pgx.Conn) error {
	prompt := promptui.Prompt{
		Label:       "Enter musician name",
	}
	name, err := prompt.Run()
	if err != nil {
		panic(err)
	}

	prompt.Label = "Enter birth date(dd-mm-yyyy)"
	prompt.Validate = func(s string) error {
		_, err := time.Parse(TIME_FORMAT, s)
		return err
	}
	dateStr, err := prompt.Run()
	date, err := time.Parse(TIME_FORMAT, dateStr)
	fmt.Println(date)

	prompt.Label = "Enter biography"
	prompt.Validate = nil
	bio, _ := prompt.Run()

	availInstrs := getInstruments(conn)

	prompt.Label = "more instruments?"
	prompt.IsConfirm = true

	selectPrompt := promptui.Select{
		Label:             "choose instrument",
		Items:             availInstrs,
	}

	instruments := make([]string, 0, len(availInstrs))
	for {
		i, _, _ := selectPrompt.Run()
		instruments = append(instruments, availInstrs[i])
		next, _ := prompt.Run()
		if next != "" && next != "y" {
			break
		}
	}

	instrStr := "{"
	for i, instr := range instruments {
		if i == len(instruments) - 1 {
			instrStr += instr + "}"
		} else {
			instrStr += instr + ","
		}
	}

	_, err = conn.Exec(context.Background(),
		"call insert_musician($1, $2, $3, $4);", name, bio, date, instrStr)
	return err
}

func addInstrument(conn *pgx.Conn) error {
	prompt := promptui.Prompt {
		Label:       "Enter musician name",
	}
	name, err := prompt.Run()
	if err != nil {
		panic(err)
	}

	availInstrs := getInstruments(conn)

	prompt.Label = "more instruments?"
	prompt.IsConfirm = true
	selectPrompt := promptui.Select{
		Label:             "choose instrument",
		Items:             availInstrs,
	}
	instruments := make([]string, 0, len(availInstrs))

	for {
		i, _, _ := selectPrompt.Run()
		instruments = append(instruments, availInstrs[i])
		next, _ := prompt.Run()
		if next != "" && next != "y" {
			break
		}
	}

	stmt := "insert into Instrument(musicianID, type) values" +
		"((select musicianID from musician where musicianName=$1), $2);"
	for _, instr := range instruments {
		_, err = conn.Exec(context.Background(), stmt, name, instr)
		if err != nil {
			return err
		}
	}
	return nil
}

func main() {
	prompt := promptui.Prompt{
		Label:    "server",
		Default: "localhost",
	}
	server, err := prompt.Run()
	if err != nil {
		fmt.Printf("Prompt failed %v\n", err)
		return
	}

	prompt = promptui.Prompt{
		Label:    "port",
		Default: "5432",
	}
	port, err := prompt.Run()
	if err != nil {
		fmt.Printf("Prompt failed %v\n", err)
		return
	}

	prompt = promptui.Prompt{
		Label:    "database",
		Default: "MusicDB",
	}
	db, err := prompt.Run()
	if err != nil {
		fmt.Printf("Prompt failed %v\n", err)
		return
	}

	prompt = promptui.Prompt{
		Label:    "user",
		Default: "postgres",
	}
	user, err := prompt.Run()
	if err != nil {
		fmt.Printf("Prompt failed %v\n", err)
		return
	}

	prompt = promptui.Prompt{
		Label:       "password",
		Mask:        '*',
		HideEntered: true,
	}
	pass, err := prompt.Run()
	if err != nil {
		fmt.Printf("Prompt failed %v\n", err)
		return
	}

	url := fmt.Sprintf("postgres://%s:%s@%s:%s/%s", user, pass, server, port, db)
	//fmt.Println(url)

	conn, err := pgx.Connect(context.Background(), url)
	if err != nil {
		panic(err)
	}

	commands := []string{"add musician", "add instrument", "add album", "exit"}
	options := promptui.Select{
		Label:             "choose action",
		Items:             commands,
		CursorPos:         0,
		Templates:         nil,
		Searcher:          nil,
		StartInSearchMode: false,
		Pointer:           nil,
		Stdin:             nil,
		Stdout:            nil,
	}

	exit := false
	for {
		i, _, err := options.Run()
		if err != nil {
			panic(err)
		}
		switch i {
		case 0:
			err = addMusician(conn)
			if err != nil {
				fmt.Println(err)
			} else {
				fmt.Println("successful")
			}
		case 1:
			err = addInstrument(conn)
			if err != nil {
				fmt.Println(err)
			} else {
				fmt.Println("successful")
			}
		case 2:
			err = addAlbum(conn)
			if err != nil {
				fmt.Println(err)
			} else {
				fmt.Println("successful")
			}
		case len(commands) - 1:
			exit = true
		}
		if exit {
			break
		}
	}

	conn.Close(context.Background())
}