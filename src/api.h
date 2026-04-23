#pragma once
#include <stdint.h>

struct UsageData {
    float    h5;
    float    d7;
    uint32_t h5ResetEpoch;
    uint32_t d7ResetEpoch;
    bool     ok;
    char     error[64];
};

bool fetchUsage(const char* token, UsageData& out);
