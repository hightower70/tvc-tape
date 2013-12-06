/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave Level Control (Automatic)                                            */
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
#include "WaveLevelControl.h"

///////////////////////////////////////////////////////////////////////////////
// Consts
#define	LOOK_AHEAD_BUFFER_LENGTH  64
#define INPUT_SAMPLE_AMPLITUDE 37500 // it should be 32768 but the digital filter slightly alters the amplitude
#define TARGET_SAMPLE_AMPLITUDE 120
#define HIGH_THRESHOLD 4096
#define LOW_THRESHOLD 2
//#define DEBUG_CSV
#define NOISE_KILLER_SILENCE_MAX_LENGTH 5

///////////////////////////////////////////////////////////////////////////////
// Types
typedef enum 
{
	SST_Unknown,
	SST_Rising,
	SST_Falling,
	SST_Constant
} SignalSlopeType;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static INT32 l_envelope[LOOK_AHEAD_BUFFER_LENGTH];
static INT32 l_look_ahead_buffer[LOOK_AHEAD_BUFFER_LENGTH];
static BYTE l_look_ahead_buffer_index = 0;
static BYTE l_last_peak_index;
static SignalSlopeType l_prev_slope;
static INT32 l_prev_sample;
static INT32 l_peak_threshold = 10;
static WaveLevelControlModeType l_mode = WLCMT_NoiseKiller;
static INT32 l_noise_killer_silence_length = 0;

#ifdef DEBUG_CSV
static int l_sample_counter;
static FILE* l_debug_output;
#endif

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static void DetectEnvelope(INT32 in_sample);
static void CalculateEnvelope(INT32 in_peak_level, BYTE in_peak_level_index);
static INT32 SampleABS(INT32 in_value);

