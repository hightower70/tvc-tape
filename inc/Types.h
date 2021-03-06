/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Common Type Declarations                                                  */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __Types_h
#define __Types_h

#include <wchar.h>
#include <stdint.h>

// bool type
#ifndef bool
typedef unsigned char	bool;	// always 8 bit
#endif

#ifndef false
#define false ((bool)0)
#endif

#ifndef true
#define true (!false)
#endif

#ifndef NULL
#define NULL 0
#endif

// Common macros
#ifndef LOW
#define LOW(x) ((x)&0xff)
#endif

#ifndef HIGH
#define HIGH(x) ((x)>>8)
#endif

#ifndef BV
#define BV(x) (1<<(x))
#endif

// Path length
#ifndef MAX_PATH_LENGTH
#define MAX_PATH_LENGTH 256
#endif

// Path separator
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/';
#endif

#endif