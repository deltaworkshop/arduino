#include <avr/sleep.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

/*

Pin change interrupts.

Pin               Mask / Flag / Enable

D0	  PCINT16 (PCMSK2 / PCIF2 / PCIE2)
D1	  PCINT17 (PCMSK2 / PCIF2 / PCIE2)
D2	  PCINT18 (PCMSK2 / PCIF2 / PCIE2)
D3	  PCINT19 (PCMSK2 / PCIF2 / PCIE2)
D4	  PCINT20 (PCMSK2 / PCIF2 / PCIE2)
D5	  PCINT21 (PCMSK2 / PCIF2 / PCIE2)
D6	  PCINT22 (PCMSK2 / PCIF2 / PCIE2)
D7	  PCINT23 (PCMSK2 / PCIF2 / PCIE2)
D8	  PCINT0 (PCMSK0 / PCIF0 / PCIE0)
D9	  PCINT1 (PCMSK0 / PCIF0 / PCIE0)
D10	  PCINT2 (PCMSK0 / PCIF0 / PCIE0)
D11	  PCINT3 (PCMSK0 / PCIF0 / PCIE0)
D12	  PCINT4 (PCMSK0 / PCIF0 / PCIE0)
D13	  PCINT5 (PCMSK0 / PCIF0 / PCIE0)
A0	  PCINT8 (PCMSK1 / PCIF1 / PCIE1)
A1	  PCINT9 (PCMSK1 / PCIF1 / PCIE1)
A2	  PCINT10 (PCMSK1 / PCIF1 / PCIE1)
A3	  PCINT11 (PCMSK1 / PCIF1 / PCIE1)
A4	  PCINT12 (PCMSK1 / PCIF1 / PCIE1)
A5	  PCINT13 (PCMSK1 / PCIF1 / PCIE1)

*/
// number of items in an array for pin re-configuration between sleep and keypad routines
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

#define BEEP_PIN 10
#define SIGNAL_PIN 11

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
	{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'},
	{'*','0','#'}
};
byte rowPins[rows] = {6, 7, 8, 9}; //connect to the row pinouts of the keypad
byte colPins[cols] = {3, 4, 5}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

const byte CODE_NONE = 0;
const byte CODE_WRONG = 1;
const byte CODE_OK = 2;
const byte CODE_KEY = 3;
byte CodeStatus = 0;

const byte MODE_LAUNCHER = 1;
const byte MODE_BOMB = 2;
byte Mode = 0;

char Code[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char Input[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte Position = 0;

long SleepTimer = 60;
long NextSleep = 0;

long SignalTime = 0;

long BombTime = 0;
long EndTime = 0;
long NextTick = 0;

int TriesCount = 0;
int CurrentTries = 0;

// we don't want to do anything except wake
EMPTY_INTERRUPT (PCINT0_vect)
	EMPTY_INTERRUPT (PCINT1_vect)
	EMPTY_INTERRUPT (PCINT2_vect)

	void setup()
{
	// pin change interrupt masks (see above list)
	PCMSK2 |= _BV (PCINT22);   // pin 6
	PCMSK2 |= _BV (PCINT23);   // pin 7
	PCMSK0 |= _BV (PCINT0);    // pin 8
	PCMSK0 |= _BV (PCINT1);    // pin 9

	lcd.begin(16, 2);
	lcd.clear();
	lcd.backlight();
	lcd.setCursor(0,0);
	lcd.print("    WELCOME!");
	pinMode(BEEP_PIN, OUTPUT);
	pinMode(SIGNAL_PIN, OUTPUT);
	digitalWrite(SIGNAL_PIN, HIGH);

	beep(1000);
	setupDevice();
}

void setupDevice()
{
	// 1. Configure the device mode
	setupMode();
	// 2. Configure code
	setupCode();
	// 3. Configure bomb time
	setupBombTime();
	// 4. Configure signal time (relay ON time)
	setupSignalTime();
	// 5. Configure code entering retries count
	setupCodeRetries();

	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("PRESS ANY KEY");
	lcd.setCursor(0, 1);
	lcd.print("    TO START");

	char key = keypad.getKey();
	while (!key)
	{
		key = keypad.getKey();
	}

	lcd.clear();
	beep(1000);
	if (Mode == MODE_BOMB)
	{
		SleepTimer = 0;
		EndTime = millis() + (BombTime * 1000);
		NextTick = millis() + 1000;
		lcd.setCursor(4, 0);
		printTimeStamp(BombTime);
	}
	else
	{
		lcd.setCursor(0, 0);
		lcd.print("ENTER CODE:");
		lcd.setCursor(0, 1);
		NextSleep = millis() + (SleepTimer * 1000);
	}
}

void setupMode()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("SELECT MODE:");
	lcd.setCursor(0, 1);
	lcd.print("1-LAUNCH  2-BOMB");
	char key = keypad.getKey();
	do
	{
		key = keypad.getKey();
	} while (!key || (key != '1' && key != '2'));
	lcd.clear();
	lcd.setCursor(4, 0);
	lcd.print("SELECTED");
	if (key == '1')
	{
		Mode = MODE_LAUNCHER;
		lcd.setCursor(2, 1);
		lcd.print("LAUCNER MODE");
	}
	else
	{
		Mode = MODE_BOMB;
		SleepTimer = 0; // disable sleep timer in this mode
		lcd.setCursor(3, 1);
		lcd.print("BOMB  MODE");
	}
	beep(1000);
}

void setupCode()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("SETUP CODE:");
	lcd.setCursor(0, 1);
	lcd.blink();
	char key = keypad.getKey();
	while (Position < 16)
	{
		if (key)
		{
			Code[Position] = key;
			Position++;
			keyPress(key);
		}
		key = keypad.getKey();
	}
	Position = 0;
	lcd.noBlink();
	lcd.setCursor(0, 0);
	lcd.print("    CODE SET");
	beep(1000);
}

