/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Console text output routines                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Console.h"
#include "Main.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize console for Unicode operation
void ConsoleInit(void)
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
}

///////////////////////////////////////////////////////////////////////////////
// Displays message
void DisplayError(const wchar_t* format, ...)
{
  va_list arglist;

	if(g_output_message)
	{
	  va_start( arglist, format );
		vfwprintf( stderr, format, arglist );
		va_end( arglist );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Displays message
void DisplayMessage(const wchar_t* format, ...)
{
  va_list arglist;

	if(g_output_message)
	{
	  va_start( arglist, format );
		vfwprintf( stdout, format, arglist );
		va_end( arglist );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Displays message
void DisplayMessageAndClearToLineEnd(const wchar_t* format, ...)
{
	wchar_t buffer[SCREEN_WIDTH+1];
  va_list arglist;
	int pos;

	if(g_output_message)
	{
	  va_start( arglist, format );
		vswprintf( (wchar_t*)buffer, SCREEN_WIDTH, format, arglist );
		va_end( arglist );

		pos = wcslen(buffer);
		while(pos<SCREEN_WIDTH-1)
		{
			buffer[pos++] = ' ';
		}
		buffer[pos++] = '\r';
		buffer[pos] = '\0';

		fputws(buffer, stderr);
	}
}
 
///////////////////////////////////////////////////////////////////////////////
// Prints program logo message
void PrintLogo(void)
{
	if(g_output_message)
		fwprintf(stderr, L"TVCTape v0.9 (c)2013 Laszlo Arvai <laszlo.arvai@gmail.com>\n");
}

///////////////////////////////////////////////////////////////////////////////
// Displays progress bar
void DisplayProgressBar(wchar_t* in_title, int in_value, int in_max_value)
{
	wchar_t buffer[20];
	int pos;

	if(!g_output_message)
		return;

	if(in_max_value == 0)
		return;

	pos = in_value * 10 / in_max_value;

	memset(buffer,'=',pos);
	buffer[pos] = '\0';

	fwprintf(stdout, L"%s: %3d%% [%-10s]\r", in_title, in_value * 100 / in_max_value, buffer);
}

///////////////////////////////////////////////////////////////////////////////
// Print usage help message
void PrintHelp(void)
{
	if(g_output_message)
	{
		fwprintf(stderr,
			L" TVCTape is a free software for converting between Videoton TV Computer\n"
			L" CAS, BAS and Tape audio data format.\n"
			L"    Usage:  TVCTape [options] file1 [file2]\n\n"
			L"        -q           quiet (no screen output, only errors)\n"
			L"        -h           display this help\n\n"
			L"        -a           overrides autostart settings (0 - no autostart, 1 - autostart)\n"
			L"        -c           overrides copyprotect settings (0 - no copyprotect, 1 - copyprotected)\n"
			L"        -o           overwrite output file\n"
			L"        -d           disable strict format checking of tape format\n"
			L"        -f           fast tape signal generation\n"
			L"        -n filename  forces tape file name (stored in tape files)\n"
			L"        -s filename  saves list of file name of the created output files\n"
			L"        -l filename  load input file names from a list instead of using command line parameter\n"
			L"        -p           skip digital filter when processing wav data\n"
			L"\n"
			L"	- 'file1' and 'file2' can be 'CAS', 'BAS', 'TTP', 'BIN', 'HEX' (Intel), 'WAV' (PCM) or 'WAVE:' (wave in/out device).\n"
			L" If two files are specified then it converts from file1->file2.\n"
			L" If only one file is specified, it depends on the specified file type:\n"
			L"  CAS: Converts to WAV with the same name\n"
			L"  WAV: Converts to one or more CAS files\n"
			L"  BAS: Converts to CAS file with the same name\n"
			L"  WAVE: Converts signal from wave in to one or more CAS files\n"
			L"\n"
			L"	 Examples:\n"
			L"	  TVCTape WAVE: TEST.CAS     Converts tape signal coming from the default WaveIn device to CAS file\n");
	}
}
