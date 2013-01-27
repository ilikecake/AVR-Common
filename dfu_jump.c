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
*	\brief		DFU jumping functions
*	\author		Pat Satyshur
*	\version	1.0
*	\date		12/19/2012
*	\copyright	Copyright 2012, Pat Satyshur
*	\ingroup 	hardware
*
*	Code to jump from main program execution to the DFU bootloader. This code is based on Dean Camera's example, and requires LUFA.
*	TODO: Fix this to remove LUFA requirement?
*
*	@{
*/

#include "dfu_jump.h"

uint32_t Boot_Key ATTR_NO_INIT;

void Bootloader_Jump_Check(void)
{
	// If the reset source was the bootloader and the key is correct, clear it and jump to the bootloader
	if ((MCUSR & (1<<WDRF)) && (Boot_Key == MAGIC_BOOT_KEY))
	{
		Boot_Key = 0;
		((void (*)(void))BOOTLOADER_START_ADDRESS)(); 
	}
}

void Jump_To_Bootloader(void)
{
	// If USB is used, detach from the bus
	USB_Disable();

	// Disable all interrupts
	cli();

	// Wait two seconds for the USB detachment to register on the host
	for (uint8_t i = 0; i < 128; i++)
        _delay_ms(16);

	// Set the bootloader key to the magic value and force a reset
	Boot_Key = MAGIC_BOOT_KEY;
	wdt_enable(WDTO_250MS);
	for (;;); 
}

/** @} */