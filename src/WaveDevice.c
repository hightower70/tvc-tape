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
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define ERROR_INDEX -1
#define USER_STOP_INDEX -2

///////////////////////////////////////////////////////////////////////////////
// Types

// wave output buffer
typedef struct
{
	bool			Free;
	WAVEHDR		Header;
	BYTE			Buffer[WAVEOUT_BUFFER_LENGTH];
} WaveOutBuffer;

//wave inout buffer
typedef struct
{
	WAVEHDR	Header;
	INT16		Buffer[WAVEIN_BUFFER_LENGTH];
} WaveInBuffer;

///////////////////////////////////////////////////////////////////////////////
// Local funcitons
static void PlayWaveOutBuffer(int in_buffer_index);
static int GetFreeWaveOutBufferIndex(void);

///////////////////////////////////////////////////////////////////////////////
// Module global variables

// waveout variables
static HWAVEOUT l_waveout_handle = NULL;
static HANDLE l_waveout_event = NULL;
static WaveOutBuffer l_waveout_buffer[WAVEOUT_BUFFER_COUNT];
static int l_waveout_buffer_index;
static int l_waveout_buffer_length;

// wavein variables
static HWAVEIN l_wavein_handle = NULL;
static HANDLE l_wavein_event = NULL;
static WaveInBuffer l_wavein_buffer[WAVEIN_BUFFER_COUNT];
static int l_wavein_buffer_index;
static int l_wavein_buffer_pos;

///////////////////////////////////////////////////////////////////////////////
// Global variables
int g_wavein_peak_level;
bool g_wavein_peak_updated;
bool g_cpu_overload;

