/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* ROM (Cartridge) file format handler                                       */
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
#include "ROMLoader.h"
#include "ZX7Compress.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Module global variables

///////////////////////////////////////////////////////////////////////////////
// Loads ROM file
LoadStatus ROMLoad(wchar_t* in_file_name)
{
	return LS_Fatal;
}

///////////////////////////////////////////////////////////////////////////////
// Saves ROM file
bool ROMSave(wchar_t* in_file_name)
{
	FILE* bin_file;
	const uint8_t* loader_image;
	int loader_image_size;
	int compression;
	uint8_t *output_data;
	size_t compressed_size;

	// get loader
	loader_image = GetCartridgeLoaderBytes(g_rom_loader_type, &loader_image_size, &compression);
	if (loader_image == NULL || loader_image_size == 0)
		return false;

	// save file
	bin_file = _wfopen(in_file_name, L"wb");
	if (bin_file == NULL)
		return false;

	// write loader
	fwrite(loader_image, sizeof(uint8_t), loader_image_size, bin_file);

	// write program length
	fwrite(&g_db_buffer_length, sizeof(uint16_t), 1, bin_file);

	// write data bytes
	switch(compression)
	{
		// no compression
		case 0:
			// write program
			fwrite(&g_db_buffer, sizeof(uint8_t), g_db_buffer_length, bin_file);
			break;

		//ZX7 compression
		case 1:
			// compress cart
			output_data = ZX7Compress(ZX7Optimize(g_db_buffer, g_db_buffer_length), g_db_buffer, g_db_buffer_length, &compressed_size);

			// write compressed program
			fwrite(output_data, sizeof(uint8_t), compressed_size, bin_file);
			break;
	}

	// close file
	fclose(bin_file);

	return true;
}
