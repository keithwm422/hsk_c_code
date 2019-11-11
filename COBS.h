/*
 * COBS.h
 *
 * Declares the library for COBS encoding algorithms
 *
 */

#ifndef COBS_h
#define COBS_h

/* !!! Packet marker for COBS encoding. Make sure that the Energia library's
 *	   packet marker matches this value, or the encoding can't be read by either
 */
#define PACKETMARKER 218

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>

class COBS
{
public:

COBS();

static size_t encode(const uint8_t* buffer,
                     size_t size,
                     uint8_t* encodedBuffer);

static size_t decode(const uint8_t* encodedBuffer,
                     size_t size,
                     uint8_t* decodedBuffer);

static size_t getEncodedBufferSize(size_t unencodedBufferSize);
};

#endif // COBS_H