/*****************************************************************************/
/* Wave input                                                                */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Opens wave input device
bool WDOpenInput(wchar_t* in_file_name)
{
	MMRESULT result;
	WAVEFORMATEX wave_format;
  bool success = true;
	int i;

	// init
	g_cpu_overload= false;
	g_wavein_peak_updated = false;

  // create event
	l_wavein_event = CreateEvent( NULL, true, false, NULL); // create event for sync.
	if(l_wavein_event == NULL)
		return false;

  // prepare for opening
	ZeroMemory( &wave_format, sizeof(wave_format) );

	wave_format.wBitsPerSample		= 16;
	wave_format.wFormatTag				= WAVE_FORMAT_PCM;
	wave_format.nChannels 				= 1;
	wave_format.nSamplesPerSec		= SAMPLE_RATE;
	wave_format.nAvgBytesPerSec		= wave_format.nSamplesPerSec * wave_format.wBitsPerSample / 8;
	wave_format.nBlockAlign 			= wave_format.wBitsPerSample * wave_format.nChannels / 8;

  // open device
  result = waveInOpen( &l_wavein_handle, WAVE_MAPPER, &wave_format, (DWORD)l_wavein_event, 0, CALLBACK_EVENT );
	if( result != MMSYSERR_NOERROR )
		success = false;

	// prepare buffers (except the last one)
	for(i = 0; i < WAVEIN_BUFFER_COUNT && success; i++)
	{
		ZeroMemory( &l_wavein_buffer[i].Header, sizeof( WAVEHDR ) );
		l_wavein_buffer[i].Header.dwBufferLength = sizeof(l_wavein_buffer[i].Buffer);
		l_wavein_buffer[i].Header.lpData         = (LPSTR)(l_wavein_buffer[i].Buffer);

		if( waveInPrepareHeader( l_wavein_handle, &l_wavein_buffer[i].Header, sizeof( WAVEHDR ) ) != MMSYSERR_NOERROR )
			success = false;

		if( success && i < (WAVEIN_BUFFER_COUNT - 1) && waveInAddBuffer( l_wavein_handle, &l_wavein_buffer[i].Header, sizeof( WAVEHDR ) ) != MMSYSERR_NOERROR )
			success = false;
	}
	l_wavein_buffer_pos = WAVEIN_BUFFER_LENGTH;
	l_wavein_buffer_index = WAVEIN_BUFFER_COUNT - 1;

	ResetEvent(l_wavein_event);

	// start device
	if( success && waveInStart( l_wavein_handle ) != MMSYSERR_NOERROR )
		success = false;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Reads sample
bool WDReadSample(INT32* out_sample)
{
//	wchar_t buffer[100];
	int peak_value;
	int finished_buffer_count;
	int i;

	if(l_wavein_buffer_pos >= WAVEIN_BUFFER_LENGTH)
	{
		// check for cancel key
		SHORT s = GetAsyncKeyState(STOP_KEY);
		if( s != 0)
			return false;

		// calculate the number of the finished buffers
		finished_buffer_count = 0;
		for(i=0; i<WAVEIN_BUFFER_COUNT; i++)
		{
			if((l_wavein_buffer[l_wavein_buffer_index].Header.dwFlags & WHDR_DONE) != 0)
				finished_buffer_count++;
		}

		// determine CPU overload condition
		if(finished_buffer_count == WAVEIN_BUFFER_COUNT-1)
			g_cpu_overload = true;

		// give back buffer to the driver
		waveInAddBuffer(l_wavein_handle, &l_wavein_buffer[l_wavein_buffer_index].Header, sizeof(WAVEHDR));

		// next buffer
		l_wavein_buffer_index++;
		if(l_wavein_buffer_index >= WAVEIN_BUFFER_COUNT)
		{
			l_wavein_buffer_index = 0;
		}
		l_wavein_buffer_pos = 0;

		// check if buffer is ready
		if((l_wavein_buffer[l_wavein_buffer_index].Header.dwFlags & WHDR_DONE) == 0)
		{
			// wait while buffer is not ready
			if(WaitForSingleObject(l_wavein_event, INFINITE) == WAIT_OBJECT_0)
			{
				// buffer should be full now
				ResetEvent(l_wavein_event);
			}
			else
				return false;

			//if((l_wavein_buffer[l_wavein_buffer_index].Header.dwFlags & WHDR_DONE) == 0)
			//	return  false;
		}

		//wsprintf(buffer, L"%d, ", l_wavein_buffer_index);
		//OutputDebugString(buffer);

		// calculate peak level
		peak_value = 0;
		for(i=0; i < WAVEIN_BUFFER_LENGTH; i++)
		{
			if(l_wavein_buffer[l_wavein_buffer_index].Buffer[i] > 0 )
			{
				if(l_wavein_buffer[l_wavein_buffer_index].Buffer[i] > peak_value)
					peak_value = l_wavein_buffer[l_wavein_buffer_index].Buffer[i];
			}
			else
			{
				if(-l_wavein_buffer[l_wavein_buffer_index].Buffer[i] > peak_value)
					peak_value = -l_wavein_buffer[l_wavein_buffer_index].Buffer[i];
			}
		}

		g_wavein_peak_level = peak_value;
		g_wavein_peak_updated = true;
	}

	// return next sample from the current buffer
	*out_sample = l_wavein_buffer[l_wavein_buffer_index].Buffer[l_wavein_buffer_pos];
	l_wavein_buffer_pos++;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave input
void WDCloseInput(void)
{
	int i;

	// close wave out device
	if(l_wavein_handle != NULL)
	{
		// stop wave in
		waveInReset(l_wavein_handle);
	
		// cleanup buffers
		for(i = 0; i < WAVEIN_BUFFER_COUNT; i++)
			waveInUnprepareHeader(l_wavein_handle, &l_wavein_buffer[i].Header, sizeof( WAVEHDR )); 

		// close
		waveInClose(l_wavein_handle);

		l_wavein_handle = NULL;
	}

	if(l_wavein_event != NULL)
	{
		CloseHandle(l_wavein_event);
		l_wavein_event = NULL;
	}
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
	l_waveout_event = CreateEvent( NULL, true, false, NULL); // create event for sync.
	if(l_waveout_event == NULL)
		return false;

	ResetEvent(l_waveout_event);

  // prepare for opening
	ZeroMemory( &wave_format, sizeof(wave_format) );

	wave_format.wBitsPerSample		= 8;
	wave_format.wFormatTag				= WAVE_FORMAT_PCM;
	wave_format.nChannels 				= 1;
	wave_format.nSamplesPerSec		= SAMPLE_RATE;
	wave_format.nAvgBytesPerSec		= wave_format.nSamplesPerSec * wave_format.wBitsPerSample / 8;
	wave_format.nBlockAlign 			= wave_format.wBitsPerSample * wave_format.nChannels / 8;

  // open device
  result = waveOutOpen( &l_waveout_handle, WAVE_MAPPER, &wave_format, (DWORD)l_waveout_event, 0, CALLBACK_EVENT );
	if( result != MMSYSERR_NOERROR )
		success = false;

  // prepare buffers
	if(success)
	{
		for(i = 0; i < WAVEOUT_BUFFER_COUNT; i++)
		{
			ZeroMemory( &l_waveout_buffer[i].Header, sizeof( WAVEHDR ) );
			l_waveout_buffer[i].Header.dwBufferLength = sizeof(l_waveout_buffer[i].Buffer);
			l_waveout_buffer[i].Header.lpData         = (LPSTR)(l_waveout_buffer[i].Buffer);
			l_waveout_buffer[i].Header.dwFlags        = 0;
			l_waveout_buffer[i].Free                  = true;
		}
	}

	// init
	l_waveout_buffer_index = 0;
	l_waveout_buffer_length = 0;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Writes sample to the wave output device
bool WDWriteSample(BYTE in_sample)
{
	bool success = true;

	// get new free buffer
	if(l_waveout_buffer_index < 0)
	{
		l_waveout_buffer_index = GetFreeWaveOutBufferIndex();
		l_waveout_buffer_length = 0;

		if(l_waveout_buffer_index < 0)
			return false;
	}

	// store sample
	l_waveout_buffer[l_waveout_buffer_index].Buffer[l_waveout_buffer_length] = in_sample;

	// if the buffer is full add to the playback queue
	l_waveout_buffer_length++;
	if(l_waveout_buffer_length >=	WAVEOUT_BUFFER_LENGTH)
	{
		PlayWaveOutBuffer(l_waveout_buffer_index);

		l_waveout_buffer_index = -1;
		l_waveout_buffer_length = 0;
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
		if(l_waveout_buffer_index >= 0 && l_waveout_buffer_length != 0)
		{
			while(l_waveout_buffer_length < WAVEOUT_BUFFER_LENGTH)
				l_waveout_buffer[l_waveout_buffer_index].Buffer[l_waveout_buffer_length++] = 0x80;
	
			PlayWaveOutBuffer(l_waveout_buffer_index);

			l_waveout_buffer_index = -1;
			l_waveout_buffer_length = 0;
		}

		// wait until buffers are free
		do
		{
			// check if any buffer is still used
			finished = true;
			for(i = 0; i < WAVEOUT_BUFFER_COUNT && finished; i++)
			{
				if(!l_waveout_buffer[i].Free)
					finished = false;
			}

			// at least one buffer is used -> wait
			if(!finished)
			{
				if(WaitForSingleObject(l_waveout_event, INFINITE) == WAIT_OBJECT_0)
				{
					ResetEvent(l_waveout_event);

					// find freed buffer
					for(i = 0; i < WAVEOUT_BUFFER_COUNT; i++)
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

	if(l_waveout_event != NULL)
	{
		CloseHandle(l_waveout_event);
		l_waveout_event = NULL;
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
		for(i = 0; i < WAVEOUT_BUFFER_COUNT; i++)
		{
			if(l_waveout_buffer[i].Free)
			{
				buffer_index = i;
				break;
			}
		}

		// there is no free buffer, wait until one buffer is finished
		if( buffer_index < 0 && WaitForSingleObject(l_waveout_event, INFINITE) == WAIT_OBJECT_0 )
		{
			ResetEvent(l_waveout_event);

			// find freed buffer
			for(i = 0; i < WAVEOUT_BUFFER_COUNT; i++)
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

#else

///////////////////////////////////////////////////////////////////////////////
// Opens wave output device
bool WDOpenOutput(wchar_t* in_file_name)
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
void WDCloseOutput(bool in_force_close)
{
}

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

#endif

