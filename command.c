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
*	\brief		Command Interpreter
*	\author		Pat Satyshur
*	\version	1.2
*	\date		1/17/2013
*	\copyright	Copyright 2013, Pat Satyshur
*	\ingroup 	interface
*
*				Takes character input and runs commands based on the input. This code was inspired by J.C. Wren's code for the LPC2148 here. (http://www.jcwren.com/arm/)
*
*	@{
*/

//includes
//#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "command.h"			//Include this unless it is included in a different header file
#include "commands.h"			//An application specific command list is required
//#include "version.h"

//TODO: Add in the arrow stuff here??
#define COMMAND_STATUS_TOP_LEVEL_INPUT		0x00	//Command is waiting for user key presses and adding them to the command string.
#define COMMAND_STATUS_TOP_LEVEL_WAITING	0x01	//Command has seen an enter keypress. The command is in the process of being parsed and executed. Input is disabled in the mode.
#define COMMAND_STATUS_SUB_LEVEL_INPUT		0x02	//A sublevel command input has been requested by a command function. User key presses are being received and added to the subcommand string.
#define COMMAND_STATUS_SUB_LEVEL_WAITING	0x03	//An enter has been seen in the subcommand input. The command and arguments are available to the command function. Input is disabled in this mode.
#define COMMAND_STATUS_ANY_KEY_WAITING		0x04

//Internal Global Variables
//TODO: combine these variables if possible.
char	command[MAX_COMMAND_DESCRIPTION_LENGTH+1];
char	old_command[MAX_COMMAND_DESCRIPTION_LENGTH+1];
char	CurrentCommandArray[MAX_ARGS+1][MAX_COMMAND_DESCRIPTION_LENGTH];
uint8_t	numArgs;
uint8_t	c_pos;
volatile uint8_t CommandWaiting = 0;
volatile uint8_t CommandLevel = 0;
volatile uint8_t CommandStatus = COMMAND_STATUS_TOP_LEVEL_INPUT;

#ifdef COMMAND_USE_ARROWS
	uint8_t CommandArrow;
#endif

//Internal function declerations
static void	ClearArgs(void);
static void ClearCommand(void);
static void ParseCommand(void);
static void SaveOldCommand(void);

//---------------------------------------------------------------------------------------------
//Common commands are defined here
//---------------------------------------------------------------------------------------------
uint8_t NumCommonCommands = 2;	//Total number of common commands

//Help Function
static int HELP_C (void);
const char _F1_NAME_COMMON[] PROGMEM 			= "help";
const char _F1_DESCRIPTION_COMMON[] PROGMEM 	= "This help list";
const char _F1_HELPTEXT_COMMON[] PROGMEM 		= "'help' has no parameters";

//CPU Status Function
static int STAT_C (void);
const char _F2_NAME_COMMON[] PROGMEM 			= "stat";
const char _F2_DESCRIPTION_COMMON[] PROGMEM 	= "Show Status of CPU";
const char _F2_HELPTEXT_COMMON[] PROGMEM 		= "'stat' has no parameters" ;

static const CommandListItem CommonCommandList[] PROGMEM =
{
	{ _F1_NAME_COMMON, 0,  1, HELP_C,	_F1_DESCRIPTION_COMMON, _F1_HELPTEXT_COMMON },
	{ _F2_NAME_COMMON, 0,  0, STAT_C,	_F2_DESCRIPTION_COMMON, _F2_HELPTEXT_COMMON }
};
//---------------------------------------------------------------------------------------------
//End of common command definitions
//---------------------------------------------------------------------------------------------

//Parse the command string and split it into the CurrentCommandArray
static void ParseCommand(void)
{
	uint8_t comPos;
	uint8_t j = 0;
	uint8_t i = 0;

	//Remove trailing spaces
	while (command[c_pos-1] == 32)
	{
		c_pos--;
	}

	//Split the command string into parts seperated by the space character (32)
	CurrentCommandArray[0][0] = command[0];
	comPos = 0;
	j = 1;
	
	for(i = 1; i < c_pos; i++)
	{
		if(command[i] == 32)
		{
			if(command[i-1] != 32)	//End of argument string
			{
				CurrentCommandArray[comPos][j] = '\0';
				comPos++;
				j = 0;
			}
		}
		else
		{
			CurrentCommandArray[comPos][j] = command[i];
			j++;
		}
	}
	numArgs = comPos;
	return;
}

//TODO: set up the forward and back arrows to work using backspace
void CommandGetInputChar(uint8_t c)
{
	uint8_t outByte[2];
	outByte[0] = c;
	outByte[1] = '\0';
	
	//If we are waiting for any key press, return after a single input.
	if(CommandStatus == COMMAND_STATUS_ANY_KEY_WAITING)
	{
		command[0] = (char) c;
		CommandStatus = COMMAND_STATUS_TOP_LEVEL_WAITING;
		return;
	}
	
	//Only recieve characters if the command function is waiting for a command.
	if((CommandStatus == COMMAND_STATUS_TOP_LEVEL_INPUT) || (CommandStatus == COMMAND_STATUS_SUB_LEVEL_INPUT))
	{
		switch(c)
		{
			case 8:		//backspace
			case 127:	//delete
				if(c_pos > 0)
				{
					command[c_pos-1] = '\0';
					c_pos--;
					printf ("\b \b");	//Note: '\b' by itself does not erase the character from the command line.
				}
				break;
			
			case 13:	//enter
				printf("\n");
				if(c_pos > 0)
				{
					ParseCommand();
					if(CommandStatus == COMMAND_STATUS_TOP_LEVEL_INPUT)
					{
						CommandStatus = COMMAND_STATUS_TOP_LEVEL_WAITING;
						SaveOldCommand();
					#ifdef COMMAND_EX_COMMAND_IN_INPUT
						RunCommand();
					#endif
					}
					else if(CommandStatus == COMMAND_STATUS_SUB_LEVEL_INPUT)
					{
						CommandStatus = COMMAND_STATUS_SUB_LEVEL_WAITING;
					}
				}
				else
				{
					printf_P(PSTR(COMMAND_PROMPT));
				}
				break;
				
	#ifdef COMMAND_USE_ARROWS
			//Detect arrow key presses and escape.
			//An arrow key is seen as three seperate key presses (from my console programs):
			//	up		-	'esc','[','A' or ([27],[91],[65]) 
			//	down	-	'esc','[','B' or ([27],[91],[66]) 
			//	right	-	'esc','[','C' or ([27],[91],[67]) 
			//	left	-	'esc','[','D' or ([27],[91],[68]) 
			//NOTE: This code will not allow a '[' to be heard after an escape press
			case 0x1B :		//escape (this also denotes arrow keys)
				CommandArrow = 1;
				break;
	#endif
				
			default:
	#ifdef COMMAND_USE_ARROWS
				if (CommandArrow == 1)		
				{
					if (c == '[')					//an arrow key was pressed, do not record this to the buffer.
					{
						CommandArrow = 2;
						break;
					}
					CommandArrow = 0;				//an arrow key was not pressed, carry on...
				}
				if (CommandArrow == 2)				//Identify which arrow key is pressed, ignore all arrows for now...
				{
					if (c == 'A')					//Pressing up clears the current command and writes the last command to the screen.
					{
						for(int i = c_pos; i>0; i--)
						{
							printf ("\b \b");
						}
						strcpy(command, old_command);
						c_pos = strnlen(command, MAX_COMMAND_DESCRIPTION_LENGTH);
						printf("%s",command);
					}
					else if (c == 'B')		//down
					{
						//printf ("DOWN");
					}
					else if (c == 'C')		//right
					{
						//printf ("RIGHT");
					}
					else if (c == 'D')		//left
					{
						//printf ("LEFT");
					}
					CommandArrow = 0;
					break;
				}
	#endif
				if (c >= 32 && c_pos < MAX_COMMAND_DESCRIPTION_LENGTH)
				{
					command[c_pos] = (char) c;
					c_pos++;
					printf("%s", outByte);
				}
		}
	}
}

void RunCommand( void )
{
	if(CommandStatus == COMMAND_STATUS_TOP_LEVEL_WAITING)
	{
		//Look for the command in the project specific commands, check for the correct number of arguments, and execute
		for(int i=0; i < NumCommands; i++)			
		{
			if(strcmp_P(CurrentCommandArray[0], (PGM_P)pgm_read_word(&(AppCommandList[i].name)) ) == 0)
			{
				if(numArgs < pgm_read_word(&AppCommandList[i].minArgs))
				{
					printf_P(PSTR("Not enough arguments\n"));
				}
				else if(numArgs > pgm_read_word(&AppCommandList[i].maxArgs))
				{
					printf_P(PSTR("Too many arguments\n"));
				}
				else
				{
					((void(*)(void))pgm_read_word(&AppCommandList[i].handler))();
				}
				
				ClearCommand();
				ClearArgs();
				CommandStatus = COMMAND_STATUS_TOP_LEVEL_INPUT;
				return;
			}
		}
			
		//Look for the command in the common commands, check for the correct number of arguments, and execute
		for(int i=0; i < NumCommonCommands; i++)			
		{
			if(strcmp_P(CurrentCommandArray[0], (PGM_P)pgm_read_word(&(CommonCommandList[i].name)) ) == 0)
			{
				if(numArgs < pgm_read_word(&CommonCommandList[i].minArgs))
				{
					printf_P(PSTR("Not enough arguments\n"));
				}
				else if(numArgs > pgm_read_word(&CommonCommandList[i].maxArgs))
				{
					printf_P(PSTR("Too many arguments\n"));
				}
				else
				{
					((void(*)(void))pgm_read_word(&CommonCommandList[i].handler))();
				}
				
				ClearCommand();
				ClearArgs();
				CommandStatus = COMMAND_STATUS_TOP_LEVEL_INPUT;
				return;
			}
		}
			
		printf_P(PSTR("Invalid command. Type 'help' for command list.\n"));
		//printf("com: %s\n", command);
		ClearArgs();
		ClearCommand();
		CommandStatus = COMMAND_STATUS_TOP_LEVEL_INPUT;
	}
	
	return;
}

void GetNewCommand( void )
{
	//Clear out old command data
	ClearCommand();
	ClearArgs();
	
	//Reenable input
	CommandStatus = COMMAND_STATUS_SUB_LEVEL_INPUT;
	
	//Wait for user input
	while(CommandStatus == COMMAND_STATUS_SUB_LEVEL_INPUT) {}
	return;
}

char WaitForAnyKey( void )
{
	//Reenable input
	CommandStatus = COMMAND_STATUS_ANY_KEY_WAITING;
	
	//Wait for user input
	while(CommandStatus == COMMAND_STATUS_ANY_KEY_WAITING) {}

	return command[0];
}


//Clears arguments, call this last
static void ClearArgs(void)
{
	uint8_t i;
	uint8_t j;
	
	for(i = 0; i < MAX_COMMAND_DESCRIPTION_LENGTH; i++)
	{
		for(j = 0; j < MAX_ARGS; j++)
		{
			CurrentCommandArray[j][i] = 0;
		}
	}
	
	numArgs = 0;
	printf_P(PSTR(COMMAND_PROMPT));
	return;
}


static void ClearCommand( void )
{
	for(int i=0; i<MAX_COMMAND_DESCRIPTION_LENGTH;i++)
	{
		command[i] = '\0';
	}
	c_pos = 0;
}

static void SaveOldCommand(void)
{
	strcpy(old_command, command);
	return;
}

uint8_t NumberOfArguments( void )
{
	return numArgs;
}


void argAsChar(uint8_t argNum, char *ArgString)
{
	if(argNum > numArgs)
	{
		strcpy(ArgString, "\0");
	}
	else
	{
		strcpy(ArgString, CurrentCommandArray[argNum]);
	}
	return;
}

/**Returns argument argNum as integer.
*	\param[in]	argNum The argument number. Argument numbers start at 1. Setting argNum to 0 will give the command name as an integer, which will probably be useless. Invalid argument number or invalid characters will make the function return 0;
*	\returns The argNum argument as a up to 32-bit signed integer.
*	TODO: Fix this to handle negative numbers... (maybe done, test this later)
*	TODO: Add support for exponentials? (1E3)
*/
int32_t argAsInt(uint8_t argNum)
{
	int32_t valToReturn = 0;
	uint8_t isNegative = 0;
	uint8_t i;
	
	if(argNum > numArgs)
	{
		return 0;
	}
	
	//Handle hex input preceded by '0x' or '0X'
	if(CurrentCommandArray[argNum][1] == 'x' || CurrentCommandArray[argNum][1] == 'X')
	{
		for(int i=2; i<MAX_COMMAND_DESCRIPTION_LENGTH; i++)
		{
			if (CurrentCommandArray[argNum][i] == '\0')
			{
				break;
			}
			else if((CurrentCommandArray[argNum][i] > 96) && (CurrentCommandArray[argNum][i] < 103))	//lower case a through f
			{
				valToReturn = valToReturn*16+(10+CurrentCommandArray[argNum-1][i]-97);
			}
			else if((CurrentCommandArray[argNum][i] > 64) && (CurrentCommandArray[argNum][i] < 71))	//upper case A through F
			{
				valToReturn = valToReturn*16+(10+CurrentCommandArray[argNum-1][i]-65);
			}
			else if((CurrentCommandArray[argNum][i] > 47) && (CurrentCommandArray[argNum][i] < 58))	//0 through 9
			{
				valToReturn = valToReturn * 16 + (CurrentCommandArray[argNum][i] - 48);
			}
			else
			{
				return 0;
			}
		}
	}
	
	//Handle binary input preceded by '0b' or '0B'
	else if(CurrentCommandArray[argNum][1] == 'b' || CurrentCommandArray[argNum][1] == 'B')
	{
		for(int i=2; i<MAX_COMMAND_DESCRIPTION_LENGTH; i++)
		{
			if (CurrentCommandArray[argNum][i] == '\0')
			{
				break;
			}
			else if((CurrentCommandArray[argNum][i] == 48) || (CurrentCommandArray[argNum][i] == 49))	//0 or 1
			{
				valToReturn = valToReturn*2+(CurrentCommandArray[argNum][i]-48);
			}
			else
			{
				return 0;
			}
		}
	}
	
	//Handle decimal
	else
	{
		if(CurrentCommandArray[argNum][0] == '-')
		{
			isNegative = 1;
		}
		for(i = isNegative; i < MAX_COMMAND_DESCRIPTION_LENGTH; i++)
		{
			if (CurrentCommandArray[argNum][i] == '\0')
			{
				break;
			}
			else if((CurrentCommandArray[argNum][i] > 47) && (CurrentCommandArray[argNum][i] < 58))
			{
				valToReturn = valToReturn * 10 + (CurrentCommandArray[argNum][i] - 48);
			}
			else
			{
				return 0;
			}
		}
		if(isNegative == 1)
		{
			valToReturn = valToReturn*-1;
		}
	}
	return valToReturn;
}

//Command functions
static int HELP_C (void)
{
	uint8_t i;
	uint8_t j;
	
	if(numArgs > 0)
	{
		//Search for command in list, display help
		for(i = 0; i < NumCommands; i++)			
		{
			//printf("Compare %s to %s\n", CurrentCommandArray[0], AppCommandList[i].name);
			//if(strcmp_P(CurrentCommandArray[0], AppCommandList[i].name) == 0)
			if(strcmp_P(CurrentCommandArray[1], (PGM_P)pgm_read_word(&(AppCommandList[i].name))) == 0)
			{
				//printf_P(PSTR("%S\n"), AppCommandList[i].HelpText);
				printf_P( (PGM_P)pgm_read_word(&(AppCommandList[i].HelpText)) );
				printf_P(PSTR("\n"));
				return 0;
			}
			//if(strcmp_P(CurrentCommandArray[0], CommonCommandList[i].name) == 0)
			else if(strcmp_P(CurrentCommandArray[1], (PGM_P)pgm_read_word(&(CommonCommandList[i].name))) == 0)
			{
				//printf_P(PSTR("%S\n"), CommonCommandList[i].HelpText);
				printf_P( (PGM_P)pgm_read_word(&(CommonCommandList[i].HelpText)) );
				printf_P(PSTR("\n"));
				return 0;
			}
		}
		printf_P(PSTR("Invalid command\n"));
	}
	else
	{
		printf_P(PSTR("Command List:\n"));
		for(int i=0; i < NumCommonCommands; i++)
		{
			printf_P( (PGM_P)pgm_read_word(&(CommonCommandList[i].name)) );
			//Manually format the help outputs, there has to be a better way to do this...
			
			j=strlen_P((PGM_P)pgm_read_word(&(CommonCommandList[i].name)));
			while(j <= COMMAND_MAX_DISPLAY_LENGTH)
			{
				printf_P(PSTR(" "));
				j++;
			}
			printf_P( (PGM_P)pgm_read_word(&(CommonCommandList[i].DescText)) );
			printf_P(PSTR("\n"));
			
			//printf_P(PSTR("%S:    %S\n"), CommonCommandList[i].name, CommonCommandList[i].DescText);
		}
		for(int i=0; i < NumCommands; i++)
		{
			printf_P( (PGM_P)pgm_read_word(&(AppCommandList[i].name)) );
			j=strlen_P((PGM_P)pgm_read_word(&(AppCommandList[i].name)));
			while(j <= COMMAND_MAX_DISPLAY_LENGTH)
			{
				printf_P(PSTR(" "));
				j++;
			}
			//printf_P(PSTR(":\t\t"));
			printf_P( (PGM_P)pgm_read_word(&(AppCommandList[i].DescText)) );
			printf_P(PSTR("\n"));
			
			//printf_P(PSTR("%S:    %S\n"), AppCommandList[i].name, AppCommandList[i].DescText);
		}
	}
	return 0;
}



static int STAT_C (void)
{
	printf_P(PSTR("--------------------------------------------------\n"));
	printf_P(PSTR("Device Status:\n"));
	printf_P(PSTR("--------------------------------------------------\n"));
	
	#if COMMAND_STAT_SHOW_MEM_USAGE == 1
	printf_P(PSTR("Free memory: %d bytes\n"), StackCount());
	#endif
	/*
	printf_P(PSTR("Clocks:\n"));
	printf("CLKSEL0: %d\n", CLKSEL0);
	printf("CLKSEL1: %d\n", CLKSEL1);
	printf("CLKSTA: %d\n", CLKSTA);
	printf("CLKPR: %d\n", CLKPR);

	printf("Power Control %d\n",PRR0);
	
	printf_P(PSTR("SPI:\n"));
	printf("SPCR: %d\n",SPCR);
	printf("SPSR: %d\n",SPSR);*
	*/
	#if COMMAND_STAT_SHOW_COMPILE_STRING == 1
	printf_P(fwCompileDate);
	#endif
	return 0;
}
/** @} */