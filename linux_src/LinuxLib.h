/*
 * LinuxLib.h
 *
 * Provides helper functions for SerialPort_linux. Sets up the serial
 * connection and preps the serial bus.
 *
 * Function declarations.
 *
 */

#pragma once

#include <iostream>

#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
// #include "/usr/include/asm-generic/termbits.h"
#include "termbits2.h"
#include "/usr/include/asm-generic/ioctls.h"

/* Decide if the inputted baudrate is a default */
static int rate_to_constant(int baudrate) {
#define B(x) case x: return B ## x
	switch(baudrate) {
		B(50);     B(75);     B(110);    B(134);    B(150);
		B(200);    B(300);    B(600);    B(1200);   B(1800);
		B(2400);   B(4800);   B(9600);   B(19200);  B(38400);
		B(57600);  B(115200); B(230400); B(460800); B(500000);
		B(576000); B(921600); B(1000000); B(1152000); B(1500000);
		B(2000000); B(2500000); B(3000000); B(3500000); B(4000000);
	default: return 0;
	}
#undef B
}

/* Set up the serial port for raw data communication */
int set_interface_attribs (int portName, int speed, int parity);

/* Change the minimum bytes & timeout for the connection on the fly */
void set_mincount (int portName, int should_block);
