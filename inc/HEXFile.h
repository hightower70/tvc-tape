/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Intel HEX file format handler                                             */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __HEXFile_h
#define __HEXFile_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool HEXSave(wchar_t* in_file_name);
LoadStatus HEXLoad(wchar_t* in_file_name);

#endif