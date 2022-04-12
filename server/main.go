package main

import (
	"database/sql"
	"fmt"
	_ "github.com/jackc/pgx/v4/stdlib"
)

func main() {
	connStr := "user=anton password=<pass> dbname=MusicDB sslmode=disable"
	db, err := sql.Open("pgx", connStr)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	result, err := db.Query("select * from musician;")
	if err != nil {
		panic(err)
	}
	fmt.Println(result)
	for result.Next() {
		var id int
		var dateOfBirth, bio, name string
		result.Scan(&id, &dateOfBirth, &bio, &name)
		fmt.Println(id, dateOfBirth, bio, name)
	}
}
