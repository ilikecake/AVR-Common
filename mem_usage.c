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
*	\brief		Memory usage functions
*	\author		Pat Satyshur
*	\version	1.0
*	\date		12/19/2012
*	\copyright	Copyright 2012, Pat Satyshur
*	\ingroup 	hardware
*
*	Code to determine the total ammount of memory used. Code based on examples from Dean Camera.
*
*	@{
*/

#include <inttypes.h>
#include <avr/io.h>
#include "mem_usage.h"

extern uint8_t _end;
extern uint8_t __stack;

//Fills all memory addresses with 0xC5
void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));
void StackPaint(void)
{
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                    "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xC5 */
                    "    ldi r25,hi8(__stack)\n"
                    "    rjmp .cmp\n"
                    ".loop:\n"
                    "    st Z+,r24\n"
                    ".cmp:\n"
                    "    cpi r30,lo8(__stack)\n"
                    "    cpc r31,r25\n"
                    "    brlo .loop\n"
                    "    breq .loop"::);
}

//returns the number of unused bytes of memory
uint16_t StackCount(void)
{
    const uint8_t *p = &_end;
    uint16_t       c = 0;

    while(*p == 0xC5 && p <= &__stack)
    {
        p++;
        c++;
    }

    return c;
}

/** @} */