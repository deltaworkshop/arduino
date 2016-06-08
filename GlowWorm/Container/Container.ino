#include "OneWireSlave.h"

// request codes to container
#define REQ_LOADA 101 // 0x65
#define REQ_LOADB 102 // 0x66
#define REQ_UNLOAD 103 // 0x67

// response codes from container
#define RESP_LOAD 201 // 0xC9
#define RESP_UNLOADA 202 // 0xCA
#define RESP_UNLOADB 203 // 0xCB
#define RESP_COMPLETE 205 // 0xCD

#define RETRIES 50
#define FULL_CAPACITY 1000

int Storage[2] = {
	0, 0
};
unsigned char rom[8] = {
	0xCC, 0xAD, 0xDA, 0xCE, 0x0F, 0x00, 0x00, 0x00
};
OneWireSlave ds(12);

void setup()
{
	Serial.begin(9600);
	ds.init(rom);
	for (uint8_t led = 2; led <= 11; led++)
	{
		pinMode(led, OUTPUT);
	}
	initAnimation();
}

void loop()
{
	if(ds.waitForRequest(true)) {
		uint8_t request = ds.recv();    
		if(ds.errno == 0) {
			switch(request) {
			case REQ_LOADA:
				handleLOAD(0);
				break;
			case REQ_LOADB:
				handleLOAD(1);
				break;
			case REQ_UNLOAD:
				handleUNLOAD();
				break;
			}
		}
	}
}

void handleLOAD(uint8_t index) {
	if(Storage[0] + Storage[1] >= FULL_CAPACITY) {
		for(byte i = 0; i < RETRIES; i++) {
			ds.send(RESP_COMPLETE);
		}
	} 
	else {
		for(byte i = 0; i < RETRIES; i++) {
			ds.send(RESP_LOAD);
		}
		Storage[index]++;
		updateLevel();
	}
}

void handleUNLOAD() {
	if(Storage[0] + Storage[1] <= 0) {
		for(byte i = 0; i < RETRIES; i++) {
			ds.send(RESP_COMPLETE);
		}
	} 
	else {
		for(byte i = 0; i < RETRIES; i++) {
			ds.send((Storage[0] > 0) ? RESP_UNLOADA : RESP_UNLOADB);
		}
		Storage[(Storage[0] > 0) ? 0 : 1]--;
		updateLevel();
	}
}

void updateLevel() {
	int ledLevel = map(Storage[0] + Storage[1], 0, FULL_CAPACITY, 2, 11);
	for (int thisLed = 2; thisLed <= 11; thisLed++)
	{
		digitalWrite(thisLed, (thisLed <= ledLevel && Storage[0] + Storage[1] > 0) ? HIGH : LOW);
	}
}

void initAnimation() {
	for (uint8_t led = 2; led <= 11; led++)
	{
		digitalWrite(led, HIGH);
		if (led > 2)
		{
			digitalWrite(led - 1, LOW);
		}
		delay(100);
	}
	digitalWrite(11, LOW);
	delay(100);
	for (uint8_t led = 11; led >= 2; led--)
	{
		digitalWrite(led, HIGH);
		if (led < 11)
		{
			digitalWrite(led + 1, LOW);
		}
		delay(100);
	}
	digitalWrite(2, LOW);
}


















