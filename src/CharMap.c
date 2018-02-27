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
// Constants
#define CHARACTER_NUMBER 18

///////////////////////////////////////////////////////////////////////////////
// Types

// Unicode to TVC lookup table
typedef struct
{
	wchar_t UChar;
	char TVCChar;
} UnicodeToTVCCharMap;


///////////////////////////////////////////////////////////////////////////////
// Module global varables
extern char l_tvc_to_ansi[];
extern char l_ansi_to_tvc[];
extern wchar_t l_tvc_to_unicode[];
extern char l_tvc_to_ascii[];
extern UnicodeToTVCCharMap l_unicode_to_tvc[CHARACTER_NUMBER];


/*****************************************************************************/
/* ANSI <-> TVC conversion                                                   */
/*****************************************************************************/

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
// TVC character to ANSI character conversion
wchar_t TVCCharToUNICODEChar(char in_tvc)
{
	if((unsigned char)in_tvc < 128)
		return in_tvc;
	else
		return l_tvc_to_unicode[(unsigned char)in_tvc - 128];
}

///////////////////////////////////////////////////////////////////////////////
// TVC string to ANSI string conversion (inplace conversion is supported too)
void TVCStringToUNICODEString(wchar_t* out_destination_ansi_string, char* in_source_tvc_string)
{
	while((*in_source_tvc_string) != '\0')
	{
		*out_destination_ansi_string = TVCCharToUNICODEChar(*in_source_tvc_string);
		out_destination_ansi_string++;
		in_source_tvc_string++;
	}

	*out_destination_ansi_string = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Converts Unicode to TVC Character
char UNICODECharToTVCChar(wchar_t in_char)
{
	int first, last, middle;
	if(in_char < l_unicode_to_tvc[0].UChar)
		return (char)in_char;

	first = 0;
  last = CHARACTER_NUMBER - 1;
  while( first <= last )
  {
	  middle = (first+last)/2;
		if( l_unicode_to_tvc[middle].UChar < in_char )
			first = middle + 1;    
    else
		{
			if( l_unicode_to_tvc[middle].UChar == in_char ) 
			{
				return l_unicode_to_tvc[middle].TVCChar;
      }
      else
			{
				last = middle - 1;
			}
 		}
  }

	return '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Unicode string to TVC string conversion
void UNICODEStringToTVCString(char* out_destination_ansi_string, wchar_t* in_source_tvc_string)
{
	while((*in_source_tvc_string) != '\0')
	{
		*out_destination_ansi_string = UNICODECharToTVCChar(*in_source_tvc_string);
		out_destination_ansi_string++;
		in_source_tvc_string++;
	}

	*out_destination_ansi_string = '\0';
}

/*****************************************************************************/
/* ANSI-> UNICODE conversion                                                 */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Converts ANSII char to UNICODE char
wchar_t ANSICharToUNICODEChar(char in_char)
{
	switch(in_char)
	{
		case '\xd5': // Õ
			return L'\x0150';

		case '\xf5': // õ
			return L'\x0151';

		case '\xdb': // Û
			return L'\x0170';

		case '\xfb': // û
			return L'\x0171';

		default:
			return (wchar_t)(uint8_t)in_char;
	}
}

/*****************************************************************************/
/* ASCII codepage                                                            */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// TVC character to ASCII character conversion
char TVCCharToASCIIChar(char in_tvc)
{
	if((unsigned char)in_tvc < 128)
		return in_tvc;
	else
		return l_tvc_to_ascii[(unsigned char)in_tvc - 128];
}

///////////////////////////////////////////////////////////////////////////////
// TVC string to ASCII string conversion (inplace conversion is supported too)
void TVCStringToASCIIString(char* out_destination_ansi_string, char* in_source_tvc_string)
{
	while((*in_source_tvc_string) != '\0')
	{
		*out_destination_ansi_string = TVCCharToASCIIChar(*in_source_tvc_string);
		out_destination_ansi_string++;
		in_source_tvc_string++;
	}

	*out_destination_ansi_string = '\0';
}

/*****************************************************************************/
/* Conversion tables                                                         */
/*****************************************************************************/


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


static char l_tvc_to_ascii[128] =
{
  // 'Á', 'É', 'Í', 'Ó', 'Ö', 'Õ', 'Ú', 'Ü', 'Û', '\x0089', '\x008a', '\x008b', '\x008c', '\x008d', '\x008e', '\x008f',
	   'A', 'E', 'I', 'O', 'O', 'O', 'U', 'U', 'U',      ' ',      ' ',      ' ',      ' ',      ' ',      ' ',      ' ',
  // 'á', 'é', 'í', 'ó', 'ö', 'õ', 'ú', 'ü', 'û', '\x0099',  '\x009a',  '\x009b',  '\x009c',  '\x009d',  '\x009e',  '\x009f',
	   'a', 'e', 'i', 'o', 'o', 'o', 'u', 'u', 'u',      ' ',      ' ',      ' ',      ' ',      ' ',      ' ',      ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};

///////////////////////////////////////////////////////////////////////////////
// UNICODE codepage
static wchar_t l_tvc_to_unicode[128] =
{
  //    'Á',       'É',       'Í',       'Ó',       'Ö',       '?',       'Ú',       'Ü',       '?',  '\x0089',  '\x008a',  '\x008b',  '\x008c',  '\x008d',  '\x008e',  '\x008f',
	L'\x00c1', L'\x00c9', L'\x00cd', L'\x00d3', L'\x00d6', L'\x0150', L'\x00da', L'\x00dc', L'\x0170', L'\x0089', L'\x008a', L'\x008b', L'\x008c', L'\x008d', L'\x008e', L'\x008f',
  //    'á',       'é',       'í',       'ó',       'ö',       '?',       'ú',       'ü',       '?',  '\x0099',  '\x009a',  '\x009b',  '\x009c',  '\x009d',  '\x009e',  '\x009f',
	L'\x00e1', L'\x00e9', L'\x00ed', L'\x00f3', L'\x00f6', L'\x0151', L'\x00fa', L'\x00fc', L'\x0171', L'\x0099', L'\x009a', L'\x009b', L'\x009c', L'\x009d', L'\x009e', L'\x009f',
  L'\x00a0', L'\x00a1', L'\x00a2', L'\x00a3', L'\x00a4', L'\x00a5', L'\x00a6', L'\x00a7', L'\x00a8', L'\x00a9', L'\x00aa', L'\x00ab', L'\x00ac', L'\x00ad', L'\x00ae', L'\x00af',
  L'\x00b0', L'\x00b1', L'\x00b2', L'\x00b3', L'\x00b4', L'\x00b5', L'\x00b6', L'\x00b7', L'\x00b8', L'\x00b9', L'\x00ba', L'\x00bb', L'\x00bc', L'\x00bd', L'\x00be', L'\x00bf',
  L'\x00c0', L'\x00c1', L'\x00c2', L'\x00c3', L'\x00c4', L'\x00c5', L'\x00c6', L'\x00c7', L'\x00c8', L'\x00c9', L'\x00ca', L'\x00cb', L'\x00cc', L'\x00cd', L'\x00ce', L'\x00cf',
  L'\x00d0', L'\x00d1', L'\x00d2', L'\x00d3', L'\x00d4', L'\x00d5', L'\x00d6', L'\x00d7', L'\x00d8', L'\x00d9', L'\x00da', L'\x00db', L'\x00dc', L'\x00dd', L'\x00de', L'\x00df',
  L'\x00e0', L'\x00e1', L'\x00e2', L'\x00e3', L'\x00e4', L'\x00e5', L'\x00e6', L'\x00e7', L'\x00e8', L'\x00e9', L'\x00ea', L'\x00eb', L'\x00ec', L'\x00ed', L'\x00ee', L'\x00ef',
  L'\x00f0', L'\x00f1', L'\x00f2', L'\x00f3', L'\x00f4', L'\x00f5', L'\x00f6', L'\x00f7', L'\x00f8', L'\x00f9', L'\x00fa', L'\x00fb', L'\x00fc', L'\x00fd', L'\x00fe', L'\x00ff',
};

static UnicodeToTVCCharMap l_unicode_to_tvc[CHARACTER_NUMBER] = 
{
	{ L'\x00c1', 0x80 },
	{ L'\x00c9', 0x81 },
	{ L'\x00cd', 0x82 },
	{ L'\x00d3', 0x83 },
	{ L'\x00d6', 0x84 },
	{ L'\x00da', 0x86 },
	{ L'\x00dc', 0x87 },
	{ L'\x00e1', 0x90 },
	{ L'\x00e9', 0x91 },
	{ L'\x00ed', 0x92 },
	{ L'\x00f3', 0x93 },
	{ L'\x00f6', 0x94 },
	{ L'\x00fa', 0x96 },
	{ L'\x00fc', 0x97 },
	{ L'\x0150', 0x85 },
	{ L'\x0151', 0x95 },
	{ L'\x0170', 0x88 },
	{ L'\x0171', 0x98 }
};


