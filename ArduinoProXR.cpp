#include "ArduinoProXR.h"

// ProXR API packet structure
#define POS_START   0
#define POS_LEN     1
#define POS_PAYLOAD 2
#define START_BYTE 0xAA

// 
#ifdef PROXR_WAIT_FOR_STREAM
  #define READY_TO_WRITE stream.available()
#else
  #define READY_TO_WRITE true
#endif 

// Debugging macros
#ifdef PROXR_VERBOSE
  #define DEBUG(msg)     PROXR_DEBUG_STREAM.print(msg);
  #define DEBUG_LN(msg)  PROXR_DEBUG_STREAM.println(msg);
  #define IF_VERBOSE(do) do
#else
  #define DEBUG(msg)     
  #define DEBUG_LN(msg)
  #define IF_VERBOSE(do)
#endif



/* =============================================================================
   Setup & loop
============================================================================= */

RelayBoard::RelayBoard(Stream &stream)
  : stream(stream)
{ }

void RelayBoard::begin()
{ }

void RelayBoard::loop()
{
  if (mode == Mode::TX) transmit();
  if (mode == Mode::RX) receive();
  if (busy() && millis() >= timeout) resolve(ProXRErr::TIMEOUT);
}



/* =============================================================================
   Handle command/reply 
============================================================================= */

ProXRErr RelayBoard::send(
  const uint8_t* command, size_t len,
  ProXRCallback callback, void* context
) {
  DEBUG_LN("Queueing transaction... ");

  if (busy())              return ProXRErr::BUSY;
  if (len > PROXR_MAX_LEN) return ProXRErr::OVERFLOW;

  this->timeout  = millis() + PROXR_PACKET_TIMEOUT;
  memcpy(this->payload, command, len);
  this->len      = len;
  this->callback = callback;
  this->context  = context;

  pos = 0;
  chksum = 0;
  while (stream.available()) stream.read();

  mode = Mode::TX;
  return ProXRErr::OK; 
}

void RelayBoard::resolve(ProXRErr status)
{
  DEBUG_LN("Resolving transaction... ");

  this->status = status;
  if (callback) {
    callback(status, context, payload, status == ProXRErr::OK ? len : 0);
  }
  mode = Mode::READY;
}



/* =============================================================================
   Codec logic
============================================================================= */

void RelayBoard::transmit()
{
  IF_VERBOSE(int nbytes = 0;)
  bool done;
  for (done = false; !done && READY_TO_WRITE; pos ++) {
    uint8_t byte;

    switch (pos) {
      case POS_START:
        byte = START_BYTE;
        break;
      case POS_LEN:
        byte = len;
        break;
      default:
        int i = pos - POS_PAYLOAD;
        done = i == len;
        if (done) byte = chksum;
        else      byte = payload[i];
        break;
    }  

    if (stream.write(byte) != 1) return; 
    DEBUG(!(nbytes ++) ? "Tx: " : " ") DEBUG(byte)
    chksum += byte;
  }
  DEBUG(nbytes > 0 ? "\n" : "");

  if (done) {
    pos = 0;
    chksum = 0;
    mode = Mode::RX;
  }
}

void RelayBoard::receive()
{
  IF_VERBOSE(int nbytes = 0;)
  bool done;
  for (done = false; !done && stream.available(); pos ++) {
    int received = stream.read();
    if (received < 0) break;
    uint8_t byte = (uint8_t)received;
    DEBUG(!(nbytes ++) ? "Rx: " : " ") DEBUG(byte)

    switch (pos) {
      case POS_START:
        if (byte != START_BYTE) continue;
        break;
      case POS_LEN:
        len = byte;
        if (len > PROXR_MAX_LEN) resolve(ProXRErr::OVERFLOW);
        break;
      default:
        int i = pos - POS_PAYLOAD;
        done = i == len;
        if (!done) payload[i] = byte;
        else resolve(byte == chksum ? ProXRErr::OK : ProXRErr::CORRUPTED); 
        break;
    }

    chksum += byte;
  }
  DEBUG(nbytes > 0 ? "\n" : "");
}



/* =============================================================================
   Commands
============================================================================= */

#define HEADER_BYTE              0xFE
#define ACK_BYTE                 0x55

