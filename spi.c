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
*	\brief		SPI hardware driver for Atmel AVR8 CPUs
*	\author		Pat Satyshur
*	\version	1.0
*	\date		12/8/2012
*	\copyright	Copyright 2012, Pat Satyshur
*	\ingroup 	interface
*
*	@{
*/

#include "spi.h"


void InitSPIMaster (uint8_t setup_cpol, uint8_t setup_cpha)
{
	if ((setup_cpol > 1) || (setup_cpha > 1))
	{
		return;		//This is not allowed
	}

	//Turn on power to SPI
	PRR0 &= ~(1<<PRSPI);
	
	//Set SS, SCK and MOSI as outputs
	//Note: the SS pin must not be set as an input and pulled low, or the SPI hardware will switch to slave mode.
	DDRB |= 1|(1<<1)|(1<<2);
	
	//Setup SPI:
	//	Master Mode
	//	SCK is fosc/4
	SPCR = (1<<SPE)|(1<<MSTR)|(setup_cpol<<CPOL)|(setup_cpha<<CPHA);

	return;
}

uint8_t SPISendByte(int8_t ByteToSend)
{
	int8_t resp;
	int16_t timeout = 0;
	
	SPDR = ByteToSend;
	while((SPSR & (1<<SPIF)) != (1<<SPIF))
	{
		if(timeout > SPI_TIMEOUT)
		{
			return 0xFF;
		}
		timeout++;
	}
	
	resp = SPDR;
	return resp;
}


/** @} */
