#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ------------------------------------------------------------------------------------------------
// CONFIG
// ------------------------------------------------------------------------------------------------
const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD";

// Raspberry Pi (or test server) IP
const char* serverBase = "http://192.168.1.100:5000";
// ------------------------------------------------------------------------------------------------
// Main Code
// ------------------------------------------------------------------------------------------------

String deviceUUID;

// Simple struct for function mapping
struct NodeFunction {
	String type;
	String uuid;
};

NodeFunction functions[5];
int functionCount = 0;

String getHardwareID() {
	return WiFi.macAddress();
}

void connectWiFi() {
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(300);
	}
}

// -------- REGISTER EVERY BOOT --------
void registerDevice() {
	HTTPClient http;
	http.begin(String(serverBase) + "/register");
	http.addHeader("Content-Type", "application/json");

	StaticJsonDocument<256> doc;
	doc["hw_id"] = getHardwareID();

	JsonArray caps = doc.createNestedArray("capabilities");

	caps.add(JsonObject());
	caps[0]["type"] = "temperature";

	caps.add(JsonObject());
	caps[1]["type"] = "relay";

	String body;
	serializeJson(doc, body);

	int code = http.POST(body);

	if (code == 200) {
		StaticJsonDocument<512> resp;
		deserializeJson(resp, http.getString());

		deviceUUID = resp["device_uuid"].as<String>();

		// store function UUIDs in RAM
		JsonArray funcs = resp["functions"];
		functionCount = 0;

		for (JsonObject f : funcs) {
			functions[functionCount].type = f["type"].as<String>();
			functions[functionCount].uuid = f["uuid"].as<String>();
			functionCount++;
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

	http.POST(body);
	http.end();
}

void setup() {
	Serial.begin(115200);
	connectWiFi();
	registerDevice();
}

void loop() {
	static bool relayState = false;

	float temp = 25.0 + random(-10, 10) * 0.1;

	sendData(temp, relayState);

	delay(5000);
}