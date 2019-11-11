/*
 * iProtocol.cpp
 *
 * Declares the interface protocol for cross device communication.
 *
 * Housekeeping packet consists of header
 * 0-255 payload bytes
 * 1 byte CRCS (or checksum)
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#pragma once
#include <stdint.h>

/* Standard error types */
#define EBADDEST 		-1	// Packet received for an unknown destination
#define EBADCOMMAND		-2  // Packet contained an unknown command
#define EBADLEN			-3	// Packet was the wrong length for the command
#define EBADARGS		-4	// Packet's command arguments (packets of data)

/*******************************************************************************
* Typedef enums
*******************************************************************************/
/* Src/dst definitions. 255 is illegal for source. */
typedef enum housekeeping_id
{
	eSFC = 252,			   // Science flight computer
	eMainHsk = 1,		   // Main housekeeping board
	eMagnetHsk = 2,		 // Magnet housekeeping board
	eDCTHsk = 3,		   // DCT housekeeping board
	eDCTTemp1 = 4,	   // DCT temperature sensor housekeeping board #1
	eDCTTemp2 = 5,		 // DCT temperature sensor housekeeping board #2
	// etc...
	eBroadcast = 255	 // Illegal as a source, but dst=255 means everyone responds
} housekeeping_id;

/* Command definitions */
typedef enum housekeeping_cmd
{
	ePingPong = 0,
	eSetPriority = 1,
	//2-249 are board-specific: these are test commands
	eIntSensorRead = 2,
	eMapDevices = 3,
	eHeaterControl = 4,
	eTestHeaterControl = 5,
    eAutoPriorityPeriod = 6,
	ePacketCount = 7, 
	eTestMode = 249,
	eSendLowPriority = 250,
	eSendMedPriority = 251,
	eSendHiPriority = 252,
	eSendAll = 253,
	eReset = 254,
	eError = 255
} housekeeping_cmd;


/* Priority definitions */
typedef enum housekeeping_prio_type
{
	eNoPriority = 0,
	eLowPriority = 1,
	eMedPriority = 2,
	eHiPriority = 3
} housekeeping_prio_type_t;



/*******************************************************************************
* Typedef structs
*******************************************************************************/
typedef struct housekeeping_hdr_t
{
	uint8_t dst; 	// Destination of packet
	uint8_t src;	// Source of packet
	uint8_t cmd; 	// Command (or response) type
	uint8_t len; 	// Bytes to follow - 1
} housekeeping_hdr_t;


typedef struct housekeeping_err_t
{
	uint8_t src;	// Source of the original packet that caused the error
	uint8_t dst;	// Destination of the original packet that caused the error
	uint8_t cmd;	// Command which caused the error
	uint8_t error;	// Error that occured
} housekeeping_err_t;

typedef struct housekeeping_prio_t
{
	uint8_t command;	// The command we are setting the priority of
	uint8_t prio_type;	// The priority level we are setting it to
} housekeeping_prio_t;


/*******************************************************************************
* Functions
*******************************************************************************/

/* Computes the checksum of a message
 * Parameters are the range of an array, lowest value is closed, highest is open*/
//uint8_t computeMySum(const uint8_t * first, const uint8_t * last);

/* Returns true if two checksum values are the same */
//bool checkMySum(uint8_t & checkI, const uint8_t & checkO);

void fillChecksum(uint8_t* p);
bool verifyChecksum(uint8_t * p);


/* Find address in an array of addresses */
uint8_t * findMe(uint8_t * first, uint8_t * last, uint8_t address);


/*******************************************************************************
* Defines - global variables
*******************************************************************************/
extern housekeeping_hdr_t * hdr_in;		// pointer for loc of incoming data header
extern housekeeping_hdr_t * hdr_out;	// pointer for loc of outgoing header
extern housekeeping_err_t * hdr_err;	// pointer for loc of error header
extern housekeeping_prio_t * hdr_prio;	// pointer for loc of priority header
