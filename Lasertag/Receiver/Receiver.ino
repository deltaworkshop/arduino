#include <IRremote.h>

#define SIGNAL_PIN 8
#define SMOKE_PIN 9
#define RECV_PIN 10


// contains the damage values
const byte DamageList[16] = {1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100};
int HitPoints = 100;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup()
{
	Serial.begin(9600);
	irrecv.enableIRIn(); // Start the receiver
	pinMode(SIGNAL_PIN, OUTPUT);
	pinMode(SMOKE_PIN, OUTPUT);

	// blink with our signal at setup...
	digitalWrite(SIGNAL_PIN, HIGH);
	delay(500);
	digitalWrite(SIGNAL_PIN, LOW);
}

void loop() {
	if (HitPoints > 0 && irrecv.decode(&results)) {
		Serial.println(results.value, BIN);

		if (results.decode_type == SONY && results.bits == 8) {
			byte head = GetBits(results.value, 4, 4);
			byte tail = GetBits(results.value, 0, 4);

			// check the head == tail which is our noise-resist
			if(head == tail) {
				HitPoints -= DamageList[head];
				if (HitPoints <= 0)
				{
					digitalWrite(SIGNAL_PIN, HIGH);
					digitalWrite(SMOKE_PIN, HIGH);
					delay(1000);
					digitalWrite(SMOKE_PIN, LOW);
					return; // the object is hit, so stop capturing
				}
			}
		}
		irrecv.resume(); // Receive the next value
	}
}

byte GetBits(byte b, byte offset, byte count)
{
	return (b >> offset) & ((1 << count) - 1);
}