#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASS";

String IP = ""; // You can set this to your local IP address
String server = String("http://") + IP + ":5000";

String hw_uuid = "";
String hw_id = "";

String getMac() {
  return WiFi.macAddress();
}

bool doHandshake(String type) {
  HTTPClient http;
  http.begin(String(server) + "/handshake");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_id"] = hw_id;
  doc["type"] = type;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);

  if (code == 200) {
    String payload = http.getString();

    StaticJsonDocument<200> res;
    deserializeJson(res, payload);

    hw_uuid = res["uuid"].as<String>();
    http.end();
    return true;
  }

  http.end();
  return false;
}

const int LED_PIN = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) delay(500);

  hw_id = getMac();
  doHandshake("control");

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  HTTPClient http;
  http.begin(String(server) + "/control");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_uuid"] = hw_uuid;

  JsonObject state = doc.createNestedObject("state");
  state["led"] = digitalRead(LED_PIN) ? "on" : "off";

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);

  if (code == 200) {
    String payload = http.getString();

    StaticJsonDocument<200> res;
    deserializeJson(res, payload);

    String desired = res["state"]["led"];

    digitalWrite(LED_PIN, desired == "on" ? HIGH : LOW);
  }

  http.end();
  delay(2000);
}