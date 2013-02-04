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
*	\ingroup 	hardware
*
*	@{
*/

//TODO: make this into a common library

//All includes go here
#include "main.h"

//Useful definitions
//#define SOFT_I2C_STATUS_START			0x01
//#define SOFT_I2C_STATUS_STOP			0x00

//#define SOFT_I2C_STATUS_ARB_LOST		0x40
//#define SOFT_I2C_STATUS_BUS_ERROR		0x80
/*
#define SOFT_I2C_STATUS_RS				0x01
#define SOFT_I2C_STATUS_SLAW			0x02
#define SOFT_I2C_STATUS_SEND			0x03
#define SOFT_I2C_STATUS_SLAR			0x04
#define SOFT_I2C_STATUS_RECEIVE			0x05
#define SOFT_I2C_STATUS_STOP			0x06
*/

//I2C return codes
#define SOFT_I2C_RETURN_OK				0x00
#define SOFT_I2C_RETURN_NOACK			0x01

#define SOFT_I2C_RETURN_ARB_LOST		0x40
#define SOFT_I2C_RETURN_BUS_ERROR		0x80





//Checks is the status is ok, and returns if not
#define CheckStat(stat)  do {	if(stat != SOFT_I2C_RETURN_OK) {	\
								I2CSoft_SendStop();					\
								printf("Fail: %d\n", stat);			\
								return stat; }} while ( 0 )

/*
#define SOFT_I2C_RETURN_SLAW_ACK		0x02
#define SOFT_I2C_RETURN_SLAW_NOACK		0x03
#define SOFT_I2C_RETURN_DATA_TX_ACK		0x04
#define SOFT_I2C_RETURN_DATA_TX_NOACK	0x05
#define SOFT_I2C_RETURN_SLAR_ACK		0x06
#define SOFT_I2C_RETURN_SLAR_NOACK		0x07
#define SOFT_I2C_RETURN_DATA_RX_ACK		0x08
#define SOFT_I2C_RETURN_DATA_RX_NOACK	0x09*/

#define SOFT_I2C_RETURN_PARAMETER_ERROR	0xFE

#define SOFT_I2C_TU_COUNT				25

//Internal global variables for interrupt routine
/*volatile uint8_t I2C_Soft_BusInUse;						//Set to 1 if the bus is in use, check this before starting communication
volatile uint8_t I2C_Soft_SLA;							//Address of slave device
volatile uint8_t I2C_Soft_TX_Size;						//Number of bytes to transmit
volatile uint8_t I2C_Soft_TX_Bufffer[I2C_BUFFER_SIZE];	//Transmit buffer
volatile uint8_t I2C_Soft_RX_Size;						//Number of bytes to recieve
volatile uint8_t I2C_Soft_RX_Bufffer[I2C_BUFFER_SIZE];	//Recieve buffer

volatile uint8_t I2C_Soft_IntCounter;
volatile uint8_t I2C_Soft_Status;*/


//volatile uint8_t I2C_Soft_Status;



//Internal functions
uint8_t I2CSoft_Int_Handler(void);


void I2CSoft_Delay_TU(void);




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
	//Initalize global variables
/*	I2C_Soft_BusInUse = 0;
	I2C_Soft_IntCounter = 0;
	I2C_Soft_TX_Size = 0;
	I2C_Soft_RX_Size = 0;*/
	
	//Setup the pins for SCL and SDA
	I2CSoft_SDA_Release();
	I2CSoft_SCL_Release();	
	
	//Setup 16-bit timer 1 for 1us timing
	//TCCR1A = 0x00;		//CTC Mode
	//TCCR1B = 0x08;		//CTC Mode, clock stopped
	//OCR1AH = 0x00;
	//OCR1AL = 0x14;		//At 8MHz, 20(0x14) clock cycles equal 2.5us 0xC8
	//TCNT1H = 0x00;
	//TCNT1L = 0x00;		//Initalize the timer register to 0
	//TIMSK1 = 0x02;		//Enable interrupt on output compare A match

	return;
}



