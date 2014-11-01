/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Tape file format and FSK (Frequency Shift Keying) modulation handler      */
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
#include "DDS.h"
#include "CRC.h"
#include "TAPEFile.h"
#include "WaveDevice.h"
#include "CASFile.h"
#include "WaveMapper.h"
#include "WaveFile.h"
#include "WaveFilter.h"
#include "WaveLevelControl.h"
#include "Main.h"
#include "CharMap.h"
#include "DataBuffer.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define MIDDLE_PERIOD_BUFFER_LENGTH 256
#define SYNC_PHASE_MAX_DIFFERENCE 2
#define SECTOR_END_PERIOD_COUNT 5

///////////////////////////////////////////////////////////////////////////////
// Types

// Current state of the decoder
typedef  enum
{
	DST_Idle,
	DST_WaitingForLeading,
	DST_WaitingForSync,
	DST_SyncDetected,
	DST_ReadingData,
	DST_SectorEnd
}	DecoderStateType;

// Current state of the tape reader
typedef enum
{
	TRST_Idle,
	TRST_BlockHeader,
	TRST_SectorHeader,
	TRST_HeaderFileNameLength,
	TRST_HeaderFileName,
	TRST_HeaderProgramHeader,
	TRST_SectorEnd,
	TRST_Data
} TapeReaderStatusType;

// Signal phase type
typedef enum
{
	SPT_High,
	SPT_Low
} SignalPhaseType;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static bool EncodeByte(BYTE in_data);
static bool EncodeBlockLeading(void);
static bool EncodeBlock(BYTE* in_buffer, int in_length);
static void DisplayOutputHeaderProgress(int in_pos, int in_max_pos);
static void DisplayOutputDataProgress(int in_pos, int in_max_pos);
static void DisplayInputDataProgress(void);
static void DisplayFailedToLoad(void);
static LoadStatus DecodeSample(INT32 in_sample);
static LoadStatus DecoderRestart(void);
static void UpdateMiddleFrequency(DWORD in_frequency, DWORD in_measured_period_length);
static LoadStatus StoreByte(BYTE in_data_byte);
static int IntABS(int in_value);
static bool StoreByteInStruct(BYTE in_data_byte, void* in_struct, size_t in_size, bool in_add_to_crc);
static void SetSectorLength(BYTE in_sector_length);
static void ChangeReaderStatus(TapeReaderStatusType in_new_status);
static WORD OffsetFrequency(WORD in_frequency);

///////////////////////////////////////////////////////////////////////////////
// Module global variables

// encoder variables
static BYTE l_prev_input_percentage;
static DWORD l_prev_input_total_seconds;

// decoder variables
static BYTE l_data_byte;
static DWORD l_data_byte_index;
static BYTE l_bit_counter;
static DecoderStateType l_decoder_state = DST_Idle;
static TapeReaderStatusType l_tape_reader_status = TRST_Idle;
static SignalPhaseType l_current_phase = SPT_Low;
static SignalPhaseType l_phase_mode = SPT_Low;

static INT32 l_previous_sample;
static int l_period_high_length;
static int l_period_low_length;
static int l_sync_first_half_period_length;
static int l_sync_second_half_period_length;

static int l_middle_period_buffer[MIDDLE_PERIOD_BUFFER_LENGTH];	// buffer used for middle frequency (period) avg. calculation
static DWORD l_middle_period_buffer_index;
static WORD l_middle_period;
static DWORD l_middle_period_sum;

static TAPEBlockHeaderType l_block_header;
static TAPESectorHeaderType l_sector_header;
static BYTE l_file_name_length;
static CASProgramFileHeaderType l_program_header;
static TAPESectorEndType l_sector_end;
static WORD l_current_sector_length;
static WORD l_sector_end_period_count;
static DWORD l_sync_period_length;
static bool l_header_block_valid;

///////////////////////////////////////////////////////////////////////////////
// Global variables

static int l_demod_sample_index = 0;  // for debugging only (will be removed)

WORD g_frequency_offset = 0;
WORD g_leading_length = 4812;
WORD g_gap_length = 1000;

