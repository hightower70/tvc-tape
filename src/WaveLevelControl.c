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
#define INPUT_SAMPLE_AMPLITUDE 32000
#define TARGET_SAMPLE_AMPLITUDE 32000
#define HIGH_THRESHOLD 4096
#define LOW_THRESHOLD 2
#define NOISE_KILLER_SILENCE_MAX_LENGTH 5
//#define DEBUG_CSV

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
static int32_t l_envelope[LOOK_AHEAD_BUFFER_LENGTH];
static int32_t l_look_ahead_buffer[LOOK_AHEAD_BUFFER_LENGTH];
static uint8_t l_look_ahead_buffer_index = 0;
static uint8_t l_last_peak_index;
static SignalSlopeType l_prev_slope;
static int32_t l_prev_sample;
static int32_t l_peak_threshold = 10;
static WaveLevelControlModeType l_mode = WLCMT_NoiseKiller;
static int32_t l_noise_killer_silence_length = 0;

#ifdef DEBUG_CSV
static int l_sample_counter;
static FILE* l_debug_output;
#endif

///////////////////////////////////////////////////////////////////////////////
// Global variables
uint8_t g_wave_level_control_mode = 1;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static void DetectEnvelope(int32_t in_sample);
static void CalculateEnvelope(int32_t in_peak_level, uint8_t in_peak_level_index);
static int32_t SampleABS(int32_t in_value);

///////////////////////////////////////////////////////////////////////////////
// Initializes level control system
void WLCInit(void)
{
	uint8_t i;

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
	l_debug_output = _wfopen("debug.csv", "w+t");
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Process sampke (apply envelope control)
int32_t WLCProcessSample(int32_t in_sample)
{
	// get old sample
	int32_t sample;

	if(g_wave_level_control_mode != 1)
		return in_sample;

#ifdef DEBUG_CSV
	wchar_t buffer[1024];

	l_sample_counter++;
#endif

	// apply gain
	sample = l_look_ahead_buffer[l_look_ahead_buffer_index];
	if(l_envelope[l_look_ahead_buffer_index] == 0)
		sample = 0;
	else
		sample = (int16_t)((int64_t)sample * TARGET_SAMPLE_AMPLITUDE / l_envelope[l_look_ahead_buffer_index]);

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
static void DetectEnvelope(int32_t in_sample)
{
	uint8_t next_look_ahead_buffer_index;
	SignalSlopeType actual_slope;
	uint8_t peak_index;
	int32_t peak_sample;
	bool peak_found = false;

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

	// check for positive peak
	if(l_prev_sample > 0 && l_prev_slope == SST_Rising && actual_slope == SST_Falling && l_prev_sample > l_peak_threshold)
	{
		peak_found = true;
		peak_sample = l_prev_sample;
	}

	// check for negative peak
	if(l_prev_sample < 0 && l_prev_slope == SST_Falling && actual_slope == SST_Rising && -l_prev_sample > l_peak_threshold)
	{
		peak_found = true;
		peak_sample = -l_prev_sample;
	}

	// check operation mode
	switch (l_mode)
	{
		// automatic level control mode
		case WLCMT_LevelControl:
			if(peak_found)
			{
				// generate envelope
				CalculateEnvelope(peak_sample, peak_index);

				// store peak position
				l_last_peak_index = peak_index;
			}
			break;

		// noise killer mode (no level control only cut below a certain threshold)
		case WLCMT_NoiseKiller:
		{
			if(peak_found)
			{
				if(peak_sample < l_peak_threshold)
					peak_sample = 0;
				else
				{
					if(peak_sample < INPUT_SAMPLE_AMPLITUDE)
						peak_sample = INPUT_SAMPLE_AMPLITUDE;

					l_noise_killer_silence_length = 0;
				}

				CalculateEnvelope(peak_sample, peak_index);
				l_last_peak_index = peak_index;
			}
			else
			{
				if( SampleABS(in_sample) < l_peak_threshold)
				{
					l_noise_killer_silence_length++;
					if(l_noise_killer_silence_length == NOISE_KILLER_SILENCE_MAX_LENGTH)
					{
						CalculateEnvelope(0, peak_index);
						l_last_peak_index = peak_index;
					}

					if(l_noise_killer_silence_length > NOISE_KILLER_SILENCE_MAX_LENGTH)
					{
						l_envelope[peak_index] = 0;
						l_last_peak_index = peak_index;
					}
				}
			}
		}
		break;
	}

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
static void CalculateEnvelope(int32_t in_peak_level, uint8_t in_peak_level_index)
{
	uint8_t envelope_step_count;
	uint8_t envelope_step_index;
	uint8_t envelope_index;
	int32_t start_peak_level;
	int32_t envelope;
	int32_t sample_abs;

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
		envelope = (int32_t)start_peak_level + (in_peak_level - start_peak_level) * envelope_step_index / envelope_step_count;
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
static int32_t SampleABS(int32_t in_value)
{
	if( in_value>=0 )
		return in_value;
	else
		return -in_value;
}
