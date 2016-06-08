#include <VirtualWire.h>

#define DEVICE_ID 1
#define LED_PIN 13

void setup()
{
  Serial.begin(9600);
  vw_set_rx_pin(2);
  vw_set_ptt_inverted(true); // Необходимо для DR3100
  vw_setup(200); // Задаем скорость приема
  vw_rx_start(); // Начинаем мониторинг эфира

  //Setup the LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop()
{
  uint8_t buf[4];
  uint8_t buflen = 4;

  if (vw_get_message(buf, &buflen) && // if we received a message
  buf[0] == '*' && buf[1] != 0 && buf[3] == '#' && // and it is a command
  ((buf[2] != 0 && DEVICE_ID >= buf[1] && DEVICE_ID <= buf[2]) || // and our device is in range of recipients
  (buf[2] == 0 && DEVICE_ID == buf[1]))) // or it is exactly this device
  {
    indicate(buf); // indicate the messge was received
  }
}

void indicate(uint8_t* buf)
{
  digitalWrite(LED_PIN, HIGH);
  Serial.println((char *)buf);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}





