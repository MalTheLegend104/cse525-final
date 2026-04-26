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


// Pin
const int THERM_PIN = A0;


// Constants
const float SERIES_RESISTOR     = 10000.0; // 10k resistor
const float NOMINAL_RESISTANCE  = 10000.0; // 10k thermistor at 25°C
const float NOMINAL_TEMPERATURE = 25.0;    // °C
const float BETA_COEFFICIENT    = 3950.0;  // common value (check datasheet)

// ADC
const int ADC_MAX = 1023;

void setup() { 
  Serial.begin(115200); 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) delay(500);
  hw_id = getMac();
  doHandshake("temperature");
  
  pinMode(THERM_PIN, INPUT);

}

void loop() {
  int adc = analogRead(THERM_PIN);

  // Convert ADC value to resistance
  float voltageRatio = (float)adc / ADC_MAX;
  float resistance = SERIES_RESISTOR * (1.0 / voltageRatio - 1.0);

  float tempC;
  tempC = resistance / NOMINAL_RESISTANCE;       // (R/Ro)
  tempC = log(tempC);                            // ln(R/Ro)
  tempC /= BETA_COEFFICIENT;                     // 1/B * ln(...)
  tempC += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  tempC = 1.0 / tempC;                           // invert
  tempC -= 273.15;                               // K → °C

  float tempF = tempC * 9.0 / 5.0 + 32.0;
  HTTPClient http;
  http.begin(String(server) + "/control");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["hw_uuid"] = hw_uuid;
  
  JsonObject state = doc.createNestedObject("state");
  state["temperature"] = tempC;

  String body;
  serializeJson(doc, body);
  int code = http.POST(body);
  if (code != 200) {
    Serial.println("Failed to send temperature data");
  }
  http.end();

  delay(1000);
}