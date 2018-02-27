/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Intel HEX file format handler                                             */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Include files
#include <stdio.h>
#include <ctype.h>
#include "Types.h"
#include "Console.h"
#include "Main.h"
#include "HEXFile.h"
#include "BASFile.h"
#include "DataBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////////////////
// Module global variables

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static bool WriteHexDigits(FILE *in_file, int in_digit_number, uint32_t in_value );
static void AddToChecksum(uint16_t *in_checksum, uint8_t in_value);
static bool ReadHexDigits( FILE *in_file, int in_digit_number, uint16_t* in_value);

///////////////////////////////////////////////////////////////////////////////
// Loads HEX file
LoadStatus HEXLoad(wchar_t* in_file_name)
{
	FILE* hex_file;
	bool success = true;
	int ch;
	int line = 1;
	uint16_t address_low;
	uint16_t address_high;
	uint16_t segment_address;
	uint16_t address;
	uint16_t rec_len;
	uint16_t rec_type;
	uint16_t data;
	uint16_t checksum, file_checksum;
	int i;
	int address_offset;
	bool first_data_record;
	int pos;

	// initialize
	address_high		= 0;
	address_low			= 0;
	segment_address	= 0;
	first_data_record = true;
	address_offset = 0;
	pos = 0;
	InitDataBuffer();

	// open CAS file
	hex_file = _wfopen(in_file_name, L"rb");
	if(hex_file == NULL)
		return LS_Fatal;

	while(success && !feof(hex_file))
	{
		// skip empty lines
		ch = fgetc(hex_file);

		if( ch == '\n' || ch == EOF )
			continue;

		// read header
		if( ch != ':' )
		{
			DisplayError(L"Illegal format at line %d\n", line );
			success = false;
		}

		checksum = 0;

		if(success)
		{
			// read record length
			success = ReadHexDigits(hex_file, 2, &rec_len);
			AddToChecksum(&checksum, (uint8_t)rec_len);
		}

		if(success)
		{
			// read address
			success = ReadHexDigits(hex_file, 4, &address_low);
		}

		AddToChecksum(&checksum, (uint8_t)(address_low >> 8));
		AddToChecksum(&checksum, (uint8_t)(address_low & 0xff));

		if(success)
		{
			// read record type
			success = ReadHexDigits(hex_file, 2, &rec_type);
			AddToChecksum(&checksum, (uint8_t)rec_type);
		}

		if(success)
		{
			// read record data
			switch(rec_type)
			{
				// data record
				case 0:
					i = 0;
					address = (uint32_t)(address_high << 16) + address_low + (segment_address << 4);

					// calculate address offset
					if(first_data_record)
					{
						first_data_record = false;
						address_offset = -address;
						pos = 0;
					}
					else
					{
						pos = address + address_offset;
					}

					while(success && i < rec_len)
					{
						success = ReadHexDigits(hex_file, 2, &data);

						if( pos < DB_MAX_DATA_LENGTH && pos >= 0)
						{
							g_db_buffer[pos++] = (uint8_t)data;
							AddToChecksum(&checksum, (uint8_t)data);
						}
						else
						{
							DisplayError(L"Illegal address at line: %d\n", line );
							success = false;
						}

						i++;
					}
					break;

				// EOF record
				case 1:
					break;


				// extended linear address record				
				case 4:
					success = ReadHexDigits(hex_file, 4, &address_high );

					AddToChecksum(&checksum, (uint8_t)(address_high >> 8));
					AddToChecksum(&checksum, (uint8_t)(address_high & 0xff));
					break;

				// extended segment address record
				case 2:
					success = ReadHexDigits(hex_file, 4, &segment_address);

					AddToChecksum(&checksum, (uint8_t)(address_high >> 8));
					AddToChecksum(&checksum, (uint8_t)(address_high & 0xff));
					break;

				default:
					DisplayError(L"Illegal record type at line: %d\n", line );
					success = false;
					break;
			}
		}

		// read checksum
		if(success)
		{
			success = ReadHexDigits(hex_file, 2, &file_checksum);
			if( success && ((file_checksum + checksum) & 0xff ) != 0)
			{
				DisplayError(L"Bad checksum at line: %d\n",line );
				success = false;
			}
		}

		if(success)
		{
			// read new line
			while(!feof(hex_file) && fgetc(hex_file) != '\n' );
			line++;
		}
	}

	fclose(hex_file);

	if(success)
		g_db_buffer_length = pos;

	if(success)
		return LS_Success;
	else
		return LS_Fatal;
}