#define CMD_TEST_COMMS           0x85
#define CMD_ENABLE_AUTO_REFRESH  0x7D
#define CMD_DISABLE_AUTO_REFRESH 0x7E
#define CMD_READ_AUTO_REFRESH    0x88
#define CMD_MANUAL_REFRESH       0x89
#define CMD_SET_DEFAULT_STATE    0x8E   
#define CMD_READ_DEFAULT_STATE   0x8F
#define OFFSET_TURN_RELAY_OFF    0x64
#define OFFSET_TURN_RELAY_ON     0x6C
#define OFFSET_READ_RELAY        0x74
#define CMD_TURN_BANK_OFF        0x81
#define CMD_TURN_BANK_ON         0x82
#define CMD_INVERT_BANK          0x83
#define CMD_REVERSE_BANK         0x84
#define CMD_SET_BANK_STATUS      0x8C
#define CMD_READ_BANK_STATUS     0x7C

#define SEND_COMMAND(...) do { \
  uint8_t cmd[] = { HEADER_BYTE, __VA_ARGS__ }; \
  return send(cmd, sizeof(cmd)); \
} while(0)

ProXRErr RelayBoard::testComms()
{
  SEND_COMMAND(CMD_TEST_COMMS);
}

ProXRErr RelayBoard::enableAutoRefresh()
{
  SEND_COMMAND(CMD_ENABLE_AUTO_REFRESH);
}

ProXRErr RelayBoard::disableAutoRefresh()
{
  SEND_COMMAND(CMD_DISABLE_AUTO_REFRESH);
}

ProXRErr RelayBoard::autoRefreshEnabled()
{
  SEND_COMMAND(CMD_READ_AUTO_REFRESH);
}

ProXRErr RelayBoard::manualRefresh()
{
  SEND_COMMAND(CMD_MANUAL_REFRESH);
}

ProXRErr RelayBoard::setDefaultState(uint8_t bank)
{
  SEND_COMMAND(CMD_SET_DEFAULT_STATE, bank);
}

ProXRErr RelayBoard::setDefaultState()
{
  setDefaultState(0);
}

ProXRErr RelayBoard::readDefaultState(uint8_t bank)
{
  SEND_COMMAND(CMD_READ_DEFAULT_STATE, bank);
}

ProXRErr RelayBoard::readDefaultState()
{
  readDefaultState(0);
}

ProXRErr RelayBoard::turnRelayOff(uint8_t bank, uint8_t relay)
{ 
  SEND_COMMAND(OFFSET_TURN_RELAY_OFF + relay - 1, bank); 
}

ProXRErr RelayBoard::turnRelayOn(uint8_t bank, uint8_t relay)
{
  SEND_COMMAND(OFFSET_TURN_RELAY_ON + relay - 1, bank); 
}

ProXRErr RelayBoard::readRelayStatus(uint8_t bank, uint8_t relay)
{
  SEND_COMMAND(OFFSET_READ_RELAY + relay - 1, bank);
}

ProXRErr RelayBoard::turnBankOff(uint8_t bank)
{
  SEND_COMMAND(CMD_TURN_BANK_OFF, bank);
}

ProXRErr RelayBoard::turnBankOn(uint8_t bank)
{
  SEND_COMMAND(CMD_TURN_BANK_ON, bank);
}

ProXRErr RelayBoard::invertBank(uint8_t bank)
{
  SEND_COMMAND(CMD_INVERT_BANK, bank);
}

ProXRErr RelayBoard::reverseBank(uint8_t bank)
{
  SEND_COMMAND(CMD_REVERSE_BANK, bank);
}

ProXRErr RelayBoard::setBankStatus(uint16_t bank, uint8_t status)
{
  SEND_COMMAND(CMD_SET_BANK_STATUS, status, (uint8_t)bank, (uint8_t)(bank << 8));
}

ProXRErr RelayBoard::readBankStatus()
{
  SEND_COMMAND(CMD_READ_BANK_STATUS, 0);
}

ProXRErr RelayBoard::readBankStatus(uint8_t group)
{
  SEND_COMMAND(CMD_READ_BANK_STATUS, 0, group);
}


/* =============================================================================
   Helpers to parse replies
============================================================================= */

ProXRErr RelayBoard::parseBankStatus(const uint8_t* reply, size_t len, 
  BankStatus& bankStatus)
{
  if (len != 2) return ProXRErr::BAD_REPLY;
  bankStatus.bank   = reply[0];
  bankStatus.status = reply[1];
  return ProXRErr::OK;
}

ProXRErr RelayBoard::parseRelayStatus(const uint8_t* reply, size_t len, 
  bool& relayStatus)
{
  if (len != 1) return ProXRErr::BAD_REPLY;
  relayStatus = reply[0];
  return ProXRErr::OK;
}

ProXRErr RelayBoard::parseAcknowledgement(const uint8_t* reply, size_t len)
{
  if (len != 1) return ProXRErr::BAD_REPLY;
  if (reply[0] != ACK_BYTE) return ProXRErr::NO_ACK;
  return ProXRErr::OK;
}

