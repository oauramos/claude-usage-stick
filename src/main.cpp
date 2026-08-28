/*
 * Claude Code Usage Monitor — Standalone WiFi
 * Supports: M5StickC Plus, M5StickC Plus2, LilyGo T-Display S3, ESP32-C3-OLED
 *
 * PIN entry: A cycles the digit, B confirms
 * Dashboard (Clarity): A cycles brightness, B forces a refresh
 * Dashboard (Mango):   A flips the screen, B cycles brightness, A+B force refresh
 * A+B held on boot: factory reset → wipe NVS → re-enter setup
 *
 * Boot order differs per board: the T-Display S3 brings WiFi up before the PIN
 * (the LAN address is shown while locked, and a failed connect falls back to a
 * WiFi-reconfigure portal); every other board keeps the PIN-first order.
 *
 * ESP32-C3-OLED wiring (both buttons external, active-LOW to GND):
 *   Button A → GPIO 3     Button B → GPIO 7
 *   SDA → GPIO 5          SCL → GPIO 6
 *   GPIO 9 (BO0): download mode only — do NOT wire a button here
 */

#include "hal.h"
#include <WiFi.h>
#include "config.h"
#include "crypto.h"
#include "settings.h"
#include "app_state.h"
#include "provision.h"
#include "api.h"
#include "ui.h"
#include "panel.h"
#ifdef MANGO_UI
#include "status.h"
#endif

Settings      g_settings;
UsageData     g_usage;
#ifdef MANGO_UI
ModelStatus   g_models = {true, true, true, true, false};
#endif
char          g_token[256];
bool          g_unlocked = false;
unsigned long g_lastFetchMs = 0;

// ── PIN Entry (blocks until 4 digits confirmed) ────────
// Returns false if a web login unlocked the device mid-entry (T-Display S3):
// the token is already in g_token and the caller should skip its own decrypt.
static bool enterPin(char* pinOut, int maxLen) {
    int digits[4] = {0, 0, 0, 0};
    int pos = 0;

    while (pos < 4) {
        uiPinScreen(pos, digits);
        while (true) {
            halUpdate();
#ifdef BOARD_TDISPLAY_S3
            panelService();
            if (panelUnlockPending()) return false;
#endif
            if (halBtnAWasPressed()) { digits[pos] = (digits[pos] + 1) % 10; break; }
            if (halBtnBWasPressed()) { pos++; break; }
            delay(20);
        }
    }
    snprintf(pinOut, maxLen, "%d%d%d%d", digits[0], digits[1], digits[2], digits[3]);
    return true;
}

// ── WiFi ───────────────────────────────────────────────
static bool connectWiFi(const char* ssid, const char* pass) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    int ticks = 0;
    while (WiFi.status() != WL_CONNECTED) {
        ticks++;
        uiConnecting(ssid, ticks / 2);
        delay(500);
        if (ticks > WIFI_CONNECT_TIMEOUT_S * 2) return false;
    }
    return true;
}

// ── Setup-AP credentials (also used by the WiFi recovery portal) ──
static void makeApCreds(char* apName, size_t nameLen, char* apPass, size_t passLen) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(apName, nameLen, "ClaudeMonitor-%02X%02X", mac[4], mac[5]);

#ifdef BOARD_ESP32C3_OLED
    // No readable display during setup — use open AP so password isn't needed
    apPass[0] = '\0';
    Serial.printf("[SETUP] AP: %s (open)\n", apName);
#else
    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    uint8_t rnd[8];
    esp_fill_random(rnd, sizeof(rnd));
    size_t n = (passLen > 9) ? 8 : passLen - 1;
    for (size_t i = 0; i < n; i++) apPass[i] = alphabet[rnd[i] % (sizeof(alphabet) - 1)];
    apPass[n] = '\0';
#endif
}

// ── Sync NTP for reset countdown display ───────────────
static void syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm t;
    getLocalTime(&t, 5000);
}

// ── Fetch + draw ───────────────────────────────────────
static void refresh() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi(g_settings.ssid, g_settings.wifipass);
    }
    fetchUsage(g_token, g_usage);
#ifdef MANGO_UI
    fetchModelStatus(g_models);   // failure keeps last-known state
    uiSetModelStatus(g_models);
#endif
    g_lastFetchMs = millis();
    uiDashboard(g_usage, g_lastFetchMs, WiFi.RSSI(), halBatPercent());
}

