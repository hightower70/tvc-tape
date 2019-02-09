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
// Constants
#define SIGNAL_LEVEL_RESOLUTION 10

///////////////////////////////////////////////////////////////////////////////
// Module global variables

static HANDLE l_console_handle;

// signal level dB scale values
static int l_signal_level[SIGNAL_LEVEL_RESOLUTION] =
{
// -30    -25   -20   -15    -10     -7     -5     -3     -1      0 (level in dB)
   1012, 1799, 3200, 5690, 10119, 14294, 17995, 22654, 28520, 32000
};

///////////////////////////////////////////////////////////////////////////////
// Initialize console for Unicode operation
void ConsoleInit(void)
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
	l_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);  // Get handle to standard output

	SetConsoleTextAttribute(l_console_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
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
		fwprintf(stderr, L"TVCTape v0.3 (c)2013-2019 Laszlo Arvai <laszlo.arvai@gmail.com>\n");
}

///////////////////////////////////////////////////////////////////////////////
// Displays progress bar
void DisplayProgressBar(wchar_t* in_title, int in_value, int in_max_value)
{
	wchar_t buffer[20];
	int pos;
	int i;

	if(!g_output_message)
		return;

	if(in_max_value == 0)
		return;

	pos = in_value * 10 / in_max_value;

	for(i=0; i<pos; i++)
		buffer[i] = '=';
	buffer[pos] = '\0';

	fwprintf(stdout, L"%s: %3d%% [%-10s]\r", in_title, in_value * 100 / in_max_value, buffer);
}

