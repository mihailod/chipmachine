#pragma once

#include "rasmStructs.h"

extern "C" {
    int RasmAssemble(const char *datain, int lenin, unsigned char **dataout, int *lenout);
    int RasmAssembleInfo(const char *datain, int lenin, unsigned char **dataout, int *lenout, struct s_rasm_info **debug);
    void RasmFreeInfoStruct(struct s_rasm_info *debug);
}
