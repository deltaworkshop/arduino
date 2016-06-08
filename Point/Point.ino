#include <avr/sleep.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define BEEP_PIN 11
#define SIGNAL_PIN 10

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
const byte kpdSize = 4; //four rows and cols
char keys[kpdSize][kpdSize] = {
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};
byte rowPins[kpdSize] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[kpdSize] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, kpdSize, kpdSize );

unsigned int Melody[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622};

unsigned long EndTime;
unsigned long NextTick;
unsigned long GameTime;
byte Side = 255;
char Name[] ={'A','B','C','D'};
unsigned long Code[] = {0,0,0,0}; // Code[A,B,C,D]
unsigned long Points[] = {0,0,0,0}; // Points[A,B,C,D]
byte Leds[] = {14,15,16,17}; // Leds[A,B,C,D]

unsigned long Backlight = 0;
unsigned long Blink = 0;
bool BLevel = false;
byte Position = 0;
String StrCode = "";

void setup()
{
	lcd.begin(16, 2);
	lcd.clear();
	lcd.backlight();
	lcd.setCursor(0,0);
	lcd.print("    WELCOME!");

	for (byte i = 0; i < 4; i++)
	{
		pinMode(Leds[i], OUTPUT);
		digitalWrite(Leds[i], HIGH);
		keyTone(Name[i], 150);
		delay(150);
		digitalWrite(Leds[i], LOW);
		delay(50);
	}
	delay(200);

	pinMode(BEEP_PIN, OUTPUT);
	digitalWrite(BEEP_PIN, LOW);
	pinMode(SIGNAL_PIN, OUTPUT);
	digitalWrite(SIGNAL_PIN, LOW);

	melodyOK();
	setupDevice();

	Backlight = millis() + 60000; //+1 minute backlight initially
}

void setupDevice()
{
	// 1. Configure game time
	setupTime();
	// 2. Configure side codes
	setupCodes();
	// 3. Configure initial side
	setupSide();
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
	melodyOK();

	EndTime = millis() + (GameTime * 1000);
	NextTick = millis() + 1000;
	lcd.setCursor(4, 0);
	printTimeStamp(GameTime);
}

void setupTime()
{
	lcd.clear();
	lcd.print("GAME TIME sec:");
	lcd.setCursor(0, 1);
	String strTime = "";
	char key = keypad.getKey();
	while (key != '#')
	{
		if (key && key != '*' && key != 'A' && key != 'B' && key != 'C' && key != 'D')
		{
			strTime += key;
			keyPress(key);
		}
		key = keypad.getKey();
	}
	GameTime = strTime.toInt();	
	// don't set a time more than 99 hours (no sense to do that...)
	if (GameTime > 356400)
	{
		GameTime = 356400;
	}
	lcd.noBlink();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("GAME TIME SET:");
	lcd.setCursor(0, 1);
	printTimeStamp(GameTime);
	melodyOK();
}

void setupCodes()
{
	for (byte i = 0; i < 4; i++)
	{
		lcd.clear();
		lcd.print("CODE ");
		lcd.print(Name[i]);
		lcd.print(":");
		lcd.setCursor(0, 1);
		StrCode = "";
		char key = keypad.getKey();
		while (key != '#')
		{
			if (key && key != '*' && key != 'A' && key != 'B' && key != 'C' && key != 'D')
			{
				StrCode += key;
				keyPress(key);
			}
			key = keypad.getKey();
		}
		lcd.noBlink();
		lcd.setCursor(0, 0);
		if (StrCode.length() > 0)
		{
			Code[i] = StrCode.toInt();
			lcd.print("CODE ");
			lcd.print(Name[i]);
			lcd.print(" SET");
			melodyOK();
			StrCode = "";
		}
		else
		{
			lcd.print("SIDE ");
			lcd.print(Name[i]);
			lcd.print(" DISABLED");
			melodyOK();
		}
	}
}

void setupSide()
{
	String sides = "";
	for (byte i = 0; i < 4; i++)
	{
		if (Code[i] > 0)
		{
			sides.concat(Name[i]);
		}
	}
	lcd.clear();
	if (sides.length() == 0)
	{
		lcd.print("NO SIDES ENABLED");
		melodyOK();
		return;
	}	
	lcd.print("START SIDE: ");
	lcd.print(sides);
	lcd.setCursor(0, 1);
	char key = keypad.getKey();
	while (true)
	{
		if (key && (key == 'A' || key == 'B' || key == 'C' || key == 'D' || key == '#'))
		{
			keyPress(key);
			if (key == '#')
			{
				lcd.setCursor(0, 1);
				lcd.print("NEUTRAL");
				Side = 255;
				melodyOK();
				return;
			}
			for (byte i = 0; i < 4; i++)
			{
				if (key == Name[i])
				{
					if (Code[i] > 0)
					{
						lcd.setCursor(0, 0);
						lcd.print("SELECTED SIDE: ");
						Side = i;
						melodyOK();
						resetBlink(millis()); // activate blinking instantly
						return;
					}
					else
					{
						lcd.print(" DISABLED!");
						melodyERR();
						lcd.setCursor(0, 1);
						lcd.print("                ");
						lcd.setCursor(0, 1);
						break;
					}
				}
			}
		}
		key = keypad.getKey();
	}
}

