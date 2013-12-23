/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Console text output routines                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __Console_h
#define __Console_h


///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Windows.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void ConsoleInit(void);
void DisplayError(const wchar_t* format, ...);
void DisplayMessage(const wchar_t* format, ...);
void DisplayMessageAndClearToLineEnd(const wchar_t* format, ...);
void PrintLogo(void);
void DisplayProgressBar(wchar_t* in_title, int in_value, int in_max_value);
void PrintHelp(void);


#endif