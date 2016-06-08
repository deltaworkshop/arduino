#include <OneWire.h>

// base modes
#define MODE_UNLOAD 1
#define MODE_EMPTYFORUNLOAD 2

// request codes to container
#define REQ_LOADA 101
#define REQ_LOADB 102

// response codes from container
#define RESP_LOAD 201
#define RESP_COMPLETE 205

// resource indication and beep leds
#define LEDA 3
#define LEDB 5
#define LEDEMPTY 6
#define BEEP_PIN 9

#define RETRIES 50

#define REGEN_TIME 0 // in minutes, set 0 to disable regeneration
#define DECREASE_AMOUNT 1 // amount to decrease per time tick, set 0 to disable decrease
#define DECREASE_TIME 4 // decrease time tick in seconds, set 0 to disable decrease
#define AMOUNTA 9000
#define AMOUNTB 0

// shaft is always UNLOADING or EMPTY!
volatile uint8_t Mode = MODE_UNLOAD;
unsigned long Storage[2] = {
	AMOUNTA, AMOUNTB
};
int LedState[] = {LEDA, 0, 5}; // {current led, brightness, step}
unsigned long LedChange = 0;
unsigned long Regeneration = 0;
unsigned long Decrease = 0;
OneWire  ds(10);

void setup()
{
	Serial.begin(9600);
	pinMode(LEDA, OUTPUT);
	pinMode(LEDB, OUTPUT);
	pinMode(LEDEMPTY, OUTPUT);
	pinMode(BEEP_PIN, OUTPUT);

	if (DECREASE_AMOUNT > 0 && DECREASE_TIME > 0)
	{
		Decrease = millis() + (DECREASE_TIME * 1000);
	}

	digitalWrite(LEDA, HIGH);
	beep(200);
	digitalWrite(LEDA, LOW);
	delay(200);
	digitalWrite(LEDB, HIGH);
	beep(200);
	digitalWrite(LEDB, LOW);
	delay(200);
	digitalWrite(LEDEMPTY, HIGH);
	beep(200);
	digitalWrite(LEDEMPTY, LOW);
}

void loop()
{
	checkRegen();
	checkDecrease();
	fade();
	byte addr[8];
	if (!ds.search(addr)) {
		ds.reset_search();
		return;
	}
	if (OneWire::crc8(addr, 7) != addr[7] ||
		addr[0] != 0xCC) {
			beep(500);
			return;
	}

	// handle the base modes
	switch(Mode) {
	case MODE_UNLOAD:
		handleUNLOAD(addr);
		break;
	default:
		beep(100);
		delay(100);
		beep(100);
		delay(100);
		beep(100);
		break;
	}
}

void handleUNLOAD(byte * addr) {
	uint8_t response = RESP_LOAD;
	while(response == RESP_LOAD) {
		fade();
		if(Storage[0] == 0 && Storage[1] == 0) {
			Mode = MODE_EMPTYFORUNLOAD;
			if (REGEN_TIME > 0)
			{
				Regeneration = millis() + (REGEN_TIME * 60000);
			}
			return;
		}
		uint8_t request;
		uint8_t index;
		if(Storage[0] > 0) {
			request = REQ_LOADA;
			index = 0;
		} 
		else {
			request = REQ_LOADB;
			index = 1;
		}

		ds.reset();
		ds.select(addr);
		ds.write(request);
		delayMicroseconds(200);
		for(byte i = 0; i < RETRIES; i++) {      
			response = ds.read();      
			if(response == RESP_LOAD) {
				Storage[index]--;
				beep(20);
				delay(20);
				break;
			} 
			if(response == RESP_COMPLETE) {
				// signal if container is full
				beep(1000);
				delay(1000);
				break;
			}
		}
		checkDecrease();
	}
}

void beep(uint8_t time) {
	digitalWrite(BEEP_PIN, HIGH);
	delay(time);
	digitalWrite(BEEP_PIN, LOW);
}

void fade() {
	if (millis() >= LedChange)
	{
		if(Storage[0] > 0) {
			LedState[0] = LEDA;
			digitalWrite(LEDB, 0);
			digitalWrite(LEDEMPTY, 0);
		} else if (Storage[1] > 0) {
			LedState[0] = LEDB; 
			digitalWrite(LEDA, 0);
			digitalWrite(LEDEMPTY, 0);
		} else {
			LedState[0] = LEDEMPTY;
			digitalWrite(LEDA, 0);
			digitalWrite(LEDB, 0);
		}
		analogWrite(LedState[0], LedState[1]);
		LedChange = millis() + 51;
		LedState[1] += LedState[2];
		if (LedState[1] == 0 || LedState[1] == 255) {
			LedState[2] = -LedState[2];
		} 
	}
}

void checkRegen() {
	if(Regeneration > 0 && Regeneration <= millis()) {
		Storage[0] = AMOUNTA;
		Storage[1] = AMOUNTB;
		Regeneration = 0;
		Mode = MODE_UNLOAD;
		beep(1000);
	}
}

void checkDecrease() {
	if (Decrease == 0)
	{
		return;
	}

	if (Decrease <= millis())
	{
		if (Storage[0] > 0)
		{
			Storage[0] -= (Storage[0] > DECREASE_AMOUNT) ? DECREASE_AMOUNT : Storage[0];
		}
		else if (Storage[1] > 0)
		{
			Storage[1] -= (Storage[1] > DECREASE_AMOUNT) ? DECREASE_AMOUNT : Storage[1];
		}
		Decrease = millis() + (DECREASE_TIME * 1000);
	}
}