/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Raw Binary file format handler                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __ROMFile_h
#define __ROMFile_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Constants

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool ROMSave(wchar_t* in_file_name);
LoadStatus ROMLoad(wchar_t* in_file_name);

#endif
