package dotnet

import (
	"bytes"
	"embed"
	"fmt"

	"github.com/RTS-Framework/GRT-Develop/argument"
	"github.com/RTS-Framework/GRT-Develop/instance"
	"github.com/RTS-Framework/GRT-Develop/types"
	"github.com/RTS-Framework/GRT-MXLoader/loader"
)

// just for prevent [import _ "embed"] :)
var _ embed.FS

var (
	//go:embed template/DotNET_x86.bin
	defaultTemplateX86 []byte

	//go:embed template/DotNET_x64.bin
	defaultTemplateX64 []byte
)

// Options contains options about create instance.
type Options struct {
	// set the custom loader template, it only lacked argument stub.
	Template []byte `toml:"template" json:"template"`

	// set the command line argument about the image.
	CommandLine string `toml:"cmd_line" json:"cmd_line"`

	// set the export class name for dll.
	Class string `toml:"class" json:"class"`

	// set the export method name for dll.
	Method string `toml:"method" json:"method"`

	// set argument for call the dll export method.
	Argument string `toml:"argument" json:"argument"`

	// wait Main() or target function execute finish.
	Wait bool `toml:"wait" json:"wait"`

	// ignore instantiate options about runtime.
	IgnoreInstOpts bool `toml:"ignore_inst_opts" json:"ignore_inst_opts"`

	// set instantiate options about runtime.
	Runtime instance.Options `toml:"runtime" json:"runtime"`
}

// CreateInstance is used to create instance from template.
func CreateInstance(arch string, image loader.Payload, opts *Options) ([]byte, error) {
	if opts == nil {
		opts = new(Options)
	}
	// encode .NET image
	peImage, err := image.Encode()
	if err != nil {
		return nil, fmt.Errorf("invalid %s mode config: %s", image.Mode(), err)
	}
	// process command line
	var (
		cmdLine []byte
		class   []byte
		method  []byte
		mArg    []byte
	)
	if opts.CommandLine != "" {
		cmdLine = types.StringToUTF16(opts.CommandLine)
	}
	if opts.Class != "" {
		class = []byte(opts.Class)
	}
	if opts.Method != "" {
		method = []byte(opts.Method)
	}
	if opts.Argument != "" {
		mArg = types.StringToUTF16(opts.Argument)
	}
	// process wait flag
	wait := encodeToBOOL(opts.Wait)
	// select loader template
	var defaultTemplate []byte
	switch arch {
	case "386":
		defaultTemplate = defaultTemplateX86
	case "amd64":
		defaultTemplate = defaultTemplateX64
	default:
		return nil, fmt.Errorf("invalid architecture: %s", arch)
	}
	template := opts.Template
	if template == nil {
		template = defaultTemplate
	}
	// create instance
	inst, err := instantiateFromTemplate(opts, template)
	if err != nil {
		return nil, err
	}
	// encode arguments at tail of instance
	args := []*argument.Arg{
		{ID: 1, Data: peImage},
		{ID: 2, Data: cmdLine},
		{ID: 3, Data: class},
		{ID: 4, Data: method},
		{ID: 5, Data: mArg},
		{ID: 6, Data: wait},
	}
	stub, err := argument.Encode(args...)
	if err != nil {
		return nil, fmt.Errorf("failed to encode argument: %s", err)
	}
	return append(inst, stub...), nil
}

func encodeToBOOL(b bool) []byte {
	if b {
		return []byte{1, 0, 0, 0}
	}
	return []byte{0, 0, 0, 0}
}

func instantiateFromTemplate(opts *Options, template []byte) ([]byte, error) {
	if opts.IgnoreInstOpts {
		return bytes.Clone(template), nil
	}
	instOpts := opts.Runtime
	instOpts.SkipArguments = true
	return instance.Instantiate(template, &instOpts)
}
