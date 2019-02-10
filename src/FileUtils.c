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

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdio.h>
#include <string.h>
#include "CharMap.h"
#include "DataBuffer.h"
#include "FileUtils.h"
#include "Main.h"

///////////////////////////////////////////////////////////////////////////////
// Types
typedef struct
{
	wchar_t* Extension;
	FileTypes Type;
} FileExtensionEntry;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FileExtensionEntry l_file_extensions[] = 
{
	{ L".wav", FT_WAV },
	{ L".cas", FT_CAS },
	{ L".bas", FT_BAS },
	{ L".ttp", FT_TTP },
	{ L".bin", FT_BIN },
	{ L".hex", FT_HEX },
	{ L".rom", FT_ROM },

	{ NULL,   FT_Unknown }
};


///////////////////////////////////////////////////////////////////////////////
// Generates unique file name
void GenerateUniqueFileName(wchar_t* in_file_name)
{
	wchar_t file_name[MAX_PATH_LENGTH];
	wchar_t file_name_extension[MAX_PATH_LENGTH];
	wchar_t file_name_without_extension[MAX_PATH_LENGTH];
	wchar_t *extension;
	int counter = 0;
	wchar_t counter_string[20];
	FILE* test;

	// if overwrite enabled  -> do nothing
	if(g_overwrite_output_file)
		return;

	// if overwrite not enabled -> generate unique file name

	// split file name
	extension = wcsrchr(in_file_name, '.');
	if(extension != NULL)
	{
		wcscpy(file_name_extension, extension+1);
		*extension = '\0';
		wcscpy(file_name_without_extension, in_file_name);
	}
	else
	{
		wcscpy(file_name_without_extension, in_file_name);
		file_name_extension[0] = '\0';
	}

	do
	{
		// generate file name
		wcscpy(file_name, file_name_without_extension);

		if(counter != 0)
		{
			swprintf(counter_string, 20, L"(%d)", counter);
			wcscat(file_name, counter_string);
		}

		wcscat(file_name, L".");
		wcscat(file_name, file_name_extension);

		// check file existence
		test=_wfopen(file_name, L"r");
		if(test != NULL)
		{
			fclose(test);
			counter++;
		}
	}	while(test != NULL);

	// update file name
	wcscpy(in_file_name, file_name);
}

