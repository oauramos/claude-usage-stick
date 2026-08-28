#pragma once
#include <stdint.h>

#ifdef BOARD_TDISPLAY_S3

#include "api.h"

// 7-day usage history: one slot per 30 minutes, 336 slots, ~0.7KB. Slots are
// addressed by absolute index (epoch/1800) so gaps while the device was off
// stay visible as gaps. Persisted to LittleFS only when the slot advances
// (48 writes/day — wear-irrelevant).
static const uint16_t HIST_SLOTS    = 336;
static const uint32_t HIST_SLOT_SEC = 1800;
static const uint8_t  HIST_EMPTY    = 0xFF;

struct HistSlot {
    uint8_t h5;   // 0..100, HIST_EMPTY = no sample
    uint8_t d7;
};

void historyInit();                          // mount LittleFS (format on first boot), load ring
void historyRecord(const UsageData& u);      // call after each successful fetch
bool historySlotAdvancedTake();              // true once per new slot (chart redraw hint)
// Fills out[HIST_SLOTS] oldest→newest; newestEpoch = end of the newest slot.
void historySnapshot(HistSlot* out, uint32_t& newestEpoch);
void historyErase();                         // wipe ring + file (factory reset)
#ifdef PANEL_DEBUG
void historySeedDemo(bool clear);            // synthetic 7 days (or clear) for testing
#endif

#endif // BOARD_TDISPLAY_S3
