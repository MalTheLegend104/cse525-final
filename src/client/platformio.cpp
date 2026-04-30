// I ended up combining all the sketches together to make compiling and uploading the code super easy
// I had a ton of problems getting the ESP32s to play nicely with my home network so I had to reflash them dozens of times

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_SSID";
String IP = ""; // IP of the raspberry pi controller node
String server = String("http://") + IP + ":5000";

// Uncomment whatever you wanted to program from this block
// #define DO_TEMPERATURE
// #define DO_MOTION_AND_REED
#define DO_LED

#ifdef DO_TEMPERATURE

String hw_uuid = "";
String hw_id = "";
String func_uuid = "";

// Pin
const int THERM_PIN = A0;

// Thermistor constants
const float SERIES_RESISTOR = 10000.0; // 10k resistor
const float NOMINAL_RESISTANCE = 10000.0; // 10k thermistor at 25°C
const float NOMINAL_TEMPERATURE = 25.0;    // °C
const float BETA_COEFFICIENT = 3950.0;  // common value (check datasheet)

// ADC
const int ADC_MAX = 4096;

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
    hw_uuid = res["uuid"].as<String>();
    func_uuid = res["functions"]["temperature"].as<String>();
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

void setup() {
  Serial.begin(115200);
  WiFi.disconnect(true);
  delay(100);
  // WiFi.begin(ssid, password);
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Status: ");
    Serial.println(WiFi.status());
    delay(500);
  }

  hw_id = getMac();
  while (!doHandshake("temperature")) {
    Serial.println("Retrying handshake in 5s...");
    delay(5000);
  }

  pinMode(THERM_PIN, INPUT);
}

void loop() {
  int adc = analogRead(THERM_PIN);

  // Convert ADC value to resistance
  float voltageRatio = (float) adc / ADC_MAX;
  float resistance = SERIES_RESISTOR * (1.0 / voltageRatio - 1.0);

  // Steinhart-Hart (Beta) equation
  float tempC;
  tempC = resistance / NOMINAL_RESISTANCE;       // (R/Ro)
  tempC = log(tempC);                            // ln(R/Ro)
  tempC /= BETA_COEFFICIENT;                      // 1/B * ln(...)
  tempC += 1.0 / (NOMINAL_TEMPERATURE + 273.15);  // + (1/To)
  tempC = 1.0 / tempC;                           // invert
  tempC -= 273.15;                                // K → °C

  float tempF = tempC * 9.0 / 5.0 + 32.0;

  HTTPClient http;
  http.begin(server + "/update");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_uuid"] = hw_uuid;
  doc["func_uuid"] = func_uuid;
  JsonObject state = doc.createNestedObject("data");
  state["temperature"] = tempF; // frontend reads °F as "temperature"

  Serial.println("temp " + String(tempF));

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code == 401) {
    Serial.println("401 received, re-handshaking...");
    http.end();
    while (!doHandshake("temperature")) {
      Serial.println("Retrying handshake in 5s...");
      delay(5000);
    }
    return;
  } else if (code != 200) {
    Serial.println("Update failed, code: " + String(code));
  }

  http.end();
  delay(1000);
}

#endif

#ifdef DO_MOTION_AND_REED

String hw_uuid = "";
String hw_id = "";
String doorFuncUuid = "";
String motionFuncUuid = "";

// Pins
const int REED_PIN = 0;
const int PIR_PIN = 2;

const unsigned long MOTION_COOLDOWN_MS = 2000;

String getMac() {
  return WiFi.macAddress();
}

bool doHandshake() {
  HTTPClient http;
  http.begin(server + "/handshake");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> req;
  req["hw_id"] = hw_id;
  JsonArray functions = req.createNestedArray("functions");
  functions.add("door");
  functions.add("motion");

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
    hw_uuid = res["uuid"].as<String>();
    doorFuncUuid = res["functions"]["door"].as<String>();
    motionFuncUuid = res["functions"]["motion"].as<String>();
    Serial.println("Handshake OK");
    Serial.println("hw_uuid:        " + hw_uuid);
    Serial.println("doorFuncUuid:   " + doorFuncUuid);
    Serial.println("motionFuncUuid: " + motionFuncUuid);
    http.end();
    return true;
  }

  Serial.println("Handshake failed, code: " + String(code));
  http.end();
  return false;
}

// Returns false on 401 so the caller can trigger a re-handshake
bool postUpdate(String funcUuid, JsonObject& data) {
  HTTPClient http;
  http.begin(server + "/update");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_uuid"] = hw_uuid;
  doc["func_uuid"] = funcUuid;
  doc["data"] = data;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  if (code == 401) return false;
  if (code != 200) Serial.println("Update failed (" + funcUuid + ") code: " + String(code));
  return true;
}

void setup() {
  Serial.begin(115200);
  WiFi.disconnect(true);
  delay(100);
  // WiFi.begin(ssid, password);
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  hw_id = getMac();
  while (!doHandshake()) {
    Serial.println("Retrying handshake in 5s...");
    delay(1000);
  }

  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  Serial.println("Waiting for PIR to stabilize...");
  delay(60000);
  Serial.println("PIR ready");
}

void loop() {
  static bool lastDoor = false;
  static bool lastMotion = false;
  static unsigned long lastMotionPost = 0;

  bool doorClosed = (digitalRead(REED_PIN) == LOW);
  bool motion = (digitalRead(PIR_PIN) == HIGH);
  unsigned long now = millis();

  // --- Door update: post on any state change ---
  if (doorClosed != lastDoor) {
    Serial.println(String("Door: ") + (doorClosed ? "closed" : "open"));

    StaticJsonDocument<64> dataDoc;
    JsonObject data = dataDoc.to<JsonObject>();
    data["door_closed"] = doorClosed;

    if (!postUpdate(doorFuncUuid, data)) {
      Serial.println("401 on door, re-handshaking...");
      while (!doHandshake()) { delay(5000); }
      return;
    }
    lastDoor = doorClosed;
  }

  // --- Motion update: post on state change, re-post while active ---
  bool motionChanged = (motion != lastMotion);
  bool motionCooldownDone = (now - lastMotionPost >= MOTION_COOLDOWN_MS);

  if (motionChanged || (motion && motionCooldownDone)) {
    Serial.println(String("Motion: ") + (motion ? "detected" : "clear"));

    StaticJsonDocument<64> dataDoc;
    JsonObject data = dataDoc.to<JsonObject>();
    data["motion"] = motion;

    if (!postUpdate(motionFuncUuid, data)) {
      Serial.println("401 on motion, re-handshaking...");
      while (!doHandshake()) { delay(5000); }
      return;
    }
    lastMotion = motion;
    lastMotionPost = now;
  }

  delay(100);
}
#endif

#ifdef DO_LED

String hw_uuid = "";
String hw_id = "";
String func_uuid = "";

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
    hw_uuid = res["uuid"].as<String>();
    func_uuid = res["functions"]["light"].as<String>();
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
  // WiFi.begin(ssid, password);
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  hw_id = getMac();
  while (!doHandshake("control")) {
    Serial.println("Retrying handshake in 5s...");
    delay(1000);
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

    bool desired = res["state"]["on"];
    state["on"] = digitalRead(LED_PIN) ? true : false;
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
#endif
