/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Wave file handling                                                        */
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
#include <wchar.h>
#include "Main.h"
#include "WaveFile.h"
#include "WaveMapper.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FILE* l_input_wave_file = NULL;
uint32_t g_input_wav_file_sample_count;
uint32_t g_input_wav_file_sample_index;
static uint16_t l_input_wav_file_bits_per_sample;
static uint8_t l_input_wav_file_sample_buffer;
static uint8_t l_input_wav_file_sample_bit_pos = 0;
static FILE* l_output_wav_file = NULL;
static FormatChunkType l_output_wav_file_format_chunk;
static uint32_t l_output_wav_file_sample_count;
static uint32_t l_output_wav_file_sample_index;
static uint16_t l_output_wav_file_bits_per_sample;
static uint8_t l_output_wav_file_sample_buffer;
static uint8_t l_output_wav_file_sample_bit_pos = 0;


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static void WriteRIFFHeader(void);

/*****************************************************************************/
/* Wave input functions                                                      */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Opens wavefile for input
bool WFOpenInput(wchar_t* in_file_name)
{
	bool success;
	RIFFHeaderType riff_header;
	ChunkHeaderType chunk_header;
	FormatChunkType format_chunk;
	uint32_t pos;
	bool data_chunk_found;

	// open wave file
	data_chunk_found = false;
	success = true;
	l_input_wave_file = _wfopen(in_file_name, L"rb" );
	if(l_input_wave_file == NULL)
	{
		DisplayError(L"Error: File not found %s.\n", in_file_name);
		success = false;
	}

	// load RIFF header
	if(success)
	{
		fread(&riff_header, sizeof(riff_header), 1, l_input_wave_file);

		if( (riff_header.ChunkID != RIFF_HEADER_CHUNK_ID) || (riff_header.Format != RIFF_HEADER_FORMAT_ID) ) 
		{
			DisplayError(L"Error: Invalid file format.\n");
			success = false;
		}
	}

	// process chunks
	while(success && !feof(l_input_wave_file) && !data_chunk_found)
	{
		// read chunk header
		if( fread(&chunk_header, sizeof(chunk_header), 1, l_input_wave_file) == 1 )
		{
			pos = ftell(l_input_wave_file);

			switch (chunk_header.ChunkID)
			{
				// Format 'fmt ' chunk
				case CHUNK_ID_FORMAT:
					fread(&format_chunk, sizeof(format_chunk), 1, l_input_wave_file);

					if(format_chunk.AudioFormat != 1)
					{
						DisplayError(L"Error: Wav file is not in PCM format.\n");
						success = false;
					}

					if(format_chunk.SampleRate != 44100)
					{
						DisplayError(L"Error: Wav file sample rate is not 44100Hz.\n");
						success = false;
					}

					if(format_chunk.NumChannels != 1)
					{
						DisplayError(L"Error: Only mono format is supported.\n");
						success = false;
					}

					if((format_chunk.BitsPerSample != 1) && (format_chunk.BitsPerSample != 8) && (format_chunk.BitsPerSample != 16)) 
					{
						DisplayError(L"Error: Wav file with only 1, 8, 16 bit samples are supported.\n");
						success = false;
					}

					l_input_wav_file_bits_per_sample = format_chunk.BitsPerSample;
					
					break;

				// Data 'data' chunk
				case CHUNK_ID_DATA:
					data_chunk_found = true;
					g_input_wav_file_sample_count = chunk_header.ChunkSize * 8 / l_input_wav_file_bits_per_sample;
					break;
			}

			// move to the next chunk
			if(!data_chunk_found)
				fseek(l_input_wave_file, pos + chunk_header.ChunkSize, SEEK_SET);
		}
	}

	g_input_wav_file_sample_index = 0;
	l_input_wav_file_sample_bit_pos = 0;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Reads sample
bool WFReadSample(int32_t* out_sample)
{
	uint16_t buffer;
	size_t read_count;
	bool success = false;

	// read sample
	switch (l_input_wav_file_bits_per_sample)
	{
		case 1:
			if(l_input_wav_file_sample_bit_pos == 0)
			{
				// read data if buffer is empty
				read_count = fread(&l_input_wav_file_sample_buffer, sizeof(uint8_t), 1, l_input_wave_file);
				if(read_count == 1)
				{
					success = true;
				}
			}
			else
			{
				// there is date in the buffer
				success = true;
			}

			if(success)
			{
				// get sample
				*out_sample = ((l_input_wav_file_sample_buffer >> (7-l_input_wav_file_sample_bit_pos) & 0x01) != 0) ? MAXINT16 : MININT16;

				// next bit
				l_input_wav_file_sample_bit_pos++;
				if(l_input_wav_file_sample_bit_pos == 8)
					l_input_wav_file_sample_bit_pos = 0;
			}
			break;

		case 8:
			buffer = 0;
			read_count = fread(&buffer, sizeof(uint8_t), 1, l_input_wave_file);
			if(read_count == 1)
			{
				*out_sample = (INT16)(((buffer & 255) - BYTE_SAMPLE_ZERO_VALUE) * 256);
				success = true;
			}
			break;

		case 16:
			read_count = fread(&buffer, sizeof(uint16_t), 1, l_input_wave_file);
			if(read_count == 1)
			{
				*out_sample = (INT16)buffer;
				success = true;
			}
			break;
	}

	g_input_wav_file_sample_index++;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave input
void WFCloseInput(void)
{
	// close wave file
	if(l_input_wave_file != NULL)
	{
		fclose(l_input_wave_file);
		l_input_wave_file = NULL;
	}
}

/*****************************************************************************/
/* Wave output functions                                                     */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Creates wave file
bool WFOpenOutput(wchar_t* in_file_name, uint8_t in_bits_per_sample)
{
	ChunkHeaderType chunk_header;

	// create file
	l_output_wav_file_sample_bit_pos = 0;
	l_output_wav_file_sample_buffer = 0;
	l_output_wav_file_sample_count = 0;
	l_output_wav_file = _wfopen( in_file_name, L"w+b" );

	if( l_output_wav_file == NULL )
		return false;

	// write RIFF header
	WriteRIFFHeader();

	// write format chunk header
	chunk_header.ChunkID = CHUNK_ID_FORMAT;
	chunk_header.ChunkSize = sizeof(l_output_wav_file_format_chunk);

	fwrite( &chunk_header, sizeof(chunk_header), 1, l_output_wav_file );

	// write format chunk
	l_output_wav_file_format_chunk.AudioFormat		= 1;
	l_output_wav_file_format_chunk.SampleRate			= SAMPLE_RATE;
	l_output_wav_file_format_chunk.NumChannels		= 1;
	l_output_wav_file_format_chunk.BitsPerSample	= in_bits_per_sample;
	l_output_wav_file_format_chunk.BlockAlign			= 1;
	l_output_wav_file_format_chunk.ByteRate				= l_output_wav_file_format_chunk.SampleRate * l_output_wav_file_format_chunk.NumChannels * l_output_wav_file_format_chunk.BitsPerSample / 8;

	fwrite( &l_output_wav_file_format_chunk, sizeof(l_output_wav_file_format_chunk), 1, l_output_wav_file );

	// write chunk header
	chunk_header.ChunkID = CHUNK_ID_DATA;
	chunk_header.ChunkSize = 0;

	fwrite( &chunk_header, sizeof(chunk_header), 1, l_output_wav_file );

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Write sample to the output wave file
void WFWriteSample(int32_t in_sample)
{
	if(l_output_wav_file == NULL)
		return;

	switch(l_output_wav_file_format_chunk.BitsPerSample)
	{
		case 1:
			l_output_wav_file_sample_buffer <<= 1;
			if(in_sample > 128)
				l_output_wav_file_sample_buffer |= 1;

			l_output_wav_file_sample_bit_pos++;
			if(l_output_wav_file_sample_bit_pos > 7)
			{
				fwrite(&l_output_wav_file_sample_buffer, sizeof(uint8_t), 1, l_output_wav_file );
				l_output_wav_file_sample_bit_pos = 0;
				l_output_wav_file_sample_buffer = 0;
			}

			break;

		case 8:
			fwrite(&in_sample, sizeof(uint8_t), 1, l_output_wav_file );
			break;

		case 16:
			fwrite(&in_sample, sizeof(INT16), 1, l_output_wav_file);
	}

	l_output_wav_file_sample_count++;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave output file
void WFCloseOutput(bool in_force_close)
{
	ChunkHeaderType chunk_header;
	long pos;

	if( l_output_wav_file == NULL )
		return;

	// save remaining bits in one bits per sample mode
	if(l_output_wav_file_format_chunk.BitsPerSample == 1 && l_output_wav_file_sample_bit_pos > 0)
	{
		l_output_wav_file_sample_buffer <<= (8-l_output_wav_file_sample_bit_pos);
		fwrite(&l_output_wav_file_sample_buffer, sizeof(uint8_t), 1, l_output_wav_file );
	}

	// update riff header
	fseek( l_output_wav_file, 0, SEEK_SET );

	WriteRIFFHeader();

	// update data header
	pos = sizeof(RIFFHeaderType) + sizeof(ChunkHeaderType) + sizeof(FormatChunkType);
	fseek( l_output_wav_file, pos, SEEK_SET );

	chunk_header.ChunkID = CHUNK_ID_DATA;
	chunk_header.ChunkSize = l_output_wav_file_sample_count * l_output_wav_file_format_chunk.BitsPerSample / 8;

	fwrite( &chunk_header, sizeof(chunk_header), 1, l_output_wav_file );

	fclose(l_output_wav_file);

	l_output_wav_file = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Writes (or updates) riff file header
static void WriteRIFFHeader(void)
{
	RIFFHeaderType riff_header;
	//                 RIFF FORMAT		 fmt chunk header          format chunk content			 data chunk header				 data chunk content
	uint32_t chunk_size = sizeof(uint32_t) + sizeof(ChunkHeaderType) + sizeof(FormatChunkType) + sizeof(ChunkHeaderType) + l_output_wav_file_sample_count;

	// write header
	riff_header.ChunkID		=	RIFF_HEADER_CHUNK_ID;
	riff_header.Format		=	RIFF_HEADER_FORMAT_ID;
	riff_header.ChunkSize	= chunk_size;

	fwrite( &riff_header, sizeof(riff_header), 1, l_output_wav_file );
}

