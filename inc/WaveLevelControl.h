/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave Level Control (Automatic)                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __WaveLevelControl_h
#define __WaveLevelControl_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Types
typedef enum
{
	WLCMT_NoiseKiller,
	WLCMT_LevelControl
} WaveLevelControlModeType;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void WLCInit(void);
INT32 WLCProcessSample(INT32 in_sample);
void WLCClose(void);
void WLCSetMode(WaveLevelControlModeType in_mode);


#endif

