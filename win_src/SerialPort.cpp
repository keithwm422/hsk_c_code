/*
 * SerialPort.cpp
 *
 * Defines SerialPort class for reading, writing, and opening a port on Windows
 * for encoded packets.
 *
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#include "SerialPort.h"
#include "../COBS.h"

/*****************************************************************************
 * Contructor/Destructor
 ****************************************************************************/
/* Function flow:
 * --SerialPort instance: Opens up a serial port with specific parameters.
 * --If the port the user chose in main.cpp cannot be opened, spit out an error
 *
 * Function params:
 * portName:    Name of our opened serial port
 *
 */
SerialPort::SerialPort(const char *portName, int SerialBaud)
{
	this->connected = false;

	this->handler = CreateFileA(portName, //port name
	                            GENERIC_READ | GENERIC_WRITE, //Read/Write
	                            0, //No sharing
	                            NULL, //No security
	                            OPEN_EXISTING, //Open existing port only
	                            FILE_ATTRIBUTE_NORMAL, //Non Overlapped I/O (nothing fancy...)
	                            NULL); //Null for comm Devices
	if (this->handler == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			printf("ERROR: Handle was not attached. Reason: %s not available\n", portName);
		}
		else
		{
			printf("ERROR!!!");
		}
	}
	/* Setup serial port parameters in a DCB struct (holds serial port info) */
	else {
		DCB dcbSerialParameters = {0};

		if (!GetCommState(this->handler, &dcbSerialParameters)) {
			printf("failed to get current serial parameters");
		}
		else {
			dcbSerialParameters.BaudRate = SerialBaud; //setting baudrate
			dcbSerialParameters.ByteSize = 8; //setting bytesize = 8
			dcbSerialParameters.StopBits = ONESTOPBIT; //setting stopbits = 1
			dcbSerialParameters.Parity = NOPARITY; //setting parity = None
			dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

			if (!SetCommState(handler, &dcbSerialParameters))
			{
				printf("ALERT: could not set Serial port parameters\n");
			}
			else {
				this->connected = true;
				PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
				Sleep(WAIT_TIME);
			}
		}
	}
}

/* Define the destructor to close the port */
SerialPort::~SerialPort()
{
	if (this->connected) {
		this->connected = false;
		CloseHandle(this->handler); //Closing the serial port
	}
}

/*****************************************************************************
 * Functions
 ****************************************************************************/

/* Function flow:
 * --Sets the packet handler for this instance of SerialPort to a user-defined function
 *
 * Function Params:
 * PacketReceivedFunction:        user-defined function location in memory
 * PacketReceivedFunctionWithSender:  user-defined function location in memory.
 *                    specified when there are > 1 SerialPort instances
 *
 */
void SerialPort::setPacketHandler(PacketHandlerFunction PacketReceivedFunction)
{
	_PacketReceivedFunction = PacketReceivedFunction;
	_PacketReceivedFunctionWithSender = 0;
}
void SerialPort::setPacketHandler(PacketHandlerFunctionWithSender PacketReceivedFunctionWithSender)
{
	_PacketReceivedFunction = 0;
	_PacketReceivedFunctionWithSender = PacketReceivedFunctionWithSender;
}

/* Function flow:
 * --Reads in one byte at a time
 * --If the packetMarker is received, the function decodes the COBS encoded
 *   packet and executes the PacketReceivedFunction.
 * --Makes sure that incomplete packets are thrown out
 * --Returns the number of bytes decoded.
 *
 * Function params:
 * decodeBuffer:  The buffer into which the result will be written
 *
 * Function variables:
 * bytesRead:  total number of bytes read by ReadFile function in that read
 * data:    location where the read byte gets put by ReadFile
 * data_ptr:  pointer to data's location. Used to put byte into receiving buffer
 * time_LastByteReceived:   Time stamp for when the last byte without a complete packet
 * time_Current:            Time stamp for the current time
 * clockNeedsReset:         Bool for when the time stamp should reset
 *
 */
