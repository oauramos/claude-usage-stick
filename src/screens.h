#pragma once
#include <stdint.h>

#ifdef BOARD_TDISPLAY_S3

// Screen orchestration for the v3 carousel. Drawing lives in ui.cpp; this
// module owns which screen is current, the dwell timer, and the redraw beats.
enum Screen : uint8_t { SCR_DASH = 0, SCR_CHART, SCR_NEWS, SCR_CLOCK, SCR_COUNT };

void   screensInit();          // pick the boot screen from g_settings.uiMode
Screen screensCurrent();
void   screensShow(Screen s);  // full redraw of s (becomes current)
void   screensNext();          // manual advance — cycles ALL screens, any mode
void   screensPause(uint32_t ms);   // hold auto-advance (after button input)
void   screensTick();          // per-loop: dwell advance, 10s header beat, blink, clock minute
void   screensOnData();        // after a usage fetch: redraw what depends on it
void   screensOnNews();        // news fetch finished
void   screensOnSettings();    // mode/mask/dwell changed from the panel

#endif // BOARD_TDISPLAY_S3
