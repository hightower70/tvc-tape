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
#include "Console.h"

//////////////////////////////////////////////////////////////////////////////
// Constants
#define BAS_LINEND 0xff // line terminator
#define BAS_PRGEND 0x00 // program terminator

#define BAS_TOKEN_DATA    0xfb /* should not tokenize within DATA */
#define BAS_TOKEN_COMMENT 0xfe /* should not tokenize after '!' */
#define BAS_TOKEN_REM     0xfc /* should not tokenize after REM */
#define BAS_TOKEN_COLON   0xfd /* should tokenize after ':' (if not in comment) */

#define LINE_BUFFER_LENGTH 1024

#define MAX_BYTES_IN_A_LINE 16

#define TOKEN_COUNT 128

// status codes
#define ST_TOKENIZING			0
#define ST_QUOTATION			1 // within quotation marks
#define ST_DATA						2 // inside DATA
#define ST_REMARK					4 // inside remark line
#define ST_NON_BASIC			8 // non-basic (binary) part of the file

//////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

typedef struct
{
	BYTE LineLength;
  WORD LineNumber;
} BASLine;

#pragma pack(pop)

typedef struct
{
	int Index;
	int Length;
} TokenLength;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
static bool ParseLine(void);
static BYTE HexDigitToNumber(char in_digit);
static BYTE UnicodeToHex(int in_pos, bool* in_success);
static int TokenLengthCompare(const void* a, const void* b);

///////////////////////////////////////////////////////////////////////////////
// Module global varables
extern const wchar_t* l_tokenized_unicode_char_map[256];
extern const char* l_tokenized_ansi_char_map[256];

static char l_ansi_line_buffer[LINE_BUFFER_LENGTH];
static wchar_t l_unicode_line_buffer[LINE_BUFFER_LENGTH];

static TokenLength l_token_length[128];

static int l_line_number;
static int l_basic_line_parser_state;

///////////////////////////////////////////////////////////////////////////////
// Global variables
TextEncodingType g_bas_encoding = TET_Auto;

///////////////////////////////////////////////////////////////////////////////
// Initialize BAS file handling
void BASInit(void)
{
	int i;

	for(i=0; i<TOKEN_COUNT; i++)
	{
		l_token_length[i].Index = i+128;
		l_token_length[i].Length = strlen(l_tokenized_ansi_char_map[l_token_length[i].Index]);
	}

	qsort(l_token_length, TOKEN_COUNT, sizeof(TokenLength), TokenLengthCompare);
}

///////////////////////////////////////////////////////////////////////////////
// Token length compare function
static int TokenLengthCompare(const void* a, const void* b)
{
	return ( ((TokenLength*)b)->Length - ((TokenLength*)a)->Length );
}

