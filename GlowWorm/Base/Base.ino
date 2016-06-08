#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <Bounce2.h>

// base modes
#define MODE_UNLOAD 1
#define MODE_LOAD 2
#define MODE_EMPTYFORUNLOAD 3

// request codes to container
#define REQ_LOADA 101
#define REQ_LOADB 102
#define REQ_UNLOAD 103

// response codes from container
#define RESP_LOAD 201
#define RESP_UNLOADA 202
#define RESP_UNLOADB 203
#define RESP_COMPLETE 205

#define RETRIES 50
#define MODE_PIN 2
#define BEEP_PIN 9

// activators:
// first 7 numbers are address,
// the 8th is the activator bonus in percents 0-100 (decimal, not HEX!),
// the 9th number 0 - npt used, 1 - already used, will be ignored next time
byte ActivatorA[] = {0x28, 0xB1, 0xA2, 0x58, 0x5, 0x0, 0x0, 0x6F, 10, 0};
byte ActivatorB[] = {0x28, 0xB1, 0xB6, 0x58, 0x5, 0x0, 0x0, 0x0C, 15, 0};
byte ActivatorC[] = {0x28, 0x58, 0xBB, 0x58, 0x5, 0x0, 0x0, 0x5F, 20, 0};
byte ActivatorD[] = {0x28, 0xE2, 0x9C, 0x58, 0x5, 0x0, 0x0, 0x3E, 25, 0};

uint8_t Mode = MODE_UNLOAD;
unsigned long Storage[2] = {
	100, 100
};
OneWire  ds(10);
Bounce btn_mode = Bounce();
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup()
{
	Serial.begin(9600);
	pinMode(BEEP_PIN, OUTPUT);
	pinMode(MODE_PIN, INPUT);
	digitalWrite(MODE_PIN, HIGH);
	btn_mode.attach(MODE_PIN);
	btn_mode.interval(5);
	btn_mode.update();
	setMode();

	lcd.begin(16, 2);
	lcd.clear();
	lcd.backlight();
	printMode();
	printStorage(0);
	printStorage(1);
}

void loop()
{
	checkMode();
	byte addr[8];
	if (!ds.search(addr)) {
		ds.reset_search();
		return;
	}

	if (OneWire::crc8(addr, 7) != addr[7] ||
		(addr[0] != 0xCC && addr[0] != 0x28)) {
			beep(500);
			return;
	}

	if(addr[0] == 0xCC) { // container connected
		handleContainer(addr);
	} else if(addr[0] == 0x28) { // activator connected
		handleActivator(addr);
	} else {
		beep(500);
	}
}

void handleActivator(byte * addr) {
	// compare the addresses (first 7 bytes)
	unsigned long bonusValue = 0;
	if(byteArrayCompare(addr, ActivatorA, 8) && ActivatorA[9] == 0) {
		bonusValue = (Storage[0] * (unsigned long)ActivatorA[8]) / 100;
		ActivatorA[9] = 1;
	} else if(byteArrayCompare(addr, ActivatorB, 8) && ActivatorB[9] == 0) {
		bonusValue = (Storage[0] * (unsigned long)ActivatorB[8]) / 100;
		ActivatorB[9] = 1;
	} else if(byteArrayCompare(addr, ActivatorC, 8) && ActivatorC[9] == 0) {
		bonusValue = (Storage[0] * (unsigned long)ActivatorC[8]) / 100;
		ActivatorC[9] = 1;
	} else if(byteArrayCompare(addr, ActivatorD, 8) && ActivatorD[9] == 0) {
		bonusValue = (Storage[0] * (unsigned long)ActivatorD[8]) / 100;
		ActivatorD[9] = 1;
	} else {
		beep(500);
		return;
	}

	if (bonusValue > 0)
	{
		for (unsigned long i = 0; i < bonusValue; i++)
		{
			Storage[0]--;
			printStorage(0);
			Storage[1]++;
			printStorage(1);
			beep(20);
			delay(20);
		}
	}
	beep(500);
}

void handleContainer(byte * addr) {
	// handle the base modes
	switch(Mode) {
	case MODE_UNLOAD:
		handleUNLOAD(addr);
		break;
	case MODE_LOAD:
		handleLOAD(addr);
		break;
	default:
		printMode();
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
		if(checkMode()) {
			// break when mode is changed
			break;
		}
		if(Storage[0] == 0 && Storage[1] == 0) {
			Mode = MODE_EMPTYFORUNLOAD;
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
				printStorage(index);
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
	}
}

void handleLOAD(byte * addr) {
	uint8_t response = RESP_UNLOADA;
	while(response == RESP_UNLOADA || response == RESP_UNLOADB) {
		if(checkMode()) {
			// break when mode is changed
			break;
		}

		ds.reset();
		ds.select(addr);
		ds.write(REQ_UNLOAD);
		delayMicroseconds(200);
		for(byte i = 0; i < RETRIES; i++) {      
			response = ds.read();
			if(response == RESP_UNLOADA || response == RESP_UNLOADB) {
				Storage[(response == RESP_UNLOADA) ? 0 : 1]++;
				printStorage((response == RESP_UNLOADA) ? 0 : 1);
				beep(20);
				delay(20);
				break;
			} 
			if(response == RESP_COMPLETE) {
				// signal if container is empty
				beep(1000);
				delay(1000);
				break;
			}
		}
	}
}

void beep(uint8_t time) {
	digitalWrite(BEEP_PIN, HIGH);
	delay(time);
	digitalWrite(BEEP_PIN, LOW);
}

boolean checkMode() {
	if(btn_mode.update() == 1) {
		setMode();
		printMode();
		beep(1000);
		return true;
	}
	return false;
}

void setMode() {
	Mode = (btn_mode.read() == 0) ? MODE_LOAD :
		(Storage[0] > 0 || Storage[1] > 0) ? MODE_UNLOAD :
		MODE_EMPTYFORUNLOAD;
}

void printMode() {
	if(Mode == MODE_LOAD) {
		lcd.setCursor(0, 0);
		lcd.print("LOAD:");
		lcd.setCursor(0, 1);
		lcd.print("LOAD:");
	}
	if(Mode == MODE_UNLOAD) {
		lcd.setCursor(0, 0);
		lcd.print("UNLD:");
		lcd.setCursor(0, 1);
		lcd.print("UNLD:");
	}
	if(Mode == MODE_EMPTYFORUNLOAD) {
		lcd.setCursor(0, 0);
		lcd.print("EMPTY");
		lcd.setCursor(0, 1);
		lcd.print("EMPTY");
	}
}

void printStorage(uint8_t index) {
	unsigned long comparer = 1000000000;
	for(uint8_t i = 6; i < 16; i++) {
		lcd.setCursor(i, index);
		if(Storage[index] < comparer) {
			lcd.print("0");
			comparer = comparer / 10;
		} 
		else {
			lcd.print(Storage[index]);
			break;
		}
	}
}

boolean byteArrayCompare(byte a[], byte b[], byte array_size)
{
	for (byte i = 0; i < array_size; ++i)
		if (a[i] != b[i])
			return(false);
	return(true);
}