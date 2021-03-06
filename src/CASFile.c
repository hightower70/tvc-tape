/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* CAS file format handler                                                   */
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
#include <string.h>
#include "Types.h"
#include "Main.h"
#include "CASFile.h"
#include "DataBuffer.h"
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Module global variables

///////////////////////////////////////////////////////////////////////////////
// Loads CAS file
LoadStatus CASLoad(wchar_t* in_file_name)
{
	FILE* cas_file = NULL;
	LoadStatus load_status = LS_Success;
	CASUPMHeaderType upm_header;							
	CASProgramFileHeaderType program_header;

	// open CAS file
	cas_file = _wfopen(in_file_name, L"rb");

	if(cas_file == NULL)
		load_status = LS_Fatal;

	if(load_status == LS_Success)
	{
		// load UPM header
		ReadBlock(cas_file, &upm_header, sizeof(upm_header), &load_status);

		// load program header
		ReadBlock(cas_file, &program_header, sizeof(program_header), &load_status);

		// Check validity
		if(!CASCheckHeaderValidity(&program_header))
			load_status = LS_Fatal;

		if(!CASCheckUPMHeaderValidity(&upm_header))
			load_status = LS_Fatal;
	}

	// load program data
	if(load_status == LS_Success)
	{
		ReadBlock(cas_file, g_db_buffer, program_header.FileLength, &load_status);

		if(load_status == LS_Success)
		{
			g_db_buffer_length = program_header.FileLength;
			g_db_copy_protect = (upm_header.CopyProtect != 0);
			g_db_autostart = (program_header.Autorun != 0);

			// generate TVC filename
			PCToTVCFilename(g_db_file_name, in_file_name);
		}	
	}

	// close file
	if(cas_file != NULL)
		fclose(cas_file);
										 
	return load_status;
}

///////////////////////////////////////////////////////////////////////////////
// Saves CAS file
bool CASSave(wchar_t* in_file_name)
{
	CASUPMHeaderType upm_header;							
	CASProgramFileHeaderType program_header;
	FILE* cas_file;

	// save file
	cas_file = _wfopen(in_file_name, L"wb");
	if(cas_file == NULL)
		return false;

	// init headers
	CASInitHeader(&program_header);
	CASInitUPMHeader(&upm_header);

	fwrite(&upm_header,sizeof(upm_header), 1, cas_file);
	fwrite(&program_header,sizeof(program_header), 1, cas_file);
	fwrite(g_db_buffer, sizeof(uint8_t), (size_t)g_db_buffer_length, cas_file);

	fclose(cas_file);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Checks UPM header validity
bool CASCheckUPMHeaderValidity(CASUPMHeaderType* in_header)
{
	if( in_header->FileType == CASBLOCKHDR_FILE_UNBUFFERED )
		return true;
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
// Cheks CAS header validity
bool CASCheckHeaderValidity(CASProgramFileHeaderType* in_header)
{
	if( in_header->Zero == 0 &&
			(in_header->FileType == DB_UPMPROGTYPE_PRG ||
			in_header->FileType == DB_UPMPROGTYPE_ASCII ))
		return true;
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
// Initialize CAS Headers
void CASInitHeader(CASProgramFileHeaderType* out_header)
{
	memset(out_header, 0, sizeof(CASProgramFileHeaderType));

	out_header->Zero				= 0x00;
	out_header->FileType		= g_db_program_type;
	out_header->FileLength	= g_db_buffer_length;
	out_header->Autorun			= (g_db_autostart)?0xff:0x00;
  out_header->Version			= 0;
}

///////////////////////////////////////////////////////////////////////////////
// Initizes UPM header
void CASInitUPMHeader(CASUPMHeaderType* out_header)
{
	memset(out_header, 0, sizeof(CASUPMHeaderType));

	uint16_t cas_length = g_db_buffer_length + sizeof(CASUPMHeaderType) + sizeof(CASProgramFileHeaderType);

	out_header->FileType				= CASBLOCKHDR_FILE_UNBUFFERED;
	out_header->CopyProtect			= (g_db_copy_protect)?0xff:0x00;
	out_header->BlockNumber			= cas_length / 128;
	out_header->LastBlockBytes	= cas_length % 128;
}