void setupBombTime()
{
	if (Mode != MODE_BOMB)
	{
		return;
	}

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("BOMB TIMER sec:");
	lcd.setCursor(0, 1);
	String strTime = "";
	char key = keypad.getKey();
	while (key != '#')
	{
		if (key && key != '*')
		{
			strTime += key;
			keyPress(key);
		}
		key = keypad.getKey();
	}
	BombTime = strTime.toInt();	
	// don't set a time more than 99 hours (no sense to do that...)
	if (BombTime > 356400)
	{
		BombTime = 356400;
	}
	lcd.noBlink();
	lcd.setCursor(0, 0);
	lcd.print("SET BOMB TIME:");
	lcd.setCursor(0, 1);
	printTimeStamp(BombTime);	
	beep(1000);
}

void setupSignalTime()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("SIGNAL TIME:");
	lcd.setCursor(0, 1);
	lcd.blink();

	String strTime = "";
	char key = keypad.getKey();
	while (key != '#')
	{
		if (key && key != '*')
		{
			strTime += key;
			keyPress(key);
		}
		key = keypad.getKey();
	}
	SignalTime = strTime.toInt();	
	lcd.noBlink();
	lcd.setCursor(0, 0);
	lcd.print("SET SIGNAL TIME:");
	lcd.setCursor(0, 1);
	lcd.print(SignalTime);
	lcd.print(" seconds");
	beep(1000);
}

void setupCodeRetries()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("TRIES COUNT:");
	lcd.setCursor(0, 1);
	lcd.blink();

	String strTries = "";
	char key = keypad.getKey();
	while (key != '#')
	{
		if (key && key != '*')
		{
			strTries += key;
			keyPress(key);
		}
		key = keypad.getKey();
	}
	TriesCount = strTries.toInt();	
	lcd.noBlink();
	lcd.setCursor(0, 0);
	lcd.print("TRIES COUNT SET:");
	lcd.setCursor(0, 1);
	lcd.print(TriesCount);
	beep(1000);
}

void loop()
{
	switch (Mode)
	{
	case MODE_BOMB:
		handleBomb();
		break;
	case MODE_LAUNCHER:
		handleLauncher();
		break;
	default:
		break;
	}
	if(SleepTimer > 0 && millis() >= NextSleep)
	{
		goToSleep();
	}
}

void handleLauncher()
{
	handleCode();
	switch (CodeStatus)
	{
	case CODE_KEY:
		NextSleep = millis() + (SleepTimer * 1000);
		break;
	case CODE_WRONG:
		NextSleep = millis() + (SleepTimer * 1000);
		if (TriesCount > 0 && TriesCount <= CurrentTries)
		{
			Mode = 0;
			lcd.clear();
			lcd.setCursor(4, 0);
			lcd.print("LAUNCHER");
			lcd.setCursor(5, 1);
			lcd.print("LOCKED");
			goToSleep();
		}
		else
		{
			lcd.setCursor(0, 0);
			lcd.print("ENTER CODE:");
		}
		break;
	case CODE_OK:
		NextSleep = millis() + (SleepTimer * 1000);
		Mode = 0;
		lcd.clear();
		lcd.setCursor(4, 0);
		lcd.print("LAUNCHER");
		lcd.setCursor(3, 1);
		lcd.print("ACTIVATED!");
		beep(1000);
		handleSignal();
		goToSleep();
		break;
	default:
		break;
	}

}

void handleBomb()
{
	long time = millis();
	if (time < EndTime && time >= NextTick)
	{
		NextTick = time + 1000;
		lcd.setCursor(4, 0);
		printTimeStamp((EndTime - millis()) / 1000);
	}
	else if (time >= EndTime)
	{
		beep(1000);
		finishBomb(false);
		return;
	}

	handleCode();
	if(CodeStatus == CODE_OK)
	{
		finishBomb(true);
		return;
	}
	else if (CodeStatus = CODE_WRONG && TriesCount > 0 && TriesCount <= CurrentTries)
	{
		// engage when all tries depleted or ignore if infinite tries
		finishBomb(false);
		return;
	}
	CodeStatus = CODE_NONE; // reset code status after processing
}

