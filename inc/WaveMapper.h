/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave input/output mapping (between file and wave in/out device)           */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __WaveMapper_h
#define __WaveMapper_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define BYTE_SAMPLE_ZERO_VALUE 0x80

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool WMOpenInput(wchar_t* in_file_name);
bool WMReadSample(int32_t* out_sample);
void WMCloseInput(void);

bool WMOpenOutput(wchar_t* in_file_name);
bool WMWriteSample(uint8_t in_sample);
void WMCloseOutput(bool in_force_close);

#endif