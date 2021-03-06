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
*	\brief		I2C master software driver.
*	\author		Pat Satyshur
*	\version	1.0
*	\date		2/3/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	common
*
*	@{
*/

#include "i2c_soft.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

/** Determines timing for the internal delay generator. A smaller value here will increase the speed of the I2C clock. However, to small of a value will probably make the clock uneven */
#define SOFT_I2C_TU_COUNT				25

//Internal functions
uint8_t I2CSoft_Int_Handler(void);

/**This function generates short delays and is used for timing of the I2C bus. 
   The I2C bus speed period will be roughly four times the duration of this function.
   This function is declared weak, and can therefore be overridden by a user function */
void I2CSoft_Delay_TU(void) __attribute__((weak));

void I2CSoft_SDA_Set(void);				//Sets (pulls low) the SDA line
void I2CSoft_SCL_Set(void);				//Sets (pulls low) the SCL line
uint8_t I2CSoft_SDA_Release(void);		//releases (allows to float high) the SDA line
uint8_t I2CSoft_SCL_Release(void);		//releases (allows to float high) the SCL line

static inline uint8_t I2CSoft_SendStart(uint8_t RS);
static inline uint8_t I2CSoft_SendStop(void);
static inline uint8_t I2CSoft_WriteByte(uint8_t ByteToWrite);
static inline uint8_t I2CSoft_ReadByte(uint8_t *ByteToRead, uint8_t SendAck);

//Initalizes the hardware and variables
// Note: Gloabal interrupts are not enabled here, they must be enabled for this to work
inline void I2CSoft_Init(void)
{	
	//Setup the pins for SCL and SDA
	I2CSoft_SDA_Release();
	I2CSoft_SCL_Release();	

	return;
}

uint8_t I2CSoft_RW(uint8_t sla, uint8_t *SendData, uint8_t *RecieveData, uint8_t BytesToSend, uint8_t BytesToRecieve)
{
	uint8_t stat;
	uint8_t i;

	//Send start
	stat = I2CSoft_SendStart(0);
	if(stat != SOFT_I2C_STAT_START)
	{
		I2CSoft_SendStop();
		return stat;
	}
	
	//Send data to device
	if(BytesToSend > 0)
	{
		//Send device address
		stat = I2CSoft_WriteByte(sla<<1);
		if(stat != SOFT_I2C_STAT_DATA_TX_ACK)
		{
			I2CSoft_SendStop();
			if(stat == SOFT_I2C_STAT_DATA_TX_NOACK)
			{
				return SOFT_I2C_STAT_SLAW_NOACK;
			}
			else
			{
				return stat;
			}
		}
		
		for(i=0; i<BytesToSend; i++)
		{
			stat = I2CSoft_WriteByte(SendData[i]);
			if(stat != SOFT_I2C_STAT_DATA_TX_ACK)
			{
				I2CSoft_SendStop();
				return stat;
			}
		}
		if(BytesToRecieve > 0)
		{
			stat = I2CSoft_SendStart(1);	
			if(stat != SOFT_I2C_STAT_RSTART)
			{
				I2CSoft_SendStop();
				return stat;
			}
		}
	}

	if(BytesToRecieve > 0)
	{
		//Send device address
		stat = I2CSoft_WriteByte((sla << 1) | 0x01);
		if(stat != SOFT_I2C_STAT_DATA_TX_ACK)
		{
			I2CSoft_SendStop();
			if(stat == SOFT_I2C_STAT_DATA_TX_NOACK)
			{
				return SOFT_I2C_STAT_SLAR_NOACK;
			}
			else
			{
				return stat;
			}
		}
	
		for(i=0; i<BytesToRecieve-1; i++)
		{
			//More data to receive, send ACK
			stat = I2CSoft_ReadByte(&RecieveData[i], 1);
			if(stat != SOFT_I2C_STAT_DATA_RX_ACK)
			{
				I2CSoft_SendStop();
				return stat;
			}
		}
		//Last byte of data to read, don't send ACK
		stat = I2CSoft_ReadByte(&RecieveData[BytesToRecieve-1], 0);
		if(stat != SOFT_I2C_STAT_DATA_RX_NOACK)
		{
			I2CSoft_SendStop();
			return stat;
		}
	}
	
	//Send Stop
	stat = I2CSoft_SendStop();
	return stat;
}

void I2CSoft_Scan(void)
{
	uint8_t i;
	uint8_t stat;
	
	for(i=0; i<0x8F; i++)
	{
		//Send start
		stat = I2CSoft_SendStart(0);
		if(stat != SOFT_I2C_STAT_START)
		{
			printf_P(PSTR("Error sending start\n"));
			return;
		}
		
		//Send device address
		stat = I2CSoft_WriteByte(i<<1);
		if(stat == SOFT_I2C_STAT_DATA_TX_ACK)
		{
			printf_P(PSTR("Device responded at address 0x%02X\n"), i);
		}
		
		//Send Stop
		stat = I2CSoft_SendStop();
		if(stat != SOFT_I2C_STAT_OK)
		{
			printf_P(PSTR("Error sending stop\n"));
			return;
		}
		
		//Wait a bit
		I2CSoft_Delay_TU();
		I2CSoft_Delay_TU();
	}
}

static inline uint8_t I2CSoft_SendStart(uint8_t RS)
{
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	uint16_t i = 0;
	#endif
	
	//Bus is started, send repeated start
	if(RS)
	{
		I2CSoft_SDA_Release();
		I2CSoft_Delay_TU();
	
		#if I2C_SOFT_USE_CLOCK_STRETCH == 1
		while (I2CSoft_SCL_Release() == 0)	// Clock stretching
		{
			i++;
			if(i > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
			{
				return SOFT_I2C_STAT_BUS_ERROR;
			}
		}
		#else
		I2CSoft_SCL_Release();
		#endif
		
		I2CSoft_Delay_TU();
	}

	#if I2C_SOFT_USE_ARBITRATION == 1
	//Lost arbitration
	if(I2CSoft_SDA_Release() == 0)
	{
		return SOFT_I2C_STAT_ARB_LOST;
	}
	#else
	I2CSoft_SDA_Release()
	#endif

	I2CSoft_SDA_Set();
	I2CSoft_Delay_TU();
	I2CSoft_SCL_Set();
	
	if(RS)
	{
		return SOFT_I2C_STAT_RSTART;
	}
	return SOFT_I2C_STAT_START;

}

static inline uint8_t I2CSoft_SendStop(void)
{
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	uint16_t i = 0;
	#endif

	I2CSoft_Delay_TU();
	I2CSoft_SDA_Set();
	I2CSoft_Delay_TU();
	
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	while (I2CSoft_SCL_Release() == 0)	// Clock stretching
	{
		i++;
		if(i > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
		{
			return SOFT_I2C_STAT_BUS_ERROR;
		}
	}
	#else
		I2CSoft_SCL_Release();
	#endif
	I2CSoft_Delay_TU();
	I2CSoft_SDA_Release();
	
	
	#if I2C_SOFT_USE_ARBITRATION == 1
	I2CSoft_Delay_TU();
	if(I2CSoft_SDA_Release() == 0)
	{
		return SOFT_I2C_STAT_ARB_LOST;
	}
	#endif

	return SOFT_I2C_STAT_OK;
}


//This function will return DATA TX ACK/NOACK only
static inline uint8_t I2CSoft_WriteByte(uint8_t ByteToWrite)
{
	uint8_t i = 0;
	
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	uint16_t j = 0;
	#endif

	for(i=0; i<8; i++)
	{
		I2CSoft_Delay_TU();
		
		//Setup bit
		if((ByteToWrite & 0x80) == 0)
		{
			I2CSoft_SDA_Set();
		}
		else
		{
			I2CSoft_SDA_Release();
		}
		I2CSoft_Delay_TU();
		
		#if I2C_SOFT_USE_CLOCK_STRETCH == 1
		while (I2CSoft_SCL_Release() == 0)	// Clock stretching
		{
			j++;
			if(j > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
			{
				return SOFT_I2C_STAT_BUS_ERROR;
			}
		}
		#else
		I2CSoft_SCL_Release();
		#endif
		
		I2CSoft_Delay_TU();
		
		#if I2C_SOFT_USE_ARBITRATION == 1
		//Data is valid, check for loss of arbitration
		if((ByteToWrite & 0x80) != 0)
		{
			if(I2CSoft_SDA_Release() == 0)
			{
				return SOFT_I2C_STAT_ARB_LOST;
			}
		}
		#endif
		
		I2CSoft_Delay_TU();
		I2CSoft_SCL_Set();

	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
		j = 0;
	#endif
		ByteToWrite <<= 1;
	}

	//Get ack
	i = 0;
	I2CSoft_Delay_TU();
	I2CSoft_SDA_Release();
	I2CSoft_Delay_TU();
	
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	while (I2CSoft_SCL_Release() == 0)	// Clock stretching
	{
		j++;
		if(j > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
		{
			return SOFT_I2C_STAT_BUS_ERROR;
		}
	}
	#else
	I2CSoft_SCL_Release();
	#endif
	
	I2CSoft_Delay_TU();
	i = I2CSoft_SDA_Release();
	I2CSoft_Delay_TU();
	I2CSoft_SCL_Set();

	if(i == 0)
	{
		return SOFT_I2C_STAT_DATA_TX_ACK;
	}
	return SOFT_I2C_STAT_DATA_TX_NOACK;
}

static inline uint8_t I2CSoft_ReadByte(uint8_t *ByteToRead, uint8_t SendAck)
{
	uint8_t i = 0;
	#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	uint16_t j = 0;
	#endif
	
	
	for(i=0; i<8; i++)
	{
		I2CSoft_Delay_TU();
		
		//Setup from slave
		I2CSoft_SDA_Release();	
		I2CSoft_Delay_TU();
		
		#if I2C_SOFT_USE_CLOCK_STRETCH == 1
		while (I2CSoft_SCL_Release() == 0)	// Clock stretching
		{
			j++;
			if(j > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
			{
				return SOFT_I2C_STAT_BUS_ERROR;
			}
		}
		#else
		I2CSoft_SCL_Release();
		#endif
		
		I2CSoft_Delay_TU();
		
		//Data is valid, get data
		*ByteToRead = ((*ByteToRead << 1) | (I2CSoft_SDA_Release()));
		I2CSoft_Delay_TU();
		I2CSoft_SCL_Set();
		
		#if I2C_SOFT_USE_CLOCK_STRETCH == 1
		j = 0;
		#endif
	}
	
	//Send ack
	I2CSoft_Delay_TU();
	if(SendAck > 0)
	{
		//Send ack
		I2CSoft_SDA_Set();
	}
	else
	{
		//Send nack
		I2CSoft_SDA_Release();
	}
	I2CSoft_Delay_TU();
	
#if I2C_SOFT_USE_CLOCK_STRETCH == 1
	while (I2CSoft_SCL_Release() == 0)	// Clock stretching
	{
		j++;
		if(j > I2C_SOFT_CLOCK_STRETCH_TIMEOUT)
		{
			return SOFT_I2C_STAT_BUS_ERROR;
		}
	}
#else
		I2CSoft_SCL_Release();
#endif

	I2CSoft_Delay_TU();
	I2CSoft_Delay_TU();
	I2CSoft_SCL_Set();

	if(SendAck > 0)
	{
		return SOFT_I2C_STAT_DATA_RX_ACK;
	}
	else
	{
		return SOFT_I2C_STAT_DATA_RX_NOACK;
	}
}



//Generate delays for the I2C, this function dictates the speed of the bus
void I2CSoft_Delay_TU(void)
{
	for(uint16_t i=0; i<SOFT_I2C_TU_COUNT; i++)
	{
		for (uint8_t j=0; j<20; j++)
		{
			asm volatile ("nop");
		}
	}
}

//Functions to manipulate the I2C pins
void I2CSoft_SDA_Set(void)
{
	I2C_SDA_DDR |= (1 << I2C_SDA_PIN_NUM);			//Pin is output
	I2C_SDA_PORT &= (~(1 << I2C_SDA_PIN_NUM));		//Pin is low
	return;
}
	
void I2CSoft_SCL_Set(void)
{
	I2C_SCL_DDR |= (1 << I2C_SCL_PIN_NUM);			//Pin is output
	I2C_SCL_PORT &= (~(1 << I2C_SCL_PIN_NUM));		//Pin is low
	return;
}	
	
uint8_t I2CSoft_SDA_Release(void)
{
	I2C_SDA_DDR &= (~(1 << I2C_SDA_PIN_NUM));		//Pin is input (high impedance)
#if (I2C_SOFT_USE_INTERNAL_PULLUPS == 1)
	I2C_SDA_PORT |= (1 << I2C_SDA_PIN_NUM);			//Pullup on
#endif
	return ((I2C_SDA_PIN >> I2C_SDA_PIN_NUM) & 0x01);
}
	
uint8_t I2CSoft_SCL_Release(void)
{
	I2C_SCL_DDR &= (~(1 << I2C_SCL_PIN_NUM));		//Pin is input (high impedance)
#if (I2C_SOFT_USE_INTERNAL_PULLUPS == 1)
	I2C_SCL_PORT |= (1 << I2C_SCL_PIN_NUM);			//Pullup on
#endif
	return ((I2C_SCL_PIN >> I2C_SCL_PIN_NUM) & 0x01);
}	

/** @} */
