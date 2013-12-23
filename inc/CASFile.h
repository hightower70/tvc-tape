/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* CAS File format declarations                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __CAS_h
#define __CAS_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define CASBLOCKHDR_FILE_BUFFERED		0x01
#define CASBLOCKHDR_FILE_UNBUFFERED	0x11

///////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

// Tape/CAS Program file header
typedef struct 
{
	BYTE Zero;						// Zero
	BYTE FileType;				// Program type: 0x01 - ASCII, 0x00 - binary
	WORD FileLength;			// Length of the file
	BYTE Autorun;					// Autostart: 0xff, no autostart: 0x00
	BYTE Zeros[10];				// Zero
  BYTE Version;					// Version
} CASProgramFileHeaderType;

// CAS UPM header
typedef struct
{
	BYTE FileType;				// File type: Buffered: 0x01, non-buffered: 0x11
	BYTE CopyProtect;			// Copy Protect: 0x01	file is copy protected, 0x00 non protected
	WORD BlockNumber;			// Number of the blocks (0x80 bytes length) occupied by the program
	BYTE LastBlockBytes;	// Number of the used bytes in the last block
	BYTE Zeros[123];			// unused
} CASUPMHeaderType;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool CASLoad(wchar_t* in_file_name);
bool CASSave(wchar_t* in_file_name);

bool CASCheckUPMHeaderValidity(CASUPMHeaderType* in_header);
bool CASCheckHeaderValidity(CASProgramFileHeaderType* in_header);

void CASInitUPMHeader(CASUPMHeaderType* out_header);
void CASInitHeader(CASProgramFileHeaderType* out_header);


#endif