#pragma once

// ── Polling ──────────────────────────────────────────────
#define DEFAULT_POLL_SEC        120
#define MIN_POLL_SEC            30
#define MAX_POLL_SEC            300

// ── Security ─────────────────────────────────────────────
#define MAX_PIN_ATTEMPTS        10
#define LOCKOUT_BASE_SEC        60       // doubles each failure
#define KDF_ROUNDS              10000

// ── Display ──────────────────────────────────────────────
#ifdef BOARD_TDISPLAY_S3
  #define SCREEN_W              320
  #define SCREEN_H              170
  #define SCREEN_ROT            1
#elif defined(BOARD_TDISPLAY_S3_AMOLED)
  #define SCREEN_W              536
  #define SCREEN_H              240
  #define SCREEN_ROT            0
#else
  #define SCREEN_W              240
  #define SCREEN_H              135
  #define SCREEN_ROT            3
#endif
#define DEFAULT_BRIGHTNESS      2        // 0=off 1=dim 2=normal 3=bright

// ── Network ──────────────────────────────────────────────
#define WIFI_CONNECT_TIMEOUT_S  20
#define API_TIMEOUT_MS          15000
#define MESSAGES_ENDPOINT       "https://api.anthropic.com/v1/messages"
#define ANTHROPIC_VERSION       "2023-06-01"
#define PROBE_MODEL             "claude-haiku-4-5-20251001"

// ── NVS ──────────────────────────────────────────────────
#define NVS_NAMESPACE           "claude"
