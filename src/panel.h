#pragma once
#include <stdint.h>

#ifdef DUST_UI

// LAN control panel: HTTP on :80 in STA mode, login = the device PIN.
// Single-threaded: handlers run inside panelService(), which is called from
// loop() and from every blocking wait in the boot phases. Handlers never draw
// and never block; anything slow or destructive is deferred through the
// action flags below and drained by loop().

// Action flags returned by panelTakeAction() once they are due.
static const uint8_t PANEL_ACT_REFRESH       = 0x01;  // run refresh() now
static const uint8_t PANEL_ACT_REDRAW        = 0x02;  // redraw dashboard (no fetch)
static const uint8_t PANEL_ACT_FLIP          = 0x04;  // apply g_settings.flip, then redraw
static const uint8_t PANEL_ACT_REBOOT        = 0x08;
static const uint8_t PANEL_ACT_FACTORY_RESET = 0x10;

void    panelBegin(const char* hostname);   // mDNS + routes + server start
void    panelService();                     // pump one handleClient pass
bool    panelUnlockPending();               // a web login decrypted the token while locked
void    panelConsumeUnlock();
uint8_t panelTakeAction();                  // due actions; reboot/reset wait ~400ms so the response flushes

#endif // DUST_UI
