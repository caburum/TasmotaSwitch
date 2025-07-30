#ifndef USERINPUT_HPP
#define USERINPUT_HPP

#include "Network.hpp"
#include "StatusLight.hpp"
#include "config.h"

#define PIN_SW D4
#define PIN_CLK D6
#define PIN_DT D5

#define ENCODER_TARGET_STEP_SLOW 1
#define ENCODER_TARGET_STEP_MED 4
#define ENCODER_TARGET_STEP_FAST 8

namespace UserInput {
	enum ButtonPress {
		PRESS_NONE = 0,
		PRESS_SHORT = 50, // debounce
		PRESS_MEDIUM = 300,
		PRESS_LONG = 600
	};

	enum EncoderMode {
		OFF = 0,
		PRIMARY = 2, // white
		SECONDARY = 1, // color
	};

	volatile ButtonPress buttonPress = PRESS_NONE;
	volatile EncoderMode encoderMode = EncoderMode::OFF;
	volatile int32_t dimmerDelta = 0;
	volatile unsigned long lastInteractionTime = millis(); // for eco mode

	volatile unsigned long _buttonDownTime = 0;
	volatile bool _buttonIsDown = false;

	IRAM_ATTR void updateButtonInterrupt() {
		lastInteractionTime = millis();

		if (digitalRead(PIN_SW) == LOW) {
			// pin went LOW, button was pressed
			if (!_buttonIsDown) {
				// prevent re-triggering if already down
				_buttonDownTime = lastInteractionTime;
				_buttonIsDown = true;
			}
		} else {
			// pin went HIGH, button was released
			_buttonIsDown = false;
		}
	}

	// https://garrysblog.com/2021/03/20/reliably-debouncing-rotary-encoders-with-arduino-and-esp32/
	IRAM_ATTR void updateEncoderInterrupt() {
		static uint8_t oldAB = 3; // lookup table index
		static int8_t encval = 0; // encoder value
		static const int8_t encStates[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; // lookup table
		static unsigned long lastTime = 0;
		unsigned long currentTime = millis();
		lastInteractionTime = currentTime;

		if (encoderMode == EncoderMode::OFF) {
			encoderMode = EncoderMode::PRIMARY;
		}

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

		StatusLight::setColor(0x111100);

		dimmerDelta = constrain(dimmerDelta, -100, 100);
	}

	inline void setup() {
		pinMode(PIN_SW, INPUT_PULLUP);
		pinMode(PIN_CLK, INPUT);
		pinMode(PIN_DT, INPUT);

		attachInterrupt(digitalPinToInterrupt(PIN_SW), updateButtonInterrupt, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_CLK), updateEncoderInterrupt, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_DT), updateEncoderInterrupt, CHANGE);
	}

	inline void loop() {
		static bool pressInProgress = false;
		static ButtonPress workingState = ButtonPress::PRESS_NONE;

		if (buttonPress != ButtonPress::PRESS_NONE) {
			// already pending
		} else if (_buttonIsDown) {
			if (!pressInProgress) {
				pressInProgress = true;
				workingState = ButtonPress::PRESS_NONE;
			}

			unsigned long heldDuration = millis() - _buttonDownTime;

			if (encoderMode > EncoderMode::OFF) {
				workingState = ButtonPress::PRESS_NONE;
				encoderMode = EncoderMode::SECONDARY;
			} else if (heldDuration >= ButtonPress::PRESS_LONG && workingState < ButtonPress::PRESS_LONG) {
				workingState = ButtonPress::PRESS_LONG;
				StatusLight::setColor(0x888800); // indicates long press threshold
			} else if (heldDuration >= ButtonPress::PRESS_MEDIUM && workingState < ButtonPress::PRESS_MEDIUM) {
				workingState = ButtonPress::PRESS_MEDIUM;
				StatusLight::setColor(0x444400); // indicates medium press threshold
			} else if (heldDuration >= ButtonPress::PRESS_SHORT && workingState < ButtonPress::PRESS_SHORT) {
				workingState = ButtonPress::PRESS_SHORT;
				StatusLight::setColor(0x111100); // indicates short press threshold
			}
		} else if (pressInProgress) {
			pressInProgress = false; // reset button state
			buttonPress = workingState; // finalize the button press state

			encoderMode = EncoderMode::OFF; // reset when button is released
			dimmerDelta = 0; // reset dimmer delta

			Serial.print("button press detected: ");
			Serial.println(static_cast<int>(buttonPress));
			StatusLight::setColor(0); // turn off the status light
		}
	}
}

#endif