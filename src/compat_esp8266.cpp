#include "compat_esp8266.h"
#ifdef ESP8266

#include <string.h>

// ── LittleFS-backed config store (NVS replacement) ──────────────────────────
// ESP8266 has no NVS `Preferences`. The full key set is small and fixed, so we
// persist one packed struct and field-map each key. Writes flush immediately so
// a power loss mid-session can't lose committed config.

static const uint32_t CFG_MAGIC = 0xC0DECAFE;
static const char*    CFG_PATH  = "/config.bin";

struct ConfigStore {
    uint32_t      magic;
    char          ssid[33];
    char          wifipass[65];
    EncryptedBlob blob;
    int32_t       poll_sec;
    int32_t       brightness;
    char          dev_name[33];
    uint8_t       provisioned;
};

static ConfigStore g_cfg;
static bool        g_loaded  = false;
static bool        g_fsReady = false;

static void ensureFs() {
    if (g_fsReady) return;
    if (!LittleFS.begin()) {   // unformatted on first run — format then mount
        LittleFS.format();
        LittleFS.begin();
    }
    g_fsReady = true;
}

static void loadStore() {
    ensureFs();
    memset(&g_cfg, 0, sizeof(g_cfg));
    File f = LittleFS.open(CFG_PATH, "r");
    if (f) {
        if (f.size() == sizeof(g_cfg)) {
            f.read((uint8_t*)&g_cfg, sizeof(g_cfg));
        }
        f.close();
    }
    if (g_cfg.magic != CFG_MAGIC) {        // absent / wrong-size / corrupt
        memset(&g_cfg, 0, sizeof(g_cfg));
        g_cfg.magic = CFG_MAGIC;           // valid-but-empty
    }
    g_loaded = true;
}

static void saveStore() {
    ensureFs();
    g_cfg.magic = CFG_MAGIC;
    File f = LittleFS.open(CFG_PATH, "w");
    if (f) {
        f.write((const uint8_t*)&g_cfg, sizeof(g_cfg));
        f.close();
    }
}

bool Preferences::begin(const char* name, bool readOnly) {
    (void)name;            // single namespace — name is ignored
    _ro = readOnly;
    loadStore();
    return true;
}

void Preferences::end() { /* per-put write-through, nothing to flush */ }

bool Preferences::clear() {
    ensureFs();
    LittleFS.remove(CFG_PATH);
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.magic = CFG_MAGIC;
    g_loaded = true;
    return true;
}

size_t Preferences::putString(const char* key, const String& value) {
    if (!g_loaded) loadStore();
    char* dst = nullptr; size_t cap = 0;
    if      (!strcmp(key, "ssid"))     { dst = g_cfg.ssid;     cap = sizeof(g_cfg.ssid); }
    else if (!strcmp(key, "wifipass")) { dst = g_cfg.wifipass; cap = sizeof(g_cfg.wifipass); }
    else if (!strcmp(key, "dev_name")) { dst = g_cfg.dev_name; cap = sizeof(g_cfg.dev_name); }
    else return 0;
    strlcpy(dst, value.c_str(), cap);
    if (!_ro) saveStore();
    return strlen(dst);
}

String Preferences::getString(const char* key, const char* defaultValue) {
    if (!g_loaded) loadStore();
    const char* v = nullptr;
    if      (!strcmp(key, "ssid"))     v = g_cfg.ssid;
    else if (!strcmp(key, "wifipass")) v = g_cfg.wifipass;
    else if (!strcmp(key, "dev_name")) v = g_cfg.dev_name;
    if (v && v[0]) return String(v);
    return String(defaultValue);
}

size_t Preferences::putBytes(const char* key, const void* value, size_t len) {
    if (!g_loaded) loadStore();
    if (!strcmp(key, "blob")) {
        size_t n = len < sizeof(g_cfg.blob) ? len : sizeof(g_cfg.blob);
        memcpy(&g_cfg.blob, value, n);
        if (!_ro) saveStore();
        return n;
    }
    return 0;
}

size_t Preferences::getBytes(const char* key, void* buf, size_t maxLen) {
    if (!g_loaded) loadStore();
    if (!strcmp(key, "blob")) {
        size_t n = maxLen < sizeof(g_cfg.blob) ? maxLen : sizeof(g_cfg.blob);
        memcpy(buf, &g_cfg.blob, n);
        return n;
    }
    return 0;
}

size_t Preferences::putBool(const char* key, bool value) {
    if (!g_loaded) loadStore();
    if (!strcmp(key, "provisioned")) {
        g_cfg.provisioned = value ? 1 : 0;
        if (!_ro) saveStore();
        return 1;
    }
    return 0;
}

bool Preferences::getBool(const char* key, bool defaultValue) {
    if (!g_loaded) loadStore();
    if (!strcmp(key, "provisioned")) return g_cfg.provisioned != 0;
    return defaultValue;
}

size_t Preferences::putInt(const char* key, int32_t value) {
    if (!g_loaded) loadStore();
    if      (!strcmp(key, "poll_sec"))   { g_cfg.poll_sec   = value; if (!_ro) saveStore(); return 4; }
    else if (!strcmp(key, "brightness")) { g_cfg.brightness = value; if (!_ro) saveStore(); return 4; }
    return 0;
}

int32_t Preferences::getInt(const char* key, int32_t defaultValue) {
    if (!g_loaded) loadStore();
    // A stored 0 means "never set" for these keys (poll_sec min is 30, brightness
    // min is 1 in provisioning), so fall back to the caller's default.
    if      (!strcmp(key, "poll_sec"))   return g_cfg.poll_sec   ? g_cfg.poll_sec   : defaultValue;
    else if (!strcmp(key, "brightness")) return g_cfg.brightness ? g_cfg.brightness : defaultValue;
    return defaultValue;
}

// ── Hardware glue ───────────────────────────────────────────────────────────

void fillRandom(uint8_t* buf, size_t n) {
    size_t i = 0;
    while (i < n) {
        uint32_t r = *(volatile uint32_t*)0x3FF20E44;  // ESP8266 hardware RNG (WDEV_HWRNG)
        for (int b = 0; b < 4 && i < n; b++) { buf[i++] = (uint8_t)(r & 0xFF); r >>= 8; }
    }
}

void getMac6(uint8_t mac[6]) {
    WiFi.macAddress(mac);
}

#endif // ESP8266
