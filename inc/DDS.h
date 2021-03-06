/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Direct Digital Synthesis (wave generation)                                */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __DDS_h
#define __DDS_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define DDS_TABLE_LENGTH 256

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void InitDDS(void);
bool GenerateDDSSignal(uint32_t in_frequency, uint32_t in_cycle_count);
bool GenerateDDSSilence(uint16_t in_length_in_ms);


#endif