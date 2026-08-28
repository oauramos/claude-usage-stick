#include "screens.h"

#ifdef BOARD_TDISPLAY_S3

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "app_state.h"
#include "hal.h"
#include "ui.h"
#include "history.h"
#include "news.h"

static Screen        s_current      = SCR_DASH;
static unsigned long s_advanceAtMs  = 0;   // next auto-advance (carousel)
static unsigned long s_pauseUntilMs = 0;
static unsigned long s_lastBeatMs   = 0;   // 10s header/redraw beat
static int           s_clockMinute  = -1;

static void armDwell() {
    s_advanceAtMs = millis() + (unsigned long)g_settings.dwellS * 1000UL;
}

void screensInit() {
    uiSetModelMask(g_settings.mdlMask);
    s_current = (g_settings.uiMode == 2) ? SCR_CLOCK : SCR_DASH;
    armDwell();
}

Screen screensCurrent() { return s_current; }

void screensShow(Screen s) {
    s_current = s;
    switch (s) {
        case SCR_DASH:
            uiDashboard(g_usage, g_lastFetchMs, WiFi.RSSI(), halBatPercent());
            break;
        case SCR_CHART: {
            static HistSlot slots[HIST_SLOTS];   // 672B — off the stack
            uint32_t newest;
            historySnapshot(slots, newest);
            uiChartScreen(slots, newest, g_lastFetchMs, WiFi.RSSI());
            break;
        }
        case SCR_NEWS:
            uiNewsScreen(g_news, g_lastFetchMs, WiFi.RSSI());
            break;
        case SCR_CLOCK: {
            time_t now = time(nullptr);
            struct tm t;
            localtime_r(&now, &t);
            s_clockMinute = t.tm_min;
            uiClockScreen(g_usage, g_lastFetchMs, WiFi.RSSI());
            break;
        }
        default:
            break;
    }
}

void screensNext() {
    screensShow((Screen)((s_current + 1) % SCR_COUNT));
    armDwell();
}

void screensPause(uint32_t ms) {
    s_pauseUntilMs = millis() + ms;
}

static Screen nextEnabled(Screen from) {
    for (int step = 1; step <= SCR_COUNT; step++) {
        Screen c = (Screen)((from + step) % SCR_COUNT);
        if (g_settings.scrMask & (1 << c)) return c;
    }
    return from;
}

void screensTick() {
    unsigned long nowMs = millis();

    // Carousel auto-advance (paused briefly after any button gesture)
    if (g_settings.uiMode == 1 && (long)(nowMs - s_pauseUntilMs) >= 0 &&
        (long)(nowMs - s_advanceAtMs) >= 0) {
        screensShow(nextEnabled(s_current));
        armDwell();
    }

    // Clock follows the minute, not the beat
    if (s_current == SCR_CLOCK) {
        time_t now = time(nullptr);
        struct tm t;
        localtime_r(&now, &t);
        if (t.tm_min != s_clockMinute) screensShow(SCR_CLOCK);
    }

    // 10s beat: header label↔URL alternation + per-screen time refresh
    if (nowMs - s_lastBeatMs >= 10000) {
        s_lastBeatMs = nowMs;
        uiHeaderAlternate();
        if (s_current == SCR_DASH) {
            uiDashboardClock(g_usage, g_lastFetchMs, WiFi.RSSI());
        } else {
            screensShow(s_current);   // sprite full redraw — flicker-free
        }
    }
}

void screensOnData() {
    historyRecord(g_usage);
    switch (s_current) {
        case SCR_DASH:
            screensShow(SCR_DASH);
            break;
        case SCR_CLOCK:
            screensShow(SCR_CLOCK);
            break;
        case SCR_CHART:
            if (historySlotAdvancedTake()) screensShow(SCR_CHART);
            break;
        default:
            break;   // news doesn't depend on usage
    }
}

void screensOnNews() {
    if (s_current == SCR_NEWS) screensShow(SCR_NEWS);
}

void screensOnSettings() {
    uiSetModelMask(g_settings.mdlMask);
    uiApplyRotation(g_settings.flip);
    if (g_settings.uiMode == 2) s_current = SCR_CLOCK;
    else if (g_settings.uiMode == 0) s_current = SCR_DASH;
    armDwell();
    screensShow(s_current);
}

#endif // BOARD_TDISPLAY_S3
