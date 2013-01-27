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
*	\brief		DFU Jump header file
*	\author		Pat Satyshur
*	\version	1.0
*	\date		12/19/2012
*	\copyright	Copyright 2012, Pat Satyshur
*	\ingroup 	hardware
*
*	@{
*/


#ifndef _DFU_JUMP_H_
#define _DFU_JUMP_H_

#include <avr/wdt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <LUFA-120730/Drivers/USB/USB.h>

#define MAGIC_BOOT_KEY            0xDC42ACCA

#if (MCU==atmega32u4)||(MCU==atmega32u2)
	#define BOOTLOADER_START_ADDRESS  (32768 - 4096)
#elif (MCU==atmega16u4)||(MCU==atmega16u2)
	#define BOOTLOADER_START_ADDRESS  (16384 - 4096)	//untested
#else
	#error: Bootloader start address not defined
#endif

void Bootloader_Jump_Check(void) ATTR_INIT_SECTION(3);
void Jump_To_Bootloader(void);

#endif

/** @} */