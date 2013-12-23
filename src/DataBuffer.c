/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Common data buffer implementation (used by all file formats)              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Include files
#include <stdio.h>
#include "Types.h"
#include "Main.h"
#include "DataBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Global variables
BYTE g_db_buffer[DB_MAX_DATA_LENGTH];					// Program data buffer
WORD g_db_buffer_length = 0;									// Number of bytes in the program data buffer
WORD g_db_buffer_index = 0;										// Buffer index
bool g_db_copy_protect = false;								// Copy protected flag
bool g_db_autostart = false;									// Autostart flag
char g_db_file_name[DB_MAX_FILENAME_LENGTH+1];						// Stored file name (if exists)
BYTE g_db_program_type = DB_UPMPROGTYPE_PRG;	// Program type (Used by UPM header)
bool g_db_crc_error_detected = false;					// True if buffer content was loaded with CRC error
