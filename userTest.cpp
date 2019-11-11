/*
 * userTest.cpp
 *
 * General template functions for a specific use case of the main.cpp program.
 * --Program prompts the user for an intended destination and command, then
 * sends out a COBS encoded data packet to that destination.
 *
 */

/*****************************************************************************
 * Defines
 ****************************************************************************/
#include "userTest.h"
#include "iProtocol.h"
#include <iostream>
#include <cstdint>
#include <cstring>

using std::cin;
using std::cout;
using std::endl;

/* User input variables for setupMyPacket() */
char userIN3;

/* Buffer for outgoing data in setupMyPacket(). This can be changed if  */
uint8_t outgoingData[255] = {0};
uint32_t TempRead;

/* Buffer for the temperature reading conversion & output */
float TempC;
float TempF;
uint8_t *tmp;

/*****************************************************************************
 * Functions
 ****************************************************************************/
/* Function flow:
 * --Prints out interface protocol for addressing & commanding downstream
 * devices
 * --Elaborates typedefs located in iProtocol.h
 *
 */
void startUp(housekeeping_hdr_t *hdr_out) {
  cout << "####################################################################"
          "#";
  cout << endl;
  cout << "Destination typedefs:" << '\t' << '\t';
  cout << "Command typedefs:" << endl;

  cout << "SFC" << '\t' << '\t' << "0";
  cout << '\t' << '\t';
  cout << "ePingPong" << '\t' << '\t' << '\t' << "0" << endl;

  cout << "MainHSK" << '\t' << '\t' << "1";
  cout << '\t' << '\t';
  cout << "eSetPriority" << '\t' << '\t' << '\t' << "1" << endl;

  cout << "MagnetHSK" << '\t' << "2";
  cout << '\t' << '\t';
  cout << "Internal Temp Sensor Read" << '\t' << "2" << endl;

  cout << "eDCTHsk" << '\t' << '\t' << "3";
  cout << '\t' << '\t';
  cout << "Map all devices" << '\t' << '\t' << '\t' << "3" << endl;

  cout << "eDCTTemp1" << '\t' << "4";
  cout << '\t' << '\t';
  cout << "eSendLowPriority" << '\t' << '\t' << "250" << endl;

  cout << "eDCTTemp2" << '\t' << "5";
  cout << '\t' << '\t';
  cout << "eSendMedPriority" << '\t' << '\t' << "251" << endl;

  cout << "eBroadcast" << '\t' << "255";
  cout << '\t' << '\t';
  cout << "eSendHiPriority" << '\t' << '\t' << '\t' << "252" << endl;

  cout << '\t' << '\t' << '\t' << '\t';
  cout << "eSendAll" << '\t' << '\t' << '\t' << "253" << endl;

  cout << '\t' << '\t' << '\t' << '\t';
  cout << "Reset" << '\t' << '\t' << '\t' << '\t' << "254" << endl;

  cout << endl << endl;

  cout << "Error codes:" << '\t' << '\t' << '\t'
       << "Priority typedefs: " << endl;

  cout << "EBADDEST" << '\t' << "-1";
  cout << '\t' << '\t';
  cout << "No Priority" << '\t' << '\t' << '\t' << "0" << endl;

  cout << "EBADCOMMAND" << '\t' << "-2";
  cout << '\t' << '\t';
  cout << "Low Priority" << '\t' << '\t' << '\t' << "1" << endl;

  cout << "EBADLEN" << '\t' << '\t' << "-3";
  cout << '\t' << '\t';
  cout << "Med Priority" << '\t' << '\t' << '\t' << "2" << endl;

  cout << "EBADARGS " << '\t' << "-4";
  cout << '\t' << '\t';
  cout << "High Priority" << '\t' << '\t' << '\t' << "3" << endl;
  cout << "####################################################################"
          "#";
  cout << endl << endl;

  hdr_out->dst = eBroadcast;
  hdr_out->cmd = ePingPong;
  hdr_out->len = 0;
}

uint8_t cinNumber() {
  std::string numbuf;
  uint8_t number;

  cin >> numbuf;

  while (cin) {
    number = strtoul(numbuf.c_str(), 0, 10);
    if (number > 255) {
      cout << "Invalid input. Please input a number between 0 & 255: ";
      cin >> numbuf;
    } else
      return number;
  }
  return 0;
}

/* Function flow:
 * --Asks & accepts user input for intended destination & intended command to be
 *   sent.
 * --Asks what length to use, and whether or not to attach those bytes to the
 *   outgoing packet
 * --Builds outgoing data header
 *
 * Function params:
 * hdr_out:		Pointer to the first byte of the outgoing packet
 *              --As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * userIN:		Buffer for user inputted destination device (see
 * typedefs) userIN2:		Buffer for user inputted command (see typedefs)
 * userDST:		User inputted destination that is put in the outgoing
 * packet
 * userCMD:		User inputted command that is put in the outgoing packet
 * numbuf:		User inputted string of character numbers to be
 * converted number:		Unsigned int based on user input
 *
 */
