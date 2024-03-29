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
#include "ROMFile.h"
#include "DataBuffer.h"
#include "FileUtils.h"
#include "Console.h"
#include "WaveFilter.h"
#include "WaveLevelControl.h"
#include "WaveFile.h"
#include "COMPort.h"
#include "ROMLoader.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
static void CloseInputFileList(void);
static bool ParseWaveGenerationParameters(wchar_t* in_param);
static bool ParseWavePreprocessingParameters(wchar_t* in_param);
static bool ProcessCommandLine(int argc, wchar_t **argv);
static void UpdateStoredFilename(void);

///////////////////////////////////////////////////////////////////////////////
// Global variables
bool g_output_message = true;  

wchar_t g_input_file_name[MAX_PATH_LENGTH];
FileTypes g_input_file_type = FT_Unknown;
wchar_t g_output_file_name[MAX_PATH_LENGTH];
FileTypes g_output_file_type = FT_Unknown;
wchar_t g_output_wave_file[MAX_PATH_LENGTH];
wchar_t g_forced_tape_file_name[MAX_PATH_LENGTH];
bool g_forced_tape_file_name_enabled = false;

int g_forced_autostart = AUTOSTART_NOT_FORCED;
int g_forced_copyprotect = COPYPROTECT_NOT_FORCED;
bool g_overwrite_output_file = false;
bool g_stop_after_one_file = false;
bool g_one_bit_wave_file = false;
bool g_exclude_basic_program = false;
uint16_t g_lomem_address = 6639;
uint16_t g_checksum_start = 0;
bool g_checksum_off = false;
int g_rom_loader_type = 0;
bool g_append_container_files = false;
COMConfigType g_com_config;
int g_binary_divide_position = -1;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FILE* l_output_file_name_list = NULL;
static FILE* l_input_file_name_list = NULL;

