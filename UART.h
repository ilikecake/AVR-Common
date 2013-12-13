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
*	\brief		Basic UART driver header for Atmel AVR8 CPUs
*	\author		Pat Satyshur
*	\version	1.0
*	\date		1/21/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	common
*
*	@{
*/

#ifndef _UART_H_
#define _UART_H_

#include "stdint.h"
#include "config.h"

#ifndef UART_USER_CONFIG
	#error: UART settings not defined. See uart.h for details.
#endif

//Config Options for UART
//#define UART_BAUD 9600
//#undef UART_ENABLE_LOOPBACK

//Function definitions
#define MYUBRR 			(F_CPU/16/UART_BAUD-1)

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
	#define UARTRXINTON() 	UCSR0B |= (1<<RXCIE0)
	#define UARTRXINTOFF() 	UCSR0B &= !(1<<RXCIE0)
#elif defined (__AVR_ATmega32U4__)
	#define UARTRXINTON() 	UCSR1B |= (1<<RXCIE1)
	#define UARTRXINTOFF() 	UCSR1B &= !(1<<RXCIE1)
#else
	#error: MCU not defined/handled
#endif

extern FILE UART_stdout;

void UARTinit(void);
int UARTPutChar(char c, FILE *stream);

#endif

/** @} */