uint8_t setupMyPacket(housekeeping_hdr_t *hdr_out,
                      housekeeping_prio_t *hdr_prio) {
  /* Get user input for message destination */
  cout << "Destination #? " << '\t';
  hdr_out->dst = cinNumber();

  /* Get user input for message command */
  cout << "What command #? ";
  hdr_out->cmd = cinNumber();

  /* Get user input for the length of the message */
  cout << "What length? (A nonzero length may produce an error) " << '\t';
  hdr_out->len = cinNumber();

  /* Ask if it is a fake length for an error test */
  if (hdr_out->len == 0) {
    return hdr_out->len;
  }

  cout << "Attach 'length' number of bytes to the end of the header? (y/n) ";
  cin >> userIN3;
  while (cin && (int)hdr_out->len != 0) {
    if (userIN3 == 'y') {
      cout << "Press enter after inputting each byte as characters:";
      cout << endl;
      for (int i = 0; i < hdr_out->len; i++) {
        outgoingData[i] = cinNumber();
      }
      /* Match data, if there is any */
      matchData(hdr_out);
      return hdr_out->len;

    } else if (userIN3 == 'n') {
      return 0;
    } else {
      cout << "Attach a 'length' number of bytes to the header? (y/n) ";
      cin >> userIN3;
    }
  }
}

/* Function flow:
 * --If a ping pong command is registered in the incoming packet, this function
 *   reads off the contents of the header (src, dst, cmd, len)
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 *              --As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 */
void justReadHeader(housekeeping_hdr_t *hdr_in) {
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl << endl;
}

/* Function flow:
 * --If a device has successfully changed priority of a command, it should have
 *   attached two bytes that describe the command changed and its new priority
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 * hdr_prio:	Pointer to the first byte of the priority change information

 */
void whatToDoIfSetPriority(housekeeping_hdr_t *hdr_in,
                           housekeeping_prio_t *hdr_prio) {
  cout << "Device #" << (int)hdr_in->src;
  cout << " has successfully changed command #" << (int)hdr_prio->command;
  cout << " to priority #" << (int)hdr_prio->prio_type << "." << endl << endl;
}

/* Function flow:
 * --If a fake sensor read (FSR) command is registered in the incoming packet,
 *   this function reads off the contents of the header (src, dst, cmd, len)
 * --It then reads off the data attached in the packet
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 *              --As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * TempRead:	Buffer to put the incoming unsigned 32-bit temperature reading
 * in
 * tmp:			Dummy pointer to the current data location. Gets
 * iterated over TempC:		Float for converted temperature in Celsius
 * TempF: Float for converted temperature in Fahreheit
 *
 */
void whatToDoIfISR(housekeeping_hdr_t *hdr_in) {
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "Internal temperature of device #" << (int)hdr_in->src << ": ";

  TempRead = 0;
  tmp = (uint8_t *)&TempRead;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
    tmp = tmp + 1;
  }

  TempC = (float)(1475 - ((2475 * TempRead) / 4096)) / 10;
  TempF = (float)((TempC * 9) + 160) / 5;

  cout << TempF << " Farenheit." << endl << endl;
}

void whatToDoIfThermistorsTest(housekeeping_hdr_t *hdr_in) {
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "converting to float resistance value in ohms (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp = (uint8_t *)array_temp;
// reverse the bytes for float conversion ugh
  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
    cout << (int) *tmp << endl;
    tmp = tmp + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << res << endl;

// use a union?

}

void whatToDoIfTempProbes(housekeeping_hdr_t *hdr_in){
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "converting to float (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp = (uint8_t *)array_temp;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
    cout << (int) *tmp << endl;
    tmp = tmp + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << res << endl;

}

void whatToDoIfFloat(housekeeping_hdr_t *hdr_in){
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "converting to float (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp = (uint8_t *)array_temp;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
    cout << "raw byte " << i << ": "  << (int) *tmp << endl;
    tmp = tmp + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << "float: " << res << endl;

}

