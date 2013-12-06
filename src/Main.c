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

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Windows.h>
#include <wchar.h>
#include "Main.h"
#include "CharMap.h"
#include "Types.h"
#include "WaveMapper.h"
#include "CASFile.h"
#include "TAPEFile.h"
#include "BASFile.h"
#include "TTPFile.h"
#include "BINFile.h"
#include "HEXFile.h"
#include "DataBuffer.h"
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Types
typedef struct
{
	char* Extension;
	FileTypes Type;
} FileExtensionEntry;

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
static void PrintHelp(void);
static void PrintLogo(void);
static FileTypes DetermineFileType(char* in_file_name);
static bool StringStartsWith(const char* in_string, const char* in_prefix);
static void CloseInputFileList(void);

///////////////////////////////////////////////////////////////////////////////
// Global variables
bool g_output_message = true;  

char g_input_file_name[MAX_PATH_LENGTH];
FileTypes g_input_file_type = FT_Unknown;
char g_output_file_name[MAX_PATH_LENGTH];
FileTypes g_output_file_type = FT_Unknown;
char g_output_wave_file[MAX_PATH_LENGTH];
char g_forced_tape_file_name[MAX_PATH_LENGTH];

int g_forced_autostart = AUTOSTART_NOT_FORCED;
int g_forced_copyprotect = COPYPROTECT_NOT_FORCED;
bool g_overwrite_output_file = false;
bool g_fast_tape_signal = false;
bool g_strict_format_disabled = false;
bool g_skip_digital_filter = false;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FileExtensionEntry l_file_extensions[] = 
{
	{ ".wav", FT_WAV },
	{ ".cas", FT_CAS },
	{ ".bas", FT_BAS },
	{ ".ttp", FT_TTP },
	{ ".bin", FT_BIN },
	{ ".hex", FT_HEX },

	{ NULL,   FT_Unknown }
};

static FILE* l_output_file_name_list = NULL;
static FILE* l_input_file_name_list = NULL;

#include <io.h>
#include <fcntl.h>