///////////////////////////////////////////////////////////////////////////////
// Initializes level control system
void WLCInit(void)
{
	BYTE i;

	// initialize
	for(i = 0;i < LOOK_AHEAD_BUFFER_LENGTH; i++)
	{
		l_look_ahead_buffer[i] = 0;
	}

	l_look_ahead_buffer_index = 0;
	l_last_peak_index = 0;

	l_prev_slope = SST_Unknown;
	l_prev_sample = 0;

	l_mode = WLCMT_NoiseKiller;

#ifdef DEBUG_CSV	
	l_sample_counter = 0;
	l_debug_output = fopen("debug.csv", "w+t");
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Process sampke (apply envelope control)
INT32 WLCProcessSample(INT32 in_sample)
{
	// get old sample
	INT32 sample;

#ifdef DEBUG_CSV
	char buffer[1024];

	l_sample_counter++;
#endif

	// apply gain
	sample = l_look_ahead_buffer[l_look_ahead_buffer_index];
	if(l_envelope[l_look_ahead_buffer_index] == 0)
		sample = 0;
	else
		sample = (INT16)((INT64)sample * TARGET_SAMPLE_AMPLITUDE / l_envelope[l_look_ahead_buffer_index]);

#ifdef DEBUG_CSV
	sprintf(buffer,"%d;%d\n",l_look_ahead_buffer[l_look_ahead_buffer_index], l_envelope[l_look_ahead_buffer_index]);
	fputs(buffer,l_debug_output);
#endif

	// deteck and update peak value and sample gain multiplier
	DetectEnvelope(in_sample);

	// handle look ahead buffer
	l_look_ahead_buffer[l_look_ahead_buffer_index] = in_sample;

	if(l_look_ahead_buffer_index >= LOOK_AHEAD_BUFFER_LENGTH - 1)
	{
		l_look_ahead_buffer_index = 0;
	}
	else
	{
		l_look_ahead_buffer_index++;
	}

	return sample;
}

///////////////////////////////////////////////////////////////////////////////
// Closes level control
void WLCClose(void)
{
#ifdef DEBUG_CSV
	if(l_debug_output!=NULL)
		fclose(l_debug_output);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Sets wave level control mode
void WLCSetMode(WaveLevelControlModeType in_mode)
{
	// store mode
	l_mode = in_mode;

	// update peak threshold
	switch (in_mode)
	{
		case WLCMT_LevelControl:
			l_peak_threshold = LOW_THRESHOLD;
			break;

		case WLCMT_NoiseKiller:
			l_peak_threshold = HIGH_THRESHOLD;
			break;
	}
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Detect envelope corner points
static void DetectEnvelope(INT32 in_sample)
{
	BYTE next_look_ahead_buffer_index;
	SignalSlopeType actual_slope;
	BYTE peak_index;

	// peak element index calculation
	if(l_look_ahead_buffer_index == 0)
		peak_index = LOOK_AHEAD_BUFFER_LENGTH - 1;
	else
		peak_index = l_look_ahead_buffer_index - 1;

	// current slope
	actual_slope = SST_Constant;
	if(l_prev_sample > in_sample)
	{
		actual_slope = SST_Falling;
	}
	else
	{
		if(l_prev_sample < in_sample)
		{
			actual_slope = SST_Rising;
		}
	}

#if 0
	// check for positive peak
	if(l_prev_sample > 0 && l_prev_slope == SST_Rising && actual_slope == SST_Falling && l_prev_sample > l_peak_threshold)
	{
		if(l_mode == WLCMT_NoiseKiller
		// generate envelope
		CalculateEnvelope(l_prev_sample, peak_index);

		// store peak position
		l_last_peak_index = peak_index;
	}

	// check for negative peak
	if(l_prev_sample < 0 && l_prev_slope == SST_Falling && actual_slope == SST_Rising && -l_prev_sample > l_peak_threshold)
	{
		// generate envelope
		CalculateEnvelope(-l_prev_sample, peak_index);

		// store peak position
		l_last_peak_index = peak_index;
	}

#endif
#if 1
	// check operation mode
	switch (l_mode)
	{
		// automatic level control mode
		case WLCMT_LevelControl:
		{
			// check for positive peak
			if(l_prev_sample > 0 && l_prev_slope == SST_Rising && actual_slope == SST_Falling && l_prev_sample > l_peak_threshold)
			{
				// generate envelope
				CalculateEnvelope(l_prev_sample, peak_index);

				// store peak position
				l_last_peak_index = peak_index;
			}

			// check for negative peak
			if(l_prev_sample < 0 && l_prev_slope == SST_Falling && actual_slope == SST_Rising && -l_prev_sample > l_peak_threshold)
			{
				// generate envelope
				CalculateEnvelope(-l_prev_sample, peak_index);

				// store peak position
				l_last_peak_index = peak_index;
			}
		}
		break;

		// noise killer mode (no level control only cut below a certain threshold)
		case WLCMT_NoiseKiller:
		{
			if( SampleABS(in_sample) > l_peak_threshold)
			{
				l_envelope[peak_index] = INPUT_SAMPLE_AMPLITUDE ;
				l_noise_killer_silence_length = 0;
			}
			else
			{
				l_noise_killer_silence_length++;
				if(l_noise_killer_silence_length > NOISE_KILLER_SILENCE_MAX_LENGTH)
					l_envelope[peak_index] = 0;
				else
					l_envelope[peak_index] = INPUT_SAMPLE_AMPLITUDE;
			}

			l_last_peak_index = peak_index;
		}
		break;
	}
#endif

	// calculate next index in the look ahead buffer
	next_look_ahead_buffer_index = l_look_ahead_buffer_index + 1;
	if(next_look_ahead_buffer_index >= LOOK_AHEAD_BUFFER_LENGTH)
		next_look_ahead_buffer_index = 0;

	// if there was no peak	for a while
	if(l_last_peak_index == next_look_ahead_buffer_index)
	{
		// set gain to zero
		CalculateEnvelope(0, peak_index);

		// store peak position
		l_last_peak_index = peak_index;
	}

	// update prev state
	l_prev_sample = in_sample;
	if(actual_slope == SST_Rising || actual_slope == SST_Falling)
		l_prev_slope = actual_slope;
}

///////////////////////////////////////////////////////////////////////////////
// Generates envelope values using linear interpolation between corner points
static void CalculateEnvelope(INT32 in_peak_level, BYTE in_peak_level_index)
{
	BYTE envelope_step_count;
	BYTE envelope_step_index;
	BYTE envelope_index;
	INT32 start_peak_level;
	INT32 envelope;
	INT32 sample_abs;

	// number of envelope steps
	if(l_last_peak_index < in_peak_level_index)
		envelope_step_count = in_peak_level_index - l_last_peak_index;
	else
		envelope_step_count = LOOK_AHEAD_BUFFER_LENGTH - l_last_peak_index + in_peak_level_index;

	// calculate envelope values (envelope ramp)
	envelope_index = l_last_peak_index;
	envelope_step_index = 0;
	start_peak_level = l_envelope[l_last_peak_index];
	if(start_peak_level == 0)
		start_peak_level = 32767;

	while(envelope_index != in_peak_level_index)
	{
		envelope = (INT32)start_peak_level + (in_peak_level - start_peak_level) * envelope_step_index / envelope_step_count;
		sample_abs = SampleABS(l_look_ahead_buffer[envelope_index]);

		if(sample_abs > l_peak_threshold && envelope < sample_abs)
			envelope = sample_abs;

		l_envelope[envelope_index] = envelope;

		envelope_index++;
		if(envelope_index >= LOOK_AHEAD_BUFFER_LENGTH)
			envelope_index = 0;

		envelope_step_index++;
	}

	l_envelope[envelope_index] = in_peak_level;
}

///////////////////////////////////////////////////////////////////////////////
// Calculates absolute value
static INT32 SampleABS(INT32 in_value)
{
	if( in_value>=0 )
		return in_value;
	else
		return -in_value;
}