void loop()
{
	unsigned long time = millis();
	blink(time);
	handleBacklight(time);
	if (time < EndTime && time >= NextTick)
	{
		NextTick = time + 1000;
		lcd.setCursor(4, 0);
		printTimeStamp((EndTime - millis()) / 1000);
		if (Side != 255)
		{
			Points[Side]++;
			lcd.setCursor(15, 0);
			lcd.print(Name[Side]);
		}
	}
	else if (time >= EndTime)
	{
		if (Side != 255)
		{
			Points[Side]++;
			resetBlink(0);
		}
		lcd.clear();
		Backlight = 0;
		lcd.backlight();
		lcd.setCursor(6,0);
		lcd.print("GAME");
		lcd.setCursor(6,1);
		lcd.print("OVER");
		digitalWrite(SIGNAL_PIN, HIGH);
		delay(5000);
		digitalWrite(SIGNAL_PIN, LOW);
		handleResult();
		return;
	}

	handleCode(time);
}

void handleCode(unsigned long time)
{
	char key = keypad.getKey();
	if (key && key != 'A' && key != 'B' && key != 'C' && key != 'D')
	{	
		if (Backlight == 0)
		{
			lcd.backlight();
		}
		Backlight = time + 60000; //+1 minute backlight on keypress 

		if (key == '*')
		{
			clearCode();
			keyTone(key, 30);
			lcd.setCursor(0,1);
			lcd.print("                ");
			return;
		}
		else if(key == '#' && StrCode.length() > 0)
		{
			keyTone(key, 30);
			long code = StrCode.toInt();
			for (byte i = 0; i < 4; i++)
			{
				if (Code[i] == code)
				{
					clearCode();
					resetBlink(time);
					Side = i;
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("SIDE ");
					lcd.print(Name[Side]);
					lcd.print(" CAPTURED");
					lcd.setCursor(0,1);
					lcd.print("   THIS POINT   ");
					melodyOK();
					lcd.clear();
					return;
				}
			}
			clearCode();
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("  WRONG CODE!   ");
			melodyERR();
			lcd.clear();
			return;
		}
		else if(key != '*' && key != '#')
		{
			lcd.setCursor(Position, 1);
			StrCode += key;
			Position++;
			keyPress(key);
			if(Position == 16)
				Position = 0;
		}
	}
}

void clearCode()
{
	Position = 0;
	StrCode = "";
}

void handleResult()
{
	lcd.clear();
	lcd.print(" PRESS A B C D");
	lcd.setCursor(0, 1);
	lcd.print(" TO SEE RESULTS");
	char key = keypad.getKey();
	while (true)
	{
		if (key && (key == 'A' || key == 'B' || key == 'C' || key == 'D'))
		{
			keyTone(key,30);
			lcd.clear();
			lcd.print("SIDE ");
			lcd.print(key);
			lcd.print(":");
			lcd.setCursor(0,1);
			lcd.print(key == 'A' ? Points[0] : key == 'B' ? Points[1] : key == 'C' ? Points[2] : Points[3]);
		}
		key = keypad.getKey();
	}
}

void handleBacklight(unsigned long time)
{
	if (Backlight > 0 && time >= Backlight)
	{
		Backlight = 0;
		lcd.noBacklight();
	}
}

void melodyOK()
{
	tone(BEEP_PIN,622,100);
	delay(100);
	tone(BEEP_PIN,311,100);
	delay(100);
	tone(BEEP_PIN,622,100);
	delay(800);
}

void melodyERR()
{
	tone(BEEP_PIN,311,100);
	delay(100);
	tone(BEEP_PIN,622,100);
	delay(100);
	tone(BEEP_PIN,311,100);
	delay(800);
}

void keyPress(char key)
{
	lcd.print(key);
	keyTone(key, 30);
}

void keyTone(char key, unsigned long duration)
{
	switch (key)
	{
	case '1':
		tone(BEEP_PIN,262,duration);
		break;
	case '2':
		tone(BEEP_PIN,294,duration);
		break;
	case '3':
		tone(BEEP_PIN,349,duration);
		break;
	case 'A':
		tone(BEEP_PIN,440,duration);
		break;
	case '4':
		tone(BEEP_PIN,277,duration);
		break;
	case '5':
		tone(BEEP_PIN,330,duration);
		break;
	case '6':
		tone(BEEP_PIN,415,duration);
		break;
	case 'B':
		tone(BEEP_PIN,523,duration);
		break;
	case '7':
		tone(BEEP_PIN,311,duration);
		break;
	case '8':
		tone(BEEP_PIN,392,duration);
		break;
	case '9':
		tone(BEEP_PIN,494,duration);
		break;
	case 'C':
		tone(BEEP_PIN,587,duration);
		break;
	case '*':
		tone(BEEP_PIN,370,duration);
		break;
	case '0':
		tone(BEEP_PIN,466,duration);
		break;
	case '#':
		tone(BEEP_PIN,554,duration);
		break;
	case 'D':
		tone(BEEP_PIN,622,duration);
		break;
	default:
		break;
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

void blink(unsigned long time)
{
	if (Side != 255 && Blink > 0 && time >= Blink)
	{
		Blink = time + 500;
		BLevel = !BLevel;
		digitalWrite(Leds[Side], BLevel);
		if(BLevel)
			keyTone(Name[Side], 100);
	}
}

void resetBlink(unsigned long time)
{
	if(Side != 255)
		digitalWrite(Leds[Side], LOW);
	Blink = time;
	BLevel = false;
}




