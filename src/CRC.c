/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* CRC Calculation routines                                                  */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <CRC.h>

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static uint16_t l_crc;

///////////////////////////////////////////////////////////////////////////////
// Local functions
static uint16_t CRCAddBit(bool in_bit);

///////////////////////////////////////////////////////////////////////////////
// Initializes CRC value
void CRCReset(uint16_t in_checksum_start)
{
	l_crc = in_checksum_start;
}

///////////////////////////////////////////////////////////////////////////////
// Gets current CRC value
uint16_t CRCGet(void)
{
	return l_crc;
}

///////////////////////////////////////////////////////////////////////////////
// Calculates CRC
uint16_t CRCAddByte(uint8_t in_data)
{
	int i;

	for( i = 0; i < 8; i++)
	{
		if( (in_data & 0x01) == 0 )
		{
			CRCAddBit(false);
		}
		else
		{
			CRCAddBit(true);
		}

		in_data >>= 1;
	}

	return l_crc;
}

///////////////////////////////////////////////////////////////////////////////
// Adds buffer content to the CRC
uint16_t CRCAddBlock(uint8_t* in_buffer, int in_buffer_length)
{
	while(in_buffer_length > 0 )
	{
		CRCAddByte(*in_buffer);
		in_buffer++;
		in_buffer_length--;
	}

	return l_crc;
}

///////////////////////////////////////////////////////////////////////////////
// Adds one bit to CRC
static uint16_t CRCAddBit(bool in_bit)
{
	uint8_t A;
	uint8_t CY;

	if(in_bit!=false)					//     LD  A,80
		A = 0x80;								//     JR  NZ,L1
	else											//    
		A = 0;									// 		 XOR A
														//
	A = A ^ (HIGH(l_crc));		// L1: XOR H    
	CY = (A & 0x80);					//     RLA
														//
	if( CY != 0 )							//     JR  NC,L2
	{													//		 LD  A,H
		l_crc ^= 0x0810;				//     XOR 08
		CY = 1;                 //     LD  H,A
		                        //     LD  A,L
														//     XOR 10
		                        //     LD  L,A
	}													//     SCF
														//
	l_crc += l_crc + CY;			//     ADC HL,HL

	return l_crc;
}
