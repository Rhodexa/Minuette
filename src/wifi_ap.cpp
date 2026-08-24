#include <Arduino.h>
#include <WiFi.h>

#include "wifi_ap.h"
#include "config.h"

void wifiApStart() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
  Serial.printf("[wifi] AP up: %s\n", CONFIG_AP_SSID);
}

void wifiApStop() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[wifi] AP down");
}
