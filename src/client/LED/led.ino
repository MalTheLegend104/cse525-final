#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";

String IP = ""; // You can set this to your local IP address
String server = String("http://") + IP + ":5000";

String hw_uuid = "";
String hw_id = "";
String func_uuid   = "";

String getMac() {
  return WiFi.macAddress();
}

bool doHandshake(String type) {
  HTTPClient http;
    http.begin(server + "/handshake");
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<128> req;
    req["hw_id"] = hw_id;
    JsonArray functions = req.createNestedArray("functions");
    functions.add(type);

    String body;
    serializeJson(req, body);

    int code = http.POST(body);
    if (code == 200) {
        String payload = http.getString();
        StaticJsonDocument<512> res;
        DeserializationError err = deserializeJson(res, payload);
        if (err) {
            Serial.println("Handshake JSON parse failed");
            http.end();
            return false;
        }
        hw_uuid   = res["uuid"].as<String>();
        func_uuid = res["functions"]["control"].as<String>();
        Serial.println("Handshake OK");
        Serial.println("hw_uuid:   " + hw_uuid);
        Serial.println("func_uuid: " + func_uuid);
        http.end();
        return true;
    }

    Serial.println("Handshake failed, code: " + String(code));
    http.end();
    return false;
}

const int LED_PIN = 0;

void setup() {
  Serial.begin(115200);
  WiFi.disconnect(true); //  I found that I needed this to force the config state to default for some reason
    delay(100);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

  hw_id = getMac();
  while (!doHandshake("control")) {
    Serial.println("Retrying handshake in 5s...");
    delay(5000);
  }

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  HTTPClient http;
  http.begin(String(server) + "/control");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_uuid"] = hw_uuid;
  doc["func_uuid"] = func_uuid;
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
  } else if (code == 401) {
        Serial.println("401 received, re-handshaking...");
        http.end();
        while (!doHandshake("control")) {
            Serial.println("Retrying handshake in 5s...");
            delay(5000);
        }
        return;
    } else {
        Serial.println("Control poll failed, code: " + String(code));
    }

  http.end();
  delay(2000);
}
