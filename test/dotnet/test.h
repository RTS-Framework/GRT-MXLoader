#ifndef TEST_H
#define TEST_H

#include "build.h"
#include "c_types.h"
#include "runtime.h"
#include "pe_loader.h"

// define unit tests
#pragma warning(push)
#pragma warning(disable: 4276)
bool TestInit();
bool TestEXE();
bool TestDLL();
#pragma warning(pop)

typedef bool (*test_t)();
typedef struct { byte* Name; test_t Test; } unit;

static unit tests[] = 
{
    { "Init", TestInit },
    { "EXE",  TestEXE  },
    { "DLL",  TestDLL  },
};

#endif // TEST_H
