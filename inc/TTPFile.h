/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Raw Tape file format handler                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __TTPFile_h
#define __TTPFile_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define TTP_MAGIC 0x30505454 // 'TTP0'

///////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

// TTP File header
typedef struct 
{
	DWORD Magic;
} TTPFileHeaderType;

#pragma pack(pop)


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool TTPOpenInput(wchar_t* in_file_name);
bool TTPCreateOutput(wchar_t* in_file_name);
LoadStatus TTPLoad(void);
bool TTPSave(wchar_t* in_tape_file_name);
void TTPCloseInput(void);
void TTPCloseOutput(void);


bool TTPCheckValidity(TTPFileHeaderType* in_file_header);
void TTPInitHeader(TTPFileHeaderType* in_file_header);

#endif