void whatToDoIfPressure(housekeeping_hdr_t *hdr_in){

  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "converting to readable format : " << endl;
  uint8_t array_t[hdr_in->len];
  double pressure_value=0;
  double temperature_value=0;
  char typeOfPressure;
  char error_code[100];

  tmp = (uint8_t *)array_t;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
//    cout << (int) *tmp << endl;
    tmp = tmp + 1;
  }

  if(array_t[hdr_in->len-1]==0){
    memcpy(&pressure_value,&array_t,sizeof(pressure_value));
    memcpy(&temperature_value,&array_t+sizeof(pressure_value),sizeof(temperature_value));
    memcpy(&typeOfPressure,&array_t+sizeof(pressure_value)+sizeof(temperature_value),sizeof(typeOfPressure));
    cout << "Pressure: " << pressure_value << " , " << temperature_value << " , " << typeOfPressure << endl;
  }
  else{
    memcpy(&error_code,&array_t,sizeof(error_code));
    for(int i =0; i<100; i++) {
      cout << error_code[i];
    }
  }
  cout << " " << endl;
  cout << "DONE" << endl;

}
void whatToDoIfFlow(housekeeping_hdr_t *hdr_in){
  cout << "Reading in packet header... " << endl;
  /* Read off header data */
  cout << "Packet source: " << (int)hdr_in->src << endl;
  cout << "Intended destination: " << (int)hdr_in->dst << endl;
  cout << "Command : " << (int)hdr_in->cmd << endl;
  cout << "Length of data attached: " << (int)hdr_in->len << endl;
  cout << "converting to readable format : " << endl;
  uint8_t array_t[hdr_in->len];
  double gas_data[4];
  char gas_type[100];
  char error_code[100];
  tmp = (uint8_t *)array_t;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp = *((uint8_t *)hdr_in + 4 + i);
//    cout << (int) *tmp << " , ";
    tmp = tmp + 1;
  }

  if(array_t[hdr_in->len-1]==0){
    memcpy(&gas_data,&array_t,sizeof(gas_data));
    memcpy(&gas_type,&array_t+sizeof(gas_data),sizeof(gas_type));
    cout << "Gas Data: " << gas_data[0] << " , " << gas_data[1] << " , " << gas_data[2] << " , " << gas_data[3] << endl;
    cout << "Gas Type: ";
    for(int i =0; i<100; i++) {
      cout << gas_type[i];
    }
  }
  else{
    memcpy(&error_code,&array_t,sizeof(error_code));
    for(int i =0; i<100; i++) {
      cout << error_code[i];
    }
  }
  cout << " " << endl;
  cout << "DONE" << endl;

}

/* Function flow:
 * --Function logs any errors it receives. It displays the last error type it
 *   received and prints out a log of all errors in this program's instance
 *
 * Function params:
 * hdr_err:		Pointer to the first byte of the error diagnostic
 * received
 * errors:		Array of past errors. The current error gets put in the
 * array numErrors:	Number of errors total
 *
 */
void whatToDoIfError(housekeeping_err_t *hdr_err, uint8_t *errors,
                     uint8_t &numError) {
  /* Throw an error */
  cout << "Error received: Type " << (int)hdr_err->error - 256 << endl << endl;

  /* Log the error */
  *(errors + 4 * numError) = hdr_err->src;
  *(errors + 4 * numError + 1) = hdr_err->dst;
  *(errors + 4 * numError + 2) = hdr_err->cmd;
  *(errors + 4 * numError + 3) = hdr_err->error;
  numError += 1;

  /* Print out error history */
  cout << "ERROR LOG: Found " << (int)numError << " error(s)" << endl;

  for (int i = 0; i < numError; i++) {
    cout << "Error #" << (i + 1) << ": Origin device #";
    cout << (int)errors[4 * i] << endl;
    cout << "Error #" << (i + 1) << ": Intended for device #";
    cout << (int)errors[4 * i + 1] << endl;
    cout << "Error #" << (i + 1) << ": With the attached command #";
    cout << (int)errors[4 * i + 2] << endl;
    cout << "Error #" << (i + 1) << ": With error code ";
    cout << (int)errors[4 * i + 3] - 256 << endl << endl;
  }

  cout << "Resetting downstream devices..." << endl << endl;
}

/* Function flow:
 * --If a device receives this command, it should send back the number of
 * devices it has attached and their respective addresses. This function prints
 * both
 *
 * Function params:
 * hdr_in:		Pointer to the first byte of the incoming packet
 *
 */
void whatToDoIfMap(housekeeping_hdr_t *hdr_in) {
  cout << "Device #" << (int)hdr_in->src << " has attached devices:" << endl;

  for (int i = 0; i < hdr_in->len; i++) {
    cout << (int)*((uint8_t *)hdr_in + 4 + i) << endl;
  }
  cout << endl;
}

/* Function flow:
 * --Sets the outgoing header so that all devices receive the eReset command.
 *
 * Function params:
 * hdr_out:         Pointer to the first byte of the outgoing packet
 *
 */
void resetAll(housekeeping_hdr_t *hdr_out) {
  hdr_out->dst = eBroadcast;
  hdr_out->cmd = eReset;
  hdr_out->len = 0;
}

/* Function flow:
 * --Matches an array of data with the (global) outgoingPacket data slots
 * --Requires outgoingData as a global uint8_tarray
 *
 * Function params:
 * hdr_out:		Pointer to the first byte of the incoming packet
 *              --As a housekeeping_hdr_t type, contains a src, dst, cmd, & len
 *
 * Function variables:
 * ptr:			Dummy pointer to where we want to put this byte in the
 *outgoing packet. Gets iterated over
 *
 */
void matchData(housekeeping_hdr_t *hdr_out) {
  for (int i = 0; i < hdr_out->len; i++) {
    *((uint8_t *)hdr_out + 4 + i) = outgoingData[i];
  }
}
