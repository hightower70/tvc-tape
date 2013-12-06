/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* BAS file format handler                                                   */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
										
///////////////////////////////////////////////////////////////////////////////
// Include files
#include <stdio.h>
#include "Types.h"
#include "Main.h"
#include "CASFile.h"
#include "BASFile.h"
#include "CharMap.h"
#include "DataBuffer.h"
#include "FileUtils.h"

//////////////////////////////////////////////////////////////////////////////
// Constants
#define BAS_LINEND 0xff // line terminator
#define BAS_PRGEND 0x00 // program terminator

#define BAS_TOKEN_START   0x90
#define BAS_TOKEN_END     0xfe /* 0xff is the terminator, not a token */
#define BAS_TOKEN_DATA    0xfb /* should not tokenize within DATA */
#define BAS_TOKEN_COMMENT 0xfe /* should not tokenize after '!' */
#define BAS_TOKEN_REM     0xfc /* should not tokenize after REM */
#define BAS_TOKEN_COLON   0xfd /* should tokenize after ':' (if not in comment) */

//////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

typedef struct
{
	BYTE LineLength;
  WORD LineNumber;
} BASLINE;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Module global varables
extern const char* l_token_map[];
extern const char* l_char_map[];

///////////////////////////////////////////////////////////////////////////////
// Saves BAS file
bool BASSave(char* in_file_name)
{
	BASLINE* current_line;
	BYTE* current_data_pos;
	BASLINE* next_line;
	FILE* bas_file;
	BYTE* line_data_end;
	int state;
	int current_char;

	// get unique file name
	GenerateUniqueFileName(in_file_name);

	// create BAS file
	bas_file = fopen(in_file_name, "wt");
	if(bas_file == NULL)
	{
		fprintf(stderr, "Can't create output file\n");
		return false;
	}

	// start processing cas file
	current_line = (BASLINE*)g_db_buffer;

	while(((BYTE*)current_line - g_db_buffer) < g_db_buffer_length && current_line->LineLength != BAS_PRGEND) 
	{
		// check basic format
		if(current_line->LineLength < sizeof (BASLINE))
		{
	    fprintf(stderr, "Broken BASIC program\n");
	    return false;
		}	
		
		// set next line pointer
		next_line = (BASLINE*)((BYTE*)current_line + current_line->LineLength);

		// write line number
		fprintf(bas_file, "%4u ", current_line->LineNumber);

		current_data_pos = (BYTE*)current_line + sizeof(*current_line);
		line_data_end = (BYTE*)next_line;
		if(current_data_pos <= line_data_end - 1 && *(line_data_end - 1) == BAS_LINEND) 
			line_data_end--;

		state = 0;
		while(current_data_pos < line_data_end)
		{
			current_char = *current_data_pos;

			// decode token or encode character
			if(state == 0)
			{
				fputs(l_token_map[current_char], bas_file);
			}
			else
			{
				fputs(l_char_map[current_char], bas_file);
			}

	    if(current_char == '"')
			{
				state ^= 1;                     /* macskakörmök között nem kell tokenizálni */
			}
      else if((state&1)==0) 
			{
				if(current_char == BAS_TOKEN_DATA)
				{
					state |= 2;        /* DATA-sorban nem kell tokenizálni */
				}
	      else
				{
					if(current_char == BAS_TOKEN_COLON)
					{
						state &= ~2; /* itt a DATA-sor vége */
					}
					else
					{
						 if(current_char == BAS_TOKEN_COMMENT || current_char == BAS_TOKEN_REM)
						 {
							 state |= 4;  /* megjegyzésben nem kell tokenizálni */
						 }
					}
				}
	    }

			current_data_pos++;
		}


		fprintf (bas_file, "\n");
		current_line = next_line;
	}

	// close BAS file
	fclose(bas_file);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Token and character mapping
static const char* l_token_map[256] =
{
  "Á", "É", "Í",  "Ó", "Ö", "Õ", "Ú", "Ü", "Û", "\\t89", "\\t8a", "\\t8b", "\\t8c", "\\t8d", "\\t8e", "\\t8f",
  "á", "é", "í",  "ó", "ö", "õ", "ú", "ü", "û", "\\t99", "\\t9a", "\\t9b", "\\t9c", "\\t9d", "\\t9e", "\\t9f",

  " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",",    "-", ".", "/",
  "0", "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":", ";", "<",    "=", ">", "?",
  "@", "A", "B",  "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",    "M", "N", "O",
  "P", "Q", "R",  "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\\\", "]", "^", "_",
  "`", "a", "b",  "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",    "m", "n", "o",
  "p", "q", "r",  "s", "t", "u", "v", "w", "x", "y", "z", "{", "|",    "}", "~", "\\t7f",

  "\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87",
  "\\x88", "\\x89", "\\x8a", "\\x8b", "\\x8c", "\\x8d", "\\x8e", "\\x8f",

  "Cannot ",   "No ",       "Bad ",     "rgument",
  " missing",  ")",         "(",        "&",
  "+",         "<",         "=",        "<=",
  ">",         "<>",        ">=",       "^",
  ";",         "/",         "-",        "=<",
  ",",         "><",        "=>",       "#",
  "*",         "TOKEN#A9",  "TOKEN#AA", "POLIGON",
  "RECTANGLE", "ELLIPSE",   "BORDER",   "USING",
  "AT",	       "ATN",       "XOR",      "VOLUME",
  "TO",        "THEN",      "TAB",      "STYLE",
  "STEP",      "RATE",      "PROMPT",   "PITCH",
  "PAPER",     "PALETTE",   "PAINT",    "OR",
  "ORD",       "OFF",       "NOT",      "MODE",
  "INK",       "INKEY$",    "DURATION", "DELAY",
  "CHARACTER", "AND",       "TOKEN#CA", "TOKEN#CB",
  "EXCEPTION", "RENUMBER",  "FKEY",     "AUTO",
  "LPRINT",    "EXT",       "VERIFY",   "TRACE",
  "STOP",      "SOUND",     "SET",      "SAVE",
  "RUN",       "RETURN",    "RESTORE",  "READ",
  "RANDOMIZE", "PRINT",     "POKE",     "PLOT",
  "OUT",       "OUTPUT",    "OPEN",     "ON",
  "OK",        "NEXT",      "NEW",      "LOMEM",
  "LOAD",      "LLIST",     "LIST",     "LET",
  "INPUT",     "IF",        "GRAPHICS", "GOTO",
  "GOSUB",     "GET",       "FOR",      "END",
  "ELSE",      "DIM",       "DELETE",   "DEF",
  "CONTINUE",  "CLS",       "CLOSE",    "DATA",
  "REM",       ":",         "!",        "\\xff"
};

