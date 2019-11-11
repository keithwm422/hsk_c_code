/*
 * main.cpp
 *
 * Basic run of input/output for LaunchPad communication. All applicability is
 * defined in userTest.cpp. Only the USB port address is intended to be editted
 * in this file.
 *
 * --Serial port name on Windows. If, in 'COM#', # is > 9, portName must include
 *   these backslashes: '\\\\.\\COM#'
 */

/*******************************************************************************
 * Defines
 *******************************************************************************/
#ifdef _WIN32
#include "win_src/SerialPort.cpp"
#include "win_src/SerialPort.h"
#endif

#ifdef __linux__
#include "linux_src/LinuxLib.cpp"
#include "linux_src/LinuxLib.h"
#include "linux_src/SerialPort_linux.cpp"
#include "linux_src/SerialPort_linux.h"
#endif

#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "iProtocol.h"
#include "userTest.h"

#include <fstream>


using std::cin;
using std::cout;
using std::endl;

extern uint8_t cinNumber();

/******************************************************************************/
/* Serial port parameters */
const char *port_name = "/dev/ttyACM0";
int SerialBaud = 1152000;
/******************************************************************************/

/* Name this device */
housekeeping_id myComputer = eSFC;

/* Declarations to keep track of device list */
uint8_t downStreamDevices[254] = {0}; // Keep a list of downstream devices
uint8_t numDevices = 0; // Keep track of how many devices are downstream

/* Keep a log of errors */
uint8_t errorsReceived[254] = {0};
uint8_t numErrors = 0;

/* Create buffers for data */
uint8_t outgoingPacket[MAX_PACKET_LENGTH] = {0}; // Buffer for outgoing packet
uint8_t incomingPacket[MAX_PACKET_LENGTH] = {0}; // Buffer for incoming packet

/* Defining the variable here makes the linker connect where these variables
 * are being used (see iProtocol.h for declaration)	*/
housekeeping_hdr_t *hdr_in;
housekeeping_hdr_t *hdr_out;
housekeeping_err_t *hdr_err;
housekeeping_prio_t *hdr_prio;

/* Utility variableas: */
uint8_t checkin;         // Checksum value for read-in
uint8_t lengthBeingSent; // Actual

uint16_t numberIN;
std::string numbufIN;

/* While connected, variables to check the how long its been since read a byte
 */
int read_result = 0;
std::chrono::time_point<std::chrono::system_clock> newest_zero, newest_result;
std::chrono::duration<double> elapsed_time;

/* Set up a delay */
std::chrono::time_point<std::chrono::system_clock> timeSinceDelay;
std::chrono::duration<double> delayed_time;
bool delayOver = true;
double userTIME;

/* Bool if a reset needs to happen */
bool needs_reset = false;

/*******************************************************************************
 * Functions
 *******************************************************************************/
/* Function flow:
 * --Gets outgoing header from the userTest function 'setupMyPacket'
 * --Computes the checksum of the outgoing data
 * --Asks user what checksum to use
 *
 * Function variables:
 * numbufIN:		Character buffer for user input
 * numberIN:		Unsigned integer that resulted from the conversion
 *
 */
bool setup() {
  cout << "Standby mode? (Type 0 for no delay, or enter an integer # of "
          "seconds)";
  cout << endl;
  if (userTIME = (double)cinNumber()) {
    delayOver = false;
    timeSinceDelay = std::chrono::system_clock::now();
    return false;
  }
  delayOver = true;

  lengthBeingSent = setupMyPacket(hdr_out, hdr_prio); // Fills in rest of header

  /* Compute checksum for outgoing packet + add it to the end of packet */
  fillChecksum((uint8_t *)outgoingPacket);

  cout << "The computed checksum is "
       << (int)outgoingPacket[4 + lengthBeingSent];
  cout << ". Enter this value, or input a different checksum to check protocol "
          "reliability.";
  cout << endl;

  cin >> numbufIN;

  while (cin) {
    numberIN = strtoul(numbufIN.c_str(), 0, 10);

    if (numberIN > 255) {
      cout << "Number too big. Input a number between 0 & 255: ";
      cin >> numbufIN;
    } else {
      outgoingPacket[4 + lengthBeingSent] = (uint8_t)numberIN;
      break;
    }
  }
  cout << endl;
  return true;
}

