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
static DWORD l_data_byte_index;

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
static bool StoreByteInStruct(BYTE in_data_byte, void* in_struct, size_t in_size);

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

	CASInitHeader(&cas_program_header);

	if( UARTOpen(&g_com_config) )
	{
		UARTSendBlock((BYTE*)&cas_program_header, sizeof(cas_program_header));
		UARTSendBlock((BYTE*)&g_db_buffer, g_db_buffer_length);
	}
	else
	{
		success = false;
	}

	UARTClose();

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Receives file from the serial prot
LoadStatus COMLoad(void)
{
	LoadStatus load_status = LS_Unknown;
	COMLoadState com_load_state = COM_LS_Header;
	BYTE uart_buffer[UART_INPUT_BUFFER_LENGTH];
	DWORD bytes_received;
	CASProgramFileHeaderType cas_program_header;
	DWORD  buffer_index;

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
static bool StoreByteInStruct(BYTE in_data_byte, void* in_struct, size_t in_size)
{
	if( l_data_byte_index < in_size)
	{
		// store byte
		*((BYTE*)in_struct + l_data_byte_index) = in_data_byte;
		l_data_byte_index++;

		// check if this is the last byte of the struct
		return l_data_byte_index == in_size;
	}

	return true;
}