///////////////////////////////////////////////////////////////////////////////
// Main function
int main(int argc, char *argv[])
{
	int i;
	char input_file_name[MAX_PATH_LENGTH+1];
	char output_file_name[MAX_PATH_LENGTH+1];
	bool success = true;

	// initialize
	g_output_wave_file[0] = '\0';
	g_input_file_name[0] = '\0';
	g_output_file_name[0] = '\0';
	input_file_name[0] = '\0';
	output_file_name[0] = '\0';
	g_forced_tape_file_name[0] = '\0';

	// Start processing
	PrintLogo();

	//////////////////////
	// Parse command line
	i = 1;
	while(i < argc && success) 
	{
		// switch found
		if(argv[i][0] == '-') 
		{
			switch (tolower(argv[i][1])) 
			{
				case 'h':
					PrintHelp();
					return 0;

				case 'q':
					g_output_message = false;
					break;

				case 'a':
					if( i + 1 < argc )
					{
						i++;
						if( argv[i][0] == '0' )
							g_forced_autostart = AUTOSTART_FORCED_TO_FALSE;
						else
							g_forced_autostart = AUTOSTART_FORCED_TO_TRUE;
					}
					else
					{
						success = false;
					}	
					break;

				case 'c':
					if( i + 1 < argc )
					{
						i++;
						if( argv[i][0] == '0' )
							g_forced_copyprotect = COPYPROTECT_FORCED_TO_FALSE;
						else
							g_forced_autostart = COPYPROTECT_FORCED_TO_TRUE;
					}
					else
					{
						success = false;
					}	
					break;

				case 'o':
					g_overwrite_output_file = true;
					break;

				case 'f':
					g_fast_tape_signal = true;
					break;

				case 'w':
					if( i + 1 < argc )
					{
						i++;
						strcpy(g_output_wave_file, argv[i]);
					}
					else
					{
						success = false;
					}	
					break;

				case 'd':
					g_strict_format_disabled = true;
					break;

				case 'p':
					g_skip_digital_filter = true;
					break;

				case 'n':
					if( i + 1 < argc )
					{
						i++;
						strcpy(g_forced_tape_file_name, argv[i]);
					}
					else
					{
						success = false;
					}	
					break;

				case 's':
					if( i + 1 < argc )
					{
						i++;
						l_output_file_name_list = fopen(argv[i], "wt");
						if(l_output_file_name_list == NULL)
							success = false;
					}
					else
					{
						success = false;
					}	
					break;

				case 'l':
					if( i + 1 < argc )
					{
						i++;
						l_input_file_name_list = fopen(argv[i], "rt");
						if(l_input_file_name_list == NULL)
							success = false;
					}
					else
					{
						success = false;
					}	
					break;

				default:
					fprintf(stderr, "Error: Unknown flag: -%c\n", argv[i][1]);
					return 1;
			}

			// display error
			if(!success)
			{
				fprintf(stderr, "Error: Invalid flag: -%c\n", argv[i][1]);
				return 1;
			}
		} 
		else 
		{
			// file_name found
			if( g_input_file_name[0] == '\0' )
			{
				strcpy(g_input_file_name, argv[i]);
			}
			else
			{
				if( g_output_file_name[0] == '\0' )
				{
					strcpy(g_output_file_name, argv[i]);
				}
				else
				{
					fprintf(stderr, "Error: Too many file name specified: %s.\n", argv[i]);
					return 1;
				}
			}
		}

		if(success)
			i++;
	}

	// Display error
	if(!success)
	{
		fprintf(stderr, "Error: Invalid flag -%c.\n", argv[i][1]);
	}

	// check input file
	if(success)
	{
		if(l_input_file_name_list != NULL)
		{
			strcpy(g_output_file_name, g_input_file_name);
			g_input_file_name[0] = '\0';

			if(g_output_file_name[0] != '\0')
			{
				g_output_file_type = DetermineFileType(g_output_file_name); 
			}
			else
			{
				fclose(l_input_file_name_list);
				l_input_file_name_list = NULL;
				fprintf(stderr, "Error: No output file was specified.\n\n");
				PrintHelp();
				return 1;
			}
		}
		else
		{
			if( g_input_file_name[0] == '\0' )
			{
				fprintf(stderr, "Error: No input file was specified.\n\n");
				PrintHelp();
				return 1;
			}
		}
	}

	// create multi container file for saving
	switch(g_output_file_type)
	{
		case FT_TTP:
			GenerateUniqueFileName(g_output_file_name);
			success = TTPCreateOutput(g_output_file_name);
			break;
	}

	/////////////////////////////////
	// Loop for processing input file
	do
	{
		// load input file name from the list file
		if(l_input_file_name_list != NULL)
		{
			fgets(g_input_file_name, MAX_PATH_LENGTH, l_input_file_name_list);
		}

		// determine input file type
		g_input_file_type = DetermineFileType(g_input_file_name);
		if(g_input_file_type == FT_Unknown)
		{
			fprintf(stderr, "Error: Invalid input file type: %s.\n", g_input_file_name);
			return 1;
		}

		// handle cases when only one file was specified
		if(g_output_file_name[0] == '\0')
		{
			// depends on file type
			switch (g_input_file_type)
			{
				// CAS->WAV conversion
				case FT_CAS:
					strcpy(g_output_file_name, g_input_file_name);
					ChangeFileExtension(g_output_file_name, "wav");
					g_output_file_type = FT_WAV;
					break;

				// WAV->CAS conversion
				case FT_WAV:
					g_output_file_type = FT_CAS;
					break;

				// CAS->BAS conversion
				case FT_BAS:
					strcpy(g_output_file_name, g_input_file_name);
					ChangeFileExtension(g_output_file_name, "cas");
					g_output_file_type = FT_CAS;
					break;

				// WaveIn->CAS
				case FT_WaveInOut:
					g_output_file_type = FT_CAS;
					break;

				// TTP->CAS
				case FT_TTP:
					g_output_file_type = FT_CAS;
					break;
			}
		}
		else
		{
			// determine output file type from file name
			g_output_file_type = DetermineFileType(g_output_file_name);
		}

		// char file type
		if(g_output_file_type == FT_Unknown)
		{
			fprintf(stderr, "Error: Invalid output file type: %s.\n", g_output_file_name);
			return 1;
		}

		// check input-output type
		if((g_input_file_type == FT_WaveInOut || g_input_file_type == FT_WAV) && (g_output_file_type == FT_WaveInOut || g_output_file_type == FT_WAV))
		{
			fprintf(stderr, "Error: Wave In/Wav and Wave out/Wav can't be used together.\n");
			return 1;
		}

		// check -w switch
		if(g_output_wave_file[0] != '\0' && (g_output_file_type == FT_WAV || g_output_file_type == FT_WaveInOut))
		{
			fprintf(stderr, "Error: The -w switch can be used only when wav or wave in is used.\n");
			return 1;
		}

		// Load input file
		switch(g_input_file_type)
		{
			case FT_CAS:
				DisplayMessage("Loading CAS file:%s\n",g_input_file_name);
				success = CASLoad(g_input_file_name);
				break;

			case FT_BAS:
				break;

			case FT_WAV:
				DisplayMessage("Opening WAV file:%s\n",g_input_file_name);
				success = WMOpenInput(g_input_file_name);
				TAPEInit();
				break;

			case FT_TTP:
				DisplayMessage("Loading TTP file:%s\n",g_input_file_name);
				success = TTPOpenInput(g_input_file_name);
				break;

			case FT_HEX:
				DisplayMessage("Loading HEX file:%s\n",g_input_file_name);
				success = HEXLoad(g_input_file_name);
				break;

			case FT_BIN:
				DisplayMessage("Loading BIN file:%s\n",g_input_file_name);
				success = BINLoad(g_input_file_name);
				break;
		}

		// display error
		if(!success)
		{
			fprintf(stderr, "Error: Input file can't be opened or its format is invalid.\n");
			return 1;
		}

		// load file from multi file containers
		do
		{
			if(success)
			{
				switch(g_input_file_type)
				{
					case FT_WAV:
					case FT_WaveInOut:
						success = TAPELoad();
						break;

					case FT_TTP:
						success = TTPLoad();
						break;
				}
			}

			// update flags
			if(success)
			{
				if(g_forced_autostart != AUTOSTART_NOT_FORCED)
					g_db_autostart = (g_forced_autostart == AUTOSTART_FORCED_TO_TRUE);
				if(g_forced_copyprotect != COPYPROTECT_NOT_FORCED)
					g_forced_copyprotect = (g_forced_autostart == COPYPROTECT_FORCED_TO_TRUE);
			}

			// generate output file name
			if(success)
			{
				switch(g_output_file_type)
				{
					case FT_TTP:
					{
						int buffer_pos;
						int file_name_start_pos = 0;
						int file_name_length;

						// init
						output_file_name[0] = '\0';

						// use forced file name if exists
						if(g_forced_tape_file_name[0] != '\0')
						{
							// copy filename
							file_name_length = 0;
							while(g_forced_tape_file_name[file_name_length] != '\0' && file_name_length < DB_MAX_FILENAME_LENGTH)
							{
								output_file_name[file_name_length] = g_forced_tape_file_name[file_name_length];
								file_name_length++;
							}

							output_file_name[file_name_length] = '\0';
						}
	
						// update file name only when it is empty
						file_name_length = strlen(output_file_name);
						if(file_name_length == 0)
						{
							// find file name start
							buffer_pos = 0;
							while(g_input_file_name[buffer_pos] != '\0')
							{
								if(g_input_file_name[buffer_pos] == '\\' || g_input_file_name[buffer_pos] == ':' )
								{
									file_name_start_pos = buffer_pos;
								}

								buffer_pos++;
							}

							// copy file name
							file_name_length = 0;
							while(g_input_file_name[file_name_length] != '\0' && file_name_length < DB_MAX_FILENAME_LENGTH && g_input_file_name[file_name_length] != '.' )
							{
								output_file_name[file_name_length] = g_input_file_name[file_name_start_pos+file_name_length];
								file_name_length++;
							}
							output_file_name[file_name_length] = '\0';
						}
					}
					break;

					default:
						if(g_output_file_name[0] != '\0')
						{
							strcpy(output_file_name, g_output_file_name);
						}
						else
						{
							if(g_db_file_name[0] != '\0')
							{
								TVCStringToANSIString(output_file_name, g_db_file_name);
							}
							else
							{
								strcpy(output_file_name, "tvcdefault");
							}
						}
						AppendOutputFileExtension(output_file_name);
						GenerateUniqueFileName(output_file_name);
						break;
				}
			}

			// save file name
			if(success && l_output_file_name_list != NULL)
			{
				fputs(output_file_name, l_output_file_name_list);
				fputs("\n", l_output_file_name_list);
			}

			// Save output file
			if(success)
			{
				// Save output file
				switch(g_output_file_type)
				{
					case FT_CAS:
						DisplayMessageAndClearToLineEnd("Saving CAS file:%s", output_file_name);
						success = CASSave(output_file_name);
						break;

					case FT_BAS:
						DisplayMessageAndClearToLineEnd("Saving BAS file:%s", output_file_name);
						success = BASSave(output_file_name);
						break;

					case FT_WAV:
						DisplayMessage("Saving WAV file:%s", output_file_name);
						success = TAPESave(output_file_name);
						break;

					case FT_WaveInOut:
						DisplayMessage("Generating tape signal. Press <ESC> to stop.\n");
						success = TAPESave(output_file_name);
						break;

					case FT_TTP:
						DisplayMessageAndClearToLineEnd("Saving:%s", output_file_name);
						success = TTPSave(output_file_name);
						break;

					case FT_BIN:
						DisplayMessageAndClearToLineEnd("Saving BIN file:%s", output_file_name);
						success = BINSave(output_file_name);
						break;

					case FT_HEX:
						DisplayMessageAndClearToLineEnd("Saving HEX file:%s", output_file_name);
						success = HEXSave(output_file_name);
						break;
				}
				DisplayMessage("\n");
			}
		}	while(success && (g_input_file_type == FT_WAV || g_input_file_type == FT_WaveInOut || g_input_file_type == FT_TTP));
	}	while(l_input_file_name_list != NULL && !feof(l_input_file_name_list));

	// close list file
	if(l_output_file_name_list != NULL)
		fclose(l_output_file_name_list);
	if(l_input_file_name_list != NULL)
		fclose(l_input_file_name_list);

	// closes input file
	switch(g_input_file_type)
	{
		case FT_WAV:
		case FT_WaveInOut:
			WMCloseInput();
			TAPEClose();
			break;

		case FT_TTP:
			TTPCloseInput();
			break;
	}

	// closes output files
	switch(g_output_file_type)
	{
		case FT_WAV:
		case FT_WaveInOut:
			TAPEClose();
			break;

		case FT_TTP:
			TTPCloseOutput();
			break;
	}

	// return with status
	if(success)
		return 0;
	else
		return 1;
}

