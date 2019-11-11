/*
 * COBS.cpp
 *
 * Defines the COBS class
 *
 * /Bakercp excerpt
 * Consistent Overhead Byte Stuffing (COBS) is an encoding that removes all of
 * a defined PACKETMARKER from arbitrary binary data. The encoded data consists
 * only of bytes with values from 0x01 to 0xFF. This is useful for preparing
 * data for transmission over a serial link (RS-232 or RS-485 for example), as
 * the PACKETMARKER byte can be used to unambiguously indicate packet boundaries.
 * COBS also has the advantage of adding very little overhead (at least 1 byte,
 * plus up to an additional byte per 254 bytes of data). For messages smaller
 * than 254 bytes, the overhead is constant.
 *
 * Typical use case makes the PACKETMARKER by the 0 byte
 * --PACKETMARKER found in COBS.h
 */

#include "COBS.h"

using std::cout;
using std::endl;

/*****************************************************************************
 * Contructor
 ****************************************************************************/
COBS::COBS()
{
};

/*****************************************************************************
 * Functions
 ****************************************************************************/

/* Function Flow
 * --Encode a byte buffer with the COBS encoder.
 * --Return the number of bytes written to the \p encodedBuffer.
 *
 * Function Params:
 * buffer:			A pointer to the unencoded buffer to encode.
 * size:			The number of bytes in the \p buffer.
 * encodedBuffer:	The buffer for the encoded bytes.
 *
 * \warning The encodedBuffer must have at least getEncodedBufferSize() allocated.
 *
 */
size_t COBS::encode(const uint8_t* buffer,
                    size_t size,
                    uint8_t* encodedBuffer)
{
	size_t read_index  = 0;
	size_t write_index = 1;
	size_t code_index  = 0;
	uint8_t code       = 1;

	/* while the current byte being read is inside the message string*/
	while (read_index < size)
	{
		/* If the byte is actually the PACKETMARKER, set the last overhead byte/
		       packetMarker to be the # of bytes away the next PACKETMARKER is */
		if (buffer[read_index] == PACKETMARKER)
		{
			encodedBuffer[code_index] = (code + PACKETMARKER) & 0xFF;
			code = 1;
			code_index = write_index++;
			read_index++;
		}
		/* If the byte is not the PACKETMARKER, set the encoded buffer byte to
		         the current read byte.  */
		else
		{
			encodedBuffer[write_index++] = buffer[read_index++];
			code++;

			if (code == 0xFF)
			{
				encodedBuffer[code_index] = (code + PACKETMARKER) & 0xFF;
				code = 1;
				code_index = write_index++;
			}
		}
	}

	/* Set the previous PACKETMARKER location to the # of bytes away the next one is */
	encodedBuffer[code_index] = (code + PACKETMARKER) & 0xFF;
	/* NOTE!!!!
	    --We need an additional packetMarker here so that the message sent contains
	      an ending packetMarker
	 */
	encodedBuffer[write_index++] = PACKETMARKER;

	return write_index;
}

/* Function Flow
 * --Decode a COBS-encoded buffer.
 * --Return the number of bytes written to the \p decodedBuffer.
 *
 * Function Params:
 * encodedbuffer:	A pointer to the \p encodedBuffer to decode.
 * size:			The number of bytes in the \p encodedBuffer.
 * decodedBuffer:	The target buffer for the decoded bytes.
 *
 * \warning ecodedBuffer must have a minimum capacity of size.
 *
 */
size_t COBS::decode(const uint8_t* encodedBuffer,
                    size_t size,
                    uint8_t* decodedBuffer)
{
	if (size == 0) {
		return 0;
	}

	size_t read_index  = 0;
	size_t write_index = 0;
	uint8_t code       = 0;
	uint8_t i          = 0;

	/* while the current byte being read is inside the message string*/
	while (read_index < size)
	{
		/* the first code byte is the # of bytes we'll have to read until the
		    next packetMarker is hit */
		code = (encodedBuffer[read_index] - PACKETMARKER) & 0xFF;

		/* NOTE!!!!
		    --size + 1 vs. size
		    This is some hiccup with the C++ here. The compiler evaluates
		    the '>' operator as >=, so we need to include a +1 after size*/
		/* if the 'code' is read and says that the next zero is outside the packet,
		    this shit is broke, so end the program */
		if (read_index + code > size + 1 && code != 1)
		{
			return 0;
		}

		read_index++;

		/* write the non-encoded bits to the buffer until the next packetMarker */
		for (i = 1; i < code; i++)
		{
			decodedBuffer[write_index++] = encodedBuffer[read_index++];
		}
		/* as long as the next zero is not 255 bytes away and we are not currently
		    reading the last bit of the message, write a zero where the previous 'code'
		    marker was in the encoded message. */
		if (code != 0xFF && read_index != size)
		{
			decodedBuffer[write_index++] = PACKETMARKER;
		}
	}
	return write_index;
}

/* Function Flow
 * --Get the maximum encoded buffer size needed for a given unencoded buffer size.
 * --Returns the maximum size of the required encoded buffer.
 *
 * Function Params:
 * encodedbuffer:	unencodedBufferSize The size of the buffer to be encoded.
 *
 */
size_t COBS::getEncodedBufferSize(size_t unencodedBufferSize)
{
	return unencodedBufferSize + unencodedBufferSize / 254 + 2;
}