ProXRErr RelayBoard::parseState(const uint8_t* reply, size_t len, bool& state)
{
  if (len != 1) return ProXRErr::BAD_REPLY;
  state = reply[0];
  return ProXRErr::OK;
}

ProXRErr RelayBoard::parseStatusArray(const uint8_t* reply, size_t len,
  uint8_t* status, size_t expected)
{
  if (len != expected) return ProXRErr::BAD_REPLY;
  memcpy(status, reply, expected);
  return ProXRErr::OK;
}



/* =============================================================================
   Blocking command wrappers 
============================================================================= */

template <typename Sender, typename Parser>
ProXRErr RelayBoard::waitForReply(Sender sender, Parser parser)
{
  ProXRErr senderErr = sender();
  if (senderErr != ProXRErr::OK) return senderErr;
  
  callback = nullptr;
  while (busy()) loop();
  if (status != ProXRErr::OK) return status;

  ProXRErr parserErr = parser();
  if (parserErr != ProXRErr::OK) return parserErr;

  return ProXRErr::OK;
}

#define SEND_COMMAND_AND_WAIT(sender, parser) do{ \
  return waitForReply([&] { return sender; }, [&] { return parser; }); \
} while(0) 

ProXRErr RelayBoard::testComms_B()
{
  SEND_COMMAND_AND_WAIT(
    testComms(), 
    parseAcknowledgement(payload, len)
  );
}

ProXRErr RelayBoard::enableAutoRefresh_B()
{
  SEND_COMMAND_AND_WAIT(
    enableAutoRefresh(), 
    parseAcknowledgement(payload, len)
  );
}

ProXRErr RelayBoard::disableAutoRefresh_B()
{
  SEND_COMMAND_AND_WAIT(
    disableAutoRefresh(), 
    parseAcknowledgement(payload, len)
  );
}

ProXRErr RelayBoard::autoRefreshEnabled_B(bool& state)
{
  SEND_COMMAND_AND_WAIT(
    autoRefreshEnabled(), 
    parseState(payload, len, state)
  );
}

ProXRErr RelayBoard::manualRefresh_B()
{
  SEND_COMMAND_AND_WAIT(
    manualRefresh(), 
    parseAcknowledgement(payload, len)
  );
}

ProXRErr RelayBoard::setDefaultState_B(uint8_t bank)
{
  SEND_COMMAND_AND_WAIT(
    setDefaultState(bank), 
    parseAcknowledgement(payload, len)
  );
}

ProXRErr RelayBoard::setDefaultState_B()
{
  return setDefaultState_B(0);
}

ProXRErr RelayBoard::readDefaultState_B(uint8_t bank, uint8_t* status)
{
  size_t replyLen = bank > 0 ? 1 : PROXR_MAX_BANKS;
  SEND_COMMAND_AND_WAIT(
    setDefaultState(bank), 
    parseStatusArray(payload, len, status, replyLen)
  );
}

ProXRErr RelayBoard::readDefaultState_B(uint8_t* status)
{
  return readDefaultState_B(0, status);
}

ProXRErr RelayBoard::turnRelayOff_B(uint8_t bank, uint8_t relay, 
  BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    turnRelayOff(bank, relay), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::turnRelayOn_B(uint8_t bank, uint8_t relay, 
  BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    turnRelayOn(bank, relay), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::readRelayStatus_B(uint8_t bank, uint8_t relay, 
  bool& relayStatus)
{
  SEND_COMMAND_AND_WAIT(
    readRelayStatus(bank, relay), 
    parseRelayStatus(payload, len, relayStatus)
  );
}

ProXRErr RelayBoard::turnBankOff_B(uint8_t bank, BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    turnBankOff(bank), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::turnBankOn_B(uint8_t bank, BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    turnBankOn(bank), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::invertBank_B(uint8_t bank, BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    invertBank(bank), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::reverseBank_B(uint8_t bank, BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    reverseBank(bank), 
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::setBankStatus_B(uint16_t bank, uint8_t status, BankStatus& bankStatus)
{
  SEND_COMMAND_AND_WAIT(
    setBankStatus(bank, status),
    parseBankStatus(payload, len, bankStatus)
  );
}

ProXRErr RelayBoard::readBankStatus_B(uint8_t* status)
{
  SEND_COMMAND_AND_WAIT(
    readBankStatus(),
    parseStatusArray(payload, len, status, PROXR_MAX_BANKS)
  );
}

ProXRErr RelayBoard::readBankStatus_B(uint8_t group, uint8_t* status)
{
  SEND_COMMAND_AND_WAIT(
    readBankStatus(group),
    parseStatusArray(payload, len, status, PROXR_MAX_BANKS)
  );
}