///////////////////////////////////////////////////////////////////////////////
// Initialization of tape functions
bool TAPEOpenInput(wchar_t* in_file_name)
{
	l_prev_input_percentage = 0xff;
	l_prev_input_total_seconds = 0xffffffff;

	WLCInit();
	WLCSetMode(WLCMT_NoiseKiller);

	l_decoder_state = DST_Idle;
	l_period_high_length = 0;
	l_period_low_length = 0;
	l_header_block_valid = false;
	l_previous_sample = 0;

	return WMOpenInput(in_file_name);
}

///////////////////////////////////////////////////////////////////////////////
// TAPE Load
LoadStatus TAPELoad(void)
{
	bool success = true;
	INT32	sample;
	LoadStatus load_status = LS_Unknown;

	// scan for files
	while(load_status == LS_Unknown)
	{
		success = WMReadSample(&sample);
	 
		if(success)
		{
			sample = WFProcessSample(sample);			// Digital filter
			sample = WLCProcessSample(sample);		// Amplitude controller							
			load_status = DecodeSample(sample);		// Decoder

			WFWriteSample(sample);

			switch(load_status)
			{
				case LS_Unknown:
					DisplayInputDataProgress();
					break;

				case LS_Error:
					DisplayFailedToLoad();
					break;

				case LS_Success:
					DisplayMessage(L"\r");
					break;
			}
		}
		else
		{
			load_status = LS_Fatal;
		}
	}

	return load_status;
}

///////////////////////////////////////////////////////////////////////////////
// Closes tape file
void TAPECloseInput(void)
{
	WMCloseOutput(false);
	WLCClose();
}

///////////////////////////////////////////////////////////////////////////////
// Creates output file
bool TAPECreateOutput(wchar_t* in_file_name)
{
		// openwave output
	if(!WMOpenOutput(in_file_name))
		return false;
	else
		return true;
}

///////////////////////////////////////////////////////////////////////////////
// Closes output file
void TAPECloseOutput(void)
{
	WMCloseOutput(false);
	WLCClose();
}

