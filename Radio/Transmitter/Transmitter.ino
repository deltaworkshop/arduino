#include <Bounce2.h>
#include <VirtualWire.h>

#define BUTTON_PIN 9
#define LED_PIN 13

// Instantiate a Bounce object
Bounce debouncer = Bounce(); 
uint8_t msg[] = {'*', 1, 3, '#'};
unsigned long send_timeout;

void setup() {
  Serial.begin(9600);
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);

  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);

  //Setup the LED
  pinMode(LED_PIN,OUTPUT);

  vw_set_tx_pin(8);
  vw_set_ptt_inverted(true); // Необходимо для DR3100
  vw_setup(200); // Устанавливаем скорость передачи (бит/с)
}

void loop() {
  // Update the debouncer
  debouncer.update();

  // Get the update value
  int value = debouncer.read();

  // Turn on or off the LED
  if ( value == HIGH) {
    digitalWrite(LED_PIN, HIGH );
  } 
  else if(millis() > send_timeout) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Sending...");
    vw_send(msg, 8);
    vw_wait_tx();
    Serial.println("Done.");
    send_timeout = millis() + 1000;
  }

}








