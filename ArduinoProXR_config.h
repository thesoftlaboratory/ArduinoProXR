#ifndef ARDUINO_PROXR_CONFIG_H
#define ARDUINO_PROXR_CONFIG_H

/* Message buffer size 
 * Buffer size for ProXR message payload (not including API codec 
 * header/checksum). The largest command or reply payload in the ProXR Advanced 
 * command set is 32 bytes. Payloads longer than the value set here will result
 * in a ProXRErr::OVERFLOW error
 */
#define PROXR_MAX_LEN 32 // bytes

/* Command timeout
 * The maximum time (milliseconds) to wait from when send() is called to receive
 * the entire reply.
 */ 
#define PROXR_PACKET_TIMEOUT 25 // ms

/* Wait for stream output buffer
 * If defined, bytes will not be transmitted until Stream::availableForWrite()
 * returns a positive value. This avoids calling Stream::write() when it may 
 * block. However, in some implementations of Stream, availableForWrite() always
 * returns 0, in which case this macro should be not defined or else commands 
 * will never be sent.
 */
#undef PROXR_WAIT_FOR_STREAM

/* Enable debug info
 * If defined, debug info will be printed to the Print instanced defined by the
 * PROXR_DEBUG_STREAM macro
 */
#undef PROXR_VERBOSE
#define PROXR_VERBOSE

/* Debug info output stream
 * An instance of Print or a derived class to print debug info to, if the
 * PROXR_VERBOSE macro is defined.
 */
#define PROXR_DEBUG_STREAM Serial

#endif
