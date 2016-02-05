package cmd // code.bitsetter.de/fun/gosl/cmd

import (
	"encoding/gob"
	"fmt"
	"log"
	"net"
	"sort"
	"time"

	"github.com/spf13/cobra"
)

var cmdServer = &cobra.Command{
	Use:   "server",
	Short: "Runs Gosl as a server",
	Long: `Runs Gosl as a server

[TODO:]
gosl.json for configuration (Port/Adress, Level)
`,
	//	Run:
}

type goslClient struct {
	con *net.TCPConn
	id  int
	w   int
	h   int
}

var TotalWidth int = 0
var clients map[int]goslClient = make(map[int]goslClient)
var clientKeys []int = make([]int, 100)

func handleConn(conn *net.TCPConn) {
	var hs handshake

	log.Println("Got a connection!")

	// handshake
	dec := gob.NewDecoder(conn) // Decoder
	dec.Decode(&hs)
	log.Println("Got client! ID:", hs.ID, "dimensions:", hs.W, hs.H)
	clientKeys = append(clientKeys, hs.ID)
	sort.Ints(clientKeys)
	clients[hs.ID] = goslClient{con: conn, id: hs.ID, w: hs.W, h: hs.H}
	TotalWidth += hs.W

	// conn.Close()
}

func serveClients() {
	for { // while true

		for _, k := range clientKeys {
			id, client := k, clients[k]
			if id > 0 {
				fmt.Println("ID:", id, "Client:", client)
			}
		}
		time.Sleep(time.Second)

	}
}

func runServer(cmd *cobra.Command, args []string) {
	fmt.Println("running server ...")

	listener, err := net.ListenTCP("tcp", &net.TCPAddr{Port: SERVERPORT})
	if err != nil {
		log.Fatal(err)
		panic("Could not open Listener")
	}
	defer listener.Close()
	go serveClients()
	for {
		conn, err := listener.AcceptTCP()
		if err != nil {
			log.Fatal(err)
			panic("Listener could not accept connection!")
		}
		go handleConn(conn)
	}

}

func init() {
	cmdServer.Run = runServer
}