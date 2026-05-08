/*
 * Claude Code Usage Monitor — Standalone WiFi
 * Supports: M5StickC Plus, M5StickC Plus2, LilyGo T-Display S3, ESP32-C3-OLED
 *
 * Button A: cycle digit (PIN) / cycle brightness (dashboard)
 * Button B: confirm digit (PIN) / force refresh (dashboard)
 * A+B held on boot: factory reset → wipe NVS → re-enter setup
 *
 * ESP32-C3-OLED wiring (both buttons external, active-LOW to GND):
 *   Button A → GPIO 3     Button B → GPIO 7
 *   SDA → GPIO 5          SCL → GPIO 6
 *   GPIO 9 (BO0): download mode only — do NOT wire a button here
 */

#include "hal.h"
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "crypto.h"
#include "provision.h"
#include "api.h"
#include "ui.h"

static Preferences prefs;
static char        token[256];
static UsageData   usage;
static unsigned long lastFetch = 0;
static int         pollMs     = DEFAULT_POLL_SEC * 1000;
static uint8_t     brightness = DEFAULT_BRIGHTNESS;

// ── PIN Entry (blocks until 4 digits confirmed) ────────
static void enterPin(char* pinOut, int maxLen) {
    int digits[4] = {0, 0, 0, 0};
    int pos = 0;

    while (pos < 4) {
        uiPinScreen(pos, digits);
        while (true) {
            halUpdate();
            if (halBtnAWasPressed()) { digits[pos] = (digits[pos] + 1) % 10; break; }
            if (halBtnBWasPressed()) { pos++; break; }
            delay(20);
        }
    }
    snprintf(pinOut, maxLen, "%d%d%d%d", digits[0], digits[1], digits[2], digits[3]);
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

// ── Sync NTP for reset countdown display ───────────────
static void syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm t;
    getLocalTime(&t, 5000);
}

// ── Fetch + draw ───────────────────────────────────────
static void refresh() {
    if (WiFi.status() != WL_CONNECTED) {
        prefs.begin(NVS_NAMESPACE, true);
        connectWiFi(prefs.getString("ssid", "").c_str(),
                    prefs.getString("wifipass", "").c_str());
        prefs.end();
    }
    fetchUsage(token, usage);
    lastFetch = millis();
    uiDashboard(usage, lastFetch, WiFi.RSSI(), halBatPercent());
}

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
            prefs.begin(NVS_NAMESPACE, false);
            prefs.clear();
            prefs.end();
            uiError("NVS WIPED", "Rebooting in 2s...");
            delay(2000);
            ESP.restart();
        }
    }

    // Check provisioned
    prefs.begin(NVS_NAMESPACE, true);
    bool provisioned = prefs.getBool("provisioned", false);
    prefs.end();

    if (!provisioned) {
        uiBootProgress(50, "No config found");
        delay(400);

        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        char apName[24];
        snprintf(apName, sizeof(apName), "ClaudeMonitor-%02X%02X", mac[4], mac[5]);

#ifdef BOARD_ESP32C3_OLED
        // No readable display during setup — use open AP so password isn't needed
        const char* apPass = "";
        Serial.printf("[SETUP] AP: %s (open)\n", apName);
#else
        static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        uint8_t rnd[8];
        esp_fill_random(rnd, sizeof(rnd));
        char apPass[9];
        for (int i = 0; i < 8; i++) apPass[i] = alphabet[rnd[i] % (sizeof(alphabet) - 1)];
        apPass[8] = '\0';
#endif
        runProvisioningPortal(apName, apPass);
        return;
    }

    uiBootProgress(50, "Config loaded");
    delay(200);

    // Load NVS
    prefs.begin(NVS_NAMESPACE, true);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("wifipass", "");
    EncryptedBlob blob;
    prefs.getBytes("blob", &blob, sizeof(blob));
    pollMs     = prefs.getInt("poll_sec", DEFAULT_POLL_SEC) * 1000;
    brightness = prefs.getInt("brightness", DEFAULT_BRIGHTNESS);
    prefs.end();

    halSetBrightness(brightness);

    uiBootProgress(60, "Enter PIN...");
    delay(300);

    // PIN + decrypt loop
    int attempts = 0;
    while (attempts < MAX_PIN_ATTEMPTS) {
        char pin[9];
        enterPin(pin, sizeof(pin));

        if (decryptToken(blob, pin, token, sizeof(token))) break;

        attempts++;
        if (attempts >= MAX_PIN_ATTEMPTS) {
            uiError("MAX ATTEMPTS", "Wiping credentials...");
            prefs.begin(NVS_NAMESPACE, false);
            prefs.clear();
            prefs.end();
            delay(3000);
            ESP.restart();
        }

        int lockSec = LOCKOUT_BASE_SEC * (1 << (attempts - 1));
        if (lockSec > 3600) lockSec = 3600;
        uiLockout(attempts, MAX_PIN_ATTEMPTS, lockSec);
    }

    uiBootProgress(80, "Connecting WiFi...");

    if (!connectWiFi(ssid.c_str(), pass.c_str())) {
        uiError("WIFI FAILED", ssid.c_str());
        delay(5000);
        ESP.restart();
    }

    uiBootProgress(90, "Syncing time...");
    syncTime();

    uiBootProgress(95, "Fetching usage...");
    refresh();
}

// ── Loop ───────────────────────────────────────────────
void loop() {
    halUpdate();

    if (halBtnAWasPressed()) {
#ifdef BOARD_ESP32C3_OLED
        brightness = (brightness + 1) % 2; // on/off only — contrast change imperceptible
#else
        brightness = (brightness + 1) % 4;
#endif
        halSetBrightness(brightness);
    }

    if (halBtnBWasPressed()) {
        refresh();
    }

    if (millis() - lastFetch >= (unsigned long)pollMs) {
        refresh();
    }

    static unsigned long lastRedraw = 0;
    if (millis() - lastRedraw > 10000) {
        uiDashboard(usage, lastFetch, WiFi.RSSI(), halBatPercent());
        lastRedraw = millis();
    }

    delay(20);
}