// ── Boot phases ────────────────────────────────────────
// PIN + decrypt loop: 10 attempts wipe the credentials, lockout doubles.
static void unlockPhase(int progressPct) {
    uiBootProgress(progressPct, "Enter PIN...");
    delay(300);

    int attempts = 0;
    bool webUnlocked = false;
    while (attempts < MAX_PIN_ATTEMPTS && !webUnlocked) {
        char pin[9];
        if (!enterPin(pin, sizeof(pin))) { webUnlocked = true; break; }

        if (decryptToken(g_settings.blob, pin, g_token, sizeof(g_token))) break;

        attempts++;
        if (attempts >= MAX_PIN_ATTEMPTS) {
            uiError("MAX ATTEMPTS", "Wiping credentials...");
            settingsWipeAll();
            delay(3000);
            ESP.restart();
        }

        int lockSec = LOCKOUT_BASE_SEC * (1 << (attempts - 1));
        if (lockSec > 3600) lockSec = 3600;
        uiLockoutStatic(attempts, MAX_PIN_ATTEMPTS, lockSec);
        // The device lockout governs the buttons only; the web login keeps its
        // own throttle and stays reachable through the countdown.
        for (int s = lockSec; s > 0 && !webUnlocked; s--) {
            uiLockoutTick(s);
            for (int slice = 0; slice < 50; slice++) {
#ifdef BOARD_TDISPLAY_S3
                panelService();
                if (panelUnlockPending()) { webUnlocked = true; break; }
#endif
                delay(20);
            }
        }
    }
#ifdef BOARD_TDISPLAY_S3
    panelConsumeUnlock();
#endif
    g_unlocked = true;
    // Drain stray button edges so nothing phantom-taps the dashboard.
    halUpdate();
    halBtnAWasPressed();
    halBtnBWasPressed();
}

#ifdef BOARD_TDISPLAY_S3
// WiFi before the PIN on this board: the creds need no PIN, and the device's
// LAN address is already known (and shown) while it sits locked. A dead network
// lands in the reconfigure portal instead of the old infinite reboot loop.
static void netPhase() {
    char host[33];
    slugifyHostname(g_settings.devName, host, sizeof(host));
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(host);   // must precede WiFi.begin to land in the DHCP lease

    uiBootProgress(60, "Connecting WiFi...");
    int tries = 0;
    while (!connectWiFi(g_settings.ssid, g_settings.wifipass)) {
        if (++tries >= 3) {
            char apName[24], apPass[9];
            makeApCreds(apName, sizeof(apName), apPass, sizeof(apPass));
            runProvisioningPortal(apName, apPass, true);   // never returns
        }
        uiError("WIFI FAILED", "Retrying...");
        delay(2000);
    }

    uiBootProgress(70, "Syncing time...");
    syncTime();
    settingsApplyTZ(g_settings.tzMin);

    char url[40];
    snprintf(url, sizeof(url), "http://%s", WiFi.localIP().toString().c_str());
    uiSetNetInfo(url);
    uiSetHeaderLabel(g_settings.devName);
    panelBegin(host);
}
#else
static void netPhaseLegacy() {
    uiBootProgress(80, "Connecting WiFi...");
    if (!connectWiFi(g_settings.ssid, g_settings.wifipass)) {
        uiError("WIFI FAILED", g_settings.ssid);
        delay(5000);
        ESP.restart();
    }

    uiBootProgress(90, "Syncing time...");
    syncTime();
    settingsApplyTZ(g_settings.tzMin);
}
#endif

// ── Setup ──────────────────────────────────────────────
void setup() {
    halInit();
    uiInit();

    uiBootProgress(10, "Initializing...");
    delay(300);

    uiBootProgress(30, "Checking config...");
    delay(200);

    // Factory reset: both buttons must be held continuously for 2 seconds.
    // A single snapshot can mis-fire on boards where GPIOs float LOW briefly;
    // repeated sampling over 2 s eliminates false triggers.
    halUpdate();
    if (halBtnAIsPressed() && halBtnBIsPressed()) {
        uiBootProgress(40, "Hold A+B 2s...");
        bool held = true;
        for (int i = 0; i < 20 && held; i++) {
            delay(100);
            halUpdate();
            if (!halBtnAIsPressed() || !halBtnBIsPressed()) held = false;
        }
        if (held) {
            uiBootProgress(50, "Factory reset...");
            settingsWipeAll();
            uiError("NVS WIPED", "Rebooting in 2s...");
            delay(2000);
            ESP.restart();
        }
    }

    if (!settingsIsProvisioned()) {
        uiBootProgress(50, "No config found");
        delay(400);

        char apName[24], apPass[9];
        makeApCreds(apName, sizeof(apName), apPass, sizeof(apPass));
        runProvisioningPortal(apName, apPass);
        return;
    }

    uiBootProgress(50, "Config loaded");
    delay(200);

    settingsLoad(g_settings);
    halSetBrightness(g_settings.brightness);
#ifdef MANGO_UI
    if (g_settings.flip) uiApplyRotation(true);
#endif

#ifdef BOARD_TDISPLAY_S3
    netPhase();
    unlockPhase(85);
#else
    unlockPhase(60);
    netPhaseLegacy();
#endif

    uiBootProgress(95, "Fetching usage...");
    refresh();
}