///////////////////////////////////////////////////////////////////////////////
// Changes file name extension
void ChangeFileExtension(wchar_t* in_file_name, wchar_t* in_extension)
{
	wchar_t* extension;

	extension = wcsrchr(in_file_name, '.');

	if(extension != NULL)
	{
		extension++;
		wcscpy(extension, in_extension);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Gets filename and extension from full file name
void GetFileNameAndExtension(wchar_t* out_file_name, wchar_t* in_path)
{
	wchar_t* filename;

	filename = wcsrchr(in_path, '\\');

	if(filename != NULL)
	{
		filename++;
	}
	else
	{
		filename = in_path;
	}

	wcscpy(out_file_name, filename);
}

///////////////////////////////////////////////////////////////////////////////
// Gets filename without extension from full file name
void GetFileNameWithoutExtension(wchar_t* out_file_name, wchar_t* in_path)
{
	wchar_t* filename_start;
	wchar_t* filename_end;

	// determine start of the filename
	filename_start = wcsrchr(in_path, '\\');

	if(filename_start != NULL)
	{
		filename_start++;
	}
	else
	{
		filename_start = in_path;
	}

	// determine end of the filename
	filename_end = wcsrchr(in_path, '.');
	
	// copy filename
	while( *filename_start != '\0' && (filename_end == NULL || filename_start < filename_end))
	{
		*out_file_name++ = *filename_start++;
	}
	*out_file_name = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Convert PC filename to TVC filename
void PCToTVCFilename(char* out_tvc_file_name, wchar_t* in_file_name)
{
	wchar_t buffer[MAX_PATH_LENGTH];

	// get filename only
	GetFileNameWithoutExtension(buffer, in_file_name);

	// limit length
	if(wcslen(buffer)>DB_MAX_FILENAME_LENGTH)
		buffer[DB_MAX_FILENAME_LENGTH] = '\0';

	// convert charmap
	UNICODEStringToTVCString(out_tvc_file_name, buffer);
}

///////////////////////////////////////////////////////////////////////////////
// Generates TVC filename from file full path 
void TVCToPCFilename(wchar_t* out_tvc_file_name, char* in_file_name)
{
	int source_index, destintion_index;
	wchar_t ch;

	// handle empty file name
	if(in_file_name[0] == '\0')
	{
		in_file_name = "tvcdefault";
	}

	// convert to unicode and remove invalid path characters
	source_index = 0;
	destintion_index = 0;
	while(source_index < MAX_PATH_LENGTH && destintion_index<MAX_PATH_LENGTH-1 && in_file_name[source_index] != '\0')
	{
		ch = TVCCharToUNICODEChar(in_file_name[source_index++]);

		// skip invalid characters for file name
		if(ch != '<' && ch != '>' && ch != ':' && ch != '"' && ch !='/' && ch != '\\' && ch != '|' && ch != '?' && ch != '*')
			out_tvc_file_name[destintion_index++] = ch;
	}

	out_tvc_file_name[destintion_index] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Appends file extension to the file name is there is no extension
void AppendFileExtension(wchar_t* in_out_file_name, FileTypes in_filetypes)
{
	wchar_t* filename_pos;
	wchar_t* extension_pos;
	int i;

	// find start of the file name
	filename_pos = wcsrchr(in_out_file_name, PATH_SEPARATOR);
	if(filename_pos == NULL)
		filename_pos = in_out_file_name;

	// find extension
	extension_pos = wcschr(filename_pos, '.');
	if(extension_pos == NULL)
	{
		// there is no extension -> append a new

		// determine extension
		i = 0;
		while(l_file_extensions[i].Extension != NULL && l_file_extensions[i].Type != in_filetypes)
			i++;

		if(l_file_extensions[i].Extension != NULL && l_file_extensions[i].Type != FT_Unknown)
		{
			// append
			wcscat(in_out_file_name, l_file_extensions[i].Extension);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Gets file type
FileTypes DetermineFileType(wchar_t* in_file_name)
{
	wchar_t* input_file_name_extension;
	int i;

	// check for direct wave in/out
	if(StringStartsWith(in_file_name, L"wave:"))
	{
#ifdef ENABLE_WAVE_DEVICES
		return FT_WaveInOut;
#else
		return FT_Unknown;
#endif
	}
	else
	{
		if(StringStartsWith(in_file_name, L"com:"))
		{
			return FT_COM;
		}
		else
		{
			// determine file type by extension
			input_file_name_extension = wcsrchr(in_file_name, '.');

			if(input_file_name_extension != NULL)
			{
				i = 0;
				while(l_file_extensions[i].Extension != NULL)
				{
					if(_wcsicmp(input_file_name_extension, l_file_extensions[i].Extension ) == 0)
						return l_file_extensions[i].Type;

					i++;
				}

				return FT_Unknown;
			}
		}
	}

	return FT_Unknown;
}

///////////////////////////////////////////////////////////////////////////////
// String starts with comparision
bool StringStartsWith(const wchar_t* in_string, const wchar_t* in_prefix)
{
	size_t string_length = wcslen(in_string);
	size_t prefix_length = wcslen(in_prefix);

  return string_length < prefix_length ? false : _wcsnicmp(in_string, in_prefix, prefix_length) == 0;
}


///////////////////////////////////////////////////////////////////////////////
// Reads a block from a file and sets success flag
void ReadBlock(FILE* in_file, void* in_buffer, int in_size, LoadStatus* inout_load_status)
{
	if(*inout_load_status != LS_Success)
		return;

	if(fread(in_buffer, in_size, 1, in_file) != 1)
		*inout_load_status = LS_Fatal;
}

///////////////////////////////////////////////////////////////////////////////
// Writes a block to the file and sets success flag
void WriteBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success)
{
	if(!(*inout_success))
		return;

	if(in_size==0)
		return;

	*inout_success = (fwrite(in_buffer, in_size, 1, in_file) == 1);
}