#pragma once
#include <stdint.h>

#ifdef DUST_UI

// Anthropic news, streamed from the RSS feed in config.h. The full feed is
// ~200KB; the fetch reads only until the first NEWS_MAX_ITEMS items and
// abandons the connection.
struct NewsItem {
    char title[96];
    char date[12];   // "Aug 27"
};
struct NewsState {
    NewsItem items[5];
    uint8_t  count;
    uint32_t fetchedAtEpoch;   // 0 = never fetched
    bool     ok;               // last attempt succeeded
};

extern NewsState g_news;

// Call every loop() pass. Handles its own schedule: first fetch ~15s after the
// first successful usage poll, then every NEWS_POLL_SEC (retry in
// NEWS_RETRY_SEC on failure). Returns true when a fetch just succeeded, so the
// caller can redraw the news screen. Blocks 2–5s while fetching.
bool newsTick();

#endif // DUST_UI
