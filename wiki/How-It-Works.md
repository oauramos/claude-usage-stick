# How it works

The whole device is a loop: ask the Anthropic API a trivial question, read the rate-limit headers off the answer, draw them.

## The polling loop

1. The device sends a minimal request to the Anthropic Messages endpoint using your OAuth token — `max_tokens: 1`, so it costs essentially nothing.
2. It ignores the response body and reads two response headers:
   - `anthropic-ratelimit-unified-5h-utilization`
   - `anthropic-ratelimit-unified-7d-utilization`
3. It draws those percentages as bars, along with the reset countdowns.
4. It sleeps until the next poll — the interval is configurable from 30 s to 5 min.

On [Mango](The-UI) firmware the device also fetches model health from [status.claude.com](https://status.claude.com) and draws the Haiku / Sonnet / Opus / Fable mascots.

On [Dust](The-UI#what-dust-adds) firmware (v3) two more things happen:

- Every successful poll drops one sample into a **7-day history ring** (one slot per 30 minutes, ~0.7 KB) persisted on the device's own flash — that's what the chart screen and the panel's chart draw. Time the device spends off shows up as gaps, honestly.
- Every 6 hours it streams the **Anthropic news feed** from `raw.githubusercontent.com`, reads just the first five headlines (~10 KB of a ~200 KB file) and hangs up.

## Where your token goes

Nowhere except Anthropic. There is no backend, no telemetry, and no cloud service in the middle — the device talks straight to `api.anthropic.com` over HTTPS. The token itself is stored encrypted on the device's own flash; see [Security](Security).

## Rate-limit headers and your plan

The unified headers are what Claude Code subscriptions return, one set per window the account actually has. **Enterprise and API-billed accounts do not emit them** — the request succeeds with HTTP 200, but the headers simply aren't there, and the device can't show usage.

**[Verified Ultra accounts have no 5-hour limit](https://support.claude.com/en/articles/11647753-how-do-usage-and-length-limits-work)**, so the API sends only the `-7d-*` headers. The device works this out from each response, starting with the first reading after you unlock it, so there's no plan setting to configure. Dust boards then show the [week's pace](The-UI#weekly-only-accounts) in the freed space, calculated on the device from the 7-day percentage and reset. On other boards, the missing window's bar reads `NO LIMIT` and its reset reads `--`. See [Troubleshooting](Troubleshooting#theres-no-5-hour-bar-or-it-says-no-limit).

If your device reports `no_usage_h_200`, that's what happened: the token is valid, but the plan behind it doesn't publish unified usage. You need a Pro or Max token. See [Troubleshooting](Troubleshooting#the-device-shows-no_usage_h_200).

## Optional local proxy

The repo ships `server/usage_proxy.py`, a small caching proxy that reads the token from the macOS Keychain. It's useful if you'd rather not put a token on the device at all, or if you want several devices sharing one upstream poll. It is entirely optional — the device works standalone without it.
