/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Main File                                                                 */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __Main_h
#define __Main_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h> 
#include <wchar.h>
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Global settings
#define ENABLE_WAVE_DEVICES
#define SCREEN_WIDTH 80
#define SAMPLE_RATE 44100
#define WAVEOUT_BUFFER_LENGTH 32768
#define WAVEOUT_BUFFER_COUNT 4
#define WAVEIN_BUFFER_LENGTH 8192
#define WAVEIN_BUFFER_COUNT 16

///////////////////////////////////////////////////////////////////////////////
// Constants

// Autostart constants
#define AUTOSTART_NOT_FORCED			-1
#define AUTOSTART_FORCED_TO_FALSE	0
#define AUTOSTART_FORCED_TO_TRUE	1

// Copyprotect constants
#define COPYPROTECT_NOT_FORCED			-1
#define COPYPROTECT_FORCED_TO_FALSE	0
#define COPYPROTECT_FORCED_TO_TRUE	1

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void AppendOutputFileExtension(wchar_t* in_out_file_name);

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern wchar_t g_wave_output_file[MAX_PATH_LENGTH];

extern wchar_t g_input_file_name_name[MAX_PATH_LENGTH];
extern FileTypes g_input_file_type;
extern wchar_t g_output_file_name[MAX_PATH_LENGTH];
extern FileTypes g_output_file_type;
extern wchar_t g_output_wave_file[MAX_PATH_LENGTH];
extern wchar_t g_forced_tape_file_name[MAX_PATH_LENGTH];


extern int g_forced_autostart;
extern bool g_output_message;
extern bool g_overwrite_output_file;
extern bool g_exclude_basic_program;
extern WORD g_lomem_address;
extern bool g_one_bit_wave_file;
#endif