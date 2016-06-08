#include <IRremote.h>
#include <Bounce2.h>

/*
message format 8 bits: [weapon_type][damage]

Weapon IDs | Damage values
--------------------------
0000 = 0   | 0000 = 1
0001 = 1   | 0001 = 2
0010 = 2   | 0010 = 4
0011 = 3   | 0011 = 5
0100 = 4   | 0100 = 7
0101 = 5   | 0101 = 10
0110 = 6   | 0110 = 15
0111 = 7   | 0111 = 17
1000 = 8   | 1000 = 20
1001 = 9   | 1001 = 25
1010 = 10  | 1010 = 30
1011 = 11  | 1011 = 35
1100 = 12  | 1100 = 40
1101 = 13  | 1101 = 50
1110 = 14  | 1110 = 75
1111 = 15  | 1111 = 100

:::Example:::
Weapon type should be the same as damage - 1st four bits should be equal to last four, this is our noise-resist!
Just set the DAMAGE constant to a needed value!
If we need a damage = 50 per shot then DAMAGE = 13, so the package in binary will be the following: [1101][1101]
*/

// weapon characteristics
#define DAMAGE 15
#define AMMO 5 // count of ammo to fire before reload
#define FIRERATE 600 // shots per minute
#define RELOAD 60 // value in seconds
#define OVERHEAT 0 // not used now, for future...

// HW constants 
// WARNING: 3rd pin is reserved for IRSend library usage!
#define TRIGGER_PIN 2
#define SEND_PIN 3
#define BURST_PIN 13

// contains the damage values
const byte DamageList[16] = {1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100};
IRsend irsend;
Bounce Trigger = Bounce();
int Ammo = AMMO;
unsigned long NextReload = 0;
int FireDelay = 1000 / (FIRERATE / 60); // delay between shots in milliseconds

void setup()
{
	Serial.begin(9600);
	pinMode(TRIGGER_PIN, INPUT);
	digitalWrite(TRIGGER_PIN, HIGH);
	Trigger.attach(TRIGGER_PIN);
	Trigger.interval(5);

	pinMode(BURST_PIN, OUTPUT);
	digitalWrite(BURST_PIN, HIGH);
	delay(150);
	digitalWrite(BURST_PIN, LOW);
	delay(150);
	digitalWrite(BURST_PIN, HIGH);
	delay(150);
	digitalWrite(BURST_PIN, LOW);
	delay(150);
	digitalWrite(BURST_PIN, HIGH);
	delay(150);
	digitalWrite(BURST_PIN, LOW);
}

void loop() {
	if (Trigger.update())
	{
		// handle burst when trigger changed...
		handleBurst();
	}
	if (Trigger.read() == LOW)
	{
		if (Ammo > 0)
		{
			irsend.sendSony((DAMAGE * 16) | DAMAGE, 8);
			Ammo--;
			delay(FireDelay);
		}
	}
	if (Ammo <= 0)
	{
		if (NextReload == 0)
		{
			handleBurst(); // ... and when ammo became empty...
			NextReload = millis() + (RELOAD * 1000);
		}
		if (NextReload <= millis())
		{
			Ammo = AMMO;
			NextReload = 0;
			handleBurst(); // ... and when reloaded...
		}
	}
}

void handleBurst() {
	if (Ammo > 0 && Trigger.read() == LOW) {
		digitalWrite(BURST_PIN, HIGH);
	} else {
		digitalWrite(BURST_PIN, LOW);
	}
}