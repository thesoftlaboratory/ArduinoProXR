#ifndef ARDUINO_PROXR_H
#define ARDUINO_PROXR_H

#include <Arduino.h>
#include "ArduinoProXR_config.h"

#define PROXR_MAX_BANKS 32

enum class ProXRErr {
  OK = 0,
  BUSY,
  TIMEOUT,
  OVERFLOW,
  CORRUPTED,
  BAD_REPLY,
  NO_ACK
};

typedef void (*ProXRCallback)(
  ProXRErr status, void* context, 
  const uint8_t* response, size_t len
);

struct BankStatus {
  uint8_t bank;
  uint8_t status;
};

inline void expandByte(uint8_t byte, bool bits[8])
{
  for (int i = 0; i < 8; i ++) bits[i] = byte & (1 << i > 0);
}

inline uint8_t collapseByte(bool bits[8])
{
  uint8_t byte = 0;
  for (int i = 0; i < 8; i ++) byte |= (bits[i] > 0) << i;
  return byte;
}

class RelayBoard
{
private:

  Stream &stream;

  ProXRCallback defaultCallback;
  void* defaultContext;

  enum class Mode { READY, TX, RX } mode;
  long timeout;
  ProXRCallback callback;
  void* context;
  ProXRErr status;

  size_t len;
  uint8_t payload[PROXR_MAX_LEN];

  int pos;
  uint8_t chksum;

  void transmit();
  void receive();

  void resolve(ProXRErr status);

  template <typename Sender, typename Parser>
  ProXRErr waitForReply(Sender sender, Parser parser); 

public:
  
  RelayBoard(Stream &stream);

  void begin();
  void loop();

  void setDefaultCallback(ProXRCallback callback, void* context) {
    this->defaultCallback = callback;
    this->defaultContext = context;
  }

  bool busy() { return mode != Mode::READY; }

  ProXRErr send(const uint8_t* command, size_t len) {
    return send(command, len, defaultCallback, defaultContext);
  }
  ProXRErr send(const uint8_t* command, size_t len, 
    ProXRCallback callback, void *context); 

  ProXRErr testComms();
  ProXRErr enableAutoRefresh();
  ProXRErr disableAutoRefresh();
  ProXRErr autoRefreshEnabled();
  ProXRErr manualRefresh();
  ProXRErr setDefaultState(uint8_t bank);
  ProXRErr setDefaultState();
  ProXRErr readDefaultState(uint8_t bank);
  ProXRErr readDefaultState();
  ProXRErr turnRelayOff(uint8_t bank, uint8_t relay);
  ProXRErr turnRelayOn(uint8_t bank, uint8_t relay);
  ProXRErr readRelayStatus(uint8_t bank, uint8_t relay);
  ProXRErr turnBankOff(uint8_t bank);
  ProXRErr turnBankOn(uint8_t bank);
  ProXRErr invertBank(uint8_t bank);
  ProXRErr reverseBank(uint8_t bank);
  ProXRErr setBankStatus(uint16_t bank, uint8_t status);
  ProXRErr readBankStatus();
  ProXRErr readBankStatus(uint8_t group);

  static ProXRErr parseBankStatus(const uint8_t* reply, size_t len,
    BankStatus& bankStatus);
  static ProXRErr parseRelayStatus(const uint8_t* reply, size_t len, 
    bool& relayStatus);
  static ProXRErr parseAcknowledgement(const uint8_t* reply, size_t len);
  static ProXRErr parseState(const uint8_t* reply, size_t len, bool& state);
  static ProXRErr parseStatusArray(const uint8_t* reply, size_t len, 
    uint8_t* status, size_t expected); 

  ProXRErr testComms_B();
  ProXRErr enableAutoRefresh_B();
  ProXRErr disableAutoRefresh_B();
  ProXRErr autoRefreshEnabled_B(bool& state);
  ProXRErr manualRefresh_B();
  ProXRErr setDefaultState_B(uint8_t bank);
  ProXRErr setDefaultState_B();
  ProXRErr readDefaultState_B(uint8_t bank, uint8_t* status);
  ProXRErr readDefaultState_B(uint8_t* status);
  ProXRErr turnRelayOff_B(uint8_t bank, uint8_t relay, BankStatus& bankStatus);
  ProXRErr turnRelayOn_B(uint8_t bank, uint8_t relay, BankStatus& bankStatus);
  ProXRErr readRelayStatus_B(uint8_t bank, uint8_t relay, bool& relayStatus);
  ProXRErr turnBankOff_B(uint8_t bank, BankStatus& bankStatus);
  ProXRErr turnBankOn_B(uint8_t bank, BankStatus& bankStatus);
  ProXRErr invertBank_B(uint8_t bank, BankStatus& bankStatus);
  ProXRErr reverseBank_B(uint8_t bank, BankStatus& bankStatus);
  ProXRErr setBankStatus_B(uint16_t bank, uint8_t status, BankStatus& bankStatus);
  ProXRErr readBankStatus_B(uint8_t* status);
  ProXRErr readBankStatus_B(uint8_t group, uint8_t* status);

};

#endif
