#pragma once

// Starts WiFi AP, serves captive portal at 192.168.4.1.
// Blocks until user submits config. Encrypts token, saves to NVS, reboots.
void runProvisioningPortal(const char* apName, const char* apPass);
