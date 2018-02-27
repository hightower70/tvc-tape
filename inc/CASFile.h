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
#include "FileUtils.h"

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
	uint8_t Zero;							// Zero
	uint8_t FileType;					// Program type: 0x01 - ASCII, 0x00 - binary
	uint16_t FileLength;			// Length of the file
	uint8_t Autorun;					// Autostart: 0xff, no autostart: 0x00
	uint8_t Zeros[10];				// Zero
  uint8_t Version;					// Version
} CASProgramFileHeaderType;

// CAS UPM header
typedef struct
{
	uint8_t FileType;				// File type: Buffered: 0x01, non-buffered: 0x11
	uint8_t CopyProtect;		// Copy Protect: 0x01	file is copy protected, 0x00 non protected
	uint16_t BlockNumber;		// Number of the blocks (0x80 bytes length) occupied by the program
	uint8_t LastBlockBytes;	// Number of the used bytes in the last block
	uint8_t Zeros[123];			// unused
} CASUPMHeaderType;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
LoadStatus CASLoad(wchar_t* in_file_name);
bool CASSave(wchar_t* in_file_name);

bool CASCheckUPMHeaderValidity(CASUPMHeaderType* in_header);
bool CASCheckHeaderValidity(CASProgramFileHeaderType* in_header);

void CASInitUPMHeader(CASUPMHeaderType* out_header);
void CASInitHeader(CASProgramFileHeaderType* out_header);


#endif