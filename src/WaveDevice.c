/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave Device (In/Out) handling                                             */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#include "WaveDevice.h"

#ifdef ENABLE_WAVE_DEVICES

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Windows.h>
#include "WaveMapper.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define STOP_KEY VK_ESCAPE
#define ERROR_INDEX -1
#define USER_STOP_INDEX -2

///////////////////////////////////////////////////////////////////////////////
// Types

// wave output buffer
typedef struct
{
	bool			Free;
	WAVEHDR		Header;
	BYTE			Buffer[WAVE_OUT_BUFFER_LENGTH];
} WaveOutBuffer;

///////////////////////////////////////////////////////////////////////////////
// Local funcitons
static void PlayWaveOutBuffer(int in_buffer_index);
static int GetFreeWaveOutBufferIndex(void);

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static HWAVEOUT l_waveout_handle = NULL;
static HANDLE l_wave_out_event = NULL;
static WaveOutBuffer l_waveout_buffer[WAVE_OUT_BUFFER_COUNT];
static int l_current_waveout_buffer_index;
static int l_current_waveout_buffer_length;

/*****************************************************************************/
/* Wave input                                                                */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Opens wave input device
bool WDOpenInput(wchar_t* in_file_name)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Reads sample
bool WDReadSample(INT32* out_sample)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave input
void WDCloseInput(void)
{
}