void finishBomb(bool dectivated)
{
	if (dectivated)
	{
		lcd.setCursor(0, 1);
		lcd.print("BOMB DEACTIVATED");
		beep(1000);
		Mode = 0;
		SleepTimer = 60;
		goToSleep();
	}
	else
	{
		Mode = 0;
		lcd.setCursor(4, 0);
		printTimeStamp(0);
		lcd.setCursor(0, 1);
		lcd.print(" BOMB EXPLODED!");
		handleSignal();
		SleepTimer = 60;
		goToSleep();
	}
}

void handleCode()
{
	char key = keypad.getKey();
	if (key)
	{
		lcd.setCursor(Position, 1);
		Input[Position] = key;
		Position++;
		keyPress(key);

		if(Position == 16)
		{
			Position = 0;
			for (byte i = 0; i < 16; i++)
			{
				if (Code[i] != Input[i])
				{
					CurrentTries++;
					lcd.clear();
					lcd.setCursor(0, 0);
					lcd.print("  WRONG CODE!   ");
					if (TriesCount > 0 && TriesCount > CurrentTries)
					{
						lcd.setCursor(0, 1);
						lcd.print(TriesCount - CurrentTries);
						lcd.print(" TRIES LEFT");
					}
					beep(1000);
					lcd.clear();
					CodeStatus = CODE_WRONG;
					return;
				}
			}
			CodeStatus = CODE_OK;
			return;
		}
		CodeStatus = CODE_KEY;
	}
	else
	{
		CodeStatus = CODE_NONE;
	}
}

void printTimeStamp(long time)
{
	long hh = time / 3600;
	long mm = (time - (hh * 3600)) / 60;
	long ss = time - (hh * 3600) - (mm * 60);

	if (hh < 10)
	{
		lcd.print("0");
	}
	lcd.print(hh);
	lcd.print(":");
	if (mm < 10)
	{
		lcd.print("0");
	}
	lcd.print(mm);
	lcd.print(":");
	if (ss < 10)
	{
		lcd.print("0");
	}
	lcd.print(ss);
}

void keyPress(char key) {

	lcd.print(key);
	beep(10);
	NextSleep = millis() + SleepTimer;
}

void beep(short time)
{
	digitalWrite (BEEP_PIN, HIGH);
	delay(time);
	digitalWrite (BEEP_PIN, LOW);
}

void handleSignal()
{
	if (SignalTime > 0)
	{
		digitalWrite(SIGNAL_PIN, LOW);
		delay(SignalTime * 1000);
		digitalWrite(SIGNAL_PIN, HIGH);
	}
}

void reconfigurePins ()
{
	byte i;

	// go back to all pins as per the keypad library

	for (i = 0; i < NUMITEMS (colPins); i++)
	{
		pinMode (colPins [i], OUTPUT);
		digitalWrite (colPins [i], HIGH); 
	}  // end of for each column 

	for (i = 0; i < NUMITEMS (rowPins); i++)
	{
		pinMode (rowPins [i], INPUT);
		digitalWrite (rowPins [i], HIGH); 
	}   // end of for each row

} 

void goToSleep ()
{
	byte i;
	// set up to detect a keypress
	for (i = 0; i < NUMITEMS (colPins); i++)
	{
		pinMode (colPins [i], OUTPUT);
		digitalWrite (colPins [i], LOW);   // columns low
	}  // end of for each column

	for (i = 0; i < NUMITEMS (rowPins); i++)
	{
		pinMode (rowPins [i], INPUT);
		digitalWrite (rowPins [i], HIGH);  // rows high (pull-up)
	}  // end of for each row

	// now check no pins pressed (otherwise we wake on a key release)
	for (i = 0; i < NUMITEMS (rowPins); i++)
	{
		if (digitalRead (rowPins [i]) == LOW)
		{
			reconfigurePins ();
			return; 
		} // end of a pin pressed
	}  // end of for each row

	// overcome any debounce delays built into the keypad library
	delay (50);

	// at this point, pressing a key should connect the high in the row to the 
	// to the low in the column and trigger a pin change

	set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
	sleep_enable();

	byte old_ADCSRA = ADCSRA;
	// disable ADC to save power
	ADCSRA = 0;  

	PRR = 0xFF;  // turn off various modules

	PCIFR  |= _BV (PCIF0) | _BV (PCIF1) | _BV (PCIF2);   // clear any outstanding interrupts
	PCICR  |= _BV (PCIE0) | _BV (PCIE1) | _BV (PCIE2);   // enable pin change interrupts

	// turn off brown-out enable in software
	MCUCR = _BV (BODS) | _BV (BODSE);
	MCUCR = _BV (BODS); 
	sleep_cpu ();  

	// cancel sleep as a precaution
	sleep_disable();
	PCICR = 0;  // cancel pin change interrupts
	PRR = 0;    // enable modules again
	ADCSRA = old_ADCSRA; // re-enable ADC conversion

	// put keypad pins back how they are expected to be
	reconfigurePins ();
}  // end of goToSleep