#include <stdio.h>
#include "build.h"
#include "c_types.h"
#include "win_types.h"
#include "lib_memory.h"
#include "lib_string.h"
#include "cs-beacon/boot.h"
#include "cs-beacon/boot.c"
#include "test.h"

Runtime_M* runtime;

bool TestInit()
{
    Runtime_Opts opts = {
        .NotEraseInstruction = true,
    };
    runtime = InitRuntime(NULL, &opts);
    if (runtime == NULL)
    {
        printf_s("failed to initialize runtime: 0x%X\n", GetLastErrno());
        return false;
    }
    return true;
}

bool TestBoot()
{
    // load fake stage for test
#ifdef _WIN64
    LPSTR path = "..\\..\\loader\\cs-beacon\\testdata\\stage_x64.dat";
#elif _WIN32
    LPSTR path = "..\\..\\loader\\cs-beacon\\testdata\\stage_x86.dat";
#endif
    databuf image;
    errno err = runtime->WinFile.ReadFileA(path, &image);
    if (err != NO_ERROR)
    {
        printf_s("failed to open fake stage: 0x%X\n", err);
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
