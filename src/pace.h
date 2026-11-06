#pragma once
#include <stdint.h>

// Pace of the weekly window, worked out from what the API already reports.
// Weekly limits reset at a fixed time, so the window began exactly 7 days before
// its reset. Mirrored in web/panel/panel.html: keep the two in step.

static const uint32_t PACE_WEEK_SEC  = 7UL * 86400UL;
static const uint32_t PACE_SANE_TIME = 1700000000UL;   // below this, NTP hasn't synced

// Percent of the week gone (0-100), or -1 when the reset or the clock is unknown.
inline float weekElapsedPct(uint32_t resetEpoch, uint32_t now) {
    if (resetEpoch < PACE_WEEK_SEC || now < PACE_SANE_TIME) return -1.0f;
    uint32_t start = resetEpoch - PACE_WEEK_SEC;
    if (now <= start) return 0.0f;
    if (now >= resetEpoch) return 100.0f;
    return (float)(now - start) * 100.0f / (float)PACE_WEEK_SEC;
}

// Where usage lands at the reset if the current rate holds, or -1 during the
// first 24 hours, when so little of the week has passed that the estimate swings.
inline int weekForecastPct(float usedPct, uint32_t resetEpoch, uint32_t now) {
    float elapsed = weekElapsedPct(resetEpoch, now);
    if (usedPct < 0 || elapsed < 100.0f / 7.0f) return -1;
    return (int)(usedPct * 100.0f / elapsed + 0.5f);
}
