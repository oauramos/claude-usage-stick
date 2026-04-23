#include "provision.h"
#include "crypto.h"
#include "ui.h"
#include "config.h"
#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static WebServer  webServer(80);
static DNSServer  dnsServer;
static Preferences prefs;
static const byte DNS_PORT = 53;

static const char SETUP_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Claude Monitor Setup</title>
<style>
  :root{--bg:#191919;--card:#252525;--border:#3a3a3a;--text:#e0e0e0;
        --dim:#888;--accent:#e8733a;--cyan:#f0a050;--red:#f66}
  *{box-sizing:border-box;margin:0;font-family:system-ui,-apple-system,sans-serif}
  body{background:var(--bg);color:var(--text);padding:8px;min-height:100vh}
  .card{background:var(--card);border:1px solid var(--border);border-radius:12px;
        padding:14px 18px;max-width:420px;margin:0 auto}
  h1{color:var(--accent);font-size:1.4em;margin-bottom:2px}
  .sub{color:var(--dim);font-size:.85em;margin-bottom:12px}
  .field{margin-bottom:10px}
  label{display:block;font-size:.85em;color:var(--dim);margin-bottom:4px}
  input,select{width:100%;padding:8px 10px;border:1px solid var(--border);
               border-radius:8px;background:var(--bg);color:var(--text);font-size:1em;outline:0}
  input:focus{border-color:var(--cyan)}
  input[type=password]{font-family:monospace;letter-spacing:1px}
  .pin-input{width:160px;text-align:center;letter-spacing:10px;font-size:1.5em}
  .row{display:flex;gap:12px}
  .row .field{flex:1}
  .hint{font-size:.75em;color:var(--dim);margin-top:4px}
  .warn{font-size:.8em;color:var(--red);margin-top:2px}
  button{margin-top:12px;width:100%;padding:10px;border:none;border-radius:8px;
         background:var(--accent);color:var(--bg);font-weight:700;font-size:1em;cursor:pointer}
  button:active{opacity:.8}
  button:disabled{opacity:.5;cursor:wait}
  #status{margin-top:8px;font-size:.9em;text-align:center;min-height:1.2em}
  .ok{color:#4f4} .err{color:var(--red)}
  .divider{border-top:1px solid var(--border);margin:12px 0 10px}
  .section-label{font-size:.75em;color:var(--dim);text-transform:uppercase;
                 letter-spacing:1px;margin-bottom:10px}
</style>
</head>
<body>
<div class="card">
  <h1>Claude Usage Monitor</h1>
  <p class="sub">One-time setup. Token is AES-256 encrypted on device only.</p>

  <form id="f">
    <div class="section-label">WiFi Network (2.4 GHz only)</div>
    <div class="field">
      <label for="ssid">SSID</label>
      <input id="ssid" name="ssid" required maxlength="32" autocomplete="off"
             placeholder="Your WiFi network name">
    </div>
    <div class="field">
      <label for="wifipass">Password</label>
      <input id="wifipass" name="wifipass" type="password" maxlength="64"
             autocomplete="off" placeholder="Leave empty for open network">
    </div>

    <div class="divider"></div>
    <div class="section-label">Claude Credentials</div>

    <div class="field">
      <label for="token">OAuth Token</label>
      <input id="token" name="token" type="password" required maxlength="256"
             autocomplete="off" placeholder="sk-ant-oat01-...">
      <div class="hint">Run <code>claude setup-token</code> in terminal to generate (valid 1 year)</div>
    </div>
    <div class="field">
      <label for="pin">Encryption PIN</label>
      <input id="pin" name="pin" type="password" required class="pin-input"
             pattern="\d{4,8}" minlength="4" maxlength="8" inputmode="numeric"
             autocomplete="off" placeholder="····">
      <div class="hint">4-8 digits. You'll enter this on device buttons at each boot.</div>
      <div class="warn">PIN is never stored anywhere. Forget it = factory reset.</div>
    </div>

    <div class="divider"></div>
    <div class="section-label">Preferences</div>

    <div class="row">
      <div class="field">
        <label for="poll_sec">Refresh interval</label>
        <select id="poll_sec" name="poll_sec">
          <option value="30">30 sec</option>
          <option value="60" selected>60 sec</option>
          <option value="120">2 min</option>
          <option value="300">5 min</option>
        </select>
      </div>
      <div class="field">
        <label for="brightness">Screen brightness</label>
        <select id="brightness" name="brightness">
          <option value="1">Dim</option>
          <option value="2" selected>Normal</option>
          <option value="3">Bright</option>
        </select>
      </div>
    </div>

    <div class="field">
      <label for="device_name">Device name (header label)</label>
      <input id="device_name" name="device_name" maxlength="32"
             placeholder="Claude Monitor" autocomplete="off">
    </div>

    <button type="submit" id="btn">Save & Reboot Device</button>
    <div id="status"></div>
  </form>
</div>

<script>
document.getElementById('f').addEventListener('submit',async(e)=>{
  e.preventDefault();
  const btn=document.getElementById('btn'),st=document.getElementById('status');
  btn.disabled=true;st.className='';st.textContent='Encrypting and saving...';
  try{
    const r=await fetch('/provision',{method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:new URLSearchParams(new FormData(e.target))});
    if(r.ok){st.className='ok';st.textContent='Saved! Device rebooting now...';}
    else{st.className='err';st.textContent='Error: '+await r.text();btn.disabled=false;}
  }catch(x){st.className='ok';st.textContent='Device rebooting (connection closed).';}
});
</script>
</body>
</html>)rawhtml";

static void handleRoot() {
    webServer.send(200, "text/html", FPSTR(SETUP_HTML));
}

static void handleProvision() {
    String ssid       = webServer.arg("ssid");
    String wifipass   = webServer.arg("wifipass");
    String token      = webServer.arg("token");
    String pin        = webServer.arg("pin");
    String pollStr    = webServer.arg("poll_sec");
    String brightStr  = webServer.arg("brightness");
    String nameStr    = webServer.arg("device_name");

    if (ssid.isEmpty() || token.isEmpty() || pin.length() < 4) {
        webServer.send(400, "text/plain", "Missing required fields.");
        return;
    }

    EncryptedBlob blob;
    if (!encryptToken(token.c_str(), pin.c_str(), blob)) {
        webServer.send(500, "text/plain", "Encryption failed.");
        return;
    }

    int pollSec = pollStr.isEmpty() ? DEFAULT_POLL_SEC : constrain(pollStr.toInt(), MIN_POLL_SEC, MAX_POLL_SEC);
    int bright  = brightStr.isEmpty() ? DEFAULT_BRIGHTNESS : constrain(brightStr.toInt(), 0, 3);
    if (nameStr.isEmpty()) nameStr = "Claude Monitor";

    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString("ssid", ssid);
    prefs.putString("wifipass", wifipass);
    prefs.putBytes("blob", &blob, sizeof(blob));
    prefs.putInt("poll_sec", pollSec);
    prefs.putInt("brightness", bright);
    prefs.putString("dev_name", nameStr);
    prefs.putBool("provisioned", true);
    prefs.end();

    webServer.send(200, "text/plain", "OK");
    delay(1500);
    ESP.restart();
}

static void handleNotFound() {
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
}

void runProvisioningPortal(const char* apName, const char* apPass) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPass);
    delay(100);

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/provision", HTTP_POST, handleProvision);
    webServer.onNotFound(handleNotFound);
    webServer.begin();

    uiSetupScreen(apName, apPass);

    while (true) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        delay(2);
    }
}
