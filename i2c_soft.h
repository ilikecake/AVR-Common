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
*	\brief		I2C master software driver header.
*	\author		Pat Satyshur
*	\version	1.0
*	\date		2/3/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	common
*
*	@{
*/


//Note: the slave address TWI_SLA should only be the 7-bit address. TWI_SLA is left shifted, and the appropriate read or write bit is appended in software.

#ifndef _I2C_SOFT_H_
#define _I2C_SOFT_H_

#include "stdint.h"
#include "config.h"

#ifndef I2C_SOFT_USER_CONFIG
	#error: I2C software driver settings not defined. See i2c_soft.h for details.
#endif

/*These setting must be defined in your user code to use the software I2C module
 * #define I2C_SOFT_USER_CONFIG				//Define this in your user code to disable the above error.
 *
 * Function setup
 * #define I2C_SOFT_USE_INTERNAL_PULLUPS	1		//Set to 1 to use internal pullups on the pins
 * #define I2C_SOFT_USE_ARBITRATION			1		//Set to 1 to enable arbitration
 * #define I2C_SOFT_USE_CLOCK_STRETCH		1		//Set to 1 to enable clock stretching detection
 * #define I2C_SOFT_CLOCK_STRETCH_TIMEOUT	1000	//The timeout for the clock stretching, this is a 16-bit number
 *
 *SDA and SCL pin defintions
 * #define I2C_SDA_PORT			PORTC
 * #define I2C_SDA_DDR			DDRC
 * #define I2C_SDA_PIN			PINC
 * #define I2C_SDA_PIN_NUM		6
 * #define I2C_SCL_PORT			PORTB
 * #define I2C_SCL_DDR			DDRB
 * #define I2C_SCL_PIN			PINB
 * #define I2C_SCL_PIN_NUM		7
 */

//These status codes are based on the status codes from the atmel AVR8s
//Except that code 0x00 means all is well
#define SOFT_I2C_STAT_OK				0x00
#define SOFT_I2C_STAT_START				0x08
#define SOFT_I2C_STAT_RSTART			0x10
#define SOFT_I2C_STAT_SLAW_ACK			0x18
#define SOFT_I2C_STAT_SLAW_NOACK		0x20
#define SOFT_I2C_STAT_DATA_TX_ACK		0x28
#define SOFT_I2C_STAT_DATA_TX_NOACK		0x30
#define SOFT_I2C_STAT_ARB_LOST			0x38

#define SOFT_I2C_STAT_SLAR_ACK			0x40
#define SOFT_I2C_STAT_SLAR_NOACK		0x48
#define SOFT_I2C_STAT_DATA_RX_ACK		0x50
#define SOFT_I2C_STAT_DATA_RX_NOACK		0x58

#define SOFT_I2C_STAT_PARAMETER_ERROR	0xAE
#define SOFT_I2C_STAT_BUS_ERROR			0xAF

/** Initalize the I2C Software pins */
void I2CSoft_Init(void);

/** Read/write data on the I2C line
*	\param[in] sla The 7-bit slave address of the I2C device.
*	\param[in] *SendData A pointer to the data to send to the device.
*	\param[in] *RecieveData A pointer to the data to receive from the device.
*	\param[in] BytesToSend The number of bytes to send.
*	\param[in] BytesToRecieve The number of bytes to receive.
*
*	\return The I2C status (0x00 for OK)
*/
uint8_t I2CSoft_RW(uint8_t sla, uint8_t *SendData, uint8_t *RecieveData, uint8_t BytesToSend, uint8_t BytesToRecieve);

/** Wait for a device to become ready by repeatedly sending the device address and looking for an ACK from the device.
*	\param[in] sla The 7-bit slave address of the I2C device.
*
*	\return The I2C status (0x00 for OK)
*/
uint8_t I2CSoft_WaitForAck(uint8_t sla);

/** Scans the I2C address space and prints out the devices found */
void I2CSoft_Scan(void);



#endif

/** @} */
