## Overview

ArduinoProXR is an Arduino C++ library to communicate with Fusion Series relay boards (National Control Devices, LLC) using the ProXR Advanced command set. The author is not affiliated with National Control Devices, LLC.

Features:
- Compatible with any derived class of `Stream`
- No dynamic memory allocation
- Provides both a non-blocking interface for real-time applications as well as a blocking interface for simplicity

## Installation

Prerequisites:
- [Arduino IDE](https://docs.arduino.cc/software/ide/)
- [Git](https://git-scm.com) (optional)

### Method 1: Download as .zip

1. Download [this repository as a .zip file](https://github.com/iofarm/ArduinoProXR/archive/refs/heads/main.zip).
2. In the Arduino IDE, go to 'Sketch' > 'Include Library' > 'Add .ZIP Library'
3. Selected the downloaded .zip file and click 'Open'.

### Method 2: Clone from GitHub

1. Find the location of your sketchbook folder in the Arduino IDE settings
under 'Sketchbook location'. In the next steps, this path will be referred to as `<sketchbook>`.
2. Create a `libraries` folder within your sketchbook folder if it does not already exist.
3. Clone this repository to your libraries folder: in a command prompt, enter `git clone https://github.com/iofarm/ArduinoProXR.git <sketchbook>/libraries/ArduinoProXR`.

## Configuration

Several configuration options can be changed by editing `ArduinoProXR_config.h`.

## Usage

### Setup

Initialize `RelayBoard` using an instance of a class derived from `Stream` representing the hardware connection with the RelayBoard. For example, if your Arduino is connected to the Fusion controller via UART 2:

```cpp
RelayBoard rb(Serial2);
```

Call `RelayBoard::begin()` _after_ setting up the `Stream` instance; for example, in `setup()`:

```cpp
Serial2.begin(115200);
while (!Serial2);
rb.begin();
```

### Sending commands (non-blocking)

To send a command using the non-blocking interface, you must:

1. Queue the command using `send()` or one of its wrappers
2. Call `loop()` until the Arduino receives a reply from the relay board
3. Parse the reply within your callback function

ArduinoProXR uses callback functions matching the signature

```cpp
void (*)(
  ProXRErr status, 
  void* context, 
  const uint8_t* response, 
  size_t len
)
```

where

- `ProXRErr status` is the error code
- `void* context` is a pointer set by the user
- `const uint8_t* response` is the decoded binary response from the relay board
- `size_t len` is the length of the response

To set the callback function and `context` pointer, use

```cpp
setDefaultCallback(ProXRCallback callback, void* pointer);
```

Use `send()` to send a raw binary command, which takes the following arguments:

- `const uint8_t* command` - the binary command (not encoded)
- `size_t len` - the length of `command`
- `callback` (optional) - overrides the default callback function
- `context` (optional) - overrides the default context pointer

`send()` returns a `ProXRErr` error code indicating whether the command was successfuly queued - see __Error codes__ for details.

A number of wrappers for `send()` are included for specific commands. These wrappers do not currently allow you to override the default callback and context. See __Commands__ for details.

Your callback function can use one of the parser functions to parse and validate the response bytes. Each parser function takes the response bytes, response length, and reference(s) to variables to store the data in; and returns a `ProXRErr` return code indicating if there was a problem parsing the response. The struct `BankStatus` is used for responses that include a bank number and the status of relays in that bank.

### Sending commands (blocking)

Each wrapper function for a specific command also includes a blocking version indicated by the suffix `_B`. These functions will block until a response is received or until the command times out. They will also parse the responses, so some require references to variabels to store the response data in. See __Commands__ for details.

## Commands

The following wrappers for `send()` are available. Please see the [ProXR Advanced command set documentation](https://ncd.io/blog/fusion-proxr-advanced-quick-start-guide/) for details on using these commands.

| Non-blocking | Response parser | Blocking wrapper |
|:-------------------|:-------------------|:-------------------|
|`testComms()`|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`testComms_B()`|
|`enableAutoRefresh()`|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`enableAutoRefresh_B()`|
|`disableAutoRefresh()`|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`disableAutoRefresh_B()`|
|`autoRefreshEnabled()`|`parseState(const uint8_t* reply, size_t len, bool& state)`|`autoRefreshEnabled_B(bool& state)`|
|`manualRefresh()`|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`manualRefresh_B()`|
|`setDefaultState(uint8_t bank)`|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`setDefaultState_B(uint8_t bank)`|
|`setDefaultState()`\*|`parseAcknowledgement(const uint8_t* reply, size_t len)`|`setDefaultState_B()`|
|`readDefaultState(uint8_t bank)`|`parseStatusArray(const uint8_t* reply, size_t len, uint8_t* status, size_t expected)`\*\*|`readDefaultState_B(uint8_t bank, uint8_t* status)`\*\*|
|`readDefaultState()`|`parseStatusArray(const uint8_t* reply, size_t len, uint8_t* status, size_t expected)`\*\*\*|`readDefaultState_B(uint8_t* status)`\*\*\*|
|`turnRelayOff(uint8_t bank, uint8_t relay)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`turnRelayOff_B(uint8_t bank, uint8_t relay, BankStatus& bankStatus)`|
|`turnRelayOn(uint8_t bank, uint8_t relay)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`turnRelayOn_B(uint8_t bank, uint8_t relay, BankStatus& bankStatus)`|
|`readRelayStatus(uint8_t bank, uint8_t relay)`|`parseRelayStatus(const uint8_t* reply, size_t len, bool& relayStatus)`|`readRelayStatus_B(uint8_t bank, uint8_t relay, bool& relayStatus)`|
|`turnBankOff(uint8_t bank)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`turnBankOff_B(uint8_t bank, BankStatus& bankStatus)`|
|`turnBankOn(uint8_t bank)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`turnBankOn_B(uint8_t bank, BankStatus& bankStatus)`|
|`invertBank(uint8_t bank)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`invertBank_B(uint8_t bank, BankStatus& bankStatus)`|
|`reverseBank(uint8_t bank)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`reverseBank_B(uint8_t bank, BankStatus& bankStatus)`|
|`setBankStatus(uint16_t bank, uint8_t status)`|`parseBankStatus(const uint8_t* reply, size_t len, BankStatus& bankStatus)`|`setBankStatus_B(uint16_t bank, uint8_t status, BankStatus& bankStatus)`|
|`readBankStatus()`|`parseStatusArray(const uint8_t* reply, size_t len, uint8_t* status, size_t expected)`\*\*\*|`readBankStatus_B(uint8_t* status)`\*\*\*|
|`readBankStatus(uint8_t group)`|`parseStatusArray(const uint8_t* reply, size_t len, uint8_t* status, size_t expected)`\*\*\*|`readBankStatus_B(uint8_t group, uint8_t* status)`\*\*\*|

\* `bank` defaults to `0` (all banks)  
\*\* 1 byte will be written to `status`, unless `bank == 0`, in which case 32 bytes will be written  
\*\*\* 32 bytes will be written to `status`  

## Error codes

API functions return a `ProXRErr` enum. `0` indicates success, while positive values indicate an error:

| Return code | Meaning |
|:---------|:-----------------------------|
|`OK` (`0`)|Success|
|`BUSY`|Could not queue command because another command is already queued|
|`TIMEOUT`|Did not receive a response within the specified interval|
|`OVERFLOW`|The command or response length was greater than the available buffer size|
|`CORRUPTED`|The checksum did not match the packet|
|`BAD_REPLY`|The response was not formatted as expected|
|`NO_ACK`|Did not receive an acknowledgement code (`0x55`)|
