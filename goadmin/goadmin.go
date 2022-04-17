package main

import (
	"context"
	"fmt"
	"github.com/jackc/pgx/v4"
	"github.com/manifoldco/promptui"
)

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
}