#pragma warning(disable : 4996)

///////////////////////////////////////////////////////////////////////////////
// Close input file list file
static void CloseInputFileList(void)
{
	if(l_input_file_name_list != NULL)
	{
		fclose(l_input_file_name_list);
		l_input_file_name_list = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Gets file type
static FileTypes DetermineFileType(char* in_file_name)
{
	char* input_file_name_extension;
	int i;

	// check for direct wave in/out
	if(StringStartsWith(in_file_name, "wave:"))
	{
#ifdef ENABLE_WAVE_DEVICES
		return FT_WaveInOut;
#else
		return FT_Unknown;
#endif
	}
	else
	{
		// determine file type by extension
		input_file_name_extension = strrchr(in_file_name, '.');

		if(input_file_name_extension != NULL)
		{
			i = 0;
			while(l_file_extensions[i].Extension != NULL)
			{
				if(strcmpi(input_file_name_extension, l_file_extensions[i].Extension ) == 0)
					return l_file_extensions[i].Type;

				i++;
			}

			return FT_Unknown;
		}
	}

	return FT_Unknown;
}

///////////////////////////////////////////////////////////////////////////////
// Appends file extension to the file name is there is no extension
void AppendOutputFileExtension(char* in_out_file_name)
{
	char* filename_pos;
	char* extension_pos;
	int i;

	// find start of the file name
	filename_pos = strrchr(in_out_file_name, PATH_SEPARATOR);
	if(filename_pos == NULL)
		filename_pos = in_out_file_name;

	// find extension
	extension_pos = strchr(filename_pos, '.');
	if(extension_pos == NULL)
	{
		// there is no extension -> append a new

		// determine extension
		i = 0;
		while(l_file_extensions[i].Extension != NULL && l_file_extensions[i].Type != g_output_file_type)
			i++;

		if(l_file_extensions[i].Type != FT_Unknown)
		{
			// append
			strcat(in_out_file_name, l_file_extensions[i].Extension);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Displays message
void DisplayMessage(const char* format, ...)
{
  va_list arglist;

	if(g_output_message)
	{
	  va_start( arglist, format );
		vfprintf( stderr, format, arglist );
		va_end( arglist );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Displays message
void DisplayMessageAndClearToLineEnd(const char* format, ...)
{
	char buffer[SCREEN_WIDTH+1];
  va_list arglist;
	int pos;

	if(g_output_message)
	{
	  va_start( arglist, format );
		vsprintf( buffer, format, arglist );
		va_end( arglist );

		pos = strlen(buffer);
		while(pos<SCREEN_WIDTH-1)
		{
			buffer[pos++] = ' ';
		}
		buffer[pos++] = '\r';
		buffer[pos] = '\0';

		fputs(buffer, stderr);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Displays progress bar
void DisplayProgressBar(char* in_title, int in_value, int in_max_value)
{
	char buffer[20];
	int pos;

	if(!g_output_message)
		return;

	if(in_max_value == 0)
		return;

	pos = in_value * 10 / in_max_value;

	memset(buffer,'=',pos);
	buffer[pos] = '\0';

	fprintf(stdout,"%s: %3d%% [%-10s]\r",in_title, in_value * 100 / in_max_value, buffer);
}

///////////////////////////////////////////////////////////////////////////////
// String starts with comparision
static bool StringStartsWith(const char* in_string, const char* in_prefix)
{
	size_t string_length = strlen(in_string);
	size_t prefix_length = strlen(in_prefix);

  return string_length < prefix_length ? false : strnicmp(in_string, in_prefix, prefix_length) == 0;
}

///////////////////////////////////////////////////////////////////////////////
// Prints program logo message
static void PrintLogo(void)
{
	if(g_output_message)
		fprintf(stderr, "TVCTape v0.9 (c)2013 Laszlo Arvai <laszlo.arvai@gmail.com>\n");
}

///////////////////////////////////////////////////////////////////////////////
// Print usage help message
static void PrintHelp(void)
{
	if(g_output_message)
	{
		fprintf(stderr,
			" TVCTape is a free software for converting between Videoton TV Computer\n"
			" CAS, BAS and Tape audio data format.\n"
			"    Usage:  TVCTape [options] file1 [file2]\n\n"
			"        -q           quiet (no screen output, only errors)\n"
			"        -h           display this help\n\n"
			"        -a           overrides autostart settings (0 - no autostart, 1 - autostart)\n"
			"        -c           overrides copyprotect settings (0 - no copyprotect, 1 - copyprotected)\n"
			"        -o           overwrite output file\n"
			"        -d           disable strict format checking of tape format\n"
			"        -f           fast tape signal generation\n"
			"        -n filename  forces tape file name (stored in tape files)\n"
			"        -s filename  saves list of file name of the created output files\n"
			"        -l filename  load input file names from a list instead of using command line parameter\n"
			"        -p           skip digital filter when processing wav data\n"
			"\n"
			"	- 'file1' and 'file2' can be 'CAS', 'BAS', 'TTP', 'BIN', 'HEX' (Intel), 'WAV' (PCM) or 'WAVE:' (wave in/out device).\n"
			" If two files are specified then it converts from file1->file2.\n"
			" If only one file is specified, it depends on the specified file type:\n"
			"  CAS: Converts to WAV with the same name\n"
			"  WAV: Converts to one or more CAS files\n"
			"  BAS: Converts to CAS file with the same name\n"
			"  WAVE: Converts signal from wave in to one or more CAS files\n"
			"\n"
			"	 Examples:\n"
			"	  TVCTape WAVE: TEST.CAS     Converts tape signal coming from the default WaveIn device to CAS file\n");
	}
}

