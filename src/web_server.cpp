#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <uri/UriBraces.h>

#include "web_server.h"
#include "alarm.h"
#include "rtc.h"
#include "config.h"

// Defined in main.cpp; any real request resets the config-mode inactivity
// timer, not just the button press that started it.
extern void configModeTouch();

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;
static bool routesRegistered = false;

static void serveFile(const char *path, const char *contentType) {
  configModeTouch();
  File f = LittleFS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain", "Not found");
    return;
  }
  server.streamFile(f, contentType);
  f.close();
}

static void redirectToRoot() {
  configModeTouch();
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

static void alarmToJson(uint8_t slot, const Alarm &a, JsonObject obj) {
  obj["id"] = slot;
  obj["hour"] = a.hour;
  obj["minute"] = a.minute;
  obj["days"] = a.daysMask;
  obj["enabled"] = a.enabled;
}

static void sendJson(int status, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  server.send(status, "application/json", body);
}

static void sendError(int status, const char *message) {
  JsonDocument doc;
  doc["error"] = message;
  sendJson(status, doc);
}

static void handleGetTime() {
  configModeTouch();
  JsonDocument doc;
  doc["unixTime"] = rtcToUnixTime(rtcNow());
  sendJson(200, doc);
}

static void handlePostTime() {
  configModeTouch();
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok || !doc["unixTime"].is<uint32_t>()) {
    sendError(400, "bad request");
    return;
  }
  rtcSet(rtcFromUnixTime(doc["unixTime"].as<uint32_t>()));

  JsonDocument resp;
  resp["unixTime"] = rtcToUnixTime(rtcNow());
  sendJson(200, resp);
}

static void handleGetAlarms() {
  configModeTouch();
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t slot = 0; slot < MAX_ALARMS; slot++) {
    Alarm a;
    if (alarmGet(slot, &a)) {
      alarmToJson(slot, a, arr.add<JsonObject>());
    }
  }
  sendJson(200, doc);
}

static void handlePostAlarm() {
  configModeTouch();
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
    sendError(400, "bad request");
    return;
  }

  Alarm a;
  a.hour = doc["hour"] | 0;
  a.minute = doc["minute"] | 0;
  a.daysMask = doc["days"] | 0;
  a.enabled = doc["enabled"] | true;

  int slot = alarmAdd(a);
  if (slot < 0) {
    sendError(507, "no free alarm slots");
    return;
  }

  JsonDocument resp;
  alarmToJson((uint8_t)slot, a, resp.to<JsonObject>());
  sendJson(201, resp);
}

static void handlePutAlarm() {
  configModeTouch();
  uint8_t slot = (uint8_t)server.pathArg(0).toInt();

  Alarm existing;
  if (!alarmGet(slot, &existing)) {
    sendError(404, "not found");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
    sendError(400, "bad request");
    return;
  }

  Alarm a;
  a.hour = doc["hour"] | existing.hour;
  a.minute = doc["minute"] | existing.minute;
  a.daysMask = doc["days"] | existing.daysMask;
  a.enabled = doc["enabled"] | existing.enabled;
  alarmSet(slot, a);

  JsonDocument resp;
  alarmToJson(slot, a, resp.to<JsonObject>());
  sendJson(200, resp);
}

static void handleDeleteAlarm() {
  configModeTouch();
  uint8_t slot = (uint8_t)server.pathArg(0).toInt();

  Alarm existing;
  if (!alarmGet(slot, &existing)) {
    sendError(404, "not found");
    return;
  }

  alarmDelete(slot);
  server.send(204, "application/json", "");
}

static void registerRoutes() {
  if (routesRegistered) {
    return;
  }
  routesRegistered = true;

  server.on("/", HTTP_GET, []() { serveFile("/index.html", "text/html"); });
  server.on("/minuta_ok.png", HTTP_GET, []() { serveFile("/minuta_ok.png", "image/png"); });
  server.on("/minuta_desync.png", HTTP_GET, []() { serveFile("/minuta_desync.png", "image/png"); });

  // Captive-portal detection probes fired by iOS/Android/Windows so the
  // phone auto-opens the portal instead of just silently joining the AP.
  server.on("/generate_204", HTTP_GET, redirectToRoot);
  server.on("/gen_204", HTTP_GET, redirectToRoot);
  server.on("/hotspot-detect.html", HTTP_GET, redirectToRoot);
  server.on("/ncsi.txt", HTTP_GET, redirectToRoot);
  server.on("/connecttest.txt", HTTP_GET, redirectToRoot);
  server.onNotFound(redirectToRoot);

  server.on("/api/time", HTTP_GET, handleGetTime);
  server.on("/api/time", HTTP_POST, handlePostTime);
  server.on("/api/alarms", HTTP_GET, handleGetAlarms);
  server.on("/api/alarms", HTTP_POST, handlePostAlarm);
  server.on(UriBraces("/api/alarms/{}"), HTTP_PUT, handlePutAlarm);
  server.on(UriBraces("/api/alarms/{}"), HTTP_DELETE, handleDeleteAlarm);
}

void webServerStart() {
  if (!LittleFS.begin(true)) {
    Serial.println("[web] LittleFS mount failed");
  }

  registerRoutes();
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.begin();
  Serial.println("[web] server started");
}

void webServerStop() {
  server.stop();
  dnsServer.stop();
  Serial.println("[web] server stopped");
}

void webServerUpdate() {
  dnsServer.processNextRequest();
  server.handleClient();
}
