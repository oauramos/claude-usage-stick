#pragma once
#include <stdint.h>

// Reported instead of a percentage for a window the account doesn't have. Each
// window can be absent on its own: verified Ultra accounts have no 5-hour limit.
constexpr float USAGE_NO_WINDOW = -1.0f;

struct UsageData {
    float    h5;   // 0-100, or USAGE_NO_WINDOW
    float    d7;   // 0-100, or USAGE_NO_WINDOW
    uint32_t h5ResetEpoch;
    uint32_t d7ResetEpoch;
    bool     ok;
    char     error[64];
};

bool fetchUsage(const char* token, UsageData& out);