/* Function flow:
 * --Checks to see if the packet was received from an unknown device
 *		--If so, add that device address to the list of known devices
 * --Sorts through commands & executes defined functions in myprogram.h
 * --If there is no protocol for a command, just reads header + displays data
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 *
 * Function variables:
 * downStreamDevices:		The array of addresses of known devices
 * numDevices:				Running total of downstream devices
 *
 */
void commandCenter(const uint8_t *buffer) {
  /* Check if the device is already known */
  if (findMe(downStreamDevices, downStreamDevices + numDevices, hdr_in->src) ==
      downStreamDevices + numDevices) {
    /* If not, add it to the list of known devices */
    downStreamDevices[numDevices] = hdr_in->src;
    numDevices += 1;
  }

  /* Check commands */
  if (hdr_in->cmd == ePingPong) {
    justReadHeader(hdr_in);
    cout << endl;
  } else if (hdr_in->cmd == eSetPriority) {
    whatToDoIfSetPriority(hdr_in, hdr_prio);
  } else if (hdr_in->cmd == eIntSensorRead) {
    whatToDoIfISR(hdr_in);
  } else if (hdr_in->cmd == 7 && hdr_in->src == 3) {
    whatToDoIfThermistorsTest(hdr_in);
  } else if ((hdr_in->cmd >=2 && hdr_in->cmd<=13) && hdr_in->src == 2) {
    whatToDoIfFloat(hdr_in);
  } else if ((hdr_in->cmd ==14 || hdr_in->cmd==15) && hdr_in->src == 2) {
    whatToDoIfFlow(hdr_in);
  } else if ((hdr_in->cmd >=16 && hdr_in->cmd <=25) && hdr_in->src == 2) {
    whatToDoIfTempProbes(hdr_in);
  } else if (hdr_in->cmd ==26 && hdr_in->src == 2) {
    whatToDoIfPressure(hdr_in);
  } else if (hdr_in->cmd == eMapDevices) {
    whatToDoIfMap(hdr_in);
  } else if ((int)hdr_in->cmd < eSendAll &&
             (int)hdr_in->cmd >= eSendLowPriority && hdr_in->len == 0) {
    cout << "Device #" << (int)hdr_in->src;
    cout << " did not have any data of this priority." << endl << endl;
  } else if (hdr_in->cmd == eError) {
    whatToDoIfError(hdr_err, errorsReceived, numErrors);

//    resetAll(hdr_out);

    /* Compute checksum for outgoing packet + add it to the end of packet */
    fillChecksum((uint8_t *)outgoingPacket);
//    needs_reset = true;
  } else {
    justReadHeader(hdr_in);
    cout << "DATA: ";

    for (int i = 0; i < hdr_in->len; i++) {
      cout << (int)*((uint8_t *)hdr_in + 4 + i) << " ";
    }
    cout << endl << endl;
  }
}

/* Function flow:
 * --Called when a packet is received by the serial port instance
 * --Checks to see if the packet is meant for this device
 *		--If so, check the checksum for data corruption +
 *		  execute the command in the header
 *
 * Function params:
 * buffer:		Pointer to the location of the incoming packet
 * len:			Size of the decoded incoming packet
 *
 * Function variables:
 * checkin:		Utility variable to store a checksum in
 *
 */
