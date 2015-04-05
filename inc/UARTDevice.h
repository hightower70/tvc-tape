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

#ifndef __UARTDevice_h
#define __UARTDevice_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "Types.h"
#include "COMPort.h"


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool UARTOpen(COMConfigType* in_config);
void UARTSendBlock(BYTE* in_buffer, DWORD in_buffer_length);
DWORD UARTReceiveBlock(BYTE* in_buffer, DWORD in_buffer_length, DWORD* out_bytes_received);
void UARTClose(void);

#endif