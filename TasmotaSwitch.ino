#include "Network.hpp"
#include "Power.hpp"
#include "StatusLight.hpp"
#include "UserInput.hpp"

// #define PIN_PHOTORESISTOR A0
#define ECO_MODE false // disconnects & sleeps when inactive, tradeoff for responsiveness

void setup() {
	Serial.begin(115200);
	delay(1000); // wait for serial monitor

	Storage::setup();
	UserInput::setup();
	StatusLight::setup();
	if (!ECO_MODE) Network::connectWifi();
	StatusLight::setColor(true, true, false);
}

void loop() {
	// Serial.println("running");

	// Power::delay(1000);

	if (UserInput::toggleScheduledFlag) {
		StatusLight::setColor(false, false, false);
		UserInput::toggleScheduledFlag = false;
		networkBooleanResult_t toggleStatus = Network::power2(true);
		if (toggleStatus == NETWORK_ON) {
			StatusLight::setColor(false, true, true); // cyan
		} else if (toggleStatus == NETWORK_OFF) {
			StatusLight::setColor(true, false, true); // magenta
		} else {
			StatusLight::setColor(true, false, false); // red
		}
	}

	if (UserInput::dimmerDelta != 0) {
		StatusLight::setColor(false, false, false);
		int32_t delta = UserInput::dimmerDelta;
		String sign = (delta > 0) ? "%2b" : "%2d";
		String cmnd = "dimmer2" + sign + String(abs(delta));
		Serial.print("sending dimmer command: ");
		Serial.println(cmnd);
		Network::sendCmnd(cmnd.c_str());
		UserInput::dimmerDelta -= delta;
	}

	// if no input for a while, go to light sleep
	bool sleepFlag = ECO_MODE && millis() - UserInput::lastInteractionTime > 60e3;
	if (sleepFlag) {
		Serial.println("no input for a while, going to light sleep");
		StatusLight::setColor(false, false, true);
	}

	// clear status light
	if (StatusLight::isOn) {
		// todo: test for crash?
		Power::delay(30);

		StatusLight::setColor(false, false, false); // off
	}

	if (sleepFlag) {
		Power::lightSleep();
	}

	// Serial.println("sleeping for time");
	// Power::lightSleep(60e3); // 10e3 for demo
}
