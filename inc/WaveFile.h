/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave file handling                                                        */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __Wave_h
#define __Wave_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Const 
#define WAVE_BLOCK_LENGTH 65536
#define RIFF_HEADER_CHUNK_ID 0x46464952 /* 'RIFF' */
#define RIFF_HEADER_FORMAT_ID 0x45564157 /* 'WAVE' */
#define CHUNK_ID_FORMAT 0x20746d66 /* 'fmt ' */
#define CHUNK_ID_DATA 0x61746164 /* 'data' */

///////////////////////////////////////////////////////////////////////////////
// Wave file structs
#pragma pack(push, 1)

typedef struct 
{
	DWORD	ChunkID;		// 'RIFF'
	DWORD ChunkSize;		
	DWORD Format;			// 'WAVE'
} RIFFHeaderType;

typedef struct 
{
	DWORD	ChunkID;
	DWORD ChunkSize;		
} ChunkHeaderType;

typedef struct 
{
	WORD	AudioFormat;	// PCM = 1
	WORD	NumChannels;
	DWORD	SampleRate;
	DWORD ByteRate;
	WORD	BlockAlign;
	WORD	BitsPerSample;
} FormatChunkType;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern DWORD g_input_wav_file_sample_count;
extern DWORD g_input_wav_file_sample_index;

///////////////////////////////////////////////////////////////////////////////
// Functions prototypes
bool WFOpenInput(wchar_t* in_file_name);
bool WFReadSample(INT32* out_sample);
void WFCloseInput(void);

bool WFOpenOutput(wchar_t* in_file_name, BYTE in_bits_per_sample);
void WFWriteSample(INT32 in_sample);
void WFCloseOutput(bool in_force_close);

#endif