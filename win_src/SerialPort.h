/*
 * SerialPort.h
 *
 * Declares library for opening, reading, and writing to a serial port in COBS encoding.
 *
 */

#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <chrono>

/* Max data length is:
 *	--4 header bytes
 *	-- + 255 data bytes
 *  -- + 1 CRC (checksum) byte
 * For max packet size,
 *	-- +1 COBS overhead
 *	-- +1 COBS packet marker
 */
#define MAX_PACKET_LENGTH (4 + 255 + 1) + 2

/* define WAIT_TIME for time to wait after connecting to board */
#define WAIT_TIME 2000

class SerialPort
{
public:
SerialPort(const char *portName, int SerialBaud);
~SerialPort();

/* Function definitions */
int update(uint8_t *buffer);
bool send(uint8_t *buffer, size_t buf_size);
bool isConnected();
bool checkForBadPacket();

/* typdefs for On-package-received function */
typedef void (*PacketHandlerFunction)(const uint8_t * buffer,
                                      size_t size);
typedef void (*PacketHandlerFunctionWithSender)(const void * sender,
                                                const uint8_t * buffer,
                                                size_t size);

/* Functions for setting the packet handler	*/
void setPacketHandler(PacketHandlerFunction PacketReceivedFunction);
void setPacketHandler(PacketHandlerFunctionWithSender PacketReceivedFunctionWithSender);

private:
/* Windows required params for creating serial port instance */
HANDLE handler;
bool connected;
COMSTAT status;
DWORD errors;

/* COBS helper variables for receiving an unknown packet */
uint8_t _receiveBuffer[MAX_PACKET_LENGTH] = {0};
size_t _receiveBufferIndex = 0;

/* On-packet-received function initialization */
PacketHandlerFunction _PacketReceivedFunction = 0;
PacketHandlerFunctionWithSender _PacketReceivedFunctionWithSender = 0;

/* Timing variables for discarding incomplete packets */
std::chrono::time_point<std::chrono::system_clock> time_LastByteReceived, time_Current;
std::chrono::duration<double> byteless_interval;
bool OK_toGetCurrTime = false;
};