uint8_t I2CSoft_RW(uint8_t sla, uint8_t *SendData, uint8_t *RecieveData, uint8_t BytesToSend, uint8_t BytesToRecieve)
{
	uint8_t stat;
	uint8_t i;
	
	//verify stuff here?

	//Send start
	stat = I2CSoft_SendStart(0);
	CheckStat(stat);
	
	//Send data to device
	if(BytesToSend > 0)
	{
		//Send device address
		stat = I2CSoft_WriteByte(sla<<1);
		CheckStat(stat);
		
		for(i=0; i<BytesToSend; i++)
		{
			stat = I2CSoft_WriteByte(SendData[i]);
			CheckStat(stat);
		}
		if(BytesToRecieve > 0)
		{
			stat = I2CSoft_SendStart(1);
			CheckStat(stat);
		}
	}

	if(BytesToRecieve > 0)
	{
		//Send device address
		stat = I2CSoft_WriteByte((sla << 1) | 0x01);
		CheckStat(stat);
	
		for(i=0; i<BytesToRecieve-1; i++)
		{
			//printf_P(PSTR("acking read\n"));
			stat = I2CSoft_ReadByte(&RecieveData[i], 1);
			CheckStat(stat);
		}
		
		//printf_P(PSTR("noacking read\n"));
		stat = I2CSoft_ReadByte(&RecieveData[BytesToRecieve-1], 0);
		CheckStat(stat);
	}
	
	//Send Stop
	stat = I2CSoft_SendStop();
	//stat = I2CSoft_SendStop();
	return stat;
	
	
	/*stat = I2CSoft_WriteByte(byteToSend);
	stat = I2CSoft_WriteByte(0x04);
	I2CSoft_SendStart(1);
	I2CSoft_WriteByte(byteToSend | 0x01);
	
	I2CSoft_ReadByte(&tempByte, 0);
	
	I2CSoft_SendStop();
	
	printf("rec: %d\n", tempByte);

	if(stat == SOFT_I2C_RETURN_OK)
	{
		printf_P(PSTR("ack\n"));
	}
	else if(stat == SOFT_I2C_RETURN_NOACK)
	{
		printf_P(PSTR("no ack\n"));
	}
	else
	{
		printf_P(PSTR("error\n"));
	}*/
	
	
	
	
	
	
	
	
	/*-------------------------------------------------------
	 * the old way
	 *-------------------------------------------------------
	//Send start
	I2CSoft_SDA_Set();
	I2CSoft_Delay_TU();
	
	
	//Send byte
	for(uint8_t j = 0; j<BytesToSend+2; j++)
	{
		for(uint8_t i = 0; i<8; i++)
		{
			//setup phase, clock is low
			I2CSoft_SCL_Set();
			I2CSoft_Delay_TU();
			if(byteToSend & 0x80)	//bit is a one
			{
				I2CSoft_SDA_Release();
			}
			else	//bit is a zero
			{
				I2CSoft_SDA_Set();
			}
			byteToSend <<= 1;
			I2CSoft_Delay_TU();
			
			//clock goes high, sample phase
			I2CSoft_SCL_Release();
			I2CSoft_Delay_TU();
			I2CSoft_Delay_TU();
		}
		
		//look for ack
		I2CSoft_SCL_Set();
		I2CSoft_Delay_TU();
		I2CSoft_SDA_Release();
		I2CSoft_Delay_TU();
		I2CSoft_SCL_Release();
		I2CSoft_Delay_TU();
		ack = (I2C_SDA_PIN & (1<<I2C_SDA_PIN_NUM));
		I2CSoft_Delay_TU();
	}
	
	//Send stop
	I2CSoft_SCL_Set();
	I2CSoft_Delay_TU();

	I2CSoft_SDA_Set();
	I2CSoft_Delay_TU();
	
	//I2CSoft_Delay_TU();
	I2CSoft_SCL_Release();
	I2CSoft_Delay_TU();
	I2CSoft_SDA_Release();


	return ack;	*/
	
	
	
	
	
	
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
				return SOFT_I2C_RETURN_BUS_ERROR;
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
		return SOFT_I2C_RETURN_ARB_LOST;
	}
	#else
	I2CSoft_SDA_Release()
	#endif

	I2CSoft_SDA_Set();
	I2CSoft_Delay_TU();
	I2CSoft_SCL_Set();
	return SOFT_I2C_RETURN_OK;

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
			return SOFT_I2C_RETURN_BUS_ERROR;
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
		return SOFT_I2C_RETURN_ARB_LOST;
	}
	#endif

	return SOFT_I2C_RETURN_OK;
}

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
				return SOFT_I2C_RETURN_BUS_ERROR;
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
				return SOFT_I2C_RETURN_ARB_LOST;
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
			return SOFT_I2C_RETURN_BUS_ERROR;
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
		return SOFT_I2C_RETURN_OK;
	}
	return SOFT_I2C_RETURN_NOACK;
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
				return SOFT_I2C_RETURN_BUS_ERROR;
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
			return SOFT_I2C_RETURN_BUS_ERROR;
		}
	}
#else
		I2CSoft_SCL_Release();
#endif

	I2CSoft_Delay_TU();
	I2CSoft_Delay_TU();
	I2CSoft_SCL_Set();

	return SOFT_I2C_RETURN_OK;
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
