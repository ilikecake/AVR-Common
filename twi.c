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
*	\brief		TWI hardware driver for Atmel AVR8 CPUs
*	\author		Pat Satyshur
*	\version	1.0
*	\date		1/20/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	interface
*
*	@{
*/

#include <avr/pgmspace.h> 
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>

#include "twi.h"

//Initalizes TWI
void InitTWI(void)
{
	//This register value changes based on the MCU used. It will probably be PRR or PRR0
	//This will need to be updated when new controllers are used
	#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
		PRR &= ~(1<<PRTWI);						//Turn on TWI power
	#elif defined (__AVR_ATmega32U4__)
		PRR0 &= ~(1<<PRTWI);					//Turn on TWI power
	#else
		#error: MCU not defined/handled
	#endif
	
	DDRD &= 0xFC;	//Set SDA and SCL pins as input
	
	#ifdef TWI_USE_INTERNAL_PULLUPS
	//TODO: Is this working??
	PORTD &= 0x00;	//Enable internal pull up on SDA and SCL
	#endif
	
	TWBR = ((F_CPU/TWI_SCL_FREQ_HZ)-16)/2;
	TWSR = TWI_PRESCALE_1;
	TWCR = TWI_CONTROL_ON;

#ifdef TWI_USE_ISR
	TWI_Semaphore = 0;
#endif
}



void DeinitTWI(void)
{
	//Turn of TWI
	TWCR = 0x00;
	return;
}




/*int TWIWrite(int sla, int *data, int bytesToSend)
{
	//int recieveTWI[5];
	//int *dptrA = &recieveTWI[0];
	
	if ((sla > 0) && (sla < 128))		//check if SLA is valid
	{
		//TWI_RX_data = dptrA;
		TWI_TX_data = data;
		TWI_TX_bytes = bytesToSend;
		TWI_RX_bytes = 0;
		TWI_SLA = sla << 1;				//Format address with R/W bit 0 (write)
		//TWI_SLA = (sla << 1) | 0x01;		//Format address with R/W bit 1 (read)
		//printf("starting I2C\n");
		TWCR = TWI_CONTROL_START;
	}
	//printf("data(0) = %d\n",*data);

	return 0;
}*/

/*int TWIRead(int sla, int *ReadData)
{

	return 0;
}*/

//In polling mode, returns 0 for successful transfer, TWISR for invalid transfer.
uint8_t TWIRW(uint8_t sla, unsigned char *SendData, unsigned char *RecieveData, uint8_t BytesToSend, uint8_t BytesToRecieve)
{
#ifdef TWI_USE_ISR		//This doesnt quite work
	InitTWI();
	//printf("TWCR: %d\n",TWCR);
	//
	if (TWI_Semaphore == 0) 			//Check if TWI hardware is available
	{
		TWI_TX_bytes = BytesToSend;			//Send 1 byte
		TWI_RX_bytes = BytesToRecieve;		//Recieve 1 byte
		TWI_TX_data = SendData;
		TWI_RX_data = RecieveData;
		TWI_SLA = sla << 1;
		//printf("Data = %d\n",*TWI_TX_data);		
		TWI_Semaphore = 1;					//Take control of TWI hardware
		//printf("TWCR: %d\n",TWCR);
		//printf("TWSR: %d\n",TWSR);
		//printf("TWBR: %d\n",TWBR);
		TWCR = TWI_CONTROL_START;			//Start TWI state machine
		//printf("TWCR: %d\n",TWCR);
		while(TWI_Semaphore == 1) 
		{
			if((TWSR&TWI_STATUS_MASK) == TWI_STATUS_START_TX)
			{
				//printf("START SENT\n");
				//printf("TWCR: %d\n",TWCR);
				//printf("TWSR: %d\n",TWSR);
				//printf("TWBR: %d\n",TWBR);
				return 0;
			}
		};
		TWCR = 0;
		return 0;
	}
	return -1;
#else	//Not using interrupts (this works)
	int TWIStatus;
	uint16_t i = 0;
	
	while(TWCR != TWI_CONTROL_ON)		//Make sure the bus is not busy
	{
		i++;
		if(i > TWI_BUS_BUSY_TIMEOUT)
		{
			#ifdef _TWI_DEBUG
			printf_P(PSTR("TWI hardware did not become available\n"));
			#endif
			return 0xFF;
		}
	}		
	TWCR = TWI_CONTROL_START;				//Send start
	
	while(1)
	{
		i = 0;
		while( ((TWCR & TWI_CONTROL_INT_MASK)>>7) == 0)	//Wait for operation to complete
		{
			i++;
			if(i > TWI_BUS_BUSY_TIMEOUT)
			{
				#ifdef _TWI_DEBUG
				printf_P(PSTR("TWI hardware did not finish\n"));
				#endif
				return 0xFF;
			}
		}
	
		//Start next action
		switch( (TWSR&TWI_STATUS_MASK) )
		{
			case TWI_STATUS_START_TX: 		//Start sent
				TWDR = (sla << 1);
				TWCR = TWI_CONTROL_CONTINUE;
				break;
				
			case TWI_STATUS_RS_TX: 			//Repeated start sent
				TWDR = ((sla << 1) | 0x01);	//Enter read mode
				TWCR = TWI_CONTROL_CONTINUE;
				break;	
			
			case TWI_STATUS_SLAW_ACK:		//SLA+W Transmitted, ACK recieved
			case TWI_STATUS_DATA_TX_ACK:	//Data Transmitted, ACK recieved
				if (BytesToSend > 0)		//Send next data byte
				{
					TWDR = *SendData;
					BytesToSend--;
					SendData++;
					TWCR = TWI_CONTROL_CONTINUE;
				}
				else if (BytesToRecieve > 0)	//Send repeated start
				{
					TWCR = TWI_CONTROL_START;
				}
				else							//Send stop
				{
					TWCR = TWI_CONTROL_STOP;
					return 0;
				}
				break;
			
			case TWI_STATUS_SLAR_ACK:		//SLA+R sent, ACK recieved
				if (BytesToRecieve > 1)
				{
					TWCR = TWI_CONTROL_RX_ACK;
				}
				else
				{
					TWCR = TWI_CONTROL_RX_NOACK;
				}
				break;
			
			case TWI_STATUS_DATA_RX_NOACK:	//Data recieved, NOACK returned. Last byte of data recieved, send stop	
				*RecieveData = TWDR;
				TWCR = TWI_CONTROL_STOP;
				return 0;
				
			case TWI_STATUS_DATA_RX_ACK:	//Data recieved, ACK returned. More data available, continue
				*RecieveData = TWDR;
				BytesToRecieve--;
				RecieveData++;
				
				if (BytesToRecieve > 1)
				{
					TWCR = TWI_CONTROL_RX_ACK;
				}
				else
				{
					TWCR = TWI_CONTROL_RX_NOACK;
				}
				break;
			
			default: // execute default action
				TWIStatus = TWSR;
				TWCR = TWI_CONTROL_STOP;
				#ifdef _TWI_DEBUG
				printf_P(PSTR("Unhandled Status Code: (TWSR: 0x%02X)\nState Machine Stopped\n"), TWIStatus&TWI_STATUS_MASK);
				#endif
				return TWIStatus;
		}	
	}
	return 0;
	
	
#endif
	
}

void TWIScan( void )
{
	uint8_t endLoop;
	uint16_t i = 0;

	for(uint8_t j = 0; j < 0x8F; j++)
	{
		endLoop = 0;
		i = 0;
		
		//Make sure the bus is not busy
		while(TWCR != TWI_CONTROL_ON)		
		{
			i++;
			if(i > TWI_BUS_BUSY_TIMEOUT)
			{
				#ifdef _TWI_DEBUG
				printf_P(PSTR("TWI hardware did not become available\n"));
				#endif
				return;
			}
		}		
		TWCR = TWI_CONTROL_START;				//Send start
		
		while(endLoop == 0)
		{
			i = 0;
			//Wait for operation to complete
			while( ((TWCR & TWI_CONTROL_INT_MASK) >> 7) == 0)	
			{
				i++;
				if(i > TWI_BUS_BUSY_TIMEOUT)
				{
					#ifdef _TWI_DEBUG
					printf_P(PSTR("TWI hardware did not finish\n"));
					#endif
					return;
				}
			}
		
			//Start next action
			switch((TWSR & TWI_STATUS_MASK))
			{
				case TWI_STATUS_START_TX: 		//Start sent, send test address in write mode
					TWDR = (j << 1);
					TWCR = TWI_CONTROL_CONTINUE;
					break;
				
				case TWI_STATUS_SLAW_ACK:		//SLA+W Transmitted, ACK recieved (device is present)
					printf("Device responded at address 0x%02X\n", j);
					TWCR = TWI_CONTROL_STOP;
					endLoop = 1;
					break;
					
				case TWI_STATUS_SLAW_NOACK:		//SLA+W Transmitted, no ACK recieved (device is not present)
					TWCR = TWI_CONTROL_STOP;
					endLoop = 1;
					break;
				
				default: 						//This should not get triggered
					i = TWSR;
					TWCR = TWI_CONTROL_STOP;
					printf_P(PSTR("Unhandled Status Code: (TWSR: 0x%02X)\n"), (i & TWI_STATUS_MASK));
					endLoop = 1;
					break;
			}
		}
	}
	printf_P(PSTR("Scan complete\n"));
	return;
}






//Interrupt based TWI state machine (does not work quite right)
#ifdef TWI_USE_ISR
ISR(TWI_vect)
{
	int TWIStatus = 0;	
	switch( (TWSR&TWI_STATUS_MASK) )
	{
		case TWI_STATUS_START_TX: 		//Start sent
			TWDR = TWI_SLA;
			TWCR = TWI_CONTROL_CONTINUE;
			break;
			
		case TWI_STATUS_RS_TX: 			//Repeated start sent
			//printf("Repeated Start Sent\n");
			TWDR = (TWI_SLA | 0x01);	//Enter read mode
			TWCR = TWI_CONTROL_CONTINUE;
			break;	
		
		case TWI_STATUS_SLAW_ACK:		//SLA+W Transmitted, ACK recieved
		case TWI_STATUS_DATA_TX_ACK:	//Data Transmitted, ACK recieved
			//printf("SLA+W or Data ACKed\n");
			if (TWI_TX_bytes > 0)		//Send next data byte
			{
				//printf("Data = %d\n",*TWI_TX_data);
				TWDR = *TWI_TX_data;
				TWI_TX_bytes--;
				TWI_TX_data++;
				TWCR = TWI_CONTROL_CONTINUE;
			}
			else if (TWI_RX_bytes > 0)	//Send repeated start
			{
				TWCR = TWI_CONTROL_START;
			}
			else						//Send stop
			{
				TWCR = TWI_CONTROL_STOP;
				TWI_Semaphore = 0;
			}
			break;
		
		case TWI_STATUS_SLAR_ACK:		//SLA+R sent, ACK recieved
			//printf("SLA+R ACKed\n");
			if (TWI_RX_bytes > 1)
			{
				TWCR = TWI_CONTROL_RX_ACK;
			}
			else
			{
				TWCR = TWI_CONTROL_RX_NOACK;
			}
			break;
		
		case TWI_STATUS_DATA_RX_NOACK:	//Data recieved, NOACK returned. Last byte of data recieved, send stop	
			//printf("Data recieved, no ACK\n");
			*TWI_RX_data = TWDR;
			TWCR = TWI_CONTROL_STOP;
			//printf("TWI Recieved: %d\n", *TWI_RX_data);
			TWI_Semaphore = 0;
			break;
			
		case TWI_STATUS_DATA_RX_ACK:	//Data recieved, ACK returned. More data available, continue
			//printf("Data recieved, ACK sent\n");
			*TWI_RX_data = TWDR;
			TWI_RX_bytes--;
			TWI_RX_data++;
			
			if (TWI_RX_bytes > 1)
			{
				TWCR = TWI_CONTROL_RX_ACK;
			}
			else
			{
				TWCR = TWI_CONTROL_RX_NOACK;
			}
			break;
		
		default: // execute default action
			TWIStatus = TWSR;
			TWCR = TWI_CONTROL_STOP;
			printf_P(PSTR("TWI State Machine Stopped: (TWSR: 0x%X)\n"), TWIStatus&TWI_STATUS_MASK);
			TWI_Semaphore = 0;
			break;
	}	
}
#endif





/** @} */