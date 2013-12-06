/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* File handling helper functions                                            */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __FileUtils_h
#define __FileUtils_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void GenerateUniqueFileName(char* in_file_name);
void ReadBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success);
void WriteBlock(FILE* in_file, void* in_buffer, int in_size, bool* inout_success);

void GetFileNameAndExtension(char* in_file_name, char* in_path);
void GetFileNameWithoutExtension(char* in_file_name, char* in_path);
void ChangeFileExtension(char* in_file_name, char* in_extension);

void GenerateTVCFileName(char* out_tvc_file_name, char* in_file_name);

#endif
