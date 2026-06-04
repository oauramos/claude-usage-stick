#include "unlock.h"
#ifdef ESP8266

#include "compat_esp8266.h"   // WebServer alias, Preferences shim, ESP8266 + mDNS includes
#include "config.h"
#include "ui.h"
#include <Arduino.h>

static WebServer            unlockServer(80);
static const EncryptedBlob* s_blob     = nullptr;
static char*                s_tokenOut  = nullptr;
static size_t               s_tokenMax  = 0;
static volatile bool        s_unlocked  = false;
static int                  s_attempts  = 0;
static int                  s_pendingLock = 0;   // seconds; enforced by the run loop

static const char UNLOCK_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Claude Monitor - Unlock</title>
<style>
 *{box-sizing:border-box;margin:0;font-family:system-ui,-apple-system,sans-serif}
 body{background:#191919;color:#e0e0e0;padding:8px;min-height:100vh}
 .card{background:#252525;border:1px solid #3a3a3a;border-radius:12px;padding:18px;max-width:360px;margin:40px auto}
 h1{color:#e8733a;font-size:1.3em;margin-bottom:4px}
 .sub{color:#888;font-size:.85em;margin-bottom:14px}
 input{width:100%;padding:10px;border:1px solid #3a3a3a;border-radius:8px;background:#191919;
       color:#e0e0e0;font-size:1.5em;text-align:center;letter-spacing:10px;outline:0}
 button{margin-top:12px;width:100%;padding:10px;border:none;border-radius:8px;background:#e8733a;
        color:#191919;font-weight:700;font-size:1em;cursor:pointer}
 #status{margin-top:8px;font-size:.9em;text-align:center;min-height:1.2em}
 .err{color:#f66}
</style></head>
<body><div class="card">
 <h1>Claude Usage Monitor</h1>
 <p class="sub">Enter your PIN to unlock the dashboard.</p>
 <form id="f">
   <input id="pin" name="pin" type="password" inputmode="numeric" pattern="\d{4,8}"
          minlength="4" maxlength="8" required autocomplete="off" placeholder="****">
   <button type="submit">Unlock</button>
   <div id="status"></div>
 </form>
</div>
<script>
document.getElementById('f').addEventListener('submit',async(e)=>{
 e.preventDefault();const st=document.getElementById('status');st.className='';st.textContent='Unlocking...';
 try{const r=await fetch('/unlock',{method:'POST',
   headers:{'Content-Type':'application/x-www-form-urlencoded'},
   body:new URLSearchParams(new FormData(e.target))});
   const t=await r.text();
   if(r.ok){st.textContent='Unlocked! The device is loading your dashboard.';}
   else{st.className='err';st.textContent=t;}
 }catch(x){st.className='err';st.textContent='Connection closed.';}
});
</script>
</body></html>)rawhtml";

static void handleRoot() {
    unlockServer.send(200, "text/html", FPSTR(UNLOCK_HTML));
}

static void handleUnlock() {
    String pin = unlockServer.arg("pin");
    if (pin.length() < 4) {
        unlockServer.send(400, "text/plain", "PIN must be at least 4 digits.");
        return;
    }

    if (decryptToken(*s_blob, pin.c_str(), s_tokenOut, s_tokenMax)) {
        unlockServer.send(200, "text/plain", "OK");
        s_unlocked = true;
        return;
    }

    s_attempts++;
    if (s_attempts >= MAX_PIN_ATTEMPTS) {
        unlockServer.send(403, "text/plain", "Too many attempts. Wiping credentials.");
        delay(300);
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);
        prefs.clear();
        prefs.end();
        uiError("MAX ATTEMPTS", "Wiping credentials...");
        delay(2500);
        ESP.restart();   // does not return
    }

    int lockSec = LOCKOUT_BASE_SEC * (1 << (s_attempts - 1));
    if (lockSec > 3600) lockSec = 3600;
    s_pendingLock = lockSec;   // run loop shows the countdown and gates retries

    char msg[56];
    snprintf(msg, sizeof(msg), "Wrong PIN. Wait %ds, then try again.", lockSec);
    unlockServer.send(401, "text/plain", msg);
}

bool runWebUnlock(const EncryptedBlob& blob, char* tokenOut, size_t maxLen) {
    s_blob        = &blob;
    s_tokenOut    = tokenOut;
    s_tokenMax    = maxLen;
    s_unlocked    = false;
    s_attempts    = 0;
    s_pendingLock = 0;

    MDNS.begin("claude-tv");
    MDNS.addService("http", "tcp", 80);

    unlockServer.on("/", HTTP_GET, handleRoot);
    unlockServer.on("/unlock", HTTP_POST, handleUnlock);
    unlockServer.onNotFound(handleRoot);
    unlockServer.begin();

    String ip = WiFi.localIP().toString();
    uiUnlockScreen("claude-tv.local", ip.c_str());

    while (!s_unlocked) {
        unlockServer.handleClient();
        MDNS.update();
        if (s_pendingLock > 0) {
            int sec = s_pendingLock;
            s_pendingLock = 0;
            uiLockout(s_attempts, MAX_PIN_ATTEMPTS, sec);   // blocks for the countdown
            uiUnlockScreen("claude-tv.local", ip.c_str());
        }
        delay(2);
    }

    unlockServer.stop();
    MDNS.end();
    return true;
}

#endif // ESP8266
