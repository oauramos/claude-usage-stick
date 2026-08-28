#pragma once
#include <stdint.h>
#include <stddef.h>
#include "crypto.h"

// All NVS access lives here (namespace "claude", NVS_NAMESPACE in config.h).
// v1/v2 keys keep their original Preferences types — poll_sec and brightness
// are i32 — because writing a different type to an existing key fails with
// ESP_ERR_NVS_TYPE_MISMATCH. New keys added in v3 use u8/i32 as noted.
struct Settings {
    char          ssid[33];
    char          wifipass[65];
    EncryptedBlob blob;
    bool          hasBlob;
    int32_t       pollSec;      // i32, 30..300
    uint8_t       brightness;   // stored as i32, 0..3
    char          devName[33];
    int32_t       tzMin;        // i32, minutes east of UTC, -840..840
    uint8_t       flip;         // u8, persisted 180° screen rotation
    uint8_t       uiMode;       // u8, 0=static (dash) 1=carousel 2=clock
    uint8_t       dwellS;       // u8, carousel dwell: 5/10/15/30 s
    uint8_t       scrMask;      // u8, carousel screen set (bit per Screen)
    uint8_t       mdlMask;      // u8, which model mascots render (bit per model)
};

bool settingsIsProvisioned();
void settingsLoad(Settings& s);
void settingsPutInt(const char* key, int32_t v);
void settingsPutU8(const char* key, uint8_t v);
void settingsPutStr(const char* key, const char* v);
void settingsPutBlob(const char* key, const void* data, size_t len);
void settingsWipeAll();

// Sets the process TZ from an east-of-UTC minute offset. POSIX inverts the
// sign (TZ=GMT-3 means UTC+3); the inversion happens here so callers think in
// normal GMT± terms. Fixed offset only, no DST rules.
void settingsApplyTZ(int32_t tzMin);

// Device name → mDNS/DHCP hostname: lowercase [a-z0-9-], symbol runs collapsed
// to one dash. Empty or the portal default ("Claude Monitor") falls back to
// "claude-usage-stick".
void slugifyHostname(const char* devName, char* out, size_t outLen);
