/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave input/output mapping (file vs. wave in/out device)                   */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Main.h"
#include "WaveMapper.h"
#include "WaveDevice.h"
#include "WaveFile.h"

/*****************************************************************************/
/* Wave input                                                                */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Opens wave input
bool WMOpenInput(wchar_t* in_file_name)
{
	switch(g_input_file_type)
	{
		// open wave input
		case FT_WaveInOut:
			return WDOpenInput(in_file_name);

		// open wave file
		case FT_WAV:
			return WFOpenInput(in_file_name);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Reads next sample
bool WMReadSample(int32_t* out_sample)
{
	switch(g_input_file_type)
	{
		// open wave input
		case FT_WaveInOut:
			return WDReadSample(out_sample);

		// open wave file
		case FT_WAV:
			return WFReadSample(out_sample);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave input
void WMCloseInput(void)
{
	switch(g_input_file_type)
	{
		// open wave input
		case FT_WaveInOut:
			WDCloseInput();
			break;

		// open wave file
		case FT_WAV:
			WFCloseInput();
			break;
	}
}

/*****************************************************************************/
/* Wave output                                                               */
/*****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Opens wave output
bool WMOpenOutput(wchar_t* in_file_name)
{
	switch(g_output_file_type)
	{
		// open wave output
		case FT_WaveInOut:
			return WDOpenOutput(in_file_name);

		// create wave file
		case FT_WAV:
			return WFOpenOutput(in_file_name, g_one_bit_wave_file ? 1 : 8);

		default:
			return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Write output sample
bool WMWriteSample(uint8_t in_sample)
{
	switch(g_output_file_type)
	{
		// open wave output
		case FT_WaveInOut:
			return WDWriteSample(in_sample);

		// create wave file
		case FT_WAV:
			WFWriteSample(in_sample);
			return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave output
void WMCloseOutput(bool in_force_close)
{
	switch(g_output_file_type)
	{
		// open wave output
		case FT_WaveInOut:
			WDCloseOutput(in_force_close);
			break;

		// create wave file
		case FT_WAV:
			WFCloseOutput(in_force_close);
			break;
	}
}
