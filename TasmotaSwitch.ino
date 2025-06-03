#include "Network.hpp"
#include "Power.hpp"
#include "StatusLight.hpp"
#include "UserInput.hpp"

// #define PIN_PHOTORESISTOR A0

void setup() {
	Serial.begin(115200);
	delay(1000); // wait for serial monitor

	Storage::setup();
	UserInput::setup();
	StatusLight::setup();
}

void loop() {
	Serial.println("running");

	UserInput::loop();

	if (StatusLight::isOn) {
		// todo: test for crash?
		unsigned long startMillis = millis();
		while (millis() - startMillis < 100) {
			ESP.wdtFeed();
			optimistic_yield(10e3);
		}

		StatusLight::setColor(false, false, false); // off
	}

	// Serial.println("going to sleep");
	// Power::lightSleep(60e3); // 10e3 for demo
}
