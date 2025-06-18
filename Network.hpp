#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include "StatusLight.hpp"
#include "secrets.h"

typedef enum {
	NETWORK_OFF = 0,
	NETWORK_ON = 1,
	NETWORK_ERROR = 2
} networkBooleanResult_t;

namespace Network {
	WiFiClient client;

	/** connect to wifi if not already connected */
	void connectWifi() {
		if (WiFi.status() != WL_CONNECTED) {
			WiFi.mode(WIFI_STA);
			WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
			WiFi.persistent(true);
			WiFi.setHostname("tasmota-switch");
			WiFi.setAutoReconnect(true);

			Serial.print("wifi connecting");
			int connectionAttempts = 0;
			while (WiFi.status() != WL_CONNECTED) {
				delay(500);
				Serial.print(".");
				if (connectionAttempts++ > 50) {
					Serial.println("wifi connection failed");
					ESP.restart();
				}
			}
			Serial.println();
			Serial.print("connected to \"");
			Serial.print(WIFI_SSID);
			Serial.print("\" with ip ");
			Serial.println(WiFi.localIP());
		}
	}

	String sendCmnd(String cmnd) {
		connectWifi();
		String response;
		if (WiFi.status() == WL_CONNECTED) {
			String serverPath = URL_CMND + cmnd;

			HTTPClient http;
			http.begin(client, serverPath);

			int httpResponseCode = http.GET();

			if (httpResponseCode > 0) {
				Serial.print("http response code: ");
				Serial.println(httpResponseCode);
				response = http.getString();
				Serial.println(response);
			} else {
				Serial.print("error code: ");
				Serial.println(httpResponseCode);
			}

			http.end();
		} else {
			Serial.println("wifi disconnected");
		}
		return response;
	}

	namespace PowerState {
		static const String ON = String("ON");
		static const String OFF = String("OFF");
		static const String TOGGLE = String("TOGGLE");
	}

	networkBooleanResult_t power(char channel, String powerState) {
		String identifier = "POWER" + String(channel);

		String response = sendCmnd(identifier + "+" + powerState);
		JSONVar json = JSON.parse(response);
		if (JSON.typeof(json) == "undefined") {
			Serial.println("json parse failed");
			return NETWORK_ERROR;
		}
		if (json.hasOwnProperty(identifier)) {
			return json.hasPropertyEqual(identifier, PowerState::ON) ? NETWORK_ON : NETWORK_OFF;
		}
		return NETWORK_ERROR; // todo: don't return for channel 0
	}
}

#endif