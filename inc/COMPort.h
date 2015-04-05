/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Serial port file transfer functions                                       */
/*                                                                           */
/* Copyright (C) 2013-15 Laszlo Arvai                                        */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __COMPort_h
#define __COMPort_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Types
typedef struct
{
	int PortIndex;
	int BaudRate;
	int TransferLength;

} COMConfigType;


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void COMInit(void);
bool COMSave(wchar_t* in_tape_file_name);
LoadStatus COMLoad(void);
												 
#endif