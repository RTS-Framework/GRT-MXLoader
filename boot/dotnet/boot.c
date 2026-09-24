#include "c_types.h"
#include "win_types.h"
#include "lib_memory.h"
#include "rel_addr.h"
#include "pe_image.h"
#include "errno.h"
#include "runtime.h"
#include "boot.h"

static errno loadConfig(Runtime_M* runtime, Config* config);
static void* loadImage(Runtime_M* runtime, byte* config);
static void* loadImageFromEmbed(Runtime_M* runtime, byte* config);
static void* loadImageFromFile(Runtime_M* runtime, byte* config);
static void* loadImageFromHTTP(Runtime_M* runtime, byte* config);

errno Boot(void* ctx)
{
    // process extended context
    CTX_Test* testCtx = NULL;
    if (ctx != NULL)
    {
        switch (*(uint*)ctx)
        {
        case CTX_TYPE_TEST:
            testCtx = ctx;
            break;
        default:
            break;
        }
    }

    // initialize Gleam-RT for PE Loader
    Runtime_M* runtime = NULL;
    if (testCtx == NULL)
    {
        runtime = InitRuntime(GetFuncAddr(&Boot), NULL);
        if (runtime == NULL)
        {
            return GetLastErrno();
        }
    } else {
        runtime = testCtx->Runtime;
    }

    // store boot configuration
    Config config;
    mem_init(&config, sizeof(config));
    // initialize PE Loader
    errno err = NO_ERROR;
    for (;;)
    {
        void* image = NULL;
        if (testCtx == NULL)
        {
            // load config from argument stub
            err = loadConfig(runtime, &config);
            if (err != NO_ERROR)
            {
                break;
            }
            // prepare pe image data to config
            image = loadImage(runtime, config.Image);
            if (image == NULL)
            {
                err = GetLastErrno();
                break;
            }
        } else {
            image = testCtx->Image;
            config.CommandLine = testCtx->CommandLine;
            config.Class       = testCtx->Class;
            config.Method      = testCtx->Method;
            config.Argument    = testCtx->Argument;
            config.Wait        = true;
        }
        // prepare .NET runtime



        runtime->Memory.Free(image);
        break;
    }
    if (err != NO_ERROR)
    {
        runtime->Core.Exit();
        return err;
    }


    // exit runtime
    errno ere = runtime->Core.Exit();
    if (ere != NO_ERROR && err == NO_ERROR)
    {
        err = ere;
    }
    return err;
}

static errno loadConfig(Runtime_M* runtime, Config* config)
{
    uint32 size;
    if (!runtime->Argument.GetPointer(ARG_ID_PE_IMAGE, &config->Image, &size))
    {
        return ERR_NOT_FOUND_PE_IMAGE;
    }
    if (size == 0)
    {
        return ERR_EMPTY_PE_IMAGE_DATA;
    }
    if (!runtime->Argument.GetPointer(ARG_ID_CMDLINE, &config->CommandLine, NULL))
    {
        return ERR_NOT_FOUND_CMDLINE;
    }
    if (!runtime->Argument.GetPointer(ARG_ID_CLASS, &config->Class, NULL))
    {
        return ERR_NOT_FOUND_CLASS;
    }
    if (!runtime->Argument.GetPointer(ARG_ID_METHOD, &config->Method, NULL))
    {
        return ERR_NOT_FOUND_METHOD;
    }
    if (!runtime->Argument.GetPointer(ARG_ID_ARGUMENT, &config->Argument, NULL))
    {
        return ERR_NOT_FOUND_ARGUMENT;
    }
    if (!runtime->Argument.GetValue(ARG_ID_WAIT, &config->Wait, &size))
    {
        return ERR_NOT_FOUND_WAIT;
    }
    if (size != sizeof(BOOL))
    {
        return ERR_INVALID_WAIT;
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
