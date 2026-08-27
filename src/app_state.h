#pragma once
#include "api.h"
#include "settings.h"
#ifdef MANGO_UI
#include "status.h"
#endif

// Shared app state, defined in main.cpp. The web panel reads and writes these
// from HTTP handlers; everything runs on the single Arduino loop task, so no
// locking is needed.
extern Settings      g_settings;
extern UsageData     g_usage;
#ifdef MANGO_UI
extern ModelStatus   g_models;
#endif
extern char          g_token[256];      // decrypted OAuth token, RAM only
extern bool          g_unlocked;        // PIN accepted (buttons or web)
extern unsigned long g_lastFetchMs;
