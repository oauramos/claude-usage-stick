#!/usr/bin/env python3
"""
Local caching proxy for Claude OAuth usage data.
Reads the OAuth token from macOS Keychain, polls /api/oauth/usage
every REFRESH_INTERVAL seconds, and serves cached results on HTTP.
"""

import json
import subprocess
import time
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

HOST = "0.0.0.0"
PORT = 8266
REFRESH_INTERVAL = 600  # 10 minutes
USAGE_URL = "https://api.anthropic.com/api/oauth/usage"

_cache = {"data": None, "fetched_at": 0, "error": None}
_lock = threading.Lock()


def get_oauth_token():
    result = subprocess.run(
        ["security", "find-generic-password", "-s", "Claude Code-credentials", "-w"],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not read Claude credentials from Keychain")
    creds = json.loads(result.stdout.strip())
    return creds["claudeAiOauth"]["accessToken"]


def fetch_usage(token):
    import urllib.request
    import urllib.error

    req = urllib.request.Request(USAGE_URL)
    req.add_header("Authorization", f"Bearer {token}")
    req.add_header("anthropic-beta", "oauth-2025-04-20")
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            return json.loads(resp.read()), None
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        return None, f"HTTP {e.code}: {body}"
    except Exception as e:
        return None, str(e)


def refresh_loop():
    token = get_oauth_token()
    while True:
        data, err = fetch_usage(token)
        with _lock:
            if data:
                _cache["data"] = data
                _cache["error"] = None
            else:
                _cache["error"] = err
            _cache["fetched_at"] = int(time.time())
        status = "OK" if data else f"ERR: {err}"
        print(f"[{time.strftime('%H:%M:%S')}] refresh: {status}")
        time.sleep(REFRESH_INTERVAL)


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/usage":
            self.send_response(404)
            self.end_headers()
            return

        with _lock:
            payload = {
                "data": _cache["data"],
                "error": _cache["error"],
                "fetched_at": _cache["fetched_at"],
                "age_seconds": int(time.time()) - _cache["fetched_at"],
            }

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(payload).encode())

    def log_message(self, fmt, *args):
        pass  # silence per-request logs


if __name__ == "__main__":
    print(f"Starting usage proxy on http://{HOST}:{PORT}/usage")
    print(f"Refresh interval: {REFRESH_INTERVAL}s")

    t = threading.Thread(target=refresh_loop, daemon=True)
    t.start()

    server = HTTPServer((HOST, PORT), Handler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