///////////////////////////////////////////////////////////////////////////////
// Saves Tape file
bool TAPESave(wchar_t* in_file_name)
{
	TAPEBlockHeaderType tape_block_header;
	TAPESectorHeaderType tape_sector_header;
	CASProgramFileHeaderType cas_program_header;
	TAPESectorEndType tape_sector_end;
	BYTE sector_count;
	BYTE sector_index;
	int sector_size;
	int tape_file_name_length;
	bool success = true;

	// block leading
	DisplayOutputHeaderProgress(0, 10);
	if(success)
		success = EncodeBlockLeading();

	// write header block	start
	DisplayOutputHeaderProgress(5, 10);
	TAPEInitBlockHeader(&tape_block_header);
	tape_block_header.BlockType = TAPE_BLOCKHDR_TYPE_HEADER;
	tape_block_header.SectorsInBlock	= 1;

	if(success)
	{
		CRCReset();
		CRCAddBlock(((BYTE*)&tape_block_header.Magic), sizeof(tape_block_header) - sizeof(tape_block_header.Zero));
		success = EncodeBlock((BYTE*)&tape_block_header, sizeof(tape_block_header));
	}

	// header block sector start
	tape_file_name_length							= strlen(g_db_file_name);
	tape_sector_header.SectorNumber		= 0;
	tape_sector_header.BytesInSector	= (BYTE)(sizeof(BYTE) + tape_file_name_length + sizeof(cas_program_header));

	CRCAddBlock(((BYTE*)&tape_sector_header), sizeof(tape_sector_header));
	if(success)
		success = EncodeBlock((BYTE*)&tape_sector_header, sizeof(tape_sector_header));

	// write tape file name
	if(success)
	{
		CRCAddByte(tape_file_name_length);
		success = EncodeByte(tape_file_name_length);
	}

	if(success)
	{
		CRCAddBlock((BYTE*)&g_db_file_name, tape_file_name_length);
		success = EncodeBlock((BYTE*)&g_db_file_name, tape_file_name_length);
	}

	// write program header
	if(success)
	{
		CASInitHeader(&cas_program_header);
		CRCAddBlock((BYTE*)&cas_program_header, sizeof(cas_program_header));
		success = EncodeBlock((BYTE*)&cas_program_header, sizeof(cas_program_header));
	}

	// write sector end
	tape_sector_end.EOFFlag = TAPE_SECTOR_NOT_EOF;
	CRCAddByte(tape_sector_end.EOFFlag);

	tape_sector_end.CRC = CRCGet();

	if(success)
		success = EncodeBlock((BYTE*)&tape_sector_end, sizeof(tape_sector_end));
	DisplayOutputHeaderProgress(10, 10);

	// closing header block
	if(success)
		success = GenerateDDSSignal(OffsetFrequency(FREQ_LEADING), 5);

	if(g_output_file_type == FT_WaveInOut)
		DisplayMessage(L"\n");

	// create data block
	sector_count = (g_db_buffer_length + 255) / 256;
	sector_index = 1;

	// block leading
	DisplayOutputDataProgress(0, g_db_buffer_length);
	if(success)
		success = EncodeBlockLeading();

	// write header block
	if(success)
	{
		TAPEInitBlockHeader(&tape_block_header);
		CRCReset();
		CRCAddBlock(((BYTE*)&tape_block_header.Magic), sizeof(tape_block_header) - sizeof(tape_block_header.Zero));
		success = EncodeBlock((BYTE*)&tape_block_header, sizeof(tape_block_header));
	}

	while(sector_index <= sector_count && success)
	{
		// write sector header
		sector_size = g_db_buffer_length - 256 * (sector_index - 1);
		if( sector_size > 255 )
			sector_size = 256;
		else
		{
			sector_size = sector_size;
		}

		DisplayOutputDataProgress((sector_index-1)*256 + sector_size, g_db_buffer_length);

		tape_sector_header.SectorNumber		= sector_index;
		tape_sector_header.BytesInSector	= (sector_size > 255)? 0 : (BYTE)sector_size;

		if(success)
		{
			CRCAddBlock((BYTE*)&tape_sector_header, sizeof(tape_sector_header));
			success = EncodeBlock((BYTE*)&tape_sector_header, sizeof(tape_sector_header));
		}

		// sector data
		if(success)
		{
			CRCAddBlock((BYTE*)&g_db_buffer[(sector_index-1)*256], sector_size);
			success = EncodeBlock((BYTE*)&g_db_buffer[(sector_index-1)*256], sector_size);
		}

		// sector end
		tape_sector_end.EOFFlag = (sector_index == sector_count) ? TAPE_SECTOR_EOF : TAPE_SECTOR_NOT_EOF;
		CRCAddByte(tape_sector_end.EOFFlag);

		tape_sector_end.CRC = CRCGet();

		if(success)
			success = EncodeBlock((BYTE*)&tape_sector_end, sizeof(tape_sector_end));

		CRCReset();
		sector_index++;
	}

	// closing block
	if(success)
		success =	GenerateDDSSignal(OffsetFrequency(FREQ_LEADING), 5);

	if(success)
	{
		success = GenerateDDSSilence(50);
	}

	if(g_output_file_type == FT_WaveInOut)
		DisplayMessage(L"\n");

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Validates block header
bool TAPEValidateBlockHeader(TAPEBlockHeaderType* in_block_header)
{
	if( in_block_header->Zero != TAPE_BLOCKHDR_ZERO ||
			in_block_header->Magic != TAPE_BLOCKHDR_MAGIC ||
			in_block_header->FileType != CASBLOCKHDR_FILE_UNBUFFERED )
		return false;

	if(in_block_header->BlockType != TAPE_BLOCKHDR_TYPE_HEADER && in_block_header->BlockType != TAPE_BLOCKHDR_TYPE_DATA)
		return false;

	if(in_block_header->BlockType == TAPE_BLOCKHDR_TYPE_HEADER && in_block_header->SectorsInBlock != 1)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Initializes Tape block header
void TAPEInitBlockHeader(TAPEBlockHeaderType* out_block_header)
{
	// init header block
	out_block_header->Zero						= 0x00;
	out_block_header->Magic						= TAPE_BLOCKHDR_MAGIC;
	out_block_header->BlockType				= TAPE_BLOCKHDR_TYPE_DATA;
	out_block_header->FileType				= CASBLOCKHDR_FILE_UNBUFFERED;
	out_block_header->CopyProtect			= (g_db_copy_protect) ? 0xff : 0x00;
	out_block_header->SectorsInBlock	= (g_db_buffer_length + 255) / 256;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Display header progress
static void DisplayOutputHeaderProgress(int in_pos, int in_max_pos)
{
	if(g_output_file_type == FT_WaveInOut)
		DisplayProgressBar(L"Saving header", in_pos, in_max_pos);
}

///////////////////////////////////////////////////////////////////////////////
// Displays data progress
static void DisplayOutputDataProgress(int in_pos, int in_max_pos)
{
	if(g_output_file_type == FT_WaveInOut)
		DisplayProgressBar(L"Saving data  ", in_pos, in_max_pos);
}

///////////////////////////////////////////////////////////////////////////////
// Offsetting frequency
static WORD OffsetFrequency(WORD in_frequency)
{
	if(g_frequency_offset == 0)
		return in_frequency;

	return (WORD)((DWORD)in_frequency * (100 + g_frequency_offset) / 100);
}

///////////////////////////////////////////////////////////////////////////////
// Encodes block leading
static bool EncodeBlockLeading(void)
{
	bool success;
	WORD period_count = (WORD)((((DWORD)OffsetFrequency(FREQ_LEADING)) * g_leading_length + 500 ) / 1000);

	success = GenerateDDSSilence(g_gap_length);

	if(success)
	{
		success = GenerateDDSSignal(OffsetFrequency(FREQ_LEADING), period_count);
	}

	if(success)
		success = GenerateDDSSignal(OffsetFrequency(FREQ_SYNC), 1);

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Encodes one byte
static bool EncodeByte(BYTE in_data)
{
	int i;
	bool success = true;

	for( i = 0; i < 8 && success; i++)
	{
		if( (in_data & 0x01) == 0 )
		{
			success = GenerateDDSSignal(OffsetFrequency(FREQ_ZERO), 1);
		}
		else
		{
			success = GenerateDDSSignal(OffsetFrequency(FREQ_ONE), 1);
		}

		in_data >>= 1;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Encodes several bytes
static bool EncodeBlock(BYTE* in_buffer, int in_length)
{
	bool success = true;
	while(in_length > 0 && success)
	{
		success = EncodeByte(*in_buffer);
		in_buffer++;
		in_length--;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Display failed to load message
static void DisplayFailedToLoad(void)
{
	wchar_t buffer[DB_MAX_FILENAME_LENGTH+1];

	TVCStringToUNICODEString(buffer, g_db_file_name);
	DisplayMessageAndClearToLineEnd(L"Failed to load file: %s (signal lost)", buffer);
	DisplayMessage(L"\n");
}

///////////////////////////////////////////////////////////////////////////////
// Displays input status message
static void DisplayInputDataProgress(void)
{
	BYTE percentage;
	DWORD total_seconds;
	WORD hour, minutes, seconds;
	wchar_t buffer[DB_MAX_FILENAME_LENGTH+1];

	switch(g_input_file_type)
	{
		case FT_WAV:
			if(g_input_wav_file_sample_count != 0)
			{
				// calculate percentage
				percentage = (BYTE)((DDWORD)g_input_wav_file_sample_index * 100 / g_input_wav_file_sample_count);
				total_seconds = g_input_wav_file_sample_index / SAMPLE_RATE;

				// calculate time
				hour = (WORD)(total_seconds / (60 * 60));
				total_seconds -= (WORD)(hour * (60 * 60));
				minutes = (WORD)(total_seconds / 60);
				total_seconds -= minutes * 60;
				seconds = (WORD)total_seconds;

				if(percentage != l_prev_input_percentage || total_seconds != l_prev_input_total_seconds)
				{
					// generate file name and display status information
					if(l_header_block_valid)
					{
						TVCStringToUNICODEString(buffer, g_db_file_name);
						DisplayMessageAndClearToLineEnd(L"Processing: %3d%% (%0uh%02um%02us), Loading: %s", percentage, hour, minutes, seconds, buffer);
					}
					else
					{
						DisplayMessageAndClearToLineEnd(L"Processing: %3d%% (%0uh%02um%02us)", percentage, hour, minutes, seconds);
					}

					l_prev_input_percentage = percentage;
					l_prev_input_total_seconds = total_seconds;
				}
			}
			break;

		case FT_WaveInOut:
			{
				if(g_wavein_peak_updated)
				{
					if(l_header_block_valid)
					{
						TVCStringToUNICODEString(buffer, g_db_file_name);
						DisplaySignalLevel(g_wavein_peak_level, g_cpu_overload, L" Loading: %s", buffer);
					}
					else
					{
						DisplaySignalLevel(g_wavein_peak_level, g_cpu_overload, L"");
					}

					g_wavein_peak_updated = false;
					g_cpu_overload = false;
				}
			}
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Restarts decoder
static LoadStatus DecoderRestart(void)
{
	LoadStatus load_status = LS_Unknown;

	if(l_tape_reader_status == TRST_Data)
	{
		l_header_block_valid = false;
		load_status = LS_Error;
	}

	// reset decoder
	l_decoder_state = DST_Idle;
	WLCSetMode(WLCMT_NoiseKiller);
	l_tape_reader_status = TRST_Idle;

	return load_status;
}

///////////////////////////////////////////////////////////////////////////////
// Process one sample
static LoadStatus DecodeSample(INT32 in_sample)
{
	DWORD period_length;
	int high_period_length;
	int low_period_length;
	int oversampled_length;
	SignalPhaseType current_phase;
	bool half_period_end;
	LoadStatus load_status = LS_Unknown;

	// cache period length
	high_period_length = l_period_high_length;
	low_period_length = l_period_low_length;
	current_phase = l_current_phase;

	// detectg zero crossing (eof of the half periods)
	half_period_end = false;
	switch(current_phase)
	{
		// check for zero crossing in rising direction
		case SPT_Low:
			if(in_sample > 0)
			{
				oversampled_length = (OVERSAMPLING_RATE * l_previous_sample) / (l_previous_sample - in_sample);
				l_period_low_length += oversampled_length;
				period_length = l_period_high_length + l_period_low_length;
				l_period_high_length = OVERSAMPLING_RATE - oversampled_length;
				half_period_end = true;
				l_current_phase = SPT_High;
			}
			else
			{
				l_period_low_length += OVERSAMPLING_RATE;
			}
			break;

		// check for zero crossing in falling direction
		case SPT_High:
			if(in_sample < 0)
			{
				oversampled_length = (OVERSAMPLING_RATE * l_previous_sample) / (l_previous_sample - in_sample);
				l_period_high_length += oversampled_length;
				period_length = l_period_high_length + l_period_low_length;
				l_period_low_length = OVERSAMPLING_RATE - oversampled_length;
				half_period_end = true;
				l_current_phase = SPT_Low;
			}
			else
			{
				l_period_high_length += OVERSAMPLING_RATE;
			}
			break;
	}
	l_previous_sample = in_sample;

	// zero cross detected (half period end)
	if(half_period_end)
	{
		switch (l_decoder_state)
		{
			// waiting for an apropriate signal
			case DST_Idle:
				// looking for leading signal
				l_middle_period_buffer_index = 0;
				l_decoder_state = DST_WaitingForLeading;
				break;

			// waiting for leading signal
			case DST_WaitingForLeading:
			{
				DWORD leading_min = PERIOD_LEADING - (PERIOD_LEADING * LEADING_FREQUENCY_TOLERANCE + 50) / 100;
				DWORD leading_max = PERIOD_LEADING + (PERIOD_LEADING * LEADING_FREQUENCY_TOLERANCE + 50) / 100;

				// check for leading frequency
				if(period_length >= leading_min && period_length <= leading_max)
				{
					// frequency is ok, store it for the running average
					UpdateMiddleFrequency(FREQ_LEADING, period_length);

					// we have one buffer of leading frequency data -> averrage is valid
					if(l_middle_period_buffer_index == 0)
					{
						l_decoder_state = DST_WaitingForSync;
						WLCSetMode(WLCMT_LevelControl);
					}
				}
				else
				{
					load_status = DecoderRestart();
				}
			}
			break;

			// waiting for sync signal
			case DST_WaitingForSync:
				{
					DWORD leading_min = PERIOD_LEADING - (PERIOD_LEADING * LEADING_FREQUENCY_TOLERANCE + 50) / 100;
					DWORD leading_max = PERIOD_LEADING + (PERIOD_LEADING * LEADING_FREQUENCY_TOLERANCE + 50) / 100;
					DWORD expected_sync_period = (FREQ_MIDDLE * l_middle_period + FREQ_SYNC / 2) / FREQ_SYNC;
					DWORD sync_min = expected_sync_period - (PERIOD_SYNC * SYNC_FREQUENCY_TOLERANCE + 50) / 100;
					DWORD sync_max = expected_sync_period + (PERIOD_SYNC * SYNC_FREQUENCY_TOLERANCE + 50) / 100;

					// check for leading frequency
					if(period_length >= leading_min && period_length <= leading_max)
					{
						// frequency is ok, store it for the running average
						UpdateMiddleFrequency(FREQ_LEADING, period_length);	
					}
					else
					{
						// check for sync frequency
						if(period_length >= sync_min && period_length <= sync_max)
						{
							switch(current_phase)
							{
								case SPT_High:
									l_sync_first_half_period_length = high_period_length;
									l_sync_second_half_period_length = low_period_length;
									l_decoder_state = DST_SyncDetected;
									break;

								case SPT_Low:
									l_sync_first_half_period_length = high_period_length;
									l_sync_second_half_period_length = low_period_length;
									l_decoder_state = DST_SyncDetected;
									break;
							}
						}
						else
						{
							if(period_length < leading_min || period_length > sync_max)
								load_status = DecoderRestart();
						}
					}
				}
				break;

			//	Sync period length detected
			case DST_SyncDetected:
				{
					DWORD expected_sync_half_period = (FREQ_MIDDLE * l_middle_period / FREQ_SYNC + 1) / 2;
					DWORD expected_leading_half_period = (FREQ_MIDDLE * l_middle_period / FREQ_LEADING + 1) / 2;
					DWORD expected_zero_half_period = (FREQ_MIDDLE * l_middle_period / FREQ_ZERO + 1) / 2;
					DWORD sync_third_half_period_length;
					BYTE first_score;
					BYTE second_score;

					// determine second and third half period length
					switch(current_phase)
					{
						case SPT_High:
							sync_third_half_period_length = high_period_length;
							break;

						case SPT_Low:
							sync_third_half_period_length = low_period_length;
							break;
					}

					//Now we have the length of the three previous half period
					// determine which two contans a valid sync period. It can be the first two or second two half period.
					// scoring algorithm will decide
					first_score = 0;
					second_score = 0;

					// Score based on half period symmetry sync period is most probable has two simmetrical length half period
					if(IntABS(l_sync_first_half_period_length - l_sync_second_half_period_length) < IntABS(l_sync_second_half_period_length-sync_third_half_period_length))
					{
						first_score++;
					}
					else
					{
						second_score++;
					}

					// Score based on the first half period. If its length is closer to leading length, probably it belons to leading signal not to the sync.
					// If its closer to sync then probably it belongs to sync.
					if(IntABS(l_sync_first_half_period_length - expected_leading_half_period) < IntABS(l_sync_first_half_period_length-expected_sync_half_period))
					{
						second_score++;
					}
					else
					{
						first_score++;
					}

					// Score based on the last (third) period length. If the length is closer to zero period length (the first bit after the sync shoud be zero) then 
					// sync if located at the first period. If its length is closer to sync length than sync is located at the second half period
					if(IntABS(sync_third_half_period_length - expected_sync_half_period) < IntABS(sync_third_half_period_length - expected_zero_half_period))
					{
						second_score++;
					}
					else
					{
						first_score++;
					}

					if(first_score > second_score)
					{
						// sync is located at the first half
						l_phase_mode = l_current_phase;
					}
					else
					{
						// sync starts at the second half
						l_phase_mode = current_phase;
					}

					// read block header
					l_decoder_state = DST_ReadingData;
					l_bit_counter = 0;
					l_data_byte = 0;
					ChangeReaderStatus(TRST_BlockHeader);
				}
				break;

				// reading data bits
				case DST_ReadingData:
					if(current_phase == l_phase_mode)
					{
						l_data_byte = l_data_byte >> 1;
						if( period_length <= l_middle_period )
						{
							l_data_byte |= 0x80;
							UpdateMiddleFrequency(FREQ_ONE, period_length);
						}
						else
						{
							UpdateMiddleFrequency(FREQ_ZERO, period_length);
						}

						l_bit_counter++;
						if( l_bit_counter >= 8 )
						{
							l_bit_counter = 0;

							load_status = StoreByte(l_data_byte);
						}
					}
					break;

				// skip sector end signal
				case DST_SectorEnd:
					l_sector_end_period_count++;
					if(l_sector_end_period_count>=SECTOR_END_PERIOD_COUNT)
						load_status = DecoderRestart();
					break;
			}
	}
	else
	{
		// check for signal loss
		if((high_period_length + low_period_length) > PERIOD_SYNC * (100 + LEADING_FREQUENCY_TOLERANCE) / 100 )
			load_status = DecoderRestart();
	}

	l_demod_sample_index++;

	return load_status;
}

///////////////////////////////////////////////////////////////////////////////
// Integer absolute value
static int IntABS(int in_value)
{
	if(in_value < 0)
		return -in_value;
	else
		return in_value;
}

///////////////////////////////////////////////////////////////////////////////
// Stores data byte readed by the decoder
static LoadStatus StoreByte(BYTE in_data_byte)
{
	LoadStatus load_status = LS_Unknown;

	switch (l_tape_reader_status)
	{
		// reading header
		case TRST_BlockHeader:
			// store block header
			if(StoreByteInStruct(in_data_byte, &l_block_header, sizeof(l_block_header), false))
			{
				// check block
				if(TAPEValidateBlockHeader(&l_block_header))
				{
					// load sector header
					ChangeReaderStatus(TRST_SectorHeader);

					// new block header is coming -> invalidate current
					if(l_block_header.BlockType == TAPE_BLOCKHDR_TYPE_HEADER)
						l_header_block_valid = false;

					g_db_buffer_index = 0;
					g_db_crc_error_detected = false;
					CRCReset();
					CRCAddBlock(((BYTE*)&l_block_header + 1), sizeof(l_block_header)-1);
				}
				else
				{
					ChangeReaderStatus(TRST_Idle);
					load_status = DecoderRestart();
				}
			}
			break;

		// Load sector header
		case TRST_SectorHeader:
			if(StoreByteInStruct(in_data_byte, &l_sector_header, sizeof(l_sector_header), true))
			{
				// check sector header
				switch(l_block_header.BlockType)
				{
					// header block
					case TAPE_BLOCKHDR_TYPE_HEADER:
						// in the case of header block the sector number must be zero
						if(l_sector_header.SectorNumber == 0)
						{
							SetSectorLength(l_sector_header.BytesInSector);
							ChangeReaderStatus(TRST_HeaderFileNameLength);
						}
						else
						{
							ChangeReaderStatus(TRST_Idle);
							load_status = DecoderRestart();
						}
						break;

					// data block
					case TAPE_BLOCKHDR_TYPE_DATA:
						SetSectorLength(l_sector_header.BytesInSector);
						ChangeReaderStatus(TRST_Data);
						break;

					// unknown block
					default:
						ChangeReaderStatus(TRST_Idle);
						load_status = DecoderRestart();
						break;
				}
			}
			break;

		// File name length
		case TRST_HeaderFileNameLength:
			if(l_data_byte <= DB_MAX_FILENAME_LENGTH)
			{
				l_file_name_length = in_data_byte;
				CRCAddByte(in_data_byte);
				if(l_file_name_length == 0)
				{
					g_db_file_name[0] = '\0';
					ChangeReaderStatus(TRST_HeaderProgramHeader);
				}
				else
				{
					ChangeReaderStatus(TRST_HeaderFileName);
				}

				l_data_byte_index = 0;
			}
			else
			{
				ChangeReaderStatus(TRST_Idle);
				load_status = DecoderRestart();
			}
			break;

		// File name
		case TRST_HeaderFileName:
			if(l_data_byte_index < DB_MAX_FILENAME_LENGTH)
			{
				// replace string terminator character to space
				if(in_data_byte == '\0')
					g_db_file_name[l_data_byte_index]  = ' ';
				else
					g_db_file_name[l_data_byte_index] = in_data_byte;

				CRCAddByte(in_data_byte);

				l_data_byte_index++;
				if(l_data_byte_index == l_file_name_length)
				{
					g_db_file_name[l_data_byte_index] = '\0';

					ChangeReaderStatus(TRST_HeaderProgramHeader);
				}
			}
			else
			{
				ChangeReaderStatus(TRST_Idle);
				load_status = DecoderRestart();
			}
			break;

		// program header
		case TRST_HeaderProgramHeader:
			if(StoreByteInStruct(in_data_byte, &l_program_header, sizeof(l_program_header), true))
			{	
				ChangeReaderStatus(TRST_SectorEnd);
				g_db_buffer_length = 0;
			}
			break;

		// read sector end
		case TRST_SectorEnd:
			if(StoreByteInStruct(in_data_byte, &l_sector_end, sizeof(l_sector_end), false))
			{
				CRCAddByte(l_sector_end.EOFFlag);

				// check CRC
				switch(l_block_header.BlockType)
				{
					// header block
					case TAPE_BLOCKHDR_TYPE_HEADER:
						if(l_sector_end.CRC == CRCGet())
						{
							g_db_autostart = (l_program_header.Autorun != 0);
							g_db_buffer_length = l_program_header.FileLength;
							l_header_block_valid = true;
						}
						else
							l_header_block_valid = false;

						// no more sector in the header block
						ChangeReaderStatus(TRST_Idle);
						l_decoder_state = DST_Idle;
						l_sector_end_period_count = 0;
						break;

					// data block
					case TAPE_BLOCKHDR_TYPE_DATA:
						if(l_sector_end.CRC != CRCGet())
							g_db_crc_error_detected = true;

						// if there is no more data to read
						if(g_db_buffer_index >= g_db_buffer_length || l_sector_end.EOFFlag == TAPE_SECTOR_EOF)
						{
							ChangeReaderStatus(TRST_Idle);
							l_decoder_state = DST_Idle;
							l_sector_end_period_count = 0;

							if(l_header_block_valid)
							{
								l_header_block_valid = false;
								load_status = LS_Success;
							}
						}
						else
						{
							// read next sectors
							ChangeReaderStatus(TRST_SectorHeader);
							CRCReset();
						}
						break;
				}
			}
			break;

		// read data
		case TRST_Data:
			// store data
			g_db_buffer[g_db_buffer_index++] = in_data_byte;
			CRCAddByte(in_data_byte);
			l_data_byte_index++;

			// check sector end
			if(l_data_byte_index >= l_current_sector_length || g_db_buffer_index >= g_db_buffer_length)
			{
				// read sector end
				ChangeReaderStatus(TRST_SectorEnd);
			}
			break;
	}

	return load_status;
}

///////////////////////////////////////////////////////////////////////////////
// Changes reader status to a new value
static void ChangeReaderStatus(TapeReaderStatusType in_new_status)
{
	l_data_byte_index = 0;
	l_tape_reader_status = in_new_status;
}

///////////////////////////////////////////////////////////////////////////////
// Stores received byte in a struct
static bool StoreByteInStruct(BYTE in_data_byte, void* in_struct, size_t in_size, bool in_add_to_crc)
{
	if( l_data_byte_index < in_size)
	{
		// store byte
		*((BYTE*)in_struct + l_data_byte_index) = in_data_byte;
		l_data_byte_index++;

		// add to CRC
		if(in_add_to_crc)
			CRCAddByte(in_data_byte);

		// check if this is the last byte of the struct
		return l_data_byte_index == in_size;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Sets sector length
static void SetSectorLength(BYTE in_sector_length)
{
	if(in_sector_length == 0)
		l_current_sector_length = 256;
	else
		l_current_sector_length = in_sector_length;
}

///////////////////////////////////////////////////////////////////////////////
// Updates middle frequency period length
static void UpdateMiddleFrequency(DWORD in_frequency, DWORD in_measured_period_length)
{
	int frequency_to_remove = l_middle_period_buffer[l_middle_period_buffer_index];
	int middle_period_length = (in_measured_period_length * in_frequency + FREQ_MIDDLE / 2) / FREQ_MIDDLE;

	l_middle_period_buffer[l_middle_period_buffer_index] = middle_period_length;

	l_middle_period_buffer_index++;
	if( l_middle_period_buffer_index >= MIDDLE_PERIOD_BUFFER_LENGTH )
		l_middle_period_buffer_index = 0;

	l_middle_period_sum = l_middle_period_sum - frequency_to_remove + middle_period_length;
	l_middle_period = (WORD)(l_middle_period_sum / MIDDLE_PERIOD_BUFFER_LENGTH);
}