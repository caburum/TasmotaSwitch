#ifndef USERINPUT_HPP
#define USERINPUT_HPP

#include "Network.hpp"
#include "StatusLight.hpp"
#include "Storage.hpp"
#include "config.h"

#define PIN_SW D4
#define PIN_CLK D6
#define PIN_DT D5

#define ENCODER_TARGET_STEP_SLOW 1
#define ENCODER_TARGET_STEP_MED 4
#define ENCODER_TARGET_STEP_FAST 8

namespace UserInput {
	volatile bool toggleScheduledFlag = false;
	volatile int32_t dimmerDelta = 0;

	IRAM_ATTR void toggleButtonInterrupt() {
		// https://forum.arduino.cc/t/45110
		static unsigned long lastTime = 0;
		unsigned long currentTime = millis();
		// if interrupts come faster than 200ms, assume it's a bounce and ignore
		if (currentTime - lastTime > 200) {
			toggleScheduledFlag = true; // will be unset by main loop
			Serial.println("toggleScheduledFlag: true");
			StatusLight::setColor(true, true, false); // yellow
		}
		lastTime = currentTime;
	}

	// https://garrysblog.com/2021/03/20/reliably-debouncing-rotary-encoders-with-arduino-and-esp32/
	IRAM_ATTR void updateEncoderInterrupt() {
		using namespace StorageData;

		static uint8_t oldAB = 3; // lookup table index
		static int8_t encval = 0; // encoder value
		static const int8_t encStates[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; // lookup table
		static unsigned long lastTime = 0;
		unsigned long currentTime = millis();

		oldAB <<= 2; // Remember previous state

		if (digitalRead(PIN_DT)) oldAB |= 0x02; // add current state of pin A
		if (digitalRead(PIN_CLK)) oldAB |= 0x01; // add current state of pin B

		encval += encStates[(oldAB & 0x0f)];

		// update counter if encoder has rotated a full indent, that is at least 4 steps
		if (encval > 3) { // four steps forward
			if (currentTime - lastTime > 40) {
				dimmerDelta += ENCODER_TARGET_STEP_SLOW;
			} else if (currentTime - lastTime > 20) {
				dimmerDelta += ENCODER_TARGET_STEP_MED;
			} else {
				dimmerDelta += ENCODER_TARGET_STEP_FAST;
			}
			encval = 0;
			lastTime = currentTime;
		} else if (encval < -3) { // four steps backwards
			if (currentTime - lastTime > 40) {
				dimmerDelta -= ENCODER_TARGET_STEP_SLOW;
			} else if (currentTime - lastTime > 20) {
				dimmerDelta -= ENCODER_TARGET_STEP_MED;
			} else {
				dimmerDelta -= ENCODER_TARGET_STEP_FAST;
			}
			encval = 0;
			lastTime = currentTime;
		} else {
			return; // skip dealing with value
		}

		StatusLight::setColor(true, true, false); // yellow

		dimmerDelta = constrain(dimmerDelta, -100, 100); // todo: fix overflow

		Serial.print("dimmerDelta: ");
		Serial.println(dimmerDelta);
	}

	inline void setup() {
		pinMode(PIN_SW, INPUT_PULLUP);
		pinMode(PIN_CLK, INPUT);
		pinMode(PIN_DT, INPUT);

		attachInterrupt(digitalPinToInterrupt(PIN_SW), toggleButtonInterrupt, RISING);
		attachInterrupt(digitalPinToInterrupt(PIN_CLK), updateEncoderInterrupt, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_DT), updateEncoderInterrupt, CHANGE);
	}

	inline void loop() {
		if (toggleScheduledFlag) {
			toggleScheduledFlag = false;
			networkBooleanResult_t toggleStatus = Network::power2(true);
			if (toggleStatus == NETWORK_ON) {
				StatusLight::setColor(false, true, true); // cyan
			} else if (toggleStatus == NETWORK_OFF) {
				StatusLight::setColor(true, false, true); // magenta
			} else {
				StatusLight::setColor(true, false, false); // red
			}
		}

		if (dimmerDelta != 0) {
			Network::sendCmnd(("dimmer2+%2B" + String(dimmerDelta)).c_str());
		}
	}
}

#endif