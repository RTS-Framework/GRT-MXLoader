package dotnet

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"

	"github.com/RTS-Framework/GRT-MXLoader/loader"
)

func TestCreateInstance(t *testing.T) {
	image := loader.NewFile("DotNET.dat")

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
		template, err := os.ReadFile("../../dist/standard/DotNET_x86.bin")
		require.NoError(t, err)
		opts := Options{
			Template: template,
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("with command line", func(t *testing.T) {
		opts := Options{
			CommandLine: "-p1 123 -p2 456",
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("with method caller", func(t *testing.T) {
		opts := Options{
			Class:    "Test",
			Method:   "Method0",
			Argument: "arg0 arg1",
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("with wait", func(t *testing.T) {
		opts := Options{
			Wait: true,
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
	})

	t.Run("ignore instantiate options", func(t *testing.T) {
		template, err := os.ReadFile("../../dist/pipeline/DotNET_x86.bin")
		require.NoError(t, err)
		opts := Options{
			Template:       template,
			IgnoreInstOpts: true,
		}

		inst, err := CreateInstance("386", image, &opts)
		require.NoError(t, err)
		require.NotNil(t, inst)
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
}
