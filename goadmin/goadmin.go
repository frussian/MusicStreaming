package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgx/v4"
	"github.com/manifoldco/promptui"
	"time"
)

func addMusician(conn *pgx.Conn) {
	timeFormat := "02-01-2006"
	prompt := promptui.Prompt{
		Label:       "Enter musician name",
		IsConfirm:   false,
	}
	_, err := prompt.Run()
	if err != nil {
		panic(err)
	}

	prompt.Label = "Enter birth date(dd-mm-yyyy)"
	prompt.Validate = func(s string) error {
		_, err := time.Parse(timeFormat, s)
		return err
	}
	dateStr, err := prompt.Run()
	date, err := time.Parse(timeFormat, dateStr)
	fmt.Println(date)
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
		return
	}
	fmt.Println(conn)

	commands := []string{"add musician", "add instrument",}
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

	for {
		i, _, err := options.Run()
		if err != nil {
			panic(err)
		}
		fmt.Println(i)
		if i == 0 {
			addMusician(conn)
		}
		break
	}
}