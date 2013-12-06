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
										
///////////////////////////////////////////////////////////////////////////////
// Include files
#include "CharMap.h"

///////////////////////////////////////////////////////////////////////////////
// Module global varables
extern char l_tvc_to_ansi[];
extern char l_ansi_to_tvc[];

///////////////////////////////////////////////////////////////////////////////
// TVC character to ANSI character conversion
char TVCCharToANSIIChar(char in_tvc)
{
	if((unsigned char)in_tvc < 128)
		return in_tvc;
	else
		return l_tvc_to_ansi[(unsigned char)in_tvc - 128];
}

///////////////////////////////////////////////////////////////////////////////
// TVC string to ANSI string conversion (inplace conversion is supported too)
void TVCStringToANSIString(char* out_destination_ansi_string, char* in_source_tvc_string)
{
	while((*in_source_tvc_string) != '\0')
	{
		*out_destination_ansi_string = TVCCharToANSIIChar(*in_source_tvc_string);
		out_destination_ansi_string++;
		in_source_tvc_string++;
	}

	*out_destination_ansi_string = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// ANSI character to TVC character conversion
char ANSIICharToTVCChar(char in_ansii)
{
	if((unsigned char)in_ansii < 128)
		return in_ansii;
	else
		return l_ansi_to_tvc[(unsigned char)in_ansii - 128];
}

///////////////////////////////////////////////////////////////////////////////
// ANSI string to TVC string
void ANSIStringToTVCString(char* out_destination_ansi_string, char* in_source_tvc_string)
{
	while((*in_source_tvc_string) != '\0')
	{
		*out_destination_ansi_string = TVCCharToANSIIChar(*in_source_tvc_string);
		out_destination_ansi_string++;
		in_source_tvc_string++;
	}

	*out_destination_ansi_string = '\0';
}


///////////////////////////////////////////////////////////////////////////////
// ANSI codepage

// TVC Character encoding
// 			0		 1		2			3		 4		5		 6		7		 8			9				a				b				c				d				e				f
//	8x 'Á', 'É', 'Í',  'Ó', 'Ö', 'Õ', 'Ú', 'Ü', 'Û', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
//  9x 'á', 'é', 'í',  'ó', 'ö', 'õ', 'ú', 'ü', 'û', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',

static char l_tvc_to_ansi[128] =
{
	//  0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
	'\xc1', '\xc9', '\xcd', '\xd3', '\xd6', '\xd5', '\xda', '\xdc', '\xdb', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
	'\xe1', '\xe9', '\xed', '\xf3', '\xf6', '\xf5', '\xfa', '\xfc', '\xfb', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
};

static char l_ansi_to_tvc[128] =
{
	//  0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
	'\xc0', '\x80', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\x81', '\xca', '\xcb', '\xcc', '\x82', '\xce', '\xcf',
	'\xd0', '\xd1', '\xd2', '\x83', '\xd4', '\x85', '\x84', '\xd7', '\xd8', '\xd9', '\x86', '\x88', '\x87', '\xdd', '\xde', '\xdf',
	'\xe0', '\x90', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\x91', '\xea', '\xeb', '\xec', '\x92', '\xee', '\xef',
	'\xf0', '\xf1', '\xf2', '\x93', '\xf4', '\x95', '\x94', '\xf7', '\xf8', '\xf9', '\x96', '\x98', '\x97', '\xfd', '\xfe', '\xff',
};

