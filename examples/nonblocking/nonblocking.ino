#include <Arduino.h>
#include <ArduinoProXR.h>

#define RELAYBOARD_UART Serial4 // Change to match the Serial instance matching the UART you are using
#define RELAYBOARD_BAUD 115200

RelayBoard rb(RELAYBOARD_UART);

void callback(ProXRErr status, void* context, const uint8_t* response, size_t len);

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  
  RELAYBOARD_UART.begin(RELAYBOARD_BAUD);
  rb.setDefaultCallback(callback, nullptr);
  rb.begin();

  ProXRErr result;

  delay(2000);
  Serial.println("Sending command... ");

  BankStatus reply;
  ProXRErr result = rb.turnRelayOn(2, 3);
  if (result != ProXRErr::OK) {
    Serial.print("Failed to queue command (status "); Serial.print((int)result); Serial.println(")");
    return;
  }

  while (rb.busy()) {
    rb.loop();
  }
}

void loop()
{
  delay(1000);
}

void callback(ProXRErr status, void* context, const uint8_t* response, size_t len)
{
  if (status != ProXRErr::OK) {
    Serial.print("Relay board error (status "); Serial.print((int)status); Serial.println(")");
    return;
  }


  BankStatus reply;
  ProXRErr result = parseBankStatus(response, len, reply);
  if (result != ProXRErr::OK) {
    Serial.print("Failed to parse response (status "); Serial.print((int)result); Serial.println(")");
    return;
  }

  Serial.print("Reply: bank = "); Serial.print(reply.bank);
  Serial.print("; relay state = ");
  for (uint8_t i = 1; i > 0; i <<= 1) Serial.print((reply.status & i) > 0);
  Serial.println();
}

