#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// -------- CONFIG --------
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASS";
const char* serverBase = "http://192.168.1.100:5000";
// ------------------------

String deviceUUID;

struct Function {
	String type;
	String uuid;
};

Function functions[5];
int functionCount = 0;

// -------- UTIL --------
String getHardwareID() {
	return WiFi.macAddress();
}

void connectWiFi() {
	WiFi.begin(ssid, password);
	Serial.print("Connecting");

	while (WiFi.status() != WL_CONNECTED) {
		delay(300);
		Serial.print(".");
	}

	Serial.println("\nConnected!");
	Serial.println(WiFi.localIP());
}

// -------- REGISTER --------
void registerDevice() {
	HTTPClient http;
	http.begin(String(serverBase) + "/register");
	http.addHeader("Content-Type", "application/json");

	StaticJsonDocument<256> doc;
	doc["hw_id"] = getHardwareID();

	JsonArray caps = doc.createNestedArray("capabilities");

	JsonObject t = caps.createNestedObject();
	t["type"] = "temperature";

	JsonObject r = caps.createNestedObject();
	r["type"] = "relay";

	String body;
	serializeJson(doc, body);

	int code = http.POST(body);

	Serial.printf("Register code: %d\n", code);

	if (code == 200) {
		StaticJsonDocument<512> resp;
		DeserializationError err = deserializeJson(resp, http.getString());

		if (!err) {
			deviceUUID = resp["device_uuid"].as<String>();

			JsonArray funcs = resp["functions"];
			functionCount = 0;

			for (JsonObject f : funcs) {
				if (functionCount >= 5) break;

				functions[functionCount].type = f["type"].as<String>();
				functions[functionCount].uuid = f["uuid"].as<String>();
				functionCount++;
			}

			Serial.println("Registered:");
			Serial.println(deviceUUID);
		} else {
			Serial.println("JSON parse failed");
		}
	}

	http.end();
}

// -------- SEND DATA --------
void sendData(float temperature, bool relayState) {
	HTTPClient http;
	http.begin(String(serverBase) + "/update");
	http.addHeader("Content-Type", "application/json");

	StaticJsonDocument<256> doc;
	doc["hw_id"] = getHardwareID();

	JsonArray data = doc.createNestedArray("data");

	for (int i = 0; i < functionCount; i++) {
		JsonObject entry = data.createNestedObject();
		entry["function_uuid"] = functions[i].uuid;

		if (functions[i].type == "temperature") {
			entry["value"] = temperature;
		} else if (functions[i].type == "relay") {
			entry["value"] = relayState;
		}
	}

	String body;
	serializeJson(doc, body);

	int code = http.POST(body);
	Serial.printf("Update code: %d\n", code);

	http.end();
}

// -------- ARDUINO ENTRY --------
void setup() {
	Serial.begin(115200);
	delay(1000);

	connectWiFi();
	registerDevice();
}

void loop() {
	static bool relayState = false;

	float temp = 25.0 + random(-10, 10) * 0.1;

	sendData(temp, relayState);

	delay(5000);
}