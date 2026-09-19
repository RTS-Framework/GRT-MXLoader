package dotnet

import (
	"bytes"
	"embed"
	"errors"
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

	// wait Main() or target function execute finish.
	Wait bool `toml:"wait" json:"wait"`

	// ignore instantiate options about runtime.
	IgnoreInstOpts bool `toml:"ignore_inst_opts" json:"ignore_inst_opts"`

	// set instantiate options about runtime.
	Runtime instance.Options `toml:"runtime" json:"runtime"`

	// set additional arguments for upper image.
	// all the ID must greater than 64.
	Arguments []*argument.Arg `toml:"arguments" json:"arguments"`
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
	var cmdLine []byte
	if opts.CommandLine != "" {
		cmdLine = types.StringToUTF16(opts.CommandLine)
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
		{ID: 3, Data: wait},
	}
	// process additional arguments
	for _, arg := range opts.Arguments {
		if arg.ID <= 64 {
			return nil, errors.New("additional argument id must greater than 64")
		}
		args = append(args, arg)
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
