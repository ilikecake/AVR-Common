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
*	\brief		Header file for the SHT25 hardware driver.
*	\author		Pat Satyshur
*	\version	1.0
*	\date		2/7/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	hardware
*
*	@{
*/

#ifndef _SHT25_H_
#define _SHT25_H_

#include "stdint.h"

#define SHT25_I2C_ADDR		0x40

//Chip commands
#define SHT25_READ_TEMP_HOLD		0xE3
#define SHT25_READ_RH_HOLD			0xE5
#define SHT25_READ_TEMP_NOHOLD		0xF3
#define SHT25_READ_RH_NOHOLD		0xF5
#define SHT25_WRITE_USER_REG		0xE6
#define SHT25_READ_USER_REG			0xE7
#define SHT25_RESET					0xFE

//User register definitions
#define SHT25_UREG_RES_12b_14b		0x00
#define SHT25_UREG_RES_8b_12b		0x01
#define SHT25_UREG_RES_10b_13b		0x80
#define SHT25_UREG_RES_11b_11b		0x81

#define SHT25_UREG_BATTERY_MASK		0x40
#define SHT25_HEATER_ON				0x04
#define SHT25_HEATER_OFF			0x00
#define SHT25_OTP_OFF				0x02		//Don't turn OTP on
#define SHT25_OTP_ON				0x00

#define SHT25_UREG_RESERVED_MASK	0x38	

void SHT25_Init( void );
uint8_t SHT25_ReadUserReg(uint8_t *RegValue);
uint8_t SHT25_WriteUserReg(uint8_t RegValue);

//Use the no hold method, and poll the device to see when it is done.
uint8_t SHT25_ReadTemp(uint18_t TempValue);

//Use the no hold method, and poll the device to see when it is done.
//Return relative humidity in %
uint8_t SHT25_ReadRH(uint16_t RHValue);



/** @} */