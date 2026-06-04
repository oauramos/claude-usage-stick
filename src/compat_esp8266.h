#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ESP8266 platform-compatibility shim.
//
// Every other supported board is ESP32-family; the GeekMagic SmallTV is the lone
// ESP8266 target. This header concentrates the platform glue so the shared source
// files (main/provision/crypto/api) need only minimal `#ifdef ESP8266` include
// swaps instead of being littered with platform branches:
//
//   • aliases ESP8266WebServer -> WebServer  (matches the ESP32 class name)
//   • a tiny NVS-`Preferences`-compatible store backed by a LittleFS file
//   • fillRandom()/getMac6() replacing esp_fill_random()/esp_efuse_mac_get_default()
//
// The whole header is inert on non-ESP8266 builds.
// ─────────────────────────────────────────────────────────────────────────────
#ifdef ESP8266

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "crypto.h"

// ESP32 code uses the class name `WebServer`; on ESP8266 it's `ESP8266WebServer`
// with an identical API (.on/.arg/.send/.sendHeader/.handleClient/.onNotFound).
using WebServer = ESP8266WebServer;

// Minimal subset of the ESP32 NVS `Preferences` API, backed by a single packed
// struct persisted to LittleFS `/config.bin`. Only the fixed key set this firmware
// actually stores is supported (field-mapped) — unknown keys return the default.
class Preferences {
public:
    bool    begin(const char* name, bool readOnly = false);
    void    end();
    bool    clear();

    size_t  putString(const char* key, const String& value);
    String  getString(const char* key, const char* defaultValue = "");
    size_t  putBytes(const char* key, const void* value, size_t len);
    size_t  getBytes(const char* key, void* buf, size_t maxLen);
    size_t  putBool(const char* key, bool value);
    bool    getBool(const char* key, bool defaultValue = false);
    size_t  putInt(const char* key, int32_t value);
    int32_t getInt(const char* key, int32_t defaultValue = 0);

private:
    bool _ro = false;
};

// Hardware glue (defined in compat_esp8266.cpp), mirroring the ESP-IDF helpers the
// shared code calls on ESP32.
void fillRandom(uint8_t* buf, size_t n);   // ESP-IDF esp_fill_random() equivalent
void getMac6(uint8_t mac[6]);              // esp_efuse_mac_get_default() equivalent

#endif // ESP8266
