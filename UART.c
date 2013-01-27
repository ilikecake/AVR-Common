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

#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "UART.h"

static FILE mystdout = FDEV_SETUP_STREAM(UARTPutChar, NULL, _FDEV_SETUP_WRITE);

void UARTinit (void)
{
	
	PRR &= ~(1 << PRUSART0);		//Turn on power to USART
	
	DDRD |= (1<<1);					//PD1 as output
	DDRB &= ~(1<<0);				//PD0 as input
	
    //Set USART Baud Rate
    UBRR0H = MYUBRR >> 8;
    UBRR0L = MYUBRR;
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);		//Activate RX, TX
	
	#ifdef UART_ENABLE_LOOPBACK
	UARTRXINTON();
	#endif
    
    stdout = &mystdout; 	//Required for printf init
}

int UARTPutChar(char c, FILE *stream)
{
    if (c == '\n') UARTPutChar('\r', stream);
  
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    
    return 0;
}

#ifdef UART_ENABLE_LOOPBACK
//Interrupt driven UART recieve
ISR(USART_RX_vect)
{
	int c;
	c = UDR0;		//Get char from UART recieve buffer
	UDR0 = c;		//Send char out on UART transmit buffer
}
#endif

/** @} */