///////////////////////////////////////////////////////////////////////////////
// Loads BAS file
bool BASLoad(wchar_t* in_file_name)
{
	FILE* bas_file;
	wchar_t* open_options;
	TextEncodingType encoding = g_bas_encoding;
	DWORD bom;
	bool success = true;
	int i;

	// determine encoding in the case auto mode
	if(encoding == TET_Auto)
	{
		// read BOOM
		bas_file = _wfopen(in_file_name, L"rb");
		if(bas_file == NULL)
		{
			DisplayError(L"Can't open BAS file\n");
			return false;
		}

		fread(l_ansi_line_buffer, 3, 1, bas_file);

		fclose(bas_file);

		// check the BOM	(Byte Order Mark)
		bom = (DWORD)(BYTE)l_ansi_line_buffer[0] + ((DWORD)(BYTE)l_ansi_line_buffer[1] << 8) + ((DWORD)(BYTE)l_ansi_line_buffer[2] << 16) ;

		if(bom == 0x00BFBBEF)
		{
			encoding = TET_UTF8;
		}
		else
		{
			if((bom & 0xffff) == 0xfeff)
			{
				encoding = TET_UNICODE;
			}
		}
	}

	// set open options
	switch(encoding)
	{
		case TET_ANSI:
			open_options = L"rt";
			break;

		case TET_UTF8:
			open_options = L"rt, ccs=UTF-8";
			break;

		case TET_UNICODE:
			open_options =L"rt, ccs=UNICODE";
			break;

		default:
			return false;
	}

	// parse BAS file
	bas_file = _wfopen(in_file_name, open_options);
	if(bas_file == NULL)
	{
		DisplayError(L"Can't open BAS file\n");
		return false;
	}

	// init
	InitDataBuffer();

	// parse file
	l_basic_line_parser_state = ST_TOKENIZING;
	if(encoding == TET_ANSI)
	{
		l_line_number = 0;
		while(success && fgets(l_ansi_line_buffer, LINE_BUFFER_LENGTH, bas_file) != NULL)
		{
			// convert line to UNICODE
			i = 0;
			while(i < LINE_BUFFER_LENGTH-1 && l_ansi_line_buffer[i] != '\0')
			{
				l_unicode_line_buffer[i] = ANSICharToUNICODEChar(l_ansi_line_buffer[i]);
				i++;
			}
			l_unicode_line_buffer[i] = '\0';

			// parse line
			success = ParseLine();
			l_line_number++;
		}
	}
	else
	{
		l_line_number = 0;
		while(success && fgetws(l_unicode_line_buffer, LINE_BUFFER_LENGTH, bas_file) != NULL)
		{
			// parse line
			success = ParseLine();
			l_line_number++;
		}
	}

	// terminate basic program
	if(success && l_basic_line_parser_state == ST_TOKENIZING)
	{
		g_db_buffer[g_db_buffer_length++] = BAS_PRGEND;
	}

	// display error
	if(!success)
	{
		DisplayError(L"Syntax error at line: %d\n", l_line_number);
	}

	fclose(bas_file);

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Parse basic line
static bool ParseLine(void)
{
	WORD line_number;
	int buffer_index;
	int line_start_index;
	int token_index;
	wchar_t uch;
	bool success = true;
	BYTE current_token;
	int token_pos;
	int i;
	bool store_character;

	// init
	line_start_index = g_db_buffer_length;

	// remove line end characters
	buffer_index = 0;
	l_unicode_line_buffer[LINE_BUFFER_LENGTH-1] = '\0';
	while(l_unicode_line_buffer[buffer_index] != '\0')
	{
		uch = l_unicode_line_buffer[buffer_index];
		if(uch == '\n' ||	uch == '\r')
		{
			l_unicode_line_buffer[buffer_index] = '\0';
			break;
		}
		buffer_index++;
	}

	// skip whitespaces at line start
	buffer_index = 0;
	while(buffer_index < LINE_BUFFER_LENGTH)
	{
		uch = l_unicode_line_buffer[buffer_index];

		// check for empty line
		if(uch == '\0')
			return success;

		// check for non-whitespace
		if(iswalnum(uch))
			break;

		buffer_index++;
	}

	// parse basic lines
	if(l_basic_line_parser_state == ST_TOKENIZING)
	{
		// parse linenumber
		uch = l_unicode_line_buffer[buffer_index];
		if(iswdigit(uch))
		{
			// convert to line number
			line_number = 0;
			while(success && iswdigit(uch))
			{
				line_number = line_number * 10 + (uch - '0');
				if(line_number > 65535)
					success = false;
				uch = l_unicode_line_buffer[++buffer_index];
			}

			// skip whitespaces
			while(success && uch == ' ' || uch == '\t')
				uch = l_unicode_line_buffer[++buffer_index];

			// store line header
			if(success)
			{
				((BASLine*)&g_db_buffer[line_start_index])->LineNumber = line_number;
				g_db_buffer_length += sizeof(BASLine);
			}

			// tokenize line
			while(success && l_unicode_line_buffer[buffer_index] != '\0')
			{
				// check for escape characters
				if(l_unicode_line_buffer[buffer_index] == '\\')
				{
					buffer_index++;
					switch(l_unicode_line_buffer[buffer_index++])
					{																						 
						// store '\'
						case '\\':
							g_db_buffer[g_db_buffer_length++] = '\\';
							break;

						// store character defined by hex number
						case 'X':
						case 'x':
							g_db_buffer[g_db_buffer_length++] = UnicodeToHex(buffer_index, &success);
							buffer_index += 2;
							break;

						default:
							success = false;
							break;
					}
				}
				else
				{
					// tokenize or store characters
					store_character = true;
					if(l_basic_line_parser_state == ST_TOKENIZING)
					{
						// tokenize
						for(i = 0; i < 128 && store_character; i++)
						{
							token_index = l_token_length[i].Index;
							token_pos = 0;
							while(l_tokenized_unicode_char_map[token_index][token_pos] != '\0' && towupper(l_unicode_line_buffer[buffer_index+token_pos]) == l_tokenized_unicode_char_map[token_index][token_pos])
								token_pos++;

							if(l_tokenized_unicode_char_map[token_index][token_pos] == '\0')
							{
								// token found
								current_token = token_index;
								g_db_buffer[g_db_buffer_length++] = token_index;
								buffer_index += token_pos;
								store_character = false;
							}
						}
					}

					// store character without tokenizing
					if(store_character)
					{
						// store
						uch = l_unicode_line_buffer[buffer_index++];
						if(uch > 0x7f)
						{
							// check for hungarian characters
							current_token = (BYTE)UNICODECharToTVCChar(uch);
							if(current_token != '\0')
							{
								g_db_buffer[g_db_buffer_length++] = (BYTE)current_token - 0x80;
							}
							else
								success = false;
						}
						else
						{
							if(uch >= ' ')
							{
								current_token = (BYTE)uch;
								g_db_buffer[g_db_buffer_length++] = (BYTE)uch;
							}
							else
								success = false;
						}
					}

					// update status
					if(current_token == '"')
					{
						l_basic_line_parser_state ^= ST_QUOTATION;
					}
					else
					{
						if((l_basic_line_parser_state & ST_QUOTATION)==0) 
						{
							if(current_token == BAS_TOKEN_DATA)
							{
								l_basic_line_parser_state |= ST_DATA;
							}
							else
							{
								if(current_token == BAS_TOKEN_COLON)
								{
									l_basic_line_parser_state &= ~ST_DATA;
								}
								else
								{
									 if(current_token == BAS_TOKEN_COMMENT || current_token == BAS_TOKEN_REM)
									 {
										 l_basic_line_parser_state |= ST_REMARK;
									 }
								}
							}
						}
					}
				}
			}

			// add line terminator
			if(success)
				g_db_buffer[g_db_buffer_length++] = BAS_LINEND;

			// check and update line length
			if((g_db_buffer_length - line_start_index) > 252)
				success = false;
			else
				((BASLine*)&g_db_buffer[line_start_index])->LineLength = g_db_buffer_length - line_start_index;

			// quotes should be in pair
			if((l_basic_line_parser_state & ST_QUOTATION) != 0 && (l_basic_line_parser_state & ST_REMARK) == 0)
			{
				success = false;
			}
			else
			{
				// reset state
				l_basic_line_parser_state = ST_TOKENIZING;
			}
		}
		else
		{
			// terminate basic program and switch to non basic part parsing
			g_db_buffer[g_db_buffer_length++] = BAS_PRGEND;
			l_basic_line_parser_state = ST_NON_BASIC;
		}
	}

	// process non basic (binary part)
	if(l_basic_line_parser_state == ST_NON_BASIC)
	{
		// bytesoffset command
		if(_wcsnicmp(&l_unicode_line_buffer[buffer_index], L"BYTESOFFSET", 11) == 0)
		{
			// ignore this command
		}
		else
		{
			// autostart command
			if(_wcsnicmp(&l_unicode_line_buffer[buffer_index], L"AUTOSTART", 9) == 0)
			{
				g_db_autostart = true;
			}
			else
			{
				// bytes command
				if(_wcsnicmp(&l_unicode_line_buffer[buffer_index], L"BYTES", 5) == 0)
				{
					buffer_index += 5;

					// skip whitespaces
					while(success && l_unicode_line_buffer[buffer_index] == ' ' || l_unicode_line_buffer[buffer_index] == '\t')
						buffer_index++;

					// convert escape characters
					while(success && l_unicode_line_buffer[buffer_index] != '\0')
					{
						if(l_unicode_line_buffer[buffer_index] == '\\' && towupper(l_unicode_line_buffer[buffer_index+1]) == 'X')
						{
							buffer_index += 2;
							g_db_buffer[g_db_buffer_length++] = UnicodeToHex(buffer_index, &success);
							buffer_index += 2;
						}
						else
							success = false;
					}
				}
				else
					success = false;
			}
		}
	}	 

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Convert ASCII hex digit to value
static BYTE HexDigitToNumber(char in_digit)
{
	if(in_digit >= '0' && in_digit <= '9')
		return in_digit - '0';

	if(in_digit >= 'a' && in_digit <= 'f')
		return in_digit - 'a' + 10;

	if(in_digit >= 'A' && in_digit <= 'F')
		return in_digit - 'A' + 10;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Converts Ansi hex characters to byte
static BYTE UnicodeToHex(int in_pos, bool* in_success)
{
	if(!(*in_success))
		return 0;

	if(isxdigit(l_unicode_line_buffer[in_pos]) && isxdigit(l_unicode_line_buffer[in_pos+1]))
	{
		return (HexDigitToNumber((char)l_unicode_line_buffer[in_pos]) << 4) + HexDigitToNumber((char)l_unicode_line_buffer[in_pos+1]);
	}
	else
	{
		*in_success = false;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Saves BAS file
bool BASSave(wchar_t* in_file_name)
{
	BASLine* current_line;
	BYTE* current_data_pos;
	BASLine* next_line;
	FILE* bas_file;
	BYTE* line_data_end;
	int state;
	int current_char;
	wchar_t* open_options;
	int remaining_byte_index;
	int bytes_in_a_line;

	// set open options based on encoding type
	switch(g_bas_encoding)
	{
		case TET_ANSI:
			open_options = L"wt";
			break;

		case TET_Auto:
		case TET_UTF8:
			open_options = L"wt, ccs=UTF-8";
			break;

		case TET_UNICODE:
			open_options =L"wt, ccs=UNICODE";
			break;
	}

	// create BAS file
	bas_file = _wfopen(in_file_name, open_options);
	
	if(bas_file == NULL)
	{
		DisplayError(L"Can't create BAS file\n");
		return false;
	}

	// start processing cas file
	current_line = (BASLine*)g_db_buffer;

	while(((BYTE*)current_line - g_db_buffer) < g_db_buffer_length && current_line->LineLength != BAS_PRGEND) 
	{
		// check basic format
		if(current_line->LineLength < sizeof (BASLine))
		{
			// invalid basoc program ->stop conversion
			if(g_bas_encoding == TET_ANSI)
			{
				fputs("*** Broken BASIC program\n", bas_file);
				break;
			}
			else
			{
				fputws(L"*** Broken BASIC program\n", bas_file);
				break;
			}
		}	
		
		// set next line pointer
		next_line = (BASLine*)((BYTE*)current_line + current_line->LineLength);

		// write line number
		if(g_bas_encoding == TET_ANSI)
		{
			fprintf(bas_file, "%4u ", current_line->LineNumber);
		}
		else
		{
			fwprintf(bas_file, L"%4u ", current_line->LineNumber);
		}

		// decompress line
		current_data_pos = (BYTE*)current_line + sizeof(*current_line);
		line_data_end = (BYTE*)next_line;
		if(current_data_pos <= line_data_end - 1 && *(line_data_end - 1) == BAS_LINEND) 
			line_data_end--;

		state = ST_TOKENIZING;
		while(current_data_pos < line_data_end)
		{
			current_char = *current_data_pos;

			// decode token or character
			if(state == ST_TOKENIZING)
			{
				// store tokenized item
				if(g_bas_encoding == TET_ANSI)
				{
					fputs(l_tokenized_ansi_char_map[current_char], bas_file);
				}
				else
				{
					fputws(l_tokenized_unicode_char_map[current_char], bas_file);
				}
			}
			else
			{
				// store non tokenized item
				if(g_bas_encoding == TET_ANSI)
				{
					if(current_char < 0x80)
						fputs(l_tokenized_ansi_char_map[current_char], bas_file);
					else
						fprintf(bas_file, "\\x%02x", current_char);
				}
				else
				{
					if(current_char < 0x80)
						fputws(l_tokenized_unicode_char_map[current_char], bas_file);
					else
						fwprintf(bas_file, L"\\x%02x", current_char);
				}
			}

			// update status
	    if(current_char == '"')
			{
				state ^= ST_QUOTATION;
			}
      else
			{
				if((state & ST_QUOTATION)==0) 
				{
					if(current_char == BAS_TOKEN_DATA)
					{
						state |= ST_DATA;
					}
					else
					{
						if(current_char == BAS_TOKEN_COLON)
						{
							state &= ~ST_DATA;
						}
						else
						{
							 if(current_char == BAS_TOKEN_COMMENT || current_char == BAS_TOKEN_REM)
							 {
								 state |= ST_REMARK;
							 }
						}
					}
				}
	    }

			current_data_pos++;
		}

		if(g_bas_encoding == TET_ANSI)
			fprintf(bas_file, "\n");
		else
			fwprintf(bas_file, L"\n");

		current_line = next_line;
	}

	// write remaining data offset
	remaining_byte_index = ((BYTE*)current_line - g_db_buffer) + 1; // +1 beacuse of the BAS_PRGEND byte
	if(remaining_byte_index < g_db_buffer_length)
	{
		if(g_bas_encoding == TET_ANSI)
			fprintf(bas_file, "BYTESOFFSET %d\n", remaining_byte_index); 
		else
			fwprintf(bas_file, L"BYTESOFFSET %d\n", remaining_byte_index);
	}

	// write remaining data
	bytes_in_a_line = 0;
	while(remaining_byte_index < g_db_buffer_length)
	{
		if(bytes_in_a_line == 0)
		{
			if(g_bas_encoding == TET_ANSI)
			{
				fprintf(bas_file, "BYTES ");
			}
			else
			{
				fwprintf(bas_file, L"BYTES ");
			}
		}

		// write data
		if(g_bas_encoding == TET_ANSI)
		{
			fprintf(bas_file, "\\x%02x", g_db_buffer[remaining_byte_index]);
		}
		else
		{
			fwprintf(bas_file, L"\\x%02x", g_db_buffer[remaining_byte_index]);
		}

		remaining_byte_index++;
		bytes_in_a_line++;

		// write new line
		if(bytes_in_a_line > MAX_BYTES_IN_A_LINE)
		{
			if(g_bas_encoding == TET_ANSI)
				fprintf(bas_file, "\n");
			else
				fwprintf(bas_file, L"\n");

			bytes_in_a_line = 0;
		}
	}
	// new line
	if(bytes_in_a_line > 0)
	{
		if(g_bas_encoding == TET_ANSI)
			fprintf(bas_file, "\n");
		else
			fwprintf(bas_file, L"\n");
	}

	// write autostart
	if(g_db_autostart)
	{
		if(g_bas_encoding == TET_ANSI)
		{
				fprintf(bas_file, "AUTOSTART");
		}
		else
		{
			fwprintf(bas_file, L"AUTOSTART");
		}
	}

	// close BAS file
	fclose(bas_file);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Find end of the basic instuctions
int BASFindEnd(void)
{
	int pos = 0;
	bool basic_end_found = false;

	while(pos < g_db_buffer_length && !basic_end_found)
	{
		// check current line length
		if(g_db_buffer[pos] == BAS_PRGEND)
		{
			basic_end_found = true;
			pos++;
		}
		else
		{
			// next line
			pos += g_db_buffer[pos];
		}
	}

	if(basic_end_found)
		return pos;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Token tables
static const char* l_tokenized_ansi_char_map[256] =
{
// 			0		    1		    2			  3		    4       5       6       7		    8			   9       a         b        c        d				e        f
//	0x 'Á',    'É',    'Í',    'Ó',    'Ö',    'Õ',    'Ú',    'Ü',    'Û',  '\x09', '\x0a',   '\x0b',  '\x0c',  '\x0d',  '\x0e',  '\x0f',
//  0x 'á',    'é',    'í',    'ó',    'ö',    'õ',    'ú',    'ü',    'û',  '\x19', '\x1a',   '\x1b',  '\x1c',  '\x1d',  '\x1e',  '\x1f',
	  "\xc1", "\xc9", "\xcd", "\xd3", "\xd6", "\xd5", "\xda", "\xdc", "\xdb", "\\x09", "\\x0a", "\\x0b", "\\x0c", "\\x0d", "\\x0e", "\\x0f",
	  "\xe1", "\xe9", "\xed", "\xf3", "\xf6", "\xf5", "\xfa", "\xfc", "\xfb", "\\x19", "\\x1a", "\\x1b", "\\x1c", "\\x1d", "\\x1e", "\\x1f",

		" ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",",    "-", ".", "/",
		"0", "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":", ";", "<",    "=", ">", "?",
		"@", "A", "B",  "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",    "M", "N", "O",
		"P", "Q", "R",  "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\\\", "]", "^", "_",
		"`", "a", "b",  "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",    "m", "n", "o",
		"p", "q", "r",  "s", "t", "u", "v", "w", "x", "y", "z", "{", "|",    "}", "~", "\\x7f",

		"\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87", "\\x88", "\\x89", "\\x8a", "\\x8b", "\\x8c", "\\x8d", "\\x8e", "\\x8f",

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

static const wchar_t* l_tokenized_unicode_char_map[256] =
{
// 			 0		      1		       2          3          4          5          6          7		       8         9         a         b         c         d         e        f
//	0x  'Á',       'É',       'Í',       'Ó',       'Ö',       'Õ',       'Ú',       'Ü',       'Û',   '\x09',   '\x0a',   '\x0b',   '\x0c',   '\x0d',   '\x0e',   '\x0f',
//  0x  'á',       'é',       'í',       'ó',       'ö',       'õ',       'ú',       'ü',       'û',   '\x19',   '\x1a',   '\x1b',   '\x1c',   '\x1d',   '\x1e',   '\x1f',
	L"\x00c1", L"\x00c9", L"\x00cd", L"\x00d3", L"\x00d6", L"\x0150", L"\x00da", L"\x00dc", L"\x0170", L"\\x09", L"\\x0a", L"\\x0b", L"\\x0c", L"\\x0d", L"\\x0e", L"\\x0f",
	L"\x00e1", L"\x00e9", L"\x00ed", L"\x00f3", L"\x00f6", L"\x0151", L"\x00fa", L"\x00fc", L"\x0171", L"\\x19", L"\\x1a", L"\\x1b", L"\\x1c", L"\\x1d", L"\\x1e", L"\\x1f",

  L" ", L"!", L"\"", L"#", L"$", L"%", L"&", L"'", L"(", L")", L"*", L"+", L",",    L"-", L".", L"/",
  L"0", L"1", L"2",  L"3", L"4", L"5", L"6", L"7", L"8", L"9", L":", L";", L"<",    L"=", L">", L"?",
  L"@", L"A", L"B",  L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L",    L"M", L"N", L"O",
  L"P", L"Q", L"R",  L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z", L"[", L"\\\\", L"]", L"^", L"_",
  L"`", L"a", L"b",  L"c", L"d", L"e", L"f", L"g", L"h", L"i", L"j", L"k", L"l",    L"m", L"n", L"o",
  L"p", L"q", L"r",  L"s", L"t", L"u", L"v", L"w", L"x", L"y", L"z", L"{", L"|",    L"}", L"~", L"\\x7f",

  L"\\x80", L"\\x81", L"\\x82", L"\\x83", L"\\x84", L"\\x85", L"\\x86", L"\\x87", L"\\x88", L"\\x89", L"\\x8a", L"\\x8b", L"\\x8c", L"\\x8d", L"\\x8e", L"\\x8f",

  L"Cannot ",   L"No ",       L"Bad ",     L"rgument",
  L" missing",  L")",         L"(",        L"&",
  L"+",         L"<",         L"=",        L"<=",
  L">",         L"<>",        L">=",       L"^",
  L";",         L"/",         L"-",        L"=<",
  L",",         L"><",        L"=>",       L"#",
  L"*",         L"TOKEN#A9",  L"TOKEN#AA", L"POLIGON",
  L"RECTANGLE", L"ELLIPSE",   L"BORDER",   L"USING",
  L"AT",	       L"ATN",       L"XOR",      L"VOLUME",
  L"TO",        L"THEN",      L"TAB",      L"STYLE",
  L"STEP",      L"RATE",      L"PROMPT",   L"PITCH",
  L"PAPER",     L"PALETTE",   L"PAINT",    L"OR",
  L"ORD",       L"OFF",       L"NOT",      L"MODE",
  L"INK",       L"INKEY$",    L"DURATION", L"DELAY",
  L"CHARACTER", L"AND",       L"TOKEN#CA", L"TOKEN#CB",
  L"EXCEPTION", L"RENUMBER",  L"FKEY",     L"AUTO",
  L"LPRINT",    L"EXT",       L"VERIFY",   L"TRACE",
  L"STOP",      L"SOUND",     L"SET",      L"SAVE",
  L"RUN",       L"RETURN",    L"RESTORE",  L"READ",
  L"RANDOMIZE", L"PRINT",     L"POKE",     L"PLOT",
  L"OUT",       L"OUTPUT",    L"OPEN",     L"ON",
  L"OK",        L"NEXT",      L"NEW",      L"LOMEM",
  L"LOAD",      L"LLIST",     L"LIST",     L"LET",
  L"INPUT",     L"IF",        L"GRAPHICS", L"GOTO",
  L"GOSUB",     L"GET",       L"FOR",      L"END",
  L"ELSE",      L"DIM",       L"DELETE",   L"DEF",
  L"CONTINUE",  L"CLS",       L"CLOSE",    L"DATA",
  L"REM",       L":",         L"!",        L"\\xff"
};

