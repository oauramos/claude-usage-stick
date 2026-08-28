#include "history.h"

#ifdef DUST_UI

#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>

// NTP sanity floor: epochs below this mean the clock isn't set and slot math
// would poison the ring.
static const uint32_t TIME_SANE_EPOCH = 1700000000UL;
static const char*    HIST_PATH       = "/history.bin";

struct HistFile {
    uint32_t magic;         // 'CUH1'
    uint16_t version;
    uint16_t reserved;
    uint32_t lastAbsSlot;   // absolute slot (epoch/1800) of the newest sample
    HistSlot ring[HIST_SLOTS];
};
static const uint32_t HIST_MAGIC   = 0x31485543;   // "CUH1" little-endian
static const uint16_t HIST_VERSION = 1;

static HistFile s_hist;
static bool     s_fsOk         = false;
static bool     s_slotAdvanced = false;

static void clearRing() {
    memset(s_hist.ring, HIST_EMPTY, sizeof(s_hist.ring));
    s_hist.lastAbsSlot = 0;
}

static void persist() {
    if (!s_fsOk) return;
    File f = LittleFS.open(HIST_PATH, "w");
    if (!f) return;
    f.write((const uint8_t*)&s_hist, sizeof(s_hist));
    f.close();
}

void historyInit() {
    s_hist.magic   = HIST_MAGIC;
    s_hist.version = HIST_VERSION;
    clearRing();

    // The data partition is labeled "spiffs" in the table; LittleFS mounts it
    // fine (that label is arduino-esp32's default, passed explicitly to make
    // the dependency visible). formatOnFail covers the first boot.
    s_fsOk = LittleFS.begin(true, "/littlefs", 10, "spiffs");
    if (!s_fsOk) return;

    File f = LittleFS.open(HIST_PATH, "r");
    if (!f) return;
    HistFile onDisk;
    bool ok = f.read((uint8_t*)&onDisk, sizeof(onDisk)) == sizeof(onDisk) &&
              onDisk.magic == HIST_MAGIC && onDisk.version == HIST_VERSION;
    f.close();
    if (ok) s_hist = onDisk;
}

void historyRecord(const UsageData& u) {
    if (!u.ok) return;
    uint32_t now = (uint32_t)time(nullptr);
    if (now < TIME_SANE_EPOCH) return;

    uint32_t absSlot = now / HIST_SLOT_SEC;
    if (s_hist.lastAbsSlot == 0) {
        clearRing();
    } else if (absSlot > s_hist.lastAbsSlot) {
        // Blank everything skipped while the device was off/failing so old
        // wrap-around samples can't masquerade as fresh ones.
        uint32_t gap = absSlot - s_hist.lastAbsSlot;
        if (gap >= HIST_SLOTS) {
            clearRing();
        } else {
            for (uint32_t s = s_hist.lastAbsSlot + 1; s <= absSlot; s++) {
                s_hist.ring[s % HIST_SLOTS].h5 = HIST_EMPTY;
                s_hist.ring[s % HIST_SLOTS].d7 = HIST_EMPTY;
            }
        }
    }

    HistSlot& slot = s_hist.ring[absSlot % HIST_SLOTS];
    slot.h5 = (uint8_t)constrain((int)(u.h5 + 0.5f), 0, 100);
    slot.d7 = (uint8_t)constrain((int)(u.d7 + 0.5f), 0, 100);

    if (absSlot != s_hist.lastAbsSlot) {
        s_hist.lastAbsSlot = absSlot;
        s_slotAdvanced = true;
        persist();
    }
}

bool historySlotAdvancedTake() {
    bool r = s_slotAdvanced;
    s_slotAdvanced = false;
    return r;
}

void historySnapshot(HistSlot* out, uint32_t& newestEpoch) {
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t nowAbs = (now >= TIME_SANE_EPOCH) ? now / HIST_SLOT_SEC : s_hist.lastAbsSlot;
    if (nowAbs == 0) nowAbs = HIST_SLOTS;   // nothing recorded and no clock: all-empty window
    newestEpoch = (nowAbs + 1) * HIST_SLOT_SEC;

    for (uint16_t i = 0; i < HIST_SLOTS; i++) {
        uint32_t absSlot = nowAbs - (HIST_SLOTS - 1) + i;
        bool valid = s_hist.lastAbsSlot != 0 &&
                     absSlot <= s_hist.lastAbsSlot &&
                     absSlot + HIST_SLOTS > s_hist.lastAbsSlot;
        out[i] = valid ? s_hist.ring[absSlot % HIST_SLOTS]
                       : HistSlot{HIST_EMPTY, HIST_EMPTY};
    }
}

void historyErase() {
    clearRing();
    s_slotAdvanced = false;
    if (s_fsOk) LittleFS.remove(HIST_PATH);
}

#ifdef PANEL_DEBUG
void historySeedDemo(bool clear) {
    if (clear) {
        historyErase();
        s_slotAdvanced = true;
        return;
    }
    uint32_t now = (uint32_t)time(nullptr);
    if (now < TIME_SANE_EPOCH) return;
    uint32_t nowAbs = now / HIST_SLOT_SEC;

    // 7 days of plausible shape: the 5h line saws up and resets every ~5h,
    // the 7d line ramps slowly; two multi-hour gaps exercise the line breaks.
    for (uint16_t i = 0; i < HIST_SLOTS; i++) {
        uint32_t absSlot = nowAbs - (HIST_SLOTS - 1) + i;
        HistSlot& s = s_hist.ring[absSlot % HIST_SLOTS];
        if ((i >= 90 && i < 104) || (i >= 210 && i < 226)) {
            s.h5 = s.d7 = HIST_EMPTY;
        } else {
            s.h5 = (uint8_t)((i % 10) * 9);              // 0→81 over 5h, then reset
            s.d7 = (uint8_t)(15 + (i * 55) / HIST_SLOTS); // 15→70 slow ramp
        }
    }
    s_hist.lastAbsSlot = nowAbs;
    s_slotAdvanced = true;
    persist();
}
#endif

#endif // DUST_UI