///////////////////////////////////////////////////////////////////////////////
// Displays signal level indicator
void DisplaySignalLevel(int32_t in_peak_level, bool in_cpu_overload, const wchar_t* in_format, ...)
{
	static int prev_peak_pos = 0;
	int peak_pos;
	wchar_t buffer[SCREEN_WIDTH+1];
  va_list arglist;
	int pos;
	int i;

	// check if output messages are disabled
	if(!g_output_message)
		return;

	// length of the indicator
	peak_pos = 0;
	while(peak_pos < SIGNAL_LEVEL_RESOLUTION &&  in_peak_level > l_signal_level[peak_pos])
		peak_pos++;

	if(peak_pos < prev_peak_pos)
	{
		if(prev_peak_pos > 0)
			peak_pos = prev_peak_pos - 1;
		else
			peak_pos = 0;
	}

	prev_peak_pos = peak_pos;

	// display indicator
	pos = 0;

	fwprintf(stdout, L"Signal: [");
	pos += 9;

	for(i = 0; i < SIGNAL_LEVEL_RESOLUTION; i++)
	{
		switch(i)
		{
			case 0:
				SetConsoleTextAttribute(l_console_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				break;

			case 8:
				SetConsoleTextAttribute(l_console_handle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
				break;

			case 9:
				SetConsoleTextAttribute(l_console_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
				break;
		}

		// put character
		if(i < peak_pos)
			fputwc('=', stdout);
		else
			fputwc(' ', stdout);
	}
	pos += SIGNAL_LEVEL_RESOLUTION;

	// restore original color
	SetConsoleTextAttribute(l_console_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	fputwc(']', stdout);
	pos++;

	// CPU overload color (RED)
	SetConsoleTextAttribute(l_console_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);

	if(in_cpu_overload)
		fwprintf(stdout, L"CPU ");
	else
		fwprintf(stdout, L"    ");
	pos += 4;

	// restore original color
	SetConsoleTextAttribute(l_console_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	va_start( arglist, in_format );
	vswprintf( (wchar_t*)buffer, SCREEN_WIDTH, in_format, arglist );
	va_end( arglist );

	i = wcslen(buffer);
	pos += i;
	while(pos<SCREEN_WIDTH-1)
	{
		buffer[i++] = ' ';
		pos++;
	}
	buffer[i++] = '\r';
	buffer[i] = '\0';

	fputws(buffer, stderr);
}

///////////////////////////////////////////////////////////////////////////////
// Print usage help message
void PrintHelp(void)
{
	if(g_output_message)
	{
		fwprintf(stderr,
			L"TVCTape is a free software for converting between Videoton TV Computer\n"
			L"various program file formats.\n\n"
			L" Usage:  TVCTape [options] file1 [file2]\n\n"
			L"  -q           quiet (no screen output, only errors)\n"
			L"  -h           display this help\n"
			L"  -a x         overrides autostart settings\n"
			L"     x - autostart type (0 - no autostart, 1 - autostart)\n"
			L"  -c x         overrides copyprotect settings\n"
			L"     x - copyprotect type (0 - no copyprotect, 1 - copyprotected)\n"
			L"  -o           enable overwrite output file\n"
			L"  -b x         BAS file character encoding\n"
			L"     x - encoding type (a - ANSII, u - UNICODE, 8 - UTF8)\n"
			L"          (default: BAS write=UTF8, BAS read=autodetect)\n"
			L"  -n filename  forces tape file name (stored in tape files)\n"
			L"  -s filename  saves list of file name of the created output files\n"
			L"  -l filename  load input file names from a text file instead of using \n"
			L"               command line parameter\n"
			L"  -p f,l       digital preprocessing parameters (default = 1,1)\n"
			L"     f - digital filter type (0 - no filter, 1 - fast, 2 - strong)\n"
			L"         (default: wavein=fast, wav=strong)\n"
			L"     l - digital level control mode (0 - off, 1 - on)\n"
			L"  -g f,g,l     changes wave generation parameters\n"
			L"     f - frequency offset in percentage\n"
			L"     g - length of the gap in ms between header and data blocks\n"
			L"     l - length of the block leading signal in ms\n"
			L"         (default = 0,1000,4812; fast=50,200,500)\n"
			L"  -w filename  stores preprocessed wave data into the specified wav file\n"
			L"  -1           single bit wave file creation when WAV file output is specified\n"
			L"               or stops wave (WAV file or WaveIn) processing after loading one\n"
			L"               file successfully when wave input is selected\n"
			L"  -e           exclude BASIC lines (saves only binary data behind the Basic\n"
			L"               program storage) when\n"
			L"               saving to HEX or BIN file.\n"
			L"  -m x         sets LOMEM (memory start address of BAS program) value\n"
			L"               for HEX file export\n"
			L"  -u c,b,l,p,s,h Sets serial port (uart) parameters\n"
			L"                 when COM: input/output is selected\n"
			L"     c - com port (1(default),2,3,..)\n"
			L"     b - baud rate (110, 150, 300, 600, 1200(default), 2400, 4800, 9600, 19200 baud)\n"
			L"   	 l - length of data units (5,6,7,8(default) bit)\n"
			L"     p - parity (n - none (default), o - odd, e - even)\n"
			L"     s - stop bit count (1(default) or 2)\n"
			L"     h - handshake (n - none(default), h - hardware)\n"
			L"  -r n - CART ROM loader type (n - 0-1\n"
			L"     0 - Uncompresed program loader (default)\n"
			L"     1 - Uncompresed program loader with border coloring\n"
			L"     2 - ZX7 compressed loader\n"
			L"     3 - ZX7 compressed loader with border coloring\n"
			L"\n"
			L"  - 'file1' and 'file2' can be 'CAS', 'BAS', 'TTP', 'BIN', 'HEX' (Intel),\n"
			L"     ROM (Cart), 'WAV' (PCM), 'WAVE:' (wave in/out device),\n"
			L"     'COM:' (serial port)\n"
			L" If two files are specified then it converts from file1->file2.\n"
			L" If only one file is specified, operation depends on the specified file type:\n"
			L"  CAS: Converts to WAV with the same name\n"
			L"  WAV: Converts to one or more CAS files\n"
			L"  BAS: Converts to CAS file with the same name\n"
			L"  WAVE: Converts signal from WaveIn to one or more CAS files\n"
			L"  COM: Converts data from serial (COM) port to CAS file\n"
			L"\n"
			L" Examples:\n"
			L"  TVCTape WAVE:              Converts tape signal coming from the default\n"
			L"                             WaveIn device to one or more CAS files\n"
			L"  TVCTape WAVE: TEST.CAS     Converts tape signal coming from the default\n"
			L"                             WaveIn device to CAS file\n"
			L"  TVCTape TEST.CAS WAVE:     Converts TEST.CAS file to tape signal and\n"
			L"                             outputs it to the default WaveOut device\n");
	}
}
