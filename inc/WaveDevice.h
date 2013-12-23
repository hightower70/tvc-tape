/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave Device (In/Out) handling                                             */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __WaveDevice_h
#define __WaveDevice_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"
#include "Main.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool WDOpenInput(wchar_t* in_file_name);
bool WDReadSample(INT32* out_sample);
void WDCloseInput(void);

bool WDOpenOutput(wchar_t* in_file_name);
bool WDWriteSample(BYTE in_sample);
void WDCloseOutput(bool in_force_close);

#endif