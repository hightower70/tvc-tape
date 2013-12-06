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
#ifndef __CRC_h
#define __CRC_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void CRCReset(void);
WORD CRCGet(void);
WORD CRCAddByte(BYTE in_data);
WORD CRCAddBlock(BYTE* in_buffer, int in_buffer_length);


#endif