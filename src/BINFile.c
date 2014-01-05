/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Raw Binary file format handler                                            */
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
#include "BINFile.h"
#include "BASFile.h"
#include "DataBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Module global variables

///////////////////////////////////////////////////////////////////////////////
// Loads BIN file
bool BINLoad(wchar_t* in_file_name)
{
	FILE* bin_file;
	size_t length;

	// initialize
	InitDataBuffer();

	// open BIN file
	bin_file = _wfopen(in_file_name, L"rb");
	if(bin_file == NULL)
		return false;

	length = fread(g_db_buffer,  sizeof(BYTE), DB_MAX_DATA_LENGTH, bin_file);

	g_db_buffer_length = (WORD)length;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Saves BIN file
bool BINSave(wchar_t* in_file_name)
{
	int start_pos = 0;
	FILE* bin_file;

	// skip basic program
	if(g_exclude_basic_program)
	{
		start_pos = BASFindEnd();
	}

	// save file
	bin_file = _wfopen(in_file_name, L"wb");
	if(bin_file == NULL)
		return false;

	fwrite(&g_db_buffer[start_pos], sizeof(BYTE), g_db_buffer_length - start_pos, bin_file);

	fclose(bin_file);

	return true;}