///////////////////////////////////////////////////////////////////////////////
// Saves HEX file
bool HEXSave(wchar_t* in_file_name)
{
	FILE* hex_file;
	bool success = true;
	uint16_t rec_len;
	uint32_t address;
	uint8_t data;
	uint16_t checksum;
	int i;
	int pos;

	// skip basic program
	pos = 0;
	if(g_exclude_basic_program)
	{
		pos = BASFindEnd();
	}

	// save file
	hex_file = _wfopen(in_file_name, L"wb");
	if(hex_file == NULL)
		return false;

	// save buffer content
	address = g_lomem_address;
	while(success && pos < g_db_buffer_length)
	{
		// write header
		if(putc( ':', hex_file ) == EOF)
			success = false;

		// calculate rec_len
		rec_len = g_db_buffer_length - pos;
		if( rec_len > 16 )
			rec_len = 16;

		checksum = 0;

		if(success)
		{
			// write record length
			success = WriteHexDigits(hex_file, 2, rec_len);

			AddToChecksum(&checksum, (uint8_t)rec_len);
		}

		if(success)
		{
			// write address
			success = WriteHexDigits(hex_file, 4, address & 0xfffful);
		}

		AddToChecksum(&checksum, (uint8_t)(address >> 8));
		AddToChecksum(&checksum, (uint8_t)(address & 0xff));

		// write record type
		if(success)
			success = WriteHexDigits(hex_file, 2, 0);
		AddToChecksum(&checksum, (uint8_t)0);

		if(success)
		{
			// write record data
			i = 0;
			while( i < rec_len && success )
			{
				data = g_db_buffer[pos];

				success = WriteHexDigits(hex_file, 2, data);
				AddToChecksum(&checksum, data);

				address++;
				pos++;
				i++;
			}
		}

		// write checksum
		if(success)
		{
			success = WriteHexDigits(hex_file, 2, 0x0100 - checksum);
		}

		if(success)
		{
			// write new line
			if(fprintf(hex_file, "\n" ) <= 0)
				success = false;
		}
	}

	// write file end
	if(success)
		fprintf(hex_file, ":00000001FF\n");

	// close file
	fclose(hex_file);

	return success;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Writes a number in hex format with the given number of digits
static bool WriteHexDigits(FILE *in_file, int in_digit_number, uint32_t in_value )
{
	bool success = true;
	int digit = in_digit_number;
	int ch;
	uint32_t value;

	while(digit > 0 && success)
	{
		value = (in_value >> ((digit-1)*4)) & 0x0f;

		if( value > 9 )
			ch = (char)(value + 'A' - 10);
		else
			ch = (char)(value + '0');

		if(fputc( ch, in_file ) == EOF)
			success = false;

		digit--;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Adds one byte to the modulo 0x0100 checksum
static void AddToChecksum(uint16_t *in_checksum, uint8_t in_value)
{
	*in_checksum += in_value;
	*in_checksum &= 0xff;
}


///////////////////////////////////////////////////////////////////////////////
// Reads hexadecimal number of a given number of digits
static bool ReadHexDigits(FILE *in_file, int in_digit_number, uint16_t* in_value)
{
	bool success = true;
	int digit = 0;
	int ch;

	*in_value = 0;

	while(success && digit < in_digit_number)
	{
		ch = fgetc( in_file );
		if( ch == EOF )
		{
			success = false;
		}
		else
		{
			*in_value <<= 4;

			ch = toupper(ch);

			digit++;

			if( ch >= '0' && ch <= '9' )
			{
				*in_value += ch - '0';
			}
			else
			{
				if( ch >= 'A' && ch <= 'F' )
				{
					*in_value += ch - 'A' + 0xa;
				}
				else
				{
					success = false;
				}
			}
		}
	}

	return success;
}

