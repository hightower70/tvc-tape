/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* File handling helper functions                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __FileUtils_h
#define __FileUtils_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>


///////////////////////////////////////////////////////////////////////////////
// Types

// Possible file types
typedef enum 
{
	FT_Unknown,		// Invalid
	FT_CAS,				// CAS file
	FT_WAV,				// Wave file
	FT_BAS,				// Basic source file
	FT_TTP,				// Tape emulation file
	FT_HEX,				// Intel HEX file
	FT_BIN,				// Binary file
	FT_WaveInOut,
	FT_Dynamic // 
} FileTypes;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void ReadBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success);
void WriteBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success);

void GenerateUniqueFileName(wchar_t* in_file_name);
void GetFileNameAndExtension(wchar_t* in_file_name, wchar_t* in_path);
void GetFileNameWithoutExtension(wchar_t* in_file_name, wchar_t* in_path);
void ChangeFileExtension(wchar_t* in_file_name, wchar_t* in_extension);
void AppendFileExtension(wchar_t* in_out_file_name, FileTypes in_filetypes);

FileTypes DetermineFileType(wchar_t* in_file_name);
bool StringStartsWith(const wchar_t* in_string, const wchar_t* in_prefix);

																									 
void GenerateTVCFileName(wchar_t* out_tvc_file_name, wchar_t* in_file_name);

#endif
