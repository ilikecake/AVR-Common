/*   This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
*	\brief		TWI hardware driver header file for Atmel AVR8 CPUs
*	\author		Pat Satyshur
*	\version	1.0
*	\date		1/20/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	common
*
*	@{
*/

//Note: the slave address TWI_SLA should only be the 7-bit address. TWI_SLA is left shifted, and the appropriate read or write bit is appended in software.

#ifndef _TWI_H_
#define _TWI_H_

#include "stdint.h"
#include "config.h"

#ifndef TWI_USER_CONFIG
	#error: TWI settings not defined. See twi.h for details.
#endif

/*These setting must be defined in your user code to use the TWI module
 * #define TWI_USER_CONFIG				//Define this in your user code to disable the above error.
 * #undef TWI_USE_ISR					//Define this to enable interrupt driven TWI interface (this code does not work).
 * #define _TWI_DEBUG					//Define this to enable the output of debug messages.
 * #define TWI_USE_INTERNAL_PULLUPS		//Define this to use the internal pull-up resistors of the device.
 * #define TWI_SCL_FREQ_HZ				//The SCL frequency in Hz (100000 is a good value)
 */

//Chect the TWI status and return if an error occured.
//Note: this function must be given a variable. The TWI send command cannot be entered directly
#define TWI_CHECKSTAT(stat) if(stat > 0x00) return stat

#define TWI_BUS_BUSY_TIMEOUT	50000

#ifdef TWI_USE_ISR
	//Global variables for use with interrupt
	volatile int *TWI_TX_data;		//data to transmit pointer
	volatile int *TWI_RX_data;		//data to recieve pointer
	volatile int TWI_TX_bytes;		//Number of bytes to transmit
	volatile int TWI_RX_bytes;		//number of bytes to recieve
	volatile int TWI_SLA;			//Address of slave device
	volatile int TWI_Semaphore;
#endif

//Function Prototypes
void InitTWI(void);
void DeinitTWI(void);
uint8_t TWIRW(uint8_t sla, unsigned char *SendData, unsigned char *RecieveData, uint8_t BytesToSend, uint8_t BytesToRecieve);

void TWIScan( void );

#ifdef TWI_USE_ISR
	#define TWI_CONTROL_ON			0x05
	#define TWI_CONTROL_START		0xA5
	#define TWI_CONTROL_STOP		0x95
	#define TWI_CONTROL_CONTINUE	0x85
	#define TWI_CONTROL_RX_ACK		0xC5
	#define TWI_CONTROL_RX_NOACK	0x85
#else
	#define TWI_CONTROL_ON			0x04
	#define TWI_CONTROL_START		0xA4
	#define TWI_CONTROL_STOP		0x94
	#define TWI_CONTROL_CONTINUE	0x84
	#define TWI_CONTROL_RX_ACK		0xC4
	#define TWI_CONTROL_RX_NOACK	0x84
	#define TWI_CONTROL_INT_MASK	0x80
#endif

//Status Codes
#define TWI_STATUS_MASK				0xF8

//Common
#define TWI_STATUS_START_TX			0x08
#define TWI_STATUS_RS_TX			0x10
#define TWI_STATUS_ARB_LOST			0x38

//Master Transmitter
#define TWI_STATUS_SLAW_ACK			0x18
#define TWI_STATUS_SLAW_NOACK		0x20
#define TWI_STATUS_DATA_TX_ACK		0x28
#define TWI_STATUS_DATA_TX_NOACK	0x30

//Master Reciever
#define TWI_STATUS_SLAR_ACK			0x40
#define TWI_STATUS_SLAR_NOACK		0x48
#define TWI_STATUS_DATA_RX_ACK		0x50
#define TWI_STATUS_DATA_RX_NOACK	0x58

#define TWI_PRESCALE_MASK			0x03
#define TWI_PRESCALE_1				0x00
#define TWI_PRESCALE_4				0x01
#define TWI_PRESCALE_16				0x02
#define TWI_PRESCALE_64				0x03


#endif

/** @} */