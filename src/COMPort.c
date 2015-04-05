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
// Module global variables

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes

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
		success = false;

	return success;
}


