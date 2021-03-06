/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave filtering                                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __WaveFilter_h
#define __WaveFilter_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Types
typedef enum
{
	FT_NoFilter,
	FT_Fast,
	FT_Strong,
	FT_Auto
} FilterTypes;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
int32_t WFProcessSample(int32_t in_new_sample);
extern FilterTypes g_filter_type;

#endif