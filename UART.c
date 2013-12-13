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

FILE UART_stdout = FDEV_SETUP_STREAM(UARTPutChar, NULL, _FDEV_SETUP_WRITE);

void UARTinit (void)
{
	#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
		PRR &= ~(1<<PRUSART0);			//Turn on UART power
		DDRD |= (1<<1);					//PD1 as output (TxD)
		DDRB &= ~(1<<0);				//PD0 as input (RxD)
		
		//Set USART Baud Rate
		UBRR0H = MYUBRR >> 8;
		UBRR0L = MYUBRR;
		UCSR0B = (1<<RXEN0)|(1<<TXEN0);		//Activate RX, TX
		
		
		
		
	#elif defined (__AVR_ATmega32U4__)
		PRR0 &= ~(1<<PRUSART0);			//Turn on UART power
		DDRD |= (1<<3);					//PD3 as output (TxD)
		DDRB &= ~(1<<2);				//PD2 as input (RxD)
		
		//Set USART Baud Rate
		UBRR1H = MYUBRR >> 8;
		UBRR1L = MYUBRR;
		UCSR1B = (1<<RXEN1)|(1<<TXEN1);		//Activate RX, TX
	#else
		#error: MCU not defined/handled
	#endif
	
   
	
	#ifdef UART_ENABLE_LOOPBACK
	UARTRXINTON();	//TODO:Check if this works
	#endif
    
    //stdout = &mystdout; 	//Required for printf init
}

int UARTPutChar(char c, FILE *stream)
{
    if (c == '\n') UARTPutChar('\r', stream);
  
	#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
		loop_until_bit_is_set(UCSR0A, UDRE0);
		UDR0 = c;
	#elif defined (__AVR_ATmega32U4__)
		loop_until_bit_is_set(UCSR1A, UDRE1);
		UDR1 = c;
    #else
		#error: MCU not defined/handled
	#endif
	
    return 0;
}

#ifdef UART_ENABLE_LOOPBACK
//Interrupt driven UART loopback
ISR(USART_RX_vect)
{
	uint8_t c;

	#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
		c = UDR0;		//Get char from UART recieve buffer
		UDR0 = c;		//Send char out on UART transmit buffer
	#elif defined (__AVR_ATmega32U4__)
		c = UDR1;		//Get char from UART recieve buffer
		UDR1 = c;		//Send char out on UART transmit buffer
    #else
		#error: MCU not defined/handled
	#endif
}
#endif

/** @} */