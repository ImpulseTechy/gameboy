#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char *WIFI_SSID = "YOUR_WIFI_NAME";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *SERVER_URL = "http://192.168.1.10:4000/api/telemetry";

String playerName = "Rahul";
String branchName = "Computer";
String currentGame = "Snake";

int currentScore = 0;
int currentLevel = 1;
int framesPerSecond = 30;
unsigned long lastPostMs = 0;
unsigned long lastReconnectAttemptMs = 0;
const unsigned long POST_INTERVAL_MS = 1000;
const unsigned long WIFI_RECONNECT_MS = 4000;

void displayServerStatus(const String &message) {
  Serial.println("[NST Arcade] " + message);
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  displayServerStatus("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    displayServerStatus("WiFi connected: " + WiFi.localIP().toString());
  } else {
    displayServerStatus("WiFi connection failed");
  }
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastReconnectAttemptMs > WIFI_RECONNECT_MS) {
    lastReconnectAttemptMs = millis();
    WiFi.disconnect();
    connectWifi();
  }
}

int readBatteryPercent() {
  return 91;
}

void setArcadeScore(int score, int level, int fps) {
  currentScore = max(0, score);
  currentLevel = max(1, level);
  framesPerSecond = max(0, fps);
}

bool postTelemetryOnce() {
  if (WiFi.status() != WL_CONNECTED) {
    displayServerStatus("Packet skipped: WiFi offline");
    return false;
  }

  HTTPClient http;
  http.setTimeout(2500);
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<384> payload;
  payload["name"] = playerName;
  payload["branch"] = branchName;
  payload["game"] = currentGame;
  payload["score"] = currentScore;
  payload["level"] = currentLevel;
  payload["battery"] = readBatteryPercent();
  payload["wifi"] = WiFi.RSSI();
  payload["fps"] = framesPerSecond;
  payload["heap"] = ESP.getFreeHeap();

  String body;
  serializeJson(payload, body);

  int code = http.POST(body);
  bool ok = code >= 200 && code < 300;
  displayServerStatus(ok ? "Telemetry uploaded" : "Upload failed: HTTP " + String(code));
  http.end();
  return ok;
}

bool postTelemetryWithRetries() {
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (postTelemetryOnce()) {
      return true;
    }
    delay(250 * attempt);
  }
  displayServerStatus("Server unavailable after retries");
  return false;
}

void updateLocalGameSimulation() {
  static unsigned long lastScoreMs = 0;
  if (millis() - lastScoreMs < 1400) {
    return;
  }
  lastScoreMs = millis();
  setArcadeScore(currentScore + random(1, 7), 1 + currentScore / 25, 28 + random(0, 5));
}

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(esp_random());
  connectWifi();
  displayServerStatus("Client ready");
}

void loop() {
  ensureWifi();
  updateLocalGameSimulation();

  if (millis() - lastPostMs >= POST_INTERVAL_MS) {
    lastPostMs = millis();
    postTelemetryWithRetries();
  }
}