void checkHdr(const uint8_t *buffer, size_t len) {
  /* Check if the message was intended for this device
   * If it was, check and execute the command  */
  if (hdr_in->dst == myComputer) {
    if (verifyChecksum((uint8_t *)buffer)) {
      commandCenter(buffer);
    } else{
        cout << "Bummer, checksum did not match." << endl;
        cout << "Length of data is " << (int) hdr_in->len << endl;
        uint8_t * p= buffer;
        cout << "first byte " << (int) *p << endl;
        housekeeping_hdr_t* hdr = (housekeeping_hdr_t*) p;
        uint8_t* data = p + sizeof(housekeeping_hdr_t);
        uint8_t* cksum = data + hdr->len;
        uint8_t sum = 0;
        for (; p <= cksum; p++) sum += *p;
        cout << "checksum and calculated are: " << (int) *cksum << "," << (int) sum << endl;
      }
  }

  /* If it wasn't, cast it as an error & restart */
  else {
    cout << "Bad destination received... Restarting downstream devices."
         << endl;

//    resetAll(hdr_out);

    /* Compute checksum for outgoing packet + add it to the end of packet */
    fillChecksum((uint8_t *)outgoingPacket);
//    needs_reset = true;
  }
}

/*******************************************************************************
 * Main program
 *******************************************************************************/
int main() {

//  ofstream myfile;
//  myfile.open("bugs_test.txt");
  /* Point to data in a way that it can be read as known data structures */
  hdr_in = (housekeeping_hdr_t *)incomingPacket;
  hdr_out = (housekeeping_hdr_t *)outgoingPacket;
  hdr_err = (housekeeping_err_t *)(incomingPacket + 4);
  hdr_prio = (housekeeping_prio_t *)(incomingPacket + 4);

  /* Create the header for the first message */
  hdr_out->src = myComputer; // Source of data packet

  /* Declare an instance of the serial port connection */
  SerialPort TM4C(port_name, SerialBaud);

  /* Check if connection is established */
  if (TM4C.isConnected())
    cout << "Connection Established" << endl;
  else {
    cout << "ERROR, check port name";
    return 0;
  }

  /* Set the function that will act when a packet is received */
  TM4C.setPacketHandler(&checkHdr);

  /* Start up your program & set the outgoing packet data + send it out */
  startUp(hdr_out);
  fillChecksum((uint8_t *)outgoingPacket);
  TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);

  /* On startup: Reset number of found devices & errors to 0 */
  memset(downStreamDevices, 0, numDevices);
  numDevices = 0;
  memset(errorsReceived, 0, numErrors);
  numErrors = 0;

  /* Initialize timing variables for when the last message was received */
  newest_zero = std::chrono::system_clock::now();
  newest_result = std::chrono::system_clock::now();

  /* While the serial port is open, */
  while (TM4C.isConnected()) {
    /* Reads in 1 byte at a time until the full packet arrives.
     * If a full packet is received, update will execute PacketReceivedFunction
     * If no full packet is received, bytes are discarded   */
    read_result = TM4C.update(incomingPacket);

    /* If a packet was decoded, mark down the time it happened */
    if (read_result > 0)
      newest_result = std::chrono::system_clock::now();

    /* If a packet wasn't decoded in this run, write down the current time */
    else
      newest_zero = std::chrono::system_clock::now();

    /* Using the above variable definitions, calculate how long its been since
     * something has been decoded */
    elapsed_time = newest_zero - newest_result;
    delayed_time = std::chrono::system_clock::now() - timeSinceDelay;
    if (delayed_time.count() > userTIME) {
      delayOver = true;
    }

    /* If that ^ time is greater than 1/2 a second, prompt the user again */
    if (elapsed_time.count() > .5 && delayOver) {
      /* Check if a reset needs to be sent */
      if (needs_reset) {
        TM4C.send(outgoingPacket, 4 + hdr_out->len + 1);
        needs_reset = false;
        return 0;
      }

      /* If it doesn't, prompt the user again for packet params  */
      if (setup()) {
        /* Send out the header and packet*/
        TM4C.send(outgoingPacket, 4 + lengthBeingSent + 1);

        /* Reset the timing system */
        newest_zero = std::chrono::system_clock::now();
        newest_result = std::chrono::system_clock::now();
      }
    }
  }
}
