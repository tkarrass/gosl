package cmd // code.bitsetter.de/fun/gosl/cmd

import (
	"encoding/gob"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"strconv"
	"syscall"

	"github.com/spf13/cobra"

	"code.bitsetter.de/fun/gosl/data"
)

var cmdClient = &cobra.Command{
	Use:   "client <NUMBER>",
	Short: "Runs Gosl as a client",
	Long:  "Runs Gosl as a client",
	//Run:   runClient,
}

var (
	ServerHost  string
	renderQueue = make(chan data.Frame)
)

func register(con net.Conn, id int) error {
	w, h := data.GetXY()
	enc := gob.NewEncoder(con) // Encoder
	err := enc.Encode(data.Handshake{ID: id, H: h, W: w})
	return err
}

func render() {
	var f data.Frame
	for f = range renderQueue {
		data.RenderFrame(&f)
	}
}

func runClient(cmd *cobra.Command, args []string) {
	osChan := make(chan os.Signal)
	signal.Notify(osChan, os.Interrupt, os.Kill)
	signal.Notify(osChan, syscall.SIGTERM)

	//fmt.Println("Running client ...")

	if len(args) < 1 {
		fmt.Println("Usage: gosl", cmdClient.Use)
		os.Exit(1)
	}
	id, err := strconv.Atoi(args[0])
	if err != nil {
		fmt.Println("Error parsing client ID!")
		os.Exit(1)
	}

	con, err := net.Dial("tcp", ServerHost+":"+strconv.Itoa(ServerPort))
	if err != nil {
		fmt.Println("CONNECT ERROR:", err)
		os.Exit(1)
	}
	defer con.Close()
	register(con, id)

	data.InitNC(osChan)
	// really important???
	defer data.ExitNC()

	go render()
	run := true
	for run {
		var oFrame data.Frame
		gd := gob.NewDecoder(con) // always use new decoder
		err = gd.Decode(&oFrame)
		if err != nil {
			log.Println("RGS:", err)
			//run = false
			continue
		}
		renderQueue <- oFrame

		select {
		case _ = <-osChan:
			log.Println("Got a KILL")
			con.Close()
			close(renderQueue)
			run = false
		default:
			if data.GetChar() != 0 {
				data.ExitNC()
			}
		}
	}

}

func init() {
	CmdGosl.AddCommand(cmdClient)
	cmdClient.Run = runClient

	cmdClient.Flags().StringVarP(&ServerHost, "host", "H", "127.0.0.1", "Connect to this server")
	cmdClient.Flags().IntVarP(&ServerPort, "port", "p", 8090, "Connect to server on this port")
}
