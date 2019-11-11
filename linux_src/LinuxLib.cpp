/*
 * LinuxLib.cpp
 *
 * Provides helper functions for SerialPort_linux. Sets up the serial
 * connection and preps the serial bus.
 *
 * Function definitions
 *
 */

#include "LinuxLib.h"

/*****************************************************************************
 * Functions
 ****************************************************************************/

/* Function flow
 * --Creates a linux terminal struct (termios options) to put serial bus
   information into
 * --Sets specific serial line flags to enable raw input + output
 *
 * Function params:
 * portName			Name of our opened serial port in SerialPort_linux
 * speed			Baudrate of the proposed serial communication
 * parity			Specifies the parity (here is no parity)
 *
 * Function variables:
 * options:			A POSIX standard struct which stores our serial options
 *
 */
int set_interface_attribs (int portName, int speed, int parity)
{
	/* Check for default baudrate */
	int rate = rate_to_constant(speed);

	if (!rate)
	{
		std::cout << "HEY";
		struct termios2 options2;

		if (ioctl(portName, TCGETS2, &options2)) {
			printf("Error from TCGETS2: %s\n", strerror(errno));
			return -1;
		}
		/* Set I/O options location & baud rate speed */
		options2.c_ispeed = options2.c_ospeed = speed;
		options2.c_cflag &= ~CBAUD;
		options2.c_cflag |= BOTHER;

		/* 8 bits, no parity, no stop bits */
		options2.c_cflag &= ~PARENB;
		options2.c_cflag &= ~CSTOPB;
		options2.c_cflag &= ~CSIZE;
		options2.c_cflag |= CS8;
		//
		// /* no hardware flow control */
		options2.c_cflag &= ~CRTSCTS;
		// /* enable receiver, ignore status lines */
		options2.c_cflag |= CREAD | CLOCAL;
		//
		// /* disable input/output flow control, disable restart chars */
		options2.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
		//
		// /* disable canonical input, disable echo,
		//    disable visually erase chars,
		//    disable terminal-generated signals */
		// options2.c_lflag &= ~(ICANON | ECHO | ECHONL | ECHOE | ISIG | IEXTEN);
		//
		// /* disable output processing */
		options2.c_oflag &= ~OPOST;

		if (ioctl(portName, TCSETS2, &options2))
		{
			printf("Error from TCSETS2: %s\n", strerror(errno));
			return -1;
		}
	}

	else
	{
		struct termios options1;

		if (tcgetattr(portName, &options1) < 0) {
			printf("Error from tcgetattr: %s\n", strerror(errno));
			return -1;
		}

		cfsetospeed(&options1, (speed_t)rate);
		cfsetispeed(&options1, (speed_t)rate);

		/* Overkill: sets up termios for raw data transfer. All of these flags are set
		   below. Hope it might improve compatibility across systems. */
		cfmakeraw(&options1);

		/* 8 bits, no parity, no stop bits */
		options1.c_cflag &= ~PARENB;
		options1.c_cflag &= ~CSTOPB;
		options1.c_cflag &= ~CSIZE;
		options1.c_cflag |= CS8;

		/* no hardware flow control */
		options1.c_cflag &= ~CRTSCTS;
		/* enable receiver, ignore status lines */
		options1.c_cflag |= CREAD | CLOCAL;

		/* disable input/output flow control, disable restart chars */
		options1.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);

		/* disable canonical input, disable echo,
		   disable visually erase chars,
		   disable terminal-generated signals */
		options1.c_lflag &= ~(ICANON | ECHO | ECHONL | ECHOE | ISIG | IEXTEN);

		/* disable output processing */
		options1.c_oflag &= ~OPOST;

		/* flush the input buffer, prep for new communication. I don't think this works */
		tcflush(portName, TCIFLUSH);

		/* set all these new options. If they don't set, spit out an error */
		if (tcsetattr(portName, TCSANOW, &options1) != 0)
		{
			printf("Error from tcsetattr: %s\n", strerror(errno));
			return -1;
		}
	}


	return 0;
}

/* Function flow
 * --Set blocking variables VMIN & VTIME on the fly
 *
 * Function params:
 * portName			Name of our opened serial port in SerialPort_linux
 * mcount    The proposed minimum # bytes that can be used by our program
 *
 * Function variables:
 * options:			A POSIX standard struct which stores our serial options
 *
 */
/* Set blocking variables VMIN & VTIME on the fly */
void set_mincount(int portName, int mcount)
{
	struct termios options;

	if (tcgetattr(portName, &options) < 0) {
		printf("Error tcgetattr: %s\n", strerror(errno));
		return;
	}

	options.c_cc[VMIN] = mcount ? 1 : 0;
	options.c_cc[VTIME] = 1;

	if (tcsetattr(portName, TCSANOW, &options) < 0)
		printf("Error tcsetattr: %s\n", strerror(errno));
}
