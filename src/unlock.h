#pragma once
#ifdef ESP8266
#include <stddef.h>
#include "crypto.h"

// Button-less boot unlock for boards with no physical buttons (GeekMagic SmallTV).
// The device is already on WiFi; this hosts a one-field PIN page (mDNS claude-tv.local)
// and shows the URL on the TFT, blocking until the entered PIN decrypts the token into
// tokenOut. Reuses the same lockout/attempt-cap policy as the button enterPin() path.
// Returns true once unlocked (it does not return otherwise — wipes + reboots after the
// attempt cap, mirroring main.cpp).
bool runWebUnlock(const EncryptedBlob& blob, char* tokenOut, size_t maxLen);
#endif
