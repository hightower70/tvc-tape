/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Win32 UART (COM port) driver                                              */
/*                                                                           */
/* Copyright (C) 2013-15 Laszlo Arvai                                        */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Windows.h>
#include "Types.h"
#include "COMPort.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static HANDLE l_uart_handle = INVALID_HANDLE_VALUE;
										
///////////////////////////////////////////////////////////////////////////////
// Opens UART, returns true if success
bool UARTOpen(COMConfigType* in_config)
{
  BOOL success = TRUE;
  DCB comm_state;
  COMMTIMEOUTS time;
	wchar_t port_name[64];

	wsprintf(port_name, L"\\\\.\\COM%d", in_config->PortIndex);

	l_uart_handle = CreateFile( port_name,
															GENERIC_READ | GENERIC_WRITE,
															0,
															NULL,
															OPEN_EXISTING,
															0,
															0);

	if( l_uart_handle == INVALID_HANDLE_VALUE )
    success = FALSE;

  // setup port
  if( success )
		success = GetCommState( l_uart_handle, &comm_state );

	comm_state.BaudRate = in_config->BaudRate;
	comm_state.ByteSize = in_config->TransferLength;
  comm_state.Parity   = NOPARITY;
  comm_state.StopBits = ONESTOPBIT;

  if( success )
    success = SetCommState( l_uart_handle, &comm_state );
                 
  // set timeouts
  if( success )
    success = GetCommTimeouts( l_uart_handle, &time );

  time.ReadIntervalTimeout          = MAXDWORD;
  time.ReadTotalTimeoutMultiplier   = 0;
  time.ReadTotalTimeoutConstant     = 0;
  time.WriteTotalTimeoutConstant    = 0;
  time.WriteTotalTimeoutMultiplier  = 0;

  if( success )
    success = SetCommTimeouts( l_uart_handle, &time);

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Sends block of data to the uart
void UARTSendBlock(BYTE* in_buffer, DWORD in_buffer_length)
{
	DWORD bytes_written;

	WriteFile( l_uart_handle, in_buffer, in_buffer_length, &bytes_written, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Receives blocks of data from the uart
DWORD UARTReceiveBlock(BYTE* in_buffer, DWORD in_buffer_length, DWORD* out_bytes_received)
{
	SHORT s;

	// read block
	ReadFile(l_uart_handle, in_buffer, in_buffer_length, out_bytes_received, NULL);

	// check for stop key
	s = GetAsyncKeyState(STOP_KEY);

	return ( s != 0);
}

///////////////////////////////////////////////////////////////////////////////
// Coses uart
void UARTClose(void)
{
	if(l_uart_handle != INVALID_HANDLE_VALUE)
		CloseHandle(l_uart_handle);

	l_uart_handle = INVALID_HANDLE_VALUE;
}