/*****************************************************************************/
/* Wave output                                                               */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Opens wave output device
bool WDOpenOutput(wchar_t* in_file_name)
{
	MMRESULT result;
  WAVEFORMATEX wave_format;
  bool success = true;
	int i;

  // create event
	l_wave_out_event = CreateEvent( NULL, true, false, NULL); // create event for sync.
	ResetEvent(l_wave_out_event);

  // prepare for opening
	ZeroMemory( &wave_format, sizeof(wave_format) );

	wave_format.wBitsPerSample		= 8;
	wave_format.wFormatTag				= WAVE_FORMAT_PCM;
	wave_format.nChannels 				= 1;
	wave_format.nSamplesPerSec		= SAMPLE_RATE;
	wave_format.nAvgBytesPerSec		= wave_format.nSamplesPerSec * wave_format.wBitsPerSample / 8;
	wave_format.nBlockAlign 			= wave_format.wBitsPerSample * wave_format.nChannels / 8;

  // open device
  result = waveOutOpen( &l_waveout_handle, WAVE_MAPPER, &wave_format, (DWORD)l_wave_out_event, 0, CALLBACK_EVENT );
	if( result != MMSYSERR_NOERROR )
		success = false;

  // prepare buffers
	for(i = 0; i < WAVE_OUT_BUFFER_COUNT; i++)
	{
		ZeroMemory( &l_waveout_buffer[i].Header, sizeof( WAVEHDR ) );
		l_waveout_buffer[i].Header.dwBufferLength = WAVE_OUT_BUFFER_LENGTH;
		l_waveout_buffer[i].Header.lpData         = (LPSTR)(l_waveout_buffer[i].Buffer);
		l_waveout_buffer[i].Header.dwFlags        = 0;
		l_waveout_buffer[i].Free                  = true;
	}

	// init
	l_current_waveout_buffer_index = 0;
	l_current_waveout_buffer_length = 0;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Writes sample to the wave output device
bool WDWriteSample(BYTE in_sample)
{
	bool success = true;

	// get new free buffer
	if(l_current_waveout_buffer_index < 0)
	{
		l_current_waveout_buffer_index = GetFreeWaveOutBufferIndex();
		l_current_waveout_buffer_length = 0;

		if(l_current_waveout_buffer_index < 0)
			return false;
	}

	// store sample
	l_waveout_buffer[l_current_waveout_buffer_index].Buffer[l_current_waveout_buffer_length] = in_sample;

	// if the buffer is full add to the playback queue
	l_current_waveout_buffer_length++;
	if(l_current_waveout_buffer_length >=	WAVE_OUT_BUFFER_LENGTH)
	{
		PlayWaveOutBuffer(l_current_waveout_buffer_index);

		l_current_waveout_buffer_index = -1;
		l_current_waveout_buffer_length = 0;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave output devoce
void WDCloseOutput(bool in_force_close)
{
	int i;
	bool finished;

	if(!in_force_close)
	{
		// send remaining data
		if(l_current_waveout_buffer_index >= 0 && l_current_waveout_buffer_length != 0)
		{
			while(l_current_waveout_buffer_length < WAVE_OUT_BUFFER_LENGTH)
				l_waveout_buffer[l_current_waveout_buffer_index].Buffer[l_current_waveout_buffer_length++] = 0x80;
	
			PlayWaveOutBuffer(l_current_waveout_buffer_index);

			l_current_waveout_buffer_index = -1;
			l_current_waveout_buffer_length = 0;
		}

		// wait until buffers are free
		do
		{
			// check if any buffer is still used
			finished = true;
			for(i = 0; i < WAVE_OUT_BUFFER_COUNT && finished; i++)
			{
				if(!l_waveout_buffer[i].Free)
					finished = false;
			}

			// at least one buffer is used -> wait
			if(!finished)
			{
				if(WaitForSingleObject(l_wave_out_event, INFINITE) == WAIT_OBJECT_0)
				{
					ResetEvent(l_wave_out_event);

					// find freed buffer
					for(i = 0; i < WAVE_OUT_BUFFER_COUNT; i++)
					{
						if(!l_waveout_buffer[i].Free && (l_waveout_buffer[i].Header.dwFlags & WHDR_DONE) != 0)
						{
							// release header
							waveOutUnprepareHeader(l_waveout_handle, &l_waveout_buffer[i].Header, sizeof(WAVEHDR));
							l_waveout_buffer[i].Free = true;
							l_waveout_buffer[i].Header.dwFlags = 0;
						}
					}
				}
			}
		}	while(!finished);
	}

	// close wave out device
	if(l_waveout_handle != NULL)
	{
		waveOutReset(l_waveout_handle);
		waveOutClose(l_waveout_handle);
		l_waveout_handle = NULL;
	}

	if(l_wave_out_event != NULL)
	{
		CloseHandle(l_wave_out_event);
		l_wave_out_event = NULL;
	}
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Adds the specified buffer to the playback queue
static void PlayWaveOutBuffer(int in_buffer_index)
{
	// flag header
	l_waveout_buffer[in_buffer_index].Free = false;

  // prepare header
	waveOutPrepareHeader(l_waveout_handle, &l_waveout_buffer[in_buffer_index].Header, sizeof(WAVEHDR));

	// write header
	waveOutWrite(l_waveout_handle, &l_waveout_buffer[in_buffer_index].Header, sizeof(WAVEHDR));
}

///////////////////////////////////////////////////////////////////////////////
// Gets next free buffer inedx
static int GetFreeWaveOutBufferIndex(void)
{
	int buffer_index = -1;
	int i;

	// check for stop key
	SHORT s = GetAsyncKeyState(STOP_KEY);
	if( s != 0)
		return USER_STOP_INDEX;

	//get buffer
	do
	{
		// check for free buffer
		for(i = 0; i < WAVE_OUT_BUFFER_COUNT; i++)
		{
			if(l_waveout_buffer[i].Free)
			{
				buffer_index = i;
				break;
			}
		}

		// there is no free buffer, wait until one buffer is finished
		if( buffer_index < 0 && WaitForSingleObject(l_wave_out_event, INFINITE) == WAIT_OBJECT_0 )
		{
			ResetEvent(l_wave_out_event);

			// find freed buffer
			for(i = 0; i < WAVE_OUT_BUFFER_COUNT; i++)
			{
				if(!l_waveout_buffer[i].Free && (l_waveout_buffer[i].Header.dwFlags & WHDR_DONE) != 0)
				{
					// release header
					waveOutUnprepareHeader( l_waveout_handle, &l_waveout_buffer[i].Header, sizeof(WAVEHDR));
					l_waveout_buffer[i].Free = true;
					l_waveout_buffer[i].Header.dwFlags = 0;
				}
			}
		}
	}	while(buffer_index < 0);

	return buffer_index;
}

/*
	wchar_t buffer[10];

	wsprintf(buffer,L"%d, ",in_buffer_index);
	OutputDebugString(buffer);
*/

#else

///////////////////////////////////////////////////////////////////////////////
// Opens wave output device
bool WDOpenOutput(void)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Writes sample to the wave output device
int WDWriteSample(BYTE in_sample)
{
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave output devoce
void WDCloseInput(void)
{
}


#endif

