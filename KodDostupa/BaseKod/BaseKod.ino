#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <Bounce2.h>

#define MODE_LAUNCH 101
#define MODE_DECODE_START 102
#define MODE_DECODING 103
#define MODE_DECODE_END 104
//#define MODE_RECOVER_START 104
#define MODE_RECOVERING 105

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

byte RocketCode[] = {3, 1, 6, 5, 7, 4, 1, 8, 2, 0, 5, 4, 2, 9, 3, 4};
/*volatile uint8_t Mode = MODE_UNLOAD;
unsigned long Storage[2] = {
	100, 100
};*/
OneWire  ds(10);
//Bounce btn_mode = Bounce();
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

boolean resetFlag = false;
boolean resetByTimeFlag = false;

unsigned long previousTime = 0;
unsigned long resetTime = 0;


byte mode = 0;
int prefixCodePosition = 0;
int suffixCodePosition = 15;
byte activatorCode = 0;

long codeInterval = 600000;
long slowCodeInterval = 600000;
long fastCodeInterval = 360000;
long resetTimeInterval = 600000;

void setup()
{
	Serial.begin(9600);
	pinMode(BEEP_PIN, OUTPUT);
	pinMode(MODE_PIN, INPUT);
	digitalWrite(MODE_PIN, HIGH);
/*	btn_mode.attach(MODE_PIN);
	btn_mode.interval(5);
	btn_mode.update();
	setMode();*/

	lcd.begin(16, 2);
	lcd.clear();
	lcd.backlight();
  
        resetRocketStart();
//        lcd.setCursor(0, 0); // 1 строка
  //      lcd.print("Rocket launch in progress...");
      //  lcd.setCursor(0, 1); // 2 строка
//        lcd.print("01234567899876543210"); 
  
	//printMode();
	//printStorage(0);
	//printStorage(1);
}

void loop()
{
//	checkMode();
	byte addr[8];

	if (!ds.search(addr)) {
		ds.reset_search();
  
                if (resetFlag)        
                {
                     previousTime = millis();
                     activatorCode = 0;                 
                     
                     if (resetByTimeFlag) 
                     {                          
                            unsigned long currentResetTime = millis(); 

                            if(currentResetTime - resetTime > resetTimeInterval) {         
            
                                  beep(500);                                                         
                                  resetByTimeFlag = false;
                            }
                            else
                            {                        
                                  printDecodeInterrupt();
                            }
                     }
                     else
                     {                       
                            resetRocketStart();                            
                     }
                }             
               
                resetFlag = true;
		return;
	}

	if (OneWire::crc8(addr, 7) != addr[7] ||
		(addr[0] != 0xCC && addr[0] != 0x28)) {
			beep(500);
			return;
	}      

        resetFlag = false;       
        resetTime = millis();
        resetByTimeFlag = true;

        if(addr[0] == 0x28) 
        { // activator connected
        
              if (activatorCode == 0)
              {
        	      activatorCode = handleActivator(addr);                       
                      previousTime = millis();        	            	
              }
              else
              {                      
                      decodingStart(activatorCode);        
              }             
        }        
}

byte handleActivator(byte * addr) {

	// compare the addresses (first 7 bytes)
              if(byteArrayCompare(addr, ActivatorA, 8)) {
                      codeInterval = slowCodeInterval;
                      return 1;
	      } else if(byteArrayCompare(addr, ActivatorB, 8)) {
                      codeInterval = slowCodeInterval;
                      return 2;
              } else if(byteArrayCompare(addr, ActivatorC, 8)) {
                      codeInterval = fastCodeInterval;
                      return 3;
              } else if(byteArrayCompare(addr, ActivatorD, 8)) {
                      codeInterval = fastCodeInterval;
                      return 4;
              }    

        return 0;
}

void decodingStart(byte code)
{
       if (code == 0)
             return;        
             
       if ((prefixCodePosition == 0) && (suffixCodePosition == 15))
       {
             printDecodeStart();                
       }      
       
       if (prefixCodePosition == 16 || suffixCodePosition == -1 || ((prefixCodePosition - 1) == suffixCodePosition))
       {
             if (mode == MODE_DECODING || mode == MODE_RECOVERING)
             {
                 lcd.setCursor(0, 0);
                 lcd.print("DECOD. COMPLETE!");
                 
                 mode = MODE_DECODE_END;
             }
             //beep(500);
             return;
       }
       else
       {
            if (mode == MODE_DECODE_START || mode == MODE_RECOVERING)
            {
                lcd.setCursor(0, 0);
                lcd.print("DECODING........");
                
                mode = MODE_DECODING;
            }
       }
     
       unsigned long currentTime = millis(); 
      
       if(currentTime - previousTime > codeInterval) {         
         
            beep(500);
            previousTime = currentTime; 
            
            if (code == 1 || code == 3)
            {
                  lcd.setCursor(prefixCodePosition, 1);
                  lcd.print(RocketCode[prefixCodePosition]);      
                  prefixCodePosition++;
            }
            else if (code == 2 || code == 4)
            {
                lcd.setCursor(suffixCodePosition, 1);
                lcd.print(RocketCode[suffixCodePosition]);
                suffixCodePosition--;
            }            
       }
}

void printDecodeInterrupt()
{
      if (mode == MODE_DECODING || mode == MODE_DECODE_END)
      {
            lcd.setCursor(0, 0);
            lcd.print("RECOVERING......");
            
            mode = MODE_RECOVERING;
      }
}

void resetRocketStart() {

      if (mode != MODE_LAUNCH)
      {
            activatorCode = 0;             
            prefixCodePosition = 0;
            suffixCodePosition = 15;

            lcd.setCursor(0, 0);
            lcd.print(" ROCKET LAUNCH  ");
            lcd.setCursor(0, 1);
            lcd.print("  IN PROGRESS!  ");	
            
            mode = MODE_LAUNCH;
      }
}

void printDecodeStart() {

      if (mode == MODE_LAUNCH)
      {
            lcd.setCursor(0, 1);
            lcd.print("*##$&@**!(-&(#+@");	
            
            mode = MODE_DECODE_START;
      }
}
/*void handleContainer(byte * addr) {
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
}*/

void beep(uint8_t time) {
	digitalWrite(BEEP_PIN, HIGH);
	delay(time);
	digitalWrite(BEEP_PIN, LOW);
}

/*boolean checkMode() {
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
}/

/*void printMode() {
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
}*/

boolean byteArrayCompare(byte a[], byte b[], byte array_size)
{
	for (byte i = 0; i < array_size; ++i)
		if (a[i] != b[i])
			return(false);
	return(true);
}