int SerialPort::update(uint8_t *decodeBuffer)
{
	DWORD bytesRead;
	uint8_t data;
	uint8_t*    data_ptr;
	data_ptr = &data;

	/* Gets information about the status of the port's input buffer */
	ClearCommError(this->handler, &this->errors, &this->status);

	/* Evaluate time stamps */
	if (checkForBadPacket()) return 0;

	/* While there are bytes available to read */
	while (this->status.cbInQue > 0)
	{
		/* Evaluate time stamps */
		if (checkForBadPacket()) return 0;

		/* Read in one byte */
		ReadFile(this->handler, &data, 1, &bytesRead, NULL);

		/* If that byte is the packet marker */
		if (data == PACKETMARKER)
		{
			/* Stop the clock */
			this->OK_toGetCurrTime = false;

			/* Decode the packet */
			size_t numDecoded = COBS::decode(_receiveBuffer,
			                                 _receiveBufferIndex,
			                                 decodeBuffer);

			/* If there are not enough bytes in a packet for a header, it might
			   be a phantom signal? */
			if (numDecoded < 4) return 0;

			/* Execute whichever function was defined (with or w/o sender) */
			if (_PacketReceivedFunction)
			{
				_PacketReceivedFunction(decodeBuffer, numDecoded);
			}

			else if (_PacketReceivedFunctionWithSender)
			{
				_PacketReceivedFunctionWithSender(this, _receiveBuffer, numDecoded);
			}

			// Clear the buffer
			memset(_receiveBuffer, 0, _receiveBufferIndex);
			_receiveBufferIndex = 0;
			return(numDecoded);
		}
		else
		{
			this->time_LastByteReceived = std::chrono::system_clock::now();
			this->OK_toGetCurrTime = true;

			if ((this->_receiveBufferIndex + 1) < MAX_PACKET_LENGTH)
			{
				this->_receiveBuffer[_receiveBufferIndex++] = *data_ptr;
			}
			else
			{
				this->_receiveBufferIndex = 0;
				return 0;
			}
		}
	}

	return 0;
}

/* Function flow:
 * --Checks if a byte of a new packet has been read (OK_toGetCurrTime)
 * --Calculates how long its been since a full packet has been received
 * --If it has been over .25 seconds, the bytes are discarded
 *
 */
bool SerialPort::checkForBadPacket()
{
	if (this->OK_toGetCurrTime)
	{
		this->time_Current = std::chrono::system_clock::now();
		this->byteless_interval = this->time_Current - this->time_LastByteReceived;

		/* If an incomplete packet was received, print an error and show the buffer data */
		if (this->byteless_interval.count() >= .25)
		{
			this->OK_toGetCurrTime = false;
			std::cout << "Error: Incomplete packet received. Bytes received:";
			for (int i=0; i < this->_receiveBufferIndex; i++)
			{
				std::cout << (int) this->_receiveBuffer[this->_receiveBufferIndex];
				std::cout << " ";
			}
			this->_receiveBufferIndex = 0;
			std::cout << std::endl << std::endl;
		}
	}
}

/* Function flow:
 * --Send function takes a non-COBS encoded input, encodes it, and writes it
 *   to the serial line.
 * --Returns TRUE if the write was successful, FALSE if not.
 *
 * Function params:
 * buffer:    The message that you want to encode and send
 * buf_size:  The size of the message in bytes
 *
 * Function variables:
 * bytesSend:  total number of bytes sent by WriteFile function in that write
 * encodedBuffer:  buffer to put the encoded message into
 * numEncoded:  number of bytes that were encoded. Does not include packet delimiter
 *
 */
bool SerialPort::send(uint8_t *buffer, size_t buf_size)
{
	DWORD bytesSend;
	/* if the message is not empty & the size of the message wasn't 0 by accident */
	if (buffer != 0 && buf_size != 0)
	{
		/* Gets status of serial port */
		ClearCommError(this->handler, &this->errors, &this->status);

		uint8_t encodedBuffer[COBS::getEncodedBufferSize(buf_size)+1];

		size_t numEncoded = COBS::encode(buffer, buf_size, encodedBuffer);

		WriteFile(this->handler, (void*) encodedBuffer, numEncoded, &bytesSend, 0);

		return true;
	}
	else return false;
}

/* Function flow:
 * --Simply checks if the serial port is connected
 * --Returns the connection status of this instance of SerialPort
 *
 */
bool SerialPort::isConnected()
{
	return this->connected;
}
