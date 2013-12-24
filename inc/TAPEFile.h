/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Tape file format and FSK (Frequency Shift Keying) modulation handler      */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __TapeFormat_h
#define __TapeFormat_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define FREQ_SYNC			1359
#define FREQ_ZERO			1812
#define FREQ_LEADING	2128
#define FREQ_ONE			2577
#define FREQ_MIDDLE		((FREQ_ZERO + FREQ_ONE) / 2)

#define PERIOD_SYNC			(SAMPLE_RATE / FREQ_SYNC)
#define PERIOD_ZERO			(SAMPLE_RATE / FREQ_ZERO)
#define PERIOD_LEADING	(SAMPLE_RATE / FREQ_LEADING)
#define PERIOD_ONE			(SAMPLE_RATE / FREQ_ONE)

#define FREQ_TO_PERIOD(x) (SAMPLE_RATE / x)

#define TAPE_BLOCKHDR_ZERO						0x00
#define TAPE_BLOCKHDR_MAGIC						0x6a
#define TAPE_BLOCKHDR_TYPE_HEADER			0xff
#define TAPE_BLOCKHDR_TYPE_DATA				0x00
#define TAPE_SECTOR_EOF								0xff
#define TAPE_SECTOR_NOT_EOF						0x00

#define LEADING_FREQUENCY_TOLERANCE 30		// leading frequency tolerance in percentage
#define SYNC_FREQUENCY_TOLERANCE 10				// sync frequency tolerance in percentage

///////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

// Tape Block header
typedef struct 
{
	BYTE Zero;						// always zero byte
  BYTE Magic;						// always 0x6a
	BYTE BlockType;				// Block type: Header=0xff, data=0x00
  BYTE FileType;				// File type: Buffered: 0x01, non-buffered: 0x11
  BYTE CopyProtect;			// Copy protection: not protected=0x00
	BYTE SectorsInBlock;	// Number of sectors in this block
} TAPEBlockHeaderType;

// Tape Sector header
typedef struct 
{
	BYTE SectorNumber;
  BYTE BytesInSector;		// Sector length in bytes (0=256bytes)
} TAPESectorHeaderType;

// Tape Sector end
typedef struct 
{
	BYTE EOFFlag;
	WORD CRC;
} TAPESectorEndType;

#pragma pack(pop)
					 
///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool TAPESave(wchar_t* in_file_name);
void TAPEInit(void);
bool TAPELoad(void);
void TAPEClose(void);

/*
bool TAPEOpenInput(wchar_t* in_file_name);
bool TAPECreateOutput(wchar_t* in_file_name);
bool TAPELoad(void);
bool TAPESave(void);
void TAPECloseInput(void);
void TAPECloseOutput(void);
		*/
void TAPEInitBlockHeader(TAPEBlockHeaderType* out_block_header);
bool TAPEValidateBlockHeader(TAPEBlockHeaderType* in_block_header);

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern WORD g_frequency_offset;
extern WORD g_leading_length;
extern WORD g_gap_length;

#endif