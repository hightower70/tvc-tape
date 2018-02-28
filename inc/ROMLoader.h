/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Cartridge ROM loader binary images                                        */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __ROMLoader_h
#define __ROMLoader_h

///////////////////////////////////////////////////////////////////////////////
// Include
#include "Types.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
#define ROM_LOADER_COUNT 4


///////////////////////////////////////////////////////////////////////////////
// Function prototypes
const uint8_t* GetCartridgeLoaderBytes(int in_loader_index, int* out_loader_length, int* out_compression);

#endif
