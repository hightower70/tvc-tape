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
#include "Main.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define FREQ_SYNC			1359		// Sync frequency (Hz)
#define FREQ_ZERO			1812		// Zero bit frequency
#define FREQ_LEADING	2128		// Leading bit frequency
#define FREQ_ONE			2577		// One bit frequency
#define FREQ_MIDDLE		((FREQ_ZERO + FREQ_ONE) / 2) // Frequency used for zero-one bit detection

#define OVERSAMPLING_RATE 100l

#define PERIOD_SYNC			(SAMPLE_RATE * OVERSAMPLING_RATE / FREQ_SYNC)
#define PERIOD_ZERO			(SAMPLE_RATE * OVERSAMPLING_RATE / FREQ_ZERO)
#define PERIOD_LEADING	(SAMPLE_RATE * OVERSAMPLING_RATE  / FREQ_LEADING)
#define PERIOD_ONE			(SAMPLE_RATE * OVERSAMPLING_RATE / FREQ_ONE)

#define FREQ_TO_PERIOD(x) (SAMPLE_RATE / x)

#define TAPE_MAX_BLOCK_LENGTH					0x100
#define TAPE_BLOCKHDR_ZERO						0x00
#define TAPE_BLOCKHDR_MAGIC						0x6a
#define TAPE_BLOCKHDR_TYPE_HEADER			0xff
#define TAPE_BLOCKHDR_TYPE_DATA				0x00
#define TAPE_SECTOR_EOF								0xff
#define TAPE_SECTOR_NOT_EOF						0x00

#define LEADING_FREQUENCY_TOLERANCE 30		// leading frequency tolerance in percentage
#define SYNC_FREQUENCY_TOLERANCE 15				// sync frequency tolerance in percentage

///////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

// Tape Block header
typedef struct 
{
	uint8_t Zero;						// always zero byte
  uint8_t Magic;					// always 0x6a
	uint8_t BlockType;			// Block type: Header=0xff, data=0x00
  uint8_t FileType;				// File type: Buffered: 0x01, non-buffered: 0x11
  uint8_t CopyProtect;		// Copy protection: not protected=0x00
	uint8_t SectorsInBlock;	// Number of sectors in this block
} TAPEBlockHeaderType;

// Tape Sector header
typedef struct 
{
	uint8_t SectorNumber;
  uint8_t BytesInSector;		// Sector length in bytes (0=256bytes)
} TAPESectorHeaderType;

// Tape Sector end
typedef struct 
{
	uint8_t EOFFlag;
	uint16_t CRC;
} TAPESectorEndType;

#pragma pack(pop)
					 
///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool TAPEOpenInput(wchar_t* in_file_name);
bool TAPECreateOutput(wchar_t* in_file_name);

bool TAPESave(wchar_t* in_file_name);
LoadStatus TAPELoad(void);

void TAPECloseOutput(void);
void TAPECloseInput(void);

void TAPEInitBlockHeader(TAPEBlockHeaderType* out_block_header);
bool TAPEValidateBlockHeader(TAPEBlockHeaderType* in_block_header);

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern uint16_t g_frequency_offset;
extern uint16_t g_leading_length;
extern uint16_t g_gap_length;

#endif