#include "Network.hpp"
#include "Power.hpp"
#include "StatusLight.hpp"
#include "Storage.hpp"
#include "UserInput.hpp"

// #define PIN_PHOTORESISTOR A0
#define ECO_MODE false // disconnects & sleeps when inactive, tradeoff for responsiveness

bool clearLight = false;

void setup() {
	Serial.begin(115200);
	delay(1000); // wait for serial monitor

	Storage::setup();
	UserInput::setup();
	StatusLight::setup();
	StatusLight::setColor(0x111100);

	if (!ECO_MODE) Network::connectWifi();

	StatusLight::setColor(0x002200);
	clearLight = true;
}

void loop() {
	// Serial.println("running");

	UserInput::loop();

	switch (UserInput::buttonPress) {
		case UserInput::ButtonPress::PRESS_SHORT: {
			networkBooleanResult_t toggleStatus = Network::power('2', Network::PowerState::TOGGLE);
			// if (toggleStatus == NETWORK_ON) {
			// 	StatusLight::setColor(false, true, true); // cyan
			// } else if (toggleStatus == NETWORK_OFF) {
			// 	StatusLight::setColor(true, false, true); // magenta
			// } else {
			// 	StatusLight::setColor(true, false, false); // red
			// }

			break;
		}

		case UserInput::ButtonPress::PRESS_MEDIUM: {
			networkBooleanResult_t toggleStatus = Network::power('1', Network::PowerState::TOGGLE);
			break;
		}

		case UserInput::ButtonPress::PRESS_LONG: {
			StatusLight::setColor(0x000022);
			Power::delay(3000);

			Network::power('0', Network::PowerState::OFF);

			break;
		}

		default:
			break;
	}
	if (UserInput::buttonPress != UserInput::ButtonPress::PRESS_NONE) {
		UserInput::buttonPress = UserInput::ButtonPress::PRESS_NONE; // reset button press state
		clearLight = true;
	}

	if (UserInput::dimmerDelta != 0) {
		clearLight = true;

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
	if (clearLight) {
		clearLight = false;
		Power::delay(50);
		StatusLight::setColor(0);
	}

	if (sleepFlag) {
		Power::lightSleep();
	}

	// Serial.println("sleeping for time");
	// Power::lightSleep(60e3); // 10e3 for demo
}
