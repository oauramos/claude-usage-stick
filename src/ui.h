#pragma once
#include "api.h"
#ifdef MANGO_UI
#include "status.h"
#endif

void uiInit();
void uiBootProgress(int percent, const char* label);
void uiSetupScreen(const char* apName, const char* apPass, bool reconfigure = false);
void uiPinScreen(int pos, const int digits[4]);
void uiConnecting(const char* ssid, int attempt = 0);
void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct);
// Lightweight in-place update of the clock + reset countdowns (no bars, no full clear)
// so the periodic refresh doesn't flicker. Call when only time has passed, not data.
void uiDashboardClock(const UsageData& data, unsigned long lastFetchMs, int rssi);
void uiError(const char* title, const char* detail = nullptr);
// Lockout is drawn in two parts so the caller owns the wait: Static paints the
// chrome once, Tick repaints the remaining seconds. This lets the wait loop be
// sliced (and, on the T-Display S3, pump the web panel between slices).
void uiLockoutStatic(int attempts, int maxAttempts, int lockoutSec);
void uiLockoutTick(int secondsLeft);
#ifdef BOARD_TDISPLAY_S3
// LAN URL shown on the PIN screen and alternated into the dashboard header.
void uiSetNetInfo(const char* url);
// Header label (from dev_name, uppercased); empty falls back to "CLAUDE USAGE".
void uiSetHeaderLabel(const char* name);
#endif
#ifdef MANGO_UI
// Latest model health for the dashboard's mascot row; cached until the next call.
void uiSetModelStatus(const ModelStatus& s);
// Apply (or undo) the 180° flip and clear; caller redraws the current screen.
void uiApplyRotation(bool flipped);
// Close (true) or open (false) the healthy mascots' eyes on the dashboard.
void uiBlinkTick(bool closed);
#endif // MANGO_UI