// ── Loop ───────────────────────────────────────────────
void loop() {
    halUpdate();

#ifdef BOARD_TDISPLAY_S3
    panelService();
    panelConsumeUnlock();   // post-boot logins don't need the boot-phase handoff
    uint8_t acts = panelTakeAction();
    if (acts & PANEL_ACT_FACTORY_RESET) {
        settingsWipeAll();
        ESP.restart();
    }
    if (acts & PANEL_ACT_REBOOT) ESP.restart();
    if (acts & PANEL_ACT_FLIP) uiApplyRotation(g_settings.flip);
    if (acts & PANEL_ACT_REFRESH) {
        refresh();
    } else if (acts & (PANEL_ACT_REDRAW | PANEL_ACT_FLIP)) {
        uiDashboard(g_usage, g_lastFetchMs, WiFi.RSSI(), halBatPercent());
    }
#endif

#ifdef MANGO_UI
    // A flips the screen 180°, B cycles brightness, A+B together = force refresh
    // (the Clarity Button-B action). A single press only commits after a short
    // window so the other button can still join to form the combo.
    static unsigned long aPressAt = 0, bPressAt = 0;
    const unsigned long comboWindowMs = 350;
    if (halBtnAWasPressed()) aPressAt = millis();
    if (halBtnBWasPressed()) bPressAt = millis();

    if ((aPressAt && (bPressAt || halBtnBIsPressed())) ||
        (bPressAt && halBtnAIsPressed())) {
        aPressAt = bPressAt = 0;
        refresh();
    } else if (aPressAt && millis() - aPressAt > comboWindowMs) {
        aPressAt = 0;
        g_settings.flip = !g_settings.flip;
        uiApplyRotation(g_settings.flip);
        settingsPutU8("flip", g_settings.flip);
        uiDashboard(g_usage, g_lastFetchMs, WiFi.RSSI(), halBatPercent());
    } else if (bPressAt && millis() - bPressAt > comboWindowMs) {
        bPressAt = 0;
        g_settings.brightness = (g_settings.brightness + 1) % 4;
        halSetBrightness(g_settings.brightness);
        settingsPutInt("brightness", g_settings.brightness);
    }
#else
    if (halBtnAWasPressed()) {
#ifdef BOARD_ESP32C3_OLED
        g_settings.brightness = (g_settings.brightness + 1) % 2; // on/off only — contrast change imperceptible
#else
        g_settings.brightness = (g_settings.brightness + 1) % 4;
#endif
        halSetBrightness(g_settings.brightness);
        settingsPutInt("brightness", g_settings.brightness);
    }

    if (halBtnBWasPressed()) {
        refresh();
    }
#endif

    if (millis() - g_lastFetchMs >= (unsigned long)g_settings.pollSec * 1000UL) {
        refresh();
    }

#ifdef MANGO_UI
    // Healthy mascots blink every 2s (eyes shut for 150ms) to show liveness.
    static unsigned long lastBlink = 0;
    static bool eyesClosed = false;
    if (eyesClosed && millis() - lastBlink > 150) {
        uiBlinkTick(false);
        eyesClosed = false;
    } else if (!eyesClosed && g_usage.ok && millis() - lastBlink > 2000) {
        uiBlinkTick(true);
        eyesClosed = true;
        lastBlink = millis();
    }
#endif

    static unsigned long lastRedraw = 0;
    if (millis() - lastRedraw > 10000) {
        // Only time passed (not data) — update the clock/countdowns in place; redrawing
        // the whole dashboard here is what made the slow CrowPanel panel flicker.
        uiDashboardClock(g_usage, g_lastFetchMs, WiFi.RSSI());
        lastRedraw = millis();
    }

    delay(20);
}
