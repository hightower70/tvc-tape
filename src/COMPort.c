/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Serial port file transfer functions                                       */
/*                                                                           */
/* Copyright (C) 2013-15 Laszlo Arvai                                        */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes files
#include "COMPort.h"
#include "UARTDevice.h"
#include "Main.h"
#include "DataBuffer.h"
#include "CASFile.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define UART_INPUT_BUFFER_LENGTH 32

///////////////////////////////////////////////////////////////////////////////
// Types
typedef enum
{
	COM_LS_Header,
	COM_LS_Data
} COMLoadState;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static uint32_t l_data_byte_index;

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
static bool StoreByteInStruct(uint8_t in_data_byte, void* in_struct, size_t in_size);

///////////////////////////////////////////////////////////////////////////////
// Initializes COM port access
void COMInit(void)
{
	g_com_config.PortIndex = 1;
	g_com_config.BaudRate = 1200;
	g_com_config.TransferLength = 8;
}

///////////////////////////////////////////////////////////////////////////////
// Sends one buffer content over serial port
bool COMSave(wchar_t* in_tape_file_name)
{
	bool success = true;
	CASProgramFileHeaderType cas_program_header;
	int block_start_pos;
	int current_block_length;
	SHORT s;

	CASInitHeader(&cas_program_header);

	// open uart
	if( UARTOpen(&g_com_config) )
	{
		// send header block
		UARTSendBlock((uint8_t*)&cas_program_header, sizeof(cas_program_header));

		// send datatv blocks
		block_start_pos = 0;
		while(block_start_pos < g_db_buffer_length && success)
		{
			// determine block length
			current_block_length = g_db_buffer_length - block_start_pos;
			if(current_block_length > 16)
				current_block_length = 16;

			// display status
			DisplayProgressBar(L"Data sent", block_start_pos + current_block_length, g_db_buffer_length);

			// send data
			UARTSendBlock((uint8_t*)&g_db_buffer[block_start_pos], current_block_length);

			// next block
			block_start_pos += current_block_length;

			// check for cancel key
			s = GetAsyncKeyState(STOP_KEY);
			if( s != 0)
				success = false;
		}
	}
	else
	{
		success = false;
	}

	// close uart
	UARTClose();

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Receives file from the serial prot
LoadStatus COMLoad(void)
{
	LoadStatus load_status = LS_Unknown;
	COMLoadState com_load_state = COM_LS_Header;
	uint8_t uart_buffer[UART_INPUT_BUFFER_LENGTH];
	uint32_t bytes_received;
	CASProgramFileHeaderType cas_program_header;
	uint32_t  buffer_index;

	// Open UART
	if(!UARTOpen(&g_com_config))
		load_status = LS_Fatal;

	// receive data from the UART
	while(load_status == LS_Unknown)
	{
		if(UARTReceiveBlock(uart_buffer, UART_INPUT_BUFFER_LENGTH, &bytes_received))
		{
			// stop key pressed
			load_status = LS_Fatal;
		}
		else
		{
			// process received data
			buffer_index = 0;
			while(buffer_index < bytes_received && load_status == LS_Unknown)
			{
				switch(com_load_state)
				{
					case COM_LS_Header:
						if(StoreByteInStruct(uart_buffer[buffer_index++], &cas_program_header, sizeof(CASProgramFileHeaderType)))
						{
							if(CASCheckHeaderValidity(&cas_program_header))
							{
								g_db_buffer_length = cas_program_header.FileLength;
								g_db_autostart = cas_program_header.Autorun;
								g_db_buffer_index = 0;
								com_load_state = COM_LS_Data;
							}
							else
							{
								load_status = LS_Fatal;
							}
						}
						break;

					case COM_LS_Data:
						g_db_buffer[g_db_buffer_index++] = uart_buffer[buffer_index++];
						if(g_db_buffer_index >= g_db_buffer_length)
						{
							load_status = LS_Success;
						}
						break;
				}
			}
		}
	}

	// closes UART
	UARTClose();

	return load_status;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Stores received byte in a struct
static bool StoreByteInStruct(uint8_t in_data_byte, void* in_struct, size_t in_size)
{
	if( l_data_byte_index < in_size)
	{
		// store byte
		*((uint8_t*)in_struct + l_data_byte_index) = in_data_byte;
		l_data_byte_index++;

		// check if this is the last byte of the struct
		return l_data_byte_index == in_size;
	}

	return true;
}


