/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* BAS file format handler                                                   */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __BASFile_h
#define __BASFile_h

///////////////////////////////////////////////////////////////////////////////
// Include
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool BASSave(char* in_file_name);

#endif