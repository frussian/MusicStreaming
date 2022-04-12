package main

import (
	"fmt"

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

	fmt.Println(server, db, user, pass)
}