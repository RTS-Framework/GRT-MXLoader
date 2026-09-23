#include <stdio.h>
#include "build.h"
#include "c_types.h"
#include "win_types.h"
#include "lib_memory.h"
#include "lib_string.h"
#include "dotnet/boot.h"
#include "dotnet/boot.c"
#include "test.h"

static Runtime_M* initRuntime()
{
    Runtime_Opts opts = {
        .NotEraseInstruction = true,
    };
    Runtime_M* runtime = InitRuntime(NULL, &opts);
    if (runtime == NULL)
    {
        printf_s("failed to initialize runtime: 0x%X\n", GetLastErrno());
        return NULL;
    }
    return runtime;
}

bool TestEXE()
{
    Runtime_M* runtime = initRuntime();
    if (runtime == NULL)
    {
        return false;
    }

    // load image for test
    LPSTR path = "..\\..\\loader\\dotnet\\testdata\\dotnet_exe.dat";
    databuf image;
    errno err = runtime->WinFile.ReadFileA(path, &image);
    if (err != NO_ERROR)
    {
        printf_s("failed to open image: 0x%X\n", err);
        return false;
    }

    // build test context
    LPWSTR cmdline = L"-p1 123 -p2 abc";
    CTX_Test ctx = {
        .Type        = CTX_TYPE_TEST,
        .Image       = image.buf,
        .CommandLine = cmdline,
        .Runtime     = runtime,
    };

    // boot with test context
    err = Boot(&ctx);
    if (err != NO_ERROR)
    {
        printf_s("failed to boot: 0x%X\n", err);
        return false;
    }
    return true;
}

bool TestDLL()
{
    Runtime_M* runtime = initRuntime();
    if (runtime == NULL)
    {
        return false;
    }

    // load image for test
    LPSTR path = "..\\..\\loader\\dotnet\\testdata\\dotnet_dll.dat";
    databuf image;
    errno err = runtime->WinFile.ReadFileA(path, &image);
    if (err != NO_ERROR)
    {
        printf_s("failed to open image: 0x%X\n", err);
        return false;
    }

    // build test context
    CTX_Test ctx = {
        .Type    = CTX_TYPE_TEST,
        .Image   = image.buf,
        .Runtime = runtime,
    };

    // boot with test context
    err = Boot(&ctx);
    if (err != NO_ERROR)
    {
        printf_s("failed to boot: 0x%X\n", err);
        return false;
    }
    return true;
}
