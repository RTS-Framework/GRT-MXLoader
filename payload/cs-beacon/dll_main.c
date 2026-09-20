#include "c_types.h"
#include "win_types.h"
#include "dll_kernel32.h"

// a fake Cobalt-Strike stage for test

__declspec(dllimport)
void __stdcall ExitProcess(UINT uExitCode);

__declspec(dllimport)
void __stdcall Sleep(DWORD dwMilliseconds);

#pragma comment(linker, "/ENTRY:DllMain")
BOOL DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    // special reason for boot stage
    if (dwReason == 4) 
    {
        Sleep(1000);
        ExitProcess(0);
    }
    return true;
}
