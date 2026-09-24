#ifndef BOOT_H
#define BOOT_H

#include "errno.h"

#define MODE_EMBED_IMAGE 1
#define MODE_LOCAL_FILE  2
#define MODE_HTTP_SERVER 3

#define ARG_ID_PE_IMAGE  1
#define ARG_ID_CMDLINE   2
#define ARG_ID_CLASS     3
#define ARG_ID_METHOD    4
#define ARG_ID_ARGUMENT  5
#define ARG_ID_WAIT      6

#define EMBED_ENABLE_COMPRESSION  1
#define EMBED_DISABLE_COMPRESSION 0

#define ERR_NOT_FOUND_PE_IMAGE       0x7F00FF01
#define ERR_EMPTY_PE_IMAGE_DATA      0x7F00FF02
#define ERR_NOT_FOUND_CMDLINE        0x7F00FF03
#define ERR_NOT_FOUND_CLASS          0x7F00FF04
#define ERR_NOT_FOUND_METHOD         0x7F00FF05
#define ERR_NOT_FOUND_ARGUMENT       0x7F00FF06
#define ERR_NOT_FOUND_WAIT           0x7F00FF07
#define ERR_INVALID_WAIT             0x7F00FF08
#define ERR_INVALID_LOAD_MODE        0x7F00FF09
#define ERR_INVALID_EMBED_CONFIG     0x7F00FF0A
#define ERR_INVALID_COMPRESS_DATA    0x7F00FF0B
#define ERR_INVALID_PE_IMAGE         0x7F00FF0C
#define ERR_INVALID_HTTP_CONFIG      0x7F00FF0D
#define ERR_INVALID_HTTP_STATUS_CODE 0x7F00FF0E

#define CTX_TYPE_TEST 0x01

typedef struct {
    void* Image;
    void* CommandLine;
    void* Class;
    void* Method;
    void* Argument;
    BOOL  Wait;
} Config;

typedef struct {
    uint  Type;
    void* Image;
    void* CommandLine;
    void* Class;
    void* Method;
    void* Argument;
    void* Runtime;
} CTX_Test;

errno Boot(void* ctx);

#endif // BOOT_H
