#include "settings.h"
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

static Preferences prefs;

bool settingsIsProvisioned() {
    prefs.begin(NVS_NAMESPACE, true);
    bool p = prefs.getBool("provisioned", false);
    prefs.end();
    return p;
}

void settingsLoad(Settings& s) {
    memset(&s, 0, sizeof(s));
    prefs.begin(NVS_NAMESPACE, true);
    prefs.getString("ssid", s.ssid, sizeof(s.ssid));
    prefs.getString("wifipass", s.wifipass, sizeof(s.wifipass));
    s.hasBlob    = prefs.getBytes("blob", &s.blob, sizeof(s.blob)) == sizeof(s.blob);
    s.pollSec    = constrain(prefs.getInt("poll_sec", DEFAULT_POLL_SEC), MIN_POLL_SEC, MAX_POLL_SEC);
    s.brightness = constrain(prefs.getInt("brightness", DEFAULT_BRIGHTNESS), 0, 3);
    prefs.getString("dev_name", s.devName, sizeof(s.devName));
    s.tzMin      = constrain(prefs.getInt("tz_min", 0), -840, 840);
    s.flip       = prefs.getUChar("flip", 0) ? 1 : 0;
    prefs.end();
}

void settingsPutInt(const char* key, int32_t v) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt(key, v);
    prefs.end();
}

void settingsPutU8(const char* key, uint8_t v) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(key, v);
    prefs.end();
}

void settingsPutStr(const char* key, const char* v) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(key, v);
    prefs.end();
}

void settingsPutBlob(const char* key, const void* data, size_t len) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes(key, data, len);
    prefs.end();
}

void settingsWipeAll() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
}

void settingsApplyTZ(int32_t tzMin) {
    int32_t m = -tzMin;   // POSIX sign inversion
    char tz[16];
    snprintf(tz, sizeof(tz), "GMT%c%ld:%02ld",
             (m < 0) ? '-' : '+', labs((long)m) / 60, labs((long)m) % 60);
    setenv("TZ", tz, 1);
    tzset();
}

void slugifyHostname(const char* devName, char* out, size_t outLen) {
    size_t n = 0;
    bool dash = true;   // suppresses a leading dash
    for (const char* p = devName; *p && n + 1 < outLen; p++) {
        char c = *p;
        if (c >= 'A' && c <= 'Z') c += 32;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out[n++] = c;
            dash = false;
        } else if (!dash) {
            out[n++] = '-';
            dash = true;
        }
    }
    while (n > 0 && out[n - 1] == '-') n--;
    out[n] = '\0';
    if (n == 0 || strcmp(out, "claude-monitor") == 0) {
        strlcpy(out, "claude-usage-stick", outLen);
    }
}
