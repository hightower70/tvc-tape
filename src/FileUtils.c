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
// Generates unique file name
void GenerateUniqueFileName(char* in_file_name)
{
	char file_name[MAX_PATH_LENGTH];
	char file_name_extension[MAX_PATH_LENGTH];
	char file_name_without_extension[MAX_PATH_LENGTH];
	char *extension;
	int counter = 0;
	char counter_string[20];
	FILE* test;

	// if overwrite enabled  -> do nothing
	if(g_overwrite_output_file)
		return;

	// if overwrite not enabled -> generate unique file name

	// split file name
	extension = strrchr(in_file_name, '.');
	if(extension != NULL)
	{
		strcpy(file_name_extension, extension+1);
		*extension = '\0';
		strcpy(file_name_without_extension, in_file_name);
	}
	else
	{
		strcpy(file_name_without_extension, in_file_name);
		file_name_extension[0] = '\0';
	}

	do
	{
		// generate file name
		strcpy(file_name, file_name_without_extension);

		if(counter != 0)
		{
			sprintf(counter_string, "(%d)", counter);
			strcat(file_name, counter_string);
		}

		strcat(file_name, ".");
		strcat(file_name, file_name_extension);

		// check file existence
		test=fopen(file_name, "r");
		if(test != NULL)
		{
			fclose(test);
			counter++;
		}
	}	while(test != NULL);

	// update file name
	strcpy(in_file_name, file_name);
}

///////////////////////////////////////////////////////////////////////////////
// Changes file name extension
void ChangeFileExtension(char* in_file_name, char* in_extension)
{
	char* extension;

	extension = strrchr(in_file_name, '.');

	if(extension != NULL)
	{
		extension++;
		strcpy(extension, in_extension);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Gets filename and extension from full file name
void GetFileNameAndExtension(char* in_file_name, char* in_path)
{
	char* filename;

	filename = strrchr(in_file_name, '\\');

	if(filename != NULL)
	{
		filename++;
	}
	else
	{
		filename = in_path;
	}

	strcpy(in_file_name, filename);
}

///////////////////////////////////////////////////////////////////////////////
// Gets filename without extension from full file name
void GetFileNameWithoutExtension(char* in_file_name, char* in_path)
{
	char* filename_start;
	char* filename_end;

	// determine start of the filename
	filename_start = strrchr(in_file_name, '\\');

	if(filename_start != NULL)
	{
		filename_start++;
	}
	else
	{
		filename_start = in_path;
	}

	// determine end of the filename
	filename_end = strrchr(in_path, '.');
	
	// copy filename
	while( *filename_start != '\0' && (filename_end == NULL || filename_start < filename_end))
	{
		*in_file_name++ = *filename_start++;
	}
	*in_file_name = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Generates TVC filename from file full path 
void GenerateTVCFileName(char* out_tvc_file_name, char* in_file_name)
{
	char buffer[MAX_PATH_LENGTH];

	// get filename only
	GetFileNameWithoutExtension(buffer, in_file_name);

	// limit length
	if(strlen(buffer)>DB_MAX_FILENAME_LENGTH)
		g_db_file_name[DB_MAX_FILENAME_LENGTH] = '\0';

	// convert charmap
	ANSIStringToTVCString(out_tvc_file_name, buffer);
}


///////////////////////////////////////////////////////////////////////////////
// Reads a block from a file and sets success flag
void ReadBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success)
{
	if(!(*inout_success))
		return;

	*inout_success = (fread(in_buffer, in_size, 1, in_file) == 1);
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