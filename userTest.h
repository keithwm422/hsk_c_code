/*
 * userTest.h
 *
 * General template functions for a specific use case of the main.cpp program.
 * --Program prompts the user for an intended destination and command, then sends
 *       out a COBS encoded data packet to that destination.
 *
 */


#pragma once

#include "iProtocol.h"
#include <iostream>

/* Startup function for user interface */
void startUp(housekeeping_hdr_t * hdr_out);

/* Setting the interface protocol for this device */
uint8_t setupMyPacket(housekeeping_hdr_t * hdr_out, housekeeping_prio_t * hdr_prio);

/* Called to display information from a packet header */
void justReadHeader(housekeeping_hdr_t * hdr_in);

/* This device's response to fake sensor read command */
void whatToDoIfISR(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte thermistor resistances to floats */
void whatToDoIfThermistorsTest(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfTempProbes(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfFloat(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfPressure(housekeeping_hdr_t * hdr_in);

/* This response to convert 4 byte temp probes to floats */
void whatToDoIfFlow(housekeeping_hdr_t * hdr_in);

/* Displays the result of a set priority command */
void whatToDoIfSetPriority(housekeeping_hdr_t * hdr_in, housekeeping_prio_t * hdr_prio);

/* Reads out the error type received + prints a log of all errors since startup */
void whatToDoIfError(housekeeping_err_t * hdr_err, uint8_t * errorsReceived, uint8_t & numError);

/* Reads out the device map received from a device */
void whatToDoIfMap(housekeeping_hdr_t * hdr_in);

/* Sets up a reset command going to all devices */
void resetAll(housekeeping_hdr_t * hdr_out);

/* Puts the outgoing data inside the outgoing packet */
void matchData(housekeeping_hdr_t * hdr_out);
