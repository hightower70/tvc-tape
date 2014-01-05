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
// Types
typedef enum
{
	TET_Auto,
	TET_ANSI,
	TET_UNICODE,
	TET_UTF8
} TextEncodingType;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void BASInit(void);
bool BASLoad(wchar_t* in_file_name);
bool BASSave(wchar_t* in_file_name);
int BASFindEnd(void);


///////////////////////////////////////////////////////////////////////////////
// Global variables
TextEncodingType g_bas_encoding;


#endif