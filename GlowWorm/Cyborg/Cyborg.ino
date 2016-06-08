#include <Bounce2.h>

#define MODE_R 1
#define MODE_G 2
#define MODE_B 3

#define SENSOR_PIN 3
#define MODE_PIN 2
#define PIN_R1 9
#define PIN_R2 8
#define PIN_G1 7
#define PIN_G2 6
#define PIN_B1 5
#define PIN_B2 4

const int Blink=50;
const int Threshold=450;
const int Hits_B=10;
const int Hits_G=10;
const int Regen=10;

Bounce BtnMode = Bounce();
uint8_t Mode = MODE_R;
int HitCounter = 0;
long RegenTime = 0;

void setup()
{
	Serial.begin(9600);

	pinMode(MODE_PIN, INPUT);
	digitalWrite(MODE_PIN, HIGH);
	BtnMode.attach(MODE_PIN);
	BtnMode.interval(5);
	BtnMode.update();

	pinMode(PIN_R1, OUTPUT);
	pinMode(PIN_R2, OUTPUT);
	pinMode(PIN_G1, OUTPUT);
	pinMode(PIN_G2, OUTPUT);
	pinMode(PIN_B1, OUTPUT);
	pinMode(PIN_B2, OUTPUT);
	digitalWrite(PIN_R1, LOW);
	digitalWrite(PIN_R2, LOW);
	digitalWrite(PIN_G1, LOW);
	digitalWrite(PIN_G2, LOW);
	digitalWrite(PIN_B1, LOW);
	digitalWrite(PIN_B2, LOW);

	setMode(Hits_B > 0 ? MODE_B : Hits_G > 0 ? MODE_G : MODE_R);
	Serial.print("MODE: ");
	Serial.println(Mode, DEC);
}

void loop()
{
	if (Mode == MODE_B || Mode == MODE_G)
	{
		int val= analogRead(SENSOR_PIN);
		if (val >= Threshold)
		{
			HitCounter++;
			Serial.print(HitCounter, DEC);
			Serial.print(" : ");
			Serial.println(val, DEC);
			blink();
			if (HitCounter >= (Mode == MODE_B ? Hits_B : Hits_G))
			{
				HitCounter = 0;
				setMode(Mode == MODE_B ? MODE_G : MODE_R);
				Serial.print("MODE: ");
				Serial.println(Mode, DEC);
			}
		}
	} else if(Mode == MODE_R) {
		if (millis() >= RegenTime)
		{
			setMode(Hits_B > 0 ? MODE_B : Hits_G > 0 ? MODE_G : MODE_R);
			Serial.print("MODE: ");
			Serial.println(Mode, DEC);
		}
	}

	if(BtnMode.update() == 1 && BtnMode.read() == 0) {
		setMode(Mode == MODE_R ? (Hits_B > 0 ? MODE_B : Hits_G > 0 ? MODE_G : MODE_R) : (Mode == MODE_G ? MODE_R : Hits_G > 0 ? MODE_G : MODE_R));
	}
}

void setMode(uint8_t newMode){
	setLed(false);
	Mode = newMode;
	setLed(true);
	if (Mode == MODE_R)
	{
		RegenTime = millis()+(Regen*1000);
	}
}

void setLed(bool bOn)
{
	switch (Mode)
	{
	case MODE_B:
		digitalWrite(PIN_B1, bOn);
		digitalWrite(PIN_B2, bOn);
		break;
	case MODE_G:
		digitalWrite(PIN_G1, bOn);
		digitalWrite(PIN_G2, bOn);
		break;
	case MODE_R:
		digitalWrite(PIN_R1, bOn);
		digitalWrite(PIN_R2, bOn);
		break;
	default:
		break;
	}
}


void blink()
{
	switch (Mode)
	{
	case MODE_B:
		digitalWrite(PIN_B1, LOW);
		digitalWrite(PIN_B2, LOW);
		delay(Blink);
		digitalWrite(PIN_B1, HIGH);
		digitalWrite(PIN_B2, HIGH);
		break;
	case MODE_G:
		digitalWrite(PIN_G1, LOW);
		digitalWrite(PIN_G2, LOW);
		delay(Blink);
		digitalWrite(PIN_G1, HIGH);
		digitalWrite(PIN_G2, HIGH);
		break;
	case MODE_R:
		digitalWrite(PIN_R1, LOW);
		digitalWrite(PIN_R2, LOW);
		delay(Blink);
		digitalWrite(PIN_R1, HIGH);
		digitalWrite(PIN_R2, HIGH);
		break;
	default:
		break;
	}
}