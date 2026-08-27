#pragma once

// Starts WiFi AP, serves captive portal at 192.168.4.1.
// Blocks until user submits config. Encrypts token, saves to NVS, reboots.
// With reconfigure=true (device already provisioned, WiFi unreachable) the
// portal updates only the WiFi credentials by default: token/PIN may be left
// empty to keep the stored blob, and preferences are not touched.
void runProvisioningPortal(const char* apName, const char* apPass, bool reconfigure = false);
