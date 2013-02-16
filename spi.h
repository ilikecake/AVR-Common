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
*	\brief		SPI hardware driver header file for Atmel AVR8 CPUs
*	\author		Pat Satyshur
*	\version	1.0
*	\date		12/8/2012
*	\copyright	Copyright 2012, Pat Satyshur
*	\ingroup 	interface
*
*	@{
*/


//SPI hardware interface for Atmel ATmegaxU4 header file
//11/9/2011 - PDS

#ifndef _SPI_H_
#define _SPI_H_

#include "stdint.h"

#define SPI_TIMEOUT		10000

void InitSPIMaster (uint8_t setup_cpol, uint8_t setup_cpha);

uint8_t SPISendByte(int8_t ByteToSend);








#endif

/** @} */