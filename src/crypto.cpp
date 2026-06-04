#include "crypto.h"
#include "config.h"
#include <string.h>

#ifdef ESP8266
// ── ESP8266: rweather/Crypto (the ESP8266 core ships BearSSL, not mbedTLS) ──────
// AES-256-GCM + iterated SHA-256, byte-for-byte compatible with the mbedTLS path
// below: same KDF (PIN+MAC salt, KDF_ROUNDS iterations) and same EncryptedBlob
// layout, so a blob is portable across platforms.
#include <SHA256.h>
#include <GCM.h>
#include <AES.h>

// Hardware glue from compat_esp8266.cpp (esp_*-call replacements).
void fillRandom(uint8_t* buf, size_t n);
void getMac6(uint8_t mac[6]);

void deriveKey(const char* pin, const uint8_t* salt, size_t saltLen, uint8_t* keyOut32) {
    SHA256 h;
    uint8_t hash[32];

    h.reset();
    h.update((const uint8_t*)pin, strlen(pin));
    h.update(salt, saltLen);
    h.finalize(hash, sizeof(hash));

    for (int i = 1; i < KDF_ROUNDS; i++) {
        h.reset();
        h.update(hash, sizeof(hash));
        h.finalize(hash, sizeof(hash));
    }

    memcpy(keyOut32, hash, 32);
}

bool encryptToken(const char* plaintext, const char* pin, EncryptedBlob& blob) {
    size_t ptLen = strlen(plaintext);
    if (ptLen > sizeof(blob.ciphertext)) return false;

    getMac6(blob.salt);
    fillRandom(blob.iv, sizeof(blob.iv));

    uint8_t key[32];
    deriveKey(pin, blob.salt, sizeof(blob.salt), key);

    GCM<AES256> gcm;
    bool ok = gcm.setKey(key, 32) && gcm.setIV(blob.iv, sizeof(blob.iv));
    if (ok) {
        gcm.encrypt(blob.ciphertext, (const uint8_t*)plaintext, ptLen);
        gcm.computeTag(blob.tag, sizeof(blob.tag));
    }

    memset(key, 0, sizeof(key));
    blob.len = ptLen;
    return ok;
}

bool decryptToken(const EncryptedBlob& blob, const char* pin, char* plainOut, size_t plainMaxLen) {
    if (blob.len > plainMaxLen - 1) return false;

    uint8_t key[32];
    deriveKey(pin, blob.salt, sizeof(blob.salt), key);

    GCM<AES256> gcm;
    bool ok = gcm.setKey(key, 32) && gcm.setIV(blob.iv, sizeof(blob.iv));
    if (ok) {
        gcm.decrypt((uint8_t*)plainOut, blob.ciphertext, blob.len);
        ok = gcm.checkTag(blob.tag, sizeof(blob.tag));   // wrong PIN => tag mismatch
    }

    memset(key, 0, sizeof(key));
    if (!ok) return false;
    plainOut[blob.len] = '\0';
    return true;
}

#else
// ── ESP32: native mbedTLS ───────────────────────────────────────────────────
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include "esp_system.h"

void deriveKey(const char* pin, const uint8_t* salt, size_t saltLen, uint8_t* keyOut32) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    uint8_t hash[32];
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const uint8_t*)pin, strlen(pin));
    mbedtls_sha256_update(&ctx, salt, saltLen);
    mbedtls_sha256_finish(&ctx, hash);

    for (int i = 1; i < KDF_ROUNDS; i++) {
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, hash, 32);
        mbedtls_sha256_finish(&ctx, hash);
    }

    mbedtls_sha256_free(&ctx);
    memcpy(keyOut32, hash, 32);
}

bool encryptToken(const char* plaintext, const char* pin, EncryptedBlob& blob) {
    size_t ptLen = strlen(plaintext);
    if (ptLen > sizeof(blob.ciphertext)) return false;

    esp_efuse_mac_get_default(blob.salt);
    esp_fill_random(blob.iv, sizeof(blob.iv));

    uint8_t key[32];
    deriveKey(pin, blob.salt, sizeof(blob.salt), key);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);

    int ret = mbedtls_gcm_crypt_and_tag(
        &gcm, MBEDTLS_GCM_ENCRYPT, ptLen,
        blob.iv, sizeof(blob.iv),
        NULL, 0,
        (const uint8_t*)plaintext,
        blob.ciphertext,
        sizeof(blob.tag), blob.tag
    );

    mbedtls_gcm_free(&gcm);
    memset(key, 0, sizeof(key));
    blob.len = ptLen;
    return (ret == 0);
}

bool decryptToken(const EncryptedBlob& blob, const char* pin, char* plainOut, size_t plainMaxLen) {
    if (blob.len > plainMaxLen - 1) return false;

    uint8_t key[32];
    deriveKey(pin, blob.salt, sizeof(blob.salt), key);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);

    int ret = mbedtls_gcm_auth_decrypt(
        &gcm, blob.len,
        blob.iv, sizeof(blob.iv),
        NULL, 0,
        blob.tag, sizeof(blob.tag),
        blob.ciphertext,
        (uint8_t*)plainOut
    );

    mbedtls_gcm_free(&gcm);
    memset(key, 0, sizeof(key));

    if (ret != 0) return false;
    plainOut[blob.len] = '\0';
    return true;
}

#endif
