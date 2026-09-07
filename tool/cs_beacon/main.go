package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"github.com/RTS-Framework/GRT-MXLoader/loader/cs-beacon"
)

var (
	version string
	input   string
	output  string
)

func init() {
	flag.StringVar(&version, "v", "", "specify the version, default is 4.0")
	flag.StringVar(&input, "i", "beacon.exe", "stageless beacon exe file path")
	flag.StringVar(&output, "o", "stage.dll", "path for save stage dll file")
	flag.Parse()
}

func main() {
	data, err := os.ReadFile(input) // #nosec
	checkError(err)
	dll, err := beacon.ExtractStage(version, data)
	checkError(err)
	err = os.WriteFile(output, dll, 0600) // #nosec
	checkError(err)
	fmt.Println("extract Cobalt-Strike beacon stage successfully")
}

func checkError(err error) {
	if err != nil {
		log.Fatalln(err)
	}
}
