/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* TVC<->ANSI character coding                                               */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __CharMap_h
#define __CharMap_h
										
///////////////////////////////////////////////////////////////////////////////
// Include files
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
char TVCCharToANSIIChar(char in_tvc);
void TVCStringToANSIString(char* out_destination_ansi_string, char* in_source_tvc_string);

char ANSIICharToTVCChar(char in_ansii);
void ANSIStringToTVCString(char* out_destination_ansi_string, char* in_source_tvc_string);

wchar_t TVCCharToUNICODEChar(char in_tvc);
void TVCStringToUNICODEString(wchar_t* out_destination_ansi_string, char* in_source_tvc_string);

char TVCCharToASCIIChar(char in_tvc);
void TVCStringToASCIIString(char* out_destination_ansi_string, char* in_source_tvc_string);

char UNICODECharToTVCChar(wchar_t in_char);
void UNICODEStringToTVCString(char* out_destination_ansi_string, wchar_t* in_source_tvc_string);

void TVCToPCFilename(wchar_t* out_tvc_file_name, char* in_file_name);

wchar_t ANSICharToUNICODEChar(char in_char);


#endif