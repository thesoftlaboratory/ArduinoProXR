#include <Arduino.h>
#include <ArduinoProXR.h>

#define RELAYBOARD_UART Serial4
#define RELAYBOARD_BAUD 115200

RelayBoard rb(RELAYBOARD_UART);

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  
  RELAYBOARD_UART.begin(RELAYBOARD_BAUD);
  rb.begin();

  ProXRErr result;

  delay(2000);
  Serial.println("Sending command... ");

  BankStatus reply;
  result = rb.setBankStatus_B(1, 0x55, reply);

  Serial.print("Reply (status "); Serial.print((int)result); Serial.print(")");
  Serial.print(": bank = "); Serial.print(reply.bank);
  Serial.print("; relay state = ");
  for (uint8_t i = 1; i > 0; i <<= 1) Serial.print((reply.status & i) > 0);
  Serial.println();

}

void loop()
{
  delay(1000);
}