static const char* l_char_map[256] = 
{
  "Á", "É", "Í",  "Ó", "Ö", "Õ", "Ú", "Ü", "Û", "\\t89", "\\t8a", "\\t8b", "\\t8c", "\\t8d", "\\t8e", "\t8f",
  "á", "é", "í",  "ó", "ö", "õ", "ú", "ü", "û", "\\t99", "\\t9a", "\\t9b", "\\t9c", "\\t9d", "\\t9e", "\t9f",

  " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
  "0", "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
  "@", "A", "B",  "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",    "M", "N", "O",
  "P", "Q", "R",  "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\\\", "]", "^", "_",
  "`", "a", "b",  "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",    "m", "n", "o",
  "p", "q", "r",  "s", "t", "u", "v", "w", "x", "y", "z", "{", "|",    "}", "~", "\t7f",

  "\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87",
  "\\x88", "\\x89", "\\x8a", "\\x8b", "\\x8c", "\\x8d", "\\x8e", "\\x8f",
  "\\x90", "\\x91", "\\x92", "\\x93", "\\x94", "\\x95", "\\x96", "\\x97",
  "\\x98", "\\x99", "\\x9a", "\\x9b", "\\x9c", "\\x9d", "\\x9e", "\\x9f",

  "\\ta0", "\\ta1", "\\ta2", "\\ta3", "\\ta4", "\\ta5", "\\ta6", "\\ta7",
  "\\ta8", "\\ta9", "\\taa", "\\tab", "\\tac", "\\tad", "\\tae", "\\taf",
  "\\tb0", "\\tb1", "\\tb2", "\\tb3", "\\tb4", "\\tb5", "\\tb6", "\\tb7",
  "\\tb8", "\\tb9", "\\tba", "\\tbb", "\\tbc", "\\tbd", "\\tbe", "\\tbf",

  "\\tc0", "\\tc1", "\\tc2", "\\tc3", "\\tc4", "\\tc5", "\\tc6", "\\tc7",
  "\\tc8", "\\tc9", "\\tca", "\\tcb", "\\tcc", "\\tcd", "\\tce", "\\tcf",
  "\\td0", "\\td1", "\\td2", "\\td3", "\\td4", "\\td5", "\\td6", "\\td7",
  "\\td8", "\\td9", "\\tda", "\\tdb", "\\tdc", "\\tdd", "\\tde", "\\tdf",

  "\\xe0", "\\xe1", "\\xe2", "\\xe3", "\\xe4", "\\xe5", "\\xe6", "\\xe7",
  "\\xe8", "\\xe9", "\\xea", "\\xeb", "\\xec", "\\xed", "\\xee", "\\xef",
  "\\xf0", "\\xf1", "\\xf2", "\\xf3", "\\xf4", "\\xf5", "\\xf6", "\\xf7",
  "\\xf8", "\\xf9", "\\xfa", "\\xfb", "\\xfc", "\\xfd", "\\xfe", "\\xff"
};
