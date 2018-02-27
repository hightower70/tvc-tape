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

#ifndef __DataBuffer_h
#define __DataBuffer_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define DB_MAX_DATA_LENGTH				65535
#define DB_MAX_FILENAME_LENGTH		16

#define DB_UPMPROGTYPE_PRG		0x01
#define DB_UPMPROGTYPE_ASCII	0x00

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern uint8_t g_db_buffer[DB_MAX_DATA_LENGTH];
extern uint16_t g_db_buffer_length;
extern uint16_t g_db_buffer_index;
extern bool g_db_copy_protect;
extern bool g_db_autostart;
extern char g_db_file_name[DB_MAX_FILENAME_LENGTH+1];
extern uint8_t g_db_program_type;
extern bool g_db_crc_error_detected;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void InitDataBuffer(void);


#endif