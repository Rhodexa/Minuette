#pragma once

// Config-mode HTTP server: serves the page from LittleFS, a small REST API
// over alarm.h/rtc.h, and a captive-portal DNS redirect. Only runs while
// config mode is active.

void webServerStart();
void webServerStop();

// Call every loop() iteration while config mode is active.
void webServerUpdate();
