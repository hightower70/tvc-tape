/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Raw Tape file format handler                                              */
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
#include "TTPFile.h"
#include "TAPEFile.h"
#include "CASFile.h"
#include "DataBuffer.h"
#include "FileUtils.h"
#include "CRC.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FILE* l_ttp_input_file = NULL;
static FILE* l_ttp_output_file = NULL;

///////////////////////////////////////////////////////////////////////////////
// Open TTP file for loading data content
bool TTPOpenInput(char* in_file_name)
{
	// open TTP file
	l_ttp_input_file = fopen(in_file_name, "rb");
	if(l_ttp_input_file == NULL)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Creates TTP file
bool TTPCreateOutput(char* in_file_name)
{
	// save file
	l_ttp_output_file = fopen(in_file_name,"wb");
	if(l_ttp_output_file == NULL)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Loads TTP file
bool TTPLoad(void)
{
	TTPFileHeaderType ttp_file_header;
	TAPEBlockHeaderType block_header;
	CASProgramFileHeaderType cas_program_header;
	bool success = true;
	TAPESectorHeaderType sector_header;
	TAPESectorEndType sector_end;
	BYTE tape_file_name_length;
	BYTE sector_count;
	BYTE sector_index;
	int sector_length;
	int bytes_read;

	// Load and check TTP file header
	ReadBlock(l_ttp_input_file, &ttp_file_header, sizeof(TTPFileHeaderType), &success);
	if(!TTPCheckValidity(&ttp_file_header))
		success = false;

	// Load and check tape block header
	ReadBlock(l_ttp_input_file, &block_header, sizeof(TAPEBlockHeaderType), &success);
	if(!TAPEValidateBlockHeader(&block_header))
		success = false;

	// Load and check tape header sector header
	ReadBlock(l_ttp_input_file, &sector_header, sizeof(TAPESectorHeaderType), &success);
	if(sector_header.SectorNumber != 0)
		success = false;
	
	// Load tape file name
	ReadBlock(l_ttp_input_file, &tape_file_name_length, sizeof(BYTE), &success);
	if(success)
	{
		if(tape_file_name_length <= DB_MAX_FILENAME_LENGTH)
		{
			ReadBlock(l_ttp_input_file, g_db_file_name, tape_file_name_length, &success);
			g_db_file_name[tape_file_name_length] = '\0';
		}
		else
			success = false;
	}

	// Load and check CAS program header
	ReadBlock(l_ttp_input_file, &cas_program_header, sizeof(CASProgramFileHeaderType), &success);
	if(!CASCheckHeaderValidity(&cas_program_header))
		success = false;

	if(success)
	{
		g_db_autostart = (cas_program_header.Autorun != 0);
		g_db_copy_protect = (block_header.CopyProtect != 0);
		g_db_program_type = cas_program_header.FileType;
	}

	// load header sector end
	ReadBlock(l_ttp_input_file, &sector_end, sizeof(TAPESectorEndType), &success);

	// load block header
	ReadBlock(l_ttp_input_file, &block_header, sizeof(TAPEBlockHeaderType), &success);
	if(!TAPEValidateBlockHeader(&block_header))
		success = false;

	// load data sectors
	if(success)
	{
		if(cas_program_header.FileLength != 0)
			sector_count = (cas_program_header.FileLength + 255) / 256;
		else
			sector_count = block_header.SectorsInBlock;

		sector_index = 1;
		bytes_read = 0;
	}

	while(success && sector_index <= sector_count)
	{
		// load sector header
		ReadBlock(l_ttp_input_file, &sector_header, sizeof(TAPESectorHeaderType), &success);
		if(sector_header.SectorNumber != sector_index)
			success = false;

		// determine sector length
		if(sector_header.BytesInSector == 0)
			sector_length = 256;
		else
			sector_length = sector_header.BytesInSector;

		// check sector length
		if(success && ((bytes_read + sector_length) >= DB_MAX_DATA_LENGTH))
			success = false;

		// load sector data
		ReadBlock(l_ttp_input_file, ((BYTE*)g_db_buffer)+bytes_read, sector_length, &success);
		bytes_read += sector_length;

		// load sector end
		ReadBlock(l_ttp_input_file, &sector_end, sizeof(TAPESectorEndType), &success);

		sector_index++;
	}

	if(success)
		g_db_buffer_length = (WORD)bytes_read;
	else
		g_db_buffer_length = 0;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Checks validity of the TTP header
bool TTPCheckValidity(TTPFileHeaderType* in_file_header)
{
	return (in_file_header->Magic == TTP_MAGIC);
}

///////////////////////////////////////////////////////////////////////////////
// Initializes TTP header content
void TTPInitHeader(TTPFileHeaderType* in_file_header)
{
	in_file_header->Magic = TTP_MAGIC;
}

///////////////////////////////////////////////////////////////////////////////
// Saves one buffer content into TTP file
bool TTPSave(char* in_tape_file_name)
{
	bool success = true;
	TTPFileHeaderType ttp_file_header;
	TAPEBlockHeaderType tape_block_header;
	TAPESectorHeaderType sector_header;
	CASProgramFileHeaderType cas_program_header;
	TAPESectorEndType tape_sector_end;
	BYTE sector_count;
	BYTE sector_index;
	int sector_size;
	BYTE tape_file_name_length;

	// init
	tape_file_name_length = strlen(in_tape_file_name);

	// file header
	TTPInitHeader(&ttp_file_header);
	WriteBlock(l_ttp_output_file, &ttp_file_header, sizeof(ttp_file_header), &success);

	// tape block header
	TAPEInitBlockHeader(&tape_block_header);
	tape_block_header.SectorsInBlock	= 1;
	WriteBlock(l_ttp_output_file, &tape_block_header, sizeof(tape_block_header), &success);
	CRCReset();
	CRCAddBlock(((BYTE*)&tape_block_header.Magic), sizeof(tape_block_header) - sizeof(tape_block_header.Zero));

	// header block sector start
	sector_header.SectorNumber		= 0;
	sector_header.BytesInSector	= (BYTE)(sizeof(BYTE) + tape_file_name_length + sizeof(cas_program_header));

	CRCAddBlock(((BYTE*)&sector_header), sizeof(sector_header));
	WriteBlock(l_ttp_output_file, &sector_header, sizeof(sector_header), &success);

	// write tape file name	length
	CRCAddByte(tape_file_name_length);
	WriteBlock(l_ttp_output_file, &tape_file_name_length, sizeof(BYTE), &success);

	// write tape file name
	CRCAddBlock((BYTE*)&g_db_file_name, tape_file_name_length);
	WriteBlock(l_ttp_output_file, in_tape_file_name, tape_file_name_length, &success);

	// write program header
	CASInitHeader(&cas_program_header);
	CRCAddBlock((BYTE*)&cas_program_header, sizeof(cas_program_header));
	WriteBlock(l_ttp_output_file, &cas_program_header, sizeof(cas_program_header), &success);

	// write sector end
	tape_sector_end.EOFFlag = TAPE_SECTOR_EOF;
	CRCAddByte(0);
	tape_sector_end.CRC = CRCGet();
	WriteBlock(l_ttp_output_file, &tape_sector_end, sizeof(tape_sector_end), &success);

	// create data block
	sector_count = (g_db_buffer_length + 255) / 256;
	sector_index = 1;

	// block leading
	TAPEInitBlockHeader(&tape_block_header);
	CRCReset();
	CRCAddBlock(((BYTE*)&tape_block_header.Magic), sizeof(tape_block_header) - sizeof(tape_block_header.Zero));
	WriteBlock(l_ttp_output_file, (BYTE*)&tape_block_header, sizeof(tape_block_header), &success);
	
	while(sector_index <= sector_count && success)
	{
		// write sector header
		sector_size = g_db_buffer_length - 256 * (sector_index - 1);
		if( sector_size > 255 )
			sector_size = 256;
		else
		{
			sector_size = sector_size;
		}

		sector_header.SectorNumber	= sector_index;
		sector_header.BytesInSector	= (sector_size > 255)? 0 : (BYTE)sector_size;

		CRCAddBlock((BYTE*)&sector_header, sizeof(sector_header));
		WriteBlock(l_ttp_output_file, (BYTE*)&sector_header, sizeof(sector_header), &success);

		// sector data
		CRCAddBlock((BYTE*)&g_db_buffer[(sector_index-1)*256], sector_size);
		WriteBlock(l_ttp_output_file, (BYTE*)&g_db_buffer[(sector_index-1)*256], sector_size, &success);

		// sector end
		tape_sector_end.EOFFlag = (sector_index == sector_count) ? TAPE_SECTOR_EOF : TAPE_SECTOR_NOT_EOF;
		CRCAddByte(tape_sector_end.EOFFlag);

		tape_sector_end.CRC = CRCGet();

		WriteBlock(l_ttp_output_file, (BYTE*)&tape_sector_end, sizeof(tape_sector_end), &success);

		CRCReset();
		sector_index++;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Closes TTP Input File
void TTPCloseInput(void)
{
	if(l_ttp_input_file != NULL)
		fclose(l_ttp_input_file);

	l_ttp_input_file = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Closes TTP Output File
void TTPCloseOutput(void)
{
	if(l_ttp_output_file != NULL)
		fclose(l_ttp_output_file);

	l_ttp_output_file = NULL;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Calculates data block count based on file length
//static int TTPCalculateDataBlockCount(FILE* in_file)
//{
//	int header_block_length;
//	int data_block_length;
//	int file_length;
//	long pos;
//	int sector_count;
//
//	// get file length
//	pos = ftell(in_file);
//	fseek(in_file, 0, SEEK_END);
//	file_length = ftell(in_file);
//	fseek(in_file, pos, SEEK_SET);
//
//	header_block_length = sizeof(TTPFileHeader) + sizeof(TAPEBlockHeaderType) + sizeof(TAPESectorHeaderType) + sizeof(BYTE) + strlen(g_tape_file_name) + sizeof(CASProgramFileHeaderType) + sizeof(TAPESectorEndType);
//
//	data_block_length = file_length - header_block_length - sizeof(TAPESectorHeaderType);
//
//	sector_count = (data_block_length + 255 + sizeof(TAPESectorHeaderType) + sizeof(TAPESectorEndType)) / (256 + sizeof(TAPESectorHeaderType) + sizeof(TAPESectorEndType));
//
//	return sector_count;
//}