///////////////////////////////////////////////////////////////////////////////
// Main function
int wmain( int argc, wchar_t **argv )
{
	wchar_t input_file_name[MAX_PATH_LENGTH+1];
	wchar_t output_file_name[MAX_PATH_LENGTH+1];
	bool success = true;
	FileTypes output_file_type;
	LoadStatus load_status;
	int pos;

	// initialize
	ConsoleInit();
	BASInit();
	COMInit();
	g_output_wave_file[0] = '\0';
	g_input_file_name[0] = '\0';
	g_output_file_name[0] = '\0';
	input_file_name[0] = '\0';
	output_file_name[0] = '\0';
	g_forced_tape_file_name[0] = '\0';

	// Start processing
	PrintLogo();

	// parse command line
	success = ProcessCommandLine(argc, argv);
	if(!success)
		return 1;

	// check input file
	if(success)
	{
		if(l_input_file_name_list != NULL)
		{
			wcscpy(g_output_file_name, g_input_file_name);
			g_input_file_name[0] = '\0';

			if(g_output_file_name[0] != '\0')
			{
				g_output_file_type = DetermineFileType(g_output_file_name); 
			}
			else
			{
				fclose(l_input_file_name_list);
				l_input_file_name_list = NULL;
				DisplayError(L"Error: No output file was specified.\n\n");
				PrintHelp();
				return 1;
			}
		}
		else
		{
			if( g_input_file_name[0] == '\0' )
			{
				DisplayError(L"Error: No input file was specified.\n\n");
				PrintHelp();
				return 1;
			}
		}
	}

	// try to determine output file type
	if(success)
	{
		if(g_output_file_name[0] != '\0')
		{
			g_output_file_type = DetermineFileType(g_output_file_name); 
		}
	}
	
	// create multi container file for saving
	switch(g_output_file_type)
	{
		case FT_TTP:
			if(!g_append_container_files)
				GenerateUniqueFileName(g_output_file_name);

			if (CheckFileExists(g_output_file_name))
			{
				DisplayMessage(L"Appending to TTP file:%s\n", g_output_file_name);
			}
			else
			{
				DisplayMessage(L"Creating TTP file:%s\n", g_output_file_name);
			}
			success = TTPCreateOutput(g_output_file_name);
			break;

		case FT_WAV:
			if (!g_append_container_files)
				GenerateUniqueFileName(g_output_file_name);

			if (CheckFileExists(g_output_file_name))
			{
				DisplayMessage(L"Appending to WAV file:%s\n", g_output_file_name);
			}
			else
			{
				DisplayMessage(L"Creating WAV file:%s\n", g_output_file_name);
			}

			success = TAPECreateOutput(g_output_file_name);
			break;

		case FT_WaveInOut:
			success = TAPECreateOutput(g_output_file_name);
			break;
	}

	/////////////////////////////////
	// Loop for processing input file
	do
	{
		load_status = LS_Success;

		// load input file name from the list file
		if(l_input_file_name_list != NULL)
		{
			do
			{
				// read filename
				if(fgetws(g_input_file_name, MAX_PATH_LENGTH, l_input_file_name_list) == NULL)
				{
					load_status = LS_Fatal;
					break;
				}

				// remove trailing new line character
				pos = (int)wcslen(g_input_file_name);
				if(pos > 0 && g_input_file_name[pos-1] == L'\n')
					g_input_file_name[pos-1] = L'\0';

				// skip leading spaces
				pos = 0;
				while(g_input_file_name[pos] == L' ')
					pos++;

				// skip comment lines
			}	while(g_input_file_name[pos] == L'#');
		}

		if(load_status == LS_Success)
		{
			// determine input file type
			g_input_file_type = DetermineFileType(g_input_file_name);
			if(g_input_file_type == FT_Unknown)
			{
				DisplayError(L"Error: Invalid input file type: %s.\n", g_input_file_name);
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
						wcscpy(g_output_file_name, g_input_file_name);
						ChangeFileExtension(g_output_file_name, L"wav");
						g_output_file_type = FT_WAV;
						break;

					// WAV->CAS conversion
					case FT_WAV:
						g_output_file_type = FT_CAS;
						break;

					// CAS->BAS conversion
					case FT_BAS:
						wcscpy(g_output_file_name, g_input_file_name);
						ChangeFileExtension(g_output_file_name, L"cas");
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

					// COM -> CAS
					case FT_COM:
						g_output_file_type = FT_CAS;
						break;
				}
			}
			else
			{
				// determine output file type from file name
				g_output_file_type = DetermineFileType(g_output_file_name);

				// if wildcard used for filename then use only extension as a filetype
				if(g_output_file_name[0] == '*')
					g_output_file_name[0] = '\0';

			}

			// check output file type
			if(g_output_file_type == FT_Unknown)
			{
				DisplayError(L"Error: Invalid output file type: %s.\n", g_output_file_name);
				return 1;
			}

			// check input-output type
			if(g_input_file_type == FT_WaveInOut && g_output_file_type == FT_WaveInOut)
			{
				DisplayError(L"Error: Wave In/Wav and Wave out/Wav can't be used together.\n");
				return 1;
			}

			// check -w switch
			if(g_output_wave_file[0] != '\0' && (g_output_file_type == FT_WAV || g_output_file_type == FT_WaveInOut))
			{
				DisplayError(L"Error: The -w switch can be used only when wav or wave in is used.\n");
				return 1;
			}

			// Load input file
			switch(g_input_file_type)
			{
				case FT_CAS:
					DisplayMessage(L"Loading CAS file:%s\n",g_input_file_name);
					load_status = CASLoad(g_input_file_name);
					success = (load_status == LS_Success);
					break;

				case FT_BAS:
					DisplayMessage(L"Loading BAS file:%s\n",g_input_file_name);
					load_status = BASLoad(g_input_file_name);
					success = (load_status == LS_Success);
					break;

				case FT_WAV:
					DisplayMessage(L"Opening WAV file:%s\n",g_input_file_name);

					// set filter
					if(g_filter_type == FT_Auto)
						g_filter_type = FT_Strong;

					success = TAPEOpenInput(g_input_file_name);
					break;

				case FT_TTP:
					DisplayMessage(L"Loading TTP file:%s\n",g_input_file_name);
					success = TTPOpenInput(g_input_file_name);
					break;

				case FT_HEX:
					DisplayMessage(L"Loading HEX file:%s\n",g_input_file_name);
					load_status = HEXLoad(g_input_file_name);
					success = (load_status == LS_Success);
					break;

				case FT_BIN:
					DisplayMessage(L"Loading BIN file:%s\n",g_input_file_name);
					load_status = BINLoad(g_input_file_name);
					success = (load_status == LS_Success);
					break;

				case FT_COM:
					DisplayMessage(L"Processing COM input. Press <ESC> to stop.\n");
					load_status = COMLoad();
					success = (load_status == LS_Success);
					break;

				case FT_WaveInOut:
					DisplayMessage(L"Processing audio input. Press <ESC> to stop.\n");
					// set filter
					if(g_filter_type == FT_Auto)
						g_filter_type = FT_Fast;

					success = TAPEOpenInput(g_input_file_name);
					break;
			}
		}

		// open debug wave out file
		if(g_output_wave_file[0] != '\0')
		{
			WFOpenOutput(g_output_wave_file, 16);
		}

		// display error
		if(!success)
		{
			DisplayError(L"Error: Input file can't be opened or its format is invalid.\n");
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
						load_status = TAPELoad();
						break;

					case FT_TTP:
						load_status = TTPLoad();
						break;
				}

				if(load_status == LS_Fatal)
					success = false;
			}
				
			// save file if source file sucessfully was loaded
			if(load_status == LS_Success)
			{
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
						case FT_WaveInOut:
						case FT_WAV:
						case FT_TTP:
						{
							int buffer_pos;
							int file_name_start_pos = 0;
							int file_name_length;

							if(wcslen(g_output_file_name) > 0)
							{
								// if output file name already specified
								wcscpy(output_file_name, g_output_file_name);
								output_file_type = g_output_file_type;
							}
							else
							{
								// generate output file name
								output_file_name[0] = '\0';

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

								output_file_type = g_output_file_type;
							}
						}
						break;

						default:
							// handle dynamic output
							//TODO
							// determine output file type
							if(g_output_file_type == FT_Dynamic)
							{	
								//if(g_db_program_type == 0)
									output_file_type = FT_CAS;
								//else
								//	output_file_type = FT_BAS;
							}
							else
							{
								output_file_type = g_output_file_type;
							}

							// determine output file name
							if(g_output_file_name[0] != '\0')
							{
								wcscpy(output_file_name, g_output_file_name);
							}
							else
							{
								TVCToPCFilename(output_file_name, g_db_file_name);
							}
						
							// flag CRC error
							if(g_db_crc_error_detected)
							{
								wcscat(output_file_name, L"!");
							}

							// add extension and make it uniqie
							AppendFileExtension(output_file_name, output_file_type);
							GenerateUniqueFileName(output_file_name);
							break;
					}
				}

				// save file name
				if(success && l_output_file_name_list != NULL)
				{
					fputws(output_file_name, l_output_file_name_list);
					fputws(L"\n", l_output_file_name_list);
				}

				// Save output file
				if(success)
				{
					// Save output file
					switch(output_file_type)
					{
						// CAS
						case FT_CAS:
							if(g_db_crc_error_detected)
								DisplayMessageAndClearToLineEnd(L"Saving CAS file: %s (CRC Error)", output_file_name);
							else
								DisplayMessageAndClearToLineEnd(L"Saving CAS file: %s", output_file_name);
							success = CASSave(output_file_name);
							break;

						// BAS
						case FT_BAS:
							if(g_db_crc_error_detected)
								DisplayMessageAndClearToLineEnd(L"Saving BAS file: %s (CRC Error)", output_file_name);
							else
								DisplayMessageAndClearToLineEnd(L"Saving BAS file: %s", output_file_name);
							success = BASSave(output_file_name);
							break;

						// WAV
						case FT_WAV:
							if(l_input_file_name_list == NULL)
							{
								DisplayMessage(L"Saving WAV file: %s", output_file_name);
							}
							UpdateStoredFilename();
							success = TAPESave(output_file_name);
							break;

						// Wave Out
						case FT_WaveInOut:
							DisplayMessage(L"Generating tape signal. Press <ESC> to stop.\n");
							UpdateStoredFilename();
							success = TAPESave(output_file_name);
							break;

						// TTP
						case FT_TTP:
							if(l_input_file_name_list == NULL)
							{
								DisplayMessageAndClearToLineEnd(L"Saving: %s", output_file_name);
							}
							UpdateStoredFilename();
							success = TTPSave(output_file_name);
							break;

						// BIN
						case FT_BIN:
							DisplayMessageAndClearToLineEnd(L"Saving BIN file: %s", output_file_name);
							success = BINSave(output_file_name);
							break;

						// HEX
						case FT_HEX:
							DisplayMessageAndClearToLineEnd(L"Saving HEX file: %s", output_file_name);
							success = HEXSave(output_file_name);
							break;

						// ROM
						case FT_ROM:
							DisplayMessageAndClearToLineEnd(L"Creating ROM Cart file: %s", output_file_name);
							success = ROMSave(output_file_name);
							break;

						// COM
						case FT_COM:
							DisplayMessage(L"Sending file over 'COM%d:'. Press <ESC> to stop.\n", g_com_config.PortIndex);
							success = COMSave(output_file_name);
							break;
					}

					if(l_input_file_name_list == NULL)
						DisplayMessage(L"\n");
				}
			}
			else
			{
				// save file name
				if(load_status == LS_Error && l_output_file_name_list != NULL)
				{
					TVCStringToUNICODEString(output_file_name, g_db_file_name);
					fputws(L"#", l_output_file_name_list);
					fputws(output_file_name, l_output_file_name_list);
					fputws(L"\n", l_output_file_name_list);
				}
			}
		}	while(success && (g_input_file_type == FT_TTP || ((g_input_file_type == FT_WAV || g_input_file_type == FT_WaveInOut) && (load_status != LS_Success || !g_stop_after_one_file))));
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
			TAPECloseInput();
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
			TAPECloseOutput();
			break;

		case FT_TTP:
			TTPCloseOutput();
			break;
	}

	// debug wave out close
	if(g_output_wave_file[0] != '0')
	{
		WFCloseOutput(false);
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
// Updates stored filename (WAV, TTP, WaveOut)
static void UpdateStoredFilename(void)
{
	// check if forced file name is active
	if (g_forced_tape_file_name_enabled)
	{
		PCToTVCFilename(g_db_file_name, g_forced_tape_file_name);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Parses wave generation parameters
static bool ParseWaveGenerationParameters(wchar_t* in_param)
{
	wchar_t* token;
	wchar_t* buffer;
	int index;
	int value;

	token = wcstok( in_param, L",", &buffer); 

	index = 0;
	while( token != NULL && index < 3 )
  {												
		value = _wtoi(token);

		switch (index)
		{
			case 0:
				g_frequency_offset = value;
				break;

			case 1:
				g_gap_length = value;
				break;

			case 2:
				g_leading_length = value;
				break;
		}

    // Get next token: 
		token = wcstok( NULL, L",", &buffer);
		index++;
  }

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Parses wave preprocessing parameters
static bool ParseWavePreprocessingParameters(wchar_t* in_param)
{
	wchar_t* token;
	wchar_t* buffer;
	int index;
	int value;

	token = wcstok( in_param, L",", &buffer ); 

	index = 0;
	while( token != NULL && index < 3 )
  {												
		value = _wtoi(token);

		switch (index)
		{
			case 0:
				g_filter_type = (FilterTypes)value;
				break;

			case 1:
				g_wave_level_control_mode = value;
				break;
		}

    // Get next token: 
		token = wcstok( NULL, L",", &buffer );
		index++;
  }

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Processes commands line
static bool ProcessCommandLine(int argc, wchar_t **argv)
{
	int i;
	bool success = true;
	wchar_t* buffer;

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

				case 'f':
					g_append_container_files = true;
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

				case 'e':
					g_exclude_basic_program = true;
					break;

				case 'w':
					if( i + 1 < argc )
					{
						i++;
						wcscpy(g_output_wave_file, argv[i]);
					}
					else
					{
						success = false;
					}	
					break;

				case 'b':
					if( i + 1 < argc )
					{
						switch (tolower(argv[i + 1][0]))
						{
							case 'a':
								g_bas_encoding = TET_ANSI;
								break;

							case '8':
								g_bas_encoding = TET_UTF8;
								break;

							case 'u':
								g_bas_encoding = TET_UNICODE;
								break;

							default:
								success = false;
								break;
						}

						if (success)
							i++;
					}
					else
					{
						success = false;
					}	
					break;

				case 'p':
					if( i + 1 < argc )
					{
						success = ParseWavePreprocessingParameters(argv[i + 1]);
						if (success)
							i++;
					}
					else
					{
						success = false;
					}	
					break;

				case 'g':
					if( i + 1 < argc )
					{
						success = ParseWaveGenerationParameters(argv[i + 1]);
						if (success)
							i++;
					}
					else
					{
						success = false;
					}	
					break;

				case '1':
					g_stop_after_one_file = true;
					g_one_bit_wave_file = true;
					break;

				case 'n':
					if( i + 1 < argc )
					{
						i++;
						wcscpy(g_forced_tape_file_name, argv[i]);
						g_forced_tape_file_name_enabled = true;
					}
					else
					{
						success = false;
					}	
					break;

				case 'm':
					if( i + 1 < argc )
					{
						i++;
						g_lomem_address = _wtoi(argv[i]);
					}
					else
					{
						success = false;
					}	
					break;

				case 't':
					if (i + 1 < argc)
					{
						i++;

						if (_wcsicmp(argv[i], L"off") == 0)
						{
							g_checksum_off = true;
							g_checksum_start = 0;
						}
						else
						{
							g_checksum_off = false;
							g_checksum_start = _wtoi(argv[i]);
						}
					}
					else
					{
						success = false;
					}
					break;

				case 'd':
					if (i + 1 < argc)
					{
						g_binary_divide_position = _wtoi(argv[i + 1]);

						if (g_binary_divide_position > MAX_BINARY_DIVIDE_POSITION || g_binary_divide_position < 0)
							success = false;
						else
							i++;
					}
					else
					{
						success = false;
					}
					break;

				case 'r':
					if (i + 1 < argc)
					{
						g_rom_loader_type = _wtoi(argv[i + 1]);

						if (g_rom_loader_type >= ROM_LOADER_COUNT)
							success = false;

						if (success)
							i++;
					}
					else
					{
						success = false;
					}
					break;

				case 's':
					if( i + 1 < argc )
					{
						l_output_file_name_list = _wfopen(argv[i + 1], L"wt, ccs=UNICODE");
						if(l_output_file_name_list == NULL)
							success = false;

						if (success)
							i++;
					}
					else
					{
						success = false;
					}	
					break;

				case 'l':
					if( i + 1 < argc )
					{
						l_input_file_name_list = _wfopen(argv[i + 1], L"rt, ccs=UNICODE");
						if(l_input_file_name_list == NULL)
							success = false;

						if (success)
							i++;
					}
					else
					{
						success = false;
					}	
					break;

				case 'u':
					if( i + 1 < argc )
					{
						wchar_t* param;
						int index;
									
						index = 0;
						param = wcstok(argv[++i], L",", &buffer);
						while (param != NULL)
						{
							switch(index)
							{
								// port index 
								case 0:
									g_com_config.PortIndex = _wtoi(param);
									break;

								// baud rate
								case 1:
									g_com_config.BaudRate = _wtoi(param);
									break;

								// tansfer length
								case 2:
									g_com_config.TransferLength = _wtoi(param);
									break;
							}

							param = wcstok(NULL, L",", &buffer);
							index++;
						}
					}
					else
					{
						success = false;
					}	
					break;

				default:
					DisplayError(L"Error: Unknown flag: -%c\n", argv[i][1]);
					return false;
			}

			// display error
			if(!success)
			{
				DisplayError(L"Error: Invalid flag: -%c\n", argv[i][1]);
				return false;
			}
		} 
		else 
		{
			// file_name found
			if( g_input_file_name[0] == '\0' )
			{
				wcscpy(g_input_file_name, argv[i]);
			}
			else
			{
				if( g_output_file_name[0] == '\0' )
				{
					wcscpy(g_output_file_name, argv[i]);
				}
				else
				{
					DisplayError(L"Error: Too many file name specified: %s.\n", argv[i]);
					return false;
				}
			}
		}

		if(success)
			i++;
	}

	// Display error
	if(!success)
	{
		DisplayError(L"Error: Invalid flag -%c.\n", argv[i][1]);
	}

	return success;
}