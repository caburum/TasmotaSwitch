#ifndef STORAGE_H
#define STORAGE_H

#define OFFSET_BOOTCOUNT 0

namespace StorageData {
	uint32_t bootCount;
}

namespace Storage {
	using namespace StorageData;

	// initialize values from rtc
	inline void setup() {
		if (!ESP.rtcUserMemoryRead(OFFSET_BOOTCOUNT, &bootCount, sizeof(bootCount))) {
			Serial.println(F("rtc read failed"));
			bootCount = 0;
		}
		bootCount++;
		ESP.rtcUserMemoryWrite(OFFSET_BOOTCOUNT, &bootCount, sizeof(bootCount));
		Serial.print(F("boot count: "));
		Serial.println(bootCount);
	}

	// push values to rtc
	inline void loop() {}
}

#endif