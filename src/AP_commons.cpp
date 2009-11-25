//==================================================================//
/*
    AtomicParsley - AP_commons.cpp

    AtomicParsley is GPL software; you can freely distribute, 
    redistribute, modify & use under the terms of the GNU General
    Public License; either version 2 or its successor.

    AtomicParsley is distributed under the GPL "AS IS", without
    any warranty; without the implied warranty of merchantability
    or fitness for either an expressed or implied particular purpose.

    Please see the included GNU General Public License (GPL) for 
    your rights and further details; see the file COPYING. If you
    cannot, write to the Free Software Foundation, 59 Temple Place
    Suite 330, Boston, MA 02111-1307, USA.  Or www.fsf.org

    Copyright ©2006-2007 puck_lock
		
		----------------------
    Code Contributions by:
		
    * SLarew - prevent writing past array in Convert_multibyteUTF16_to_wchar bugfix
		
																																		*/
//==================================================================//

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <errno.h>

#if defined (WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "AP_commons.h"
#include "AP_iconv.h"
#include "AtomicParsley.h"


///////////////////////////////////////////////////////////////////////////////////////
//                               Filesytem routines                                  //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
findFileSize
  utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file

    take an ascii/utf8 filepath (which if under a unicode enabled Win32 OS was already converted from utf16le to utf8 at program start) and test if
		AP is running on a unicode enabled Win32 OS. If it is and converted to utf8 (rather than just stripped), convert the utf8 filepath to a utf16 
		(native-endian) filepath & pass that to a wide stat. Or stat it with a utf8 filepath on Unixen & win32 (stripped utf8).
----------------------*/
off_t findFileSize(const char *utf8_filepath) {
	if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
		wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(utf8_filepath);
		
		struct _stat fileStats;
		_wstat(utf16_filepath, &fileStats);
		
		free(utf16_filepath);
		utf16_filepath = NULL;
		return fileStats.st_size;
#endif
	} else {
		struct stat fileStats;
		stat(utf8_filepath, &fileStats);
		return fileStats.st_size;
	}
	return 0; //won't ever get here.... unless this is win32, set to utf8 and the folder/file had unicode.... TODO (? use isUTF8() for high ascii?)
}

/*----------------------
APar_OpenFile
  utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file
	file_flags - 3 bytes max for the flags to open the file with (read, write, binary mode....)

    take an ascii/utf8 filepath (which if under a unicode enabled Win32 OS was already converted from utf16le to utf8 at program start) and test if
		AP is running on a unicode enabled Win32 OS. If it is, convert the utf8 filepath to a utf16 (native-endian) filepath & pass that to a wide fopen
		with the 8-bit file flags changed to 16-bit file flags. Or open a utf8 file with vanilla fopen on Unixen.
----------------------*/
FILE* APar_OpenFile(const char* utf8_filepath, const char* file_flags) {
	FILE* aFile = NULL;
	if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
		wchar_t* Lfile_flags = (wchar_t *)malloc(sizeof(wchar_t)*4);
		memset(Lfile_flags, 0, sizeof(wchar_t)*4);
		mbstowcs(Lfile_flags, file_flags, strlen(file_flags) );
		
		wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(utf8_filepath);
		
		aFile = _wfopen(utf16_filepath, Lfile_flags);
		
		free(utf16_filepath);
		utf16_filepath = NULL;
#endif
	} else {
		aFile = fopen(utf8_filepath, file_flags);
	}
	
	if (!aFile) {
		fprintf(stdout, "AP error trying to fopen: %s\n", strerror(errno));
	}
	return aFile;
}

/*----------------------
openSomeFile
  utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file
	open - flag to either open or close (function does both)

    take an ascii/utf8 filepath and either open or close it; used for the main ISO Base Media File; store the resulting FILE* in a global source_file
----------------------*/
FILE* APar_OpenISOBaseMediaFile(const char* utf8file, bool open) {
	if ( open && !file_opened) {
		source_file = APar_OpenFile(utf8file, "rb");
		if (source_file != NULL) {
			file_opened = true;
		}
	} else {
		fclose(source_file);
		file_opened = false;
	}
	return source_file;
}

void TestFileExistence(const char *filePath, bool errorOut) {
	FILE *a_file = NULL;
	a_file = APar_OpenFile(filePath, "rb");
	if( (a_file == NULL) && errorOut ){
		fprintf(stderr, "AtomicParsley error: can't open %s for reading: %s\n", filePath, strerror(errno));
		exit(1);
	} else {
		fclose(a_file);
	}
}


#if defined (_MSC_VER)

///////////////////////////////////////////////////////////////////////////////////////
//                                Win32 functions                                    //
///////////////////////////////////////////////////////////////////////////////////////

int fseeko(FILE *stream, uint64_t pos, int whence) { //only using SEEK_SET here
	if (whence == SEEK_SET) {
		fpos_t fpos = pos;
		return fsetpos(stream, &fpos);
	} else {
		return -1;
	}
	return -1;
}
#endif


// http://www.flipcode.com/articles/article_advstrings01.shtml
bool IsUnicodeWinOS() {
#if defined (_MSC_VER)
  OSVERSIONINFOW		os;
  memset(&os, 0, sizeof(OSVERSIONINFOW));
  os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  return (GetVersionExW(&os) != 0);
#else
	return false;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
//                             File reading routines                                 //
///////////////////////////////////////////////////////////////////////////////////////

uint8_t APar_read8(FILE* ISObasemediafile, uint32_t pos) {
	uint8_t a_byte = 0;
	fseeko(ISObasemediafile, pos, SEEK_SET);
	fread(&a_byte, 1, 1, ISObasemediafile);
	return a_byte;
}

uint16_t APar_read16(char* buffer, FILE* ISObasemediafile, uint32_t pos) {
	fseeko(ISObasemediafile, pos, SEEK_SET);
	fread(buffer, 1, 2, ISObasemediafile);
	return UInt16FromBigEndian(buffer);
}

uint32_t APar_read32(char* buffer, FILE* ISObasemediafile, uint32_t pos) {
	fseeko(ISObasemediafile, pos, SEEK_SET);
	fread(buffer, 1, 4, ISObasemediafile);
	return UInt32FromBigEndian(buffer);
}

uint64_t APar_read64(char* buffer, FILE* ISObasemediafile, uint32_t pos) {
	fseeko(ISObasemediafile, pos, SEEK_SET);
	fread(buffer, 1, 8, ISObasemediafile);
	return UInt64FromBigEndian(buffer);
}

void APar_readX(char* buffer, FILE* ISObasemediafile, uint32_t pos, uint32_t length) {
	fseeko(ISObasemediafile, pos, SEEK_SET);
	fread(buffer, 1, length, ISObasemediafile);
	return;
}

uint32_t APar_ReadFile(char* destination_buffer, FILE* a_file, uint32_t bytes_to_read) {
	uint32_t bytes_read = 0;
	if (destination_buffer != NULL) {
		fseeko(a_file, 0, SEEK_SET); // not that 2gb support is required - malloc would probably have a few issues
		bytes_read = (uint32_t)fread(destination_buffer, 1, (size_t)bytes_to_read, a_file);
		file_size += bytes_read; //accommodate huge files embedded within small files for APar_Validate
	}
	return bytes_read;
}

uint32_t APar_FindValueInAtom(char* uint32_buffer, FILE* ISObasemediafile, short an_atom, uint32_t start_position, uint32_t eval_number) {
	uint32_t current_pos = start_position;
	memset(uint32_buffer, 0, 5);
	while (current_pos <= parsedAtoms[an_atom].AtomicLength) {
		current_pos ++;
		if (eval_number > 65535) {
			//current_pos +=4;
			if (APar_read32(uint32_buffer, ISObasemediafile, parsedAtoms[an_atom].AtomicStart + current_pos) == eval_number) {
				break;
			}
		} else {
			//current_pos +=2;
			if (APar_read16(uint32_buffer, ISObasemediafile, parsedAtoms[an_atom].AtomicStart + current_pos) == (uint16_t)eval_number) {
				break;
			}
		}
		if (current_pos >= parsedAtoms[an_atom].AtomicLength) {
			current_pos =  0;
			break;
		}
	}
	return current_pos;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                Language specifics                                 //
///////////////////////////////////////////////////////////////////////////////////////

void APar_UnpackLanguage(unsigned char lang_code[], uint16_t packed_language) {
	lang_code[3] = 0;
	lang_code[2] = (packed_language &0x1F) + 0x60;
	lang_code[1] = ((packed_language >> 5) &0x1F) + 0x60;
	lang_code[0] = ((packed_language >> 10) &0x1F) + 0x60;
	return;
}

uint16_t PackLanguage(const char* language_code, uint8_t lang_offset) { //?? is there a problem here? und does't work http://www.w3.org/WAI/ER/IG/ert/iso639.htm
	//I think Apple's 3gp asses decoder is a little off. First, it doesn't support a lot of those 3 letter language codes above on that page. for example 'zul' blocks *all* metadata from showing up. 'fre' is a no-no, but 'fra' is fine.
	//then, the spec calls for all strings to be null terminated. So then why does a '© 2005' (with a NULL at the end) show up as '© 2005' in 'pol', but '© 2005 ?' in 'fas' Farsi? Must be Apple's implementation, because the files are identical except for the uint16_t lang setting.
	
	uint16_t packed_language = 0;
	
	//fprintf(stdout, "%i, %i, %i\n", language_code[0+lang_offset], language_code[1+lang_offset], language_code[2+lang_offset]);
	
	if (language_code[0+lang_offset] < 97 || language_code[0+lang_offset] > 122 ||
			language_code[1+lang_offset] < 97 || language_code[1+lang_offset] > 122 ||
			language_code[2+lang_offset] < 97 || language_code[2+lang_offset] > 122) {
			
		return packed_language;
	}
	
	packed_language = (((language_code[0+lang_offset] - 0x60) & 0x1F) << 10 ) | 
	                   ((language_code[1+lang_offset] - 0x60) & 0x1F) << 5  | 
										  (language_code[2+lang_offset] - 0x60) & 0x1F;
	return packed_language;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                platform specifics                                 //
///////////////////////////////////////////////////////////////////////////////////////

#if (!defined HAVE_LROUNDF) && (!defined (__GLIBC__))
int lroundf(float a) {
	return a/1;
}
#endif

#ifndef HAVE_STRSEP
// use glibc's strsep only on windows when cygwin & libc are undefined; otherwise the internal strsep will be used
// This marks the point where a ./configure & makefile combo would make this easier

/* Copyright (C) 1992, 93, 96, 97, 98, 99, 2004 Free Software Foundation, Inc.
   This strsep function is part of the GNU C Library - v2.3.5; LGPL.
*/

char *strsep (char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL;

  //A frequent case is when the delimiter string contains only one character.  Here we don't need to call the expensive `strpbrk' function and instead work using `strchr'.
  if (delim[0] == '\0' || delim[1] == '\0')
    {
      char ch = delim[0];

      if (ch == '\0')
	end = NULL;
      else
	{
	  if (*begin == ch)
	    end = begin;
	  else if (*begin == '\0')
	    end = NULL;
	  else
	    end = strchr (begin + 1, ch);
	}
    }
  else

    end = strpbrk (begin, delim); //Find the end of the token.

  if (end)
    {
      *end++ = '\0'; //Terminate the token and set *STRINGP past NUL character.
      *stringp = end;
    }
  else
    *stringp = NULL; //No more delimiters; this is the last token.

  return begin;
}
#endif

void determine_MonthDay(int literal_day, int &month, int &day) {
	if (literal_day <= 31) {
		month = 1; day = literal_day;
	
	} else if (literal_day <= 59) {
		month = 2; day = literal_day - 31;
	
	} else if (literal_day <= 90) {
		month = 3; day = literal_day - 59;
	
	} else if (literal_day <= 120) {
		month = 4; day = literal_day - 90;
	
	} else if (literal_day <= 151) {
		month = 5; day = literal_day - 120;
	
	} else if (literal_day <= 181) {
		month = 6; day = literal_day - 151;
	
	} else if (literal_day <= 212) {
		month = 7; day = literal_day - 181;
		
	} else if (literal_day <= 243) {
		month = 8; day = literal_day - 212;
	
	} else if (literal_day <= 273) {
		month = 9; day = literal_day - 243;
	
	} else if (literal_day <= 304) {
		month = 10; day = literal_day - 273;
	
	} else if (literal_day <= 334) {
		month = 11; day = literal_day - 304;
	
	} else if (literal_day <= 365) {
		month = 12; day = literal_day - 334;
	}
	return;
}

char* APar_gmtime64(uint64_t total_secs, char* utc_time) {	
	//this will probably be off between Jan 1 & Feb 28 on a leap year by a day.... I'll somehow cope & deal.
	struct tm timeinfo = {0,0,0,0,0};

	int offset_year = total_secs / 31536000; //60 * 60 * 24 * 365 (ordinary year in seconds; doesn't account for leap year)
	int literal_year = 1904 + offset_year;
	int literal_days_into_year = ((total_secs % 31536000) / 86400) - (offset_year / 4); //accounts for the leap year
	
	uint32_t literal_seconds_into_day = total_secs % 86400;
	
	int month =  0;
	int days = 0;
	
	determine_MonthDay(literal_days_into_year, month, days);
	
	if (literal_days_into_year < 0 ) {
		literal_year -=1;
		literal_days_into_year = 31 +literal_days_into_year;
		month = 12;
		days = literal_days_into_year;
	}
	
	int hours = literal_seconds_into_day / 3600;
	
	timeinfo.tm_year = literal_year - 1900;
	timeinfo.tm_yday = literal_days_into_year;
	timeinfo.tm_mon = month - 1;
	timeinfo.tm_mday = days;
	timeinfo.tm_wday = (((total_secs / 86400) - (offset_year / 4)) - 5 ) % 7;
	
	timeinfo.tm_hour = hours;
	timeinfo.tm_min = (literal_seconds_into_day - (hours * 3600)) / 60;
	timeinfo.tm_sec = (int)(literal_seconds_into_day % 60);
	
	strftime(utc_time, 50 , "%a %b %d %H:%M:%S %Y", &timeinfo);
	return utc_time;
}

/*----------------------
ExtractUTC
  total_secs - the time in seconds (from Jan 1, 1904)

    Convert the seconds to a calendar date with seconds.
----------------------*/
char* APar_extract_UTC(uint64_t total_secs) {
	//2082844800 seconds between 01/01/1904 & 01/01/1970
	//  2,081,376,000 (60 seconds * 60 minutes * 24 hours * 365 days * 66 years)
	//    + 1,468,800 (60 * 60 * 24 * 17 leap days in 01/01/1904 to 01/01/1970 duration) 
	//= 2,082,844,800
	static char utc_time[50];
	memset(utc_time, 0, 50);
		
	if (total_secs > MAXTIME_32) {
		return APar_gmtime64(total_secs, utc_time);
	} else {
		if (total_secs < 2082844800) {
			return APar_gmtime64(total_secs, utc_time); //less than Unix epoch
		} else {
			total_secs -= 2082844800;
			uint32_t reduced_seconds = (uint32_t)total_secs;
			strftime(*&utc_time, 50 , "%a %b %d %H:%M:%S %Y", gmtime((time_t*)&reduced_seconds) );
			return *&utc_time;
		}
	}
	return *&utc_time;
}

uint32_t APar_get_mpeg4_time() {
#if defined(WIN32)
	FILETIME  file_time;
	uint64_t wintime = 0;
	GetSystemTimeAsFileTime (&file_time);
	wintime = (((uint64_t) file_time.dwHighDateTime << 32) | file_time.dwLowDateTime) / 10000000;
	wintime -= 9561628800;
	return (uint32_t)wintime;

#else
	uint32_t current_time_in_seconds = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	current_time_in_seconds = tv.tv_sec;
	return current_time_in_seconds + 2082844800;

#endif
	return 0;
}


/*----------------------
APar_StandardTime
	formed_time - the destination string

    Print the ISO 8601 Coordinated Universal Time (UTC) timestamp (in YYYY-MM-DDTHH:MM:SSZ form)
----------------------*/
void APar_StandardTime(char* &formed_time) {
  time_t rawtime;
  struct tm *timeinfo;

  time (&rawtime);
  timeinfo = gmtime (&rawtime);
	strftime(formed_time ,100 , "%Y-%m-%dT%H:%M:%SZ", timeinfo); //that hanging Z is there; denotes the UTC
	
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                     strings                                       //
///////////////////////////////////////////////////////////////////////////////////////

wchar_t* Convert_multibyteUTF16_to_wchar(char* input_unicode, size_t glyph_length, bool skip_BOM) { //TODO: is this like wcstombs?
	int BOM_mark_bytes = 0;
	if (skip_BOM) {
		BOM_mark_bytes = 2;
	}
	
	wchar_t* utf16_data = (wchar_t*)malloc( sizeof(wchar_t)* glyph_length ); //just to be sure there will be a trailing NULL
	wmemset(utf16_data, 0, glyph_length);
						
	for(size_t i = 0; i < glyph_length; i++) {
#if defined (__ppc__) || defined (__ppc64__)
		utf16_data[i] = (input_unicode[2*i + BOM_mark_bytes] & 0x00ff) << 8 | (input_unicode[2*i + 1 + BOM_mark_bytes]) << 0; //+2 & +3 to skip over the BOM
#else
		utf16_data[i] = (input_unicode[2*i + BOM_mark_bytes] << 8) | ((input_unicode[2*i + 1 + BOM_mark_bytes]) & 0x00ff) << 0; //+2 & +3 to skip over the BOM
#endif
	}
	return utf16_data;
}

unsigned char* Convert_multibyteUTF16_to_UTF8(char* input_utf16, size_t glyph_length, size_t byte_count) {
	unsigned char* utf8_data = (unsigned char*)malloc(sizeof(unsigned char)* glyph_length );
	memset(utf8_data, 0, glyph_length);
						
	UTF16BEToUTF8(utf8_data, glyph_length, (unsigned char*)input_utf16 + 2, byte_count);
	return utf8_data;
}

wchar_t* Convert_multibyteUTF8_to_wchar(const char* input_utf8) { //TODO: is this like mbstowcs?
	size_t string_length = strlen(input_utf8) + 1;  //account for terminating NULL
	size_t char_glyphs = mbstowcs(NULL, input_utf8, string_length); //passing NULL pre-calculates the size of wchar_t needed
			
	unsigned char* utf16_conversion = (unsigned char*)malloc( sizeof(unsigned char)* string_length * 2 );
	memset(utf16_conversion, 0, string_length * 2 );
			
	int utf_16_glyphs = UTF8ToUTF16BE(utf16_conversion, char_glyphs * 2, (unsigned char*)input_utf8, string_length);
	return Convert_multibyteUTF16_to_wchar((char*)utf16_conversion, (size_t)utf_16_glyphs, false );
}

//these flags from id3v2 2.4
//0x00 = ISO-8859-1 & terminate with 0x00.
//0x01 = UTF-16 with BOM. All frames have same encoding & terminate with 0x0000. 
//0x02 = UTF-16BE without BOM & terminate with 0x0000.
//0x03 = UTF-8 & terminated with 0x00.
//buffer can hold either ut8 or utf16 carried on 8-bit char which requires a cast	
/*----------------------
findstringNULLterm
  in_string - pointer to location of a string (can be either 8859-1, utf8 or utf16be/utf16be needing a cast to wchar)
	encodingFlag - used to denote the encoding of instring (derived from id3v2 2.4 encoding flags)
	max_len - the length of given string - there may be no NULL terminaiton, in which case it will only count to max_len

    Either find the NULL if it exists and return how many bytes into in_string that NULL exists, or it won't find a NULL and return max_len
----------------------*/
uint32_t findstringNULLterm (char* in_string, uint8_t encodingFlag, uint32_t max_len) {
	uint32_t byte_count = 0;
	
	if (encodingFlag == 0x00 || encodingFlag == 0x03) {
		char* bufptr = in_string;
		while (bufptr <= in_string+max_len) {
			if (*bufptr == 0x00) {
				break;
			}
			bufptr++;
			byte_count++;
		}		
	} else if ((encodingFlag == 0x01 || encodingFlag == 0x02) && max_len >= 2) {
		short wbufptr;
		while (byte_count <= max_len) {
			wbufptr = (*(in_string+byte_count) << 8) | *(in_string+byte_count+1);
			if (wbufptr == 0x0000) {
				break;
			}
			byte_count+=2;
		}	
	}
	if (byte_count > max_len) return max_len;
	return byte_count;
}

uint32_t skipNULLterm (char* in_string, uint8_t encodingFlag, uint32_t max_len) {
	uint32_t byte_count = 0;
	
	if (encodingFlag == 0x00 || encodingFlag == 0x03) {
		char* bufptr = in_string;
		while (bufptr <= in_string+max_len) {
			if (*bufptr == 0x00) {
				byte_count++;
				break;
			}
			bufptr++;
		}		
	} else if ((encodingFlag == 0x01 || encodingFlag == 0x02) && max_len >= 2) {
		short wbufptr;
		while (byte_count <= max_len) {
			wbufptr = (*(in_string+byte_count) << 8) | *(in_string+byte_count+1);
			if (wbufptr == 0x0000) {
				byte_count+=2;
				break;
			}
		}	
	}
	return byte_count;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                     generics                                      //
///////////////////////////////////////////////////////////////////////////////////////

uint16_t UInt16FromBigEndian(const char *string) {
#if defined (__ppc__) || defined (__ppc64__)
	uint16_t test;
	memcpy(&test,string,2);
	return test;
#else
	return ((string[0] & 0xff) << 8 | string[1] & 0xff) << 0;
#endif
}

uint32_t UInt32FromBigEndian(const char *string) {
#if defined (__ppc__) || defined (__ppc64__)
	uint32_t test;
	memcpy(&test,string,4);
	return test;
#else
	return ((string[0] & 0xff) << 24 | (string[1] & 0xff) << 16 | (string[2] & 0xff) << 8 | string[3] & 0xff) << 0;
#endif
}

uint64_t UInt64FromBigEndian(const char *string) {
#if defined (__ppc__) || defined (__ppc64__)
	uint64_t test;
	memcpy(&test,string,8);
	return test;
#else
	return (uint64_t)(string[0] & 0xff) << 54 | (uint64_t)(string[1] & 0xff) << 48 | (uint64_t)(string[2] & 0xff) << 40 | (uint64_t)(string[3] & 0xff) << 32 | 
				 (uint64_t)(string[4] & 0xff) << 24 | (uint64_t)(string[5] & 0xff) << 16 | (uint64_t)(string[6] & 0xff) <<  8 | (uint64_t)(string[7] & 0xff) <<  0;
#endif
}

void UInt16_TO_String2(uint16_t snum, char* data) {
	data[0] = (snum >>  8) & 0xff;
	data[1] = (snum >>  0) & 0xff;
	return;
}

void UInt32_TO_String4(uint32_t lnum, char* data) {
	data[0] = (lnum >> 24) & 0xff;
	data[1] = (lnum >> 16) & 0xff;
	data[2] = (lnum >>  8) & 0xff;
	data[3] = (lnum >>  0) & 0xff;
	return;
}

void UInt64_TO_String8(uint64_t ullnum, char* data) {
	data[0] = (ullnum >> 56) & 0xff;
	data[1] = (ullnum >> 48) & 0xff;
	data[2] = (ullnum >> 40) & 0xff;
	data[3] = (ullnum >> 32) & 0xff;
	data[4] = (ullnum >> 24) & 0xff;
	data[5] = (ullnum >> 16) & 0xff;
	data[6] = (ullnum >>  8) & 0xff;
	data[7] = (ullnum >>  0) & 0xff;
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        3gp asset support (for 'loci')                             //
///////////////////////////////////////////////////////////////////////////////////////

uint32_t float_to_16x16bit_fixed_point(double floating_val) {
	uint32_t fixedpoint_16bit = 0;
	int16_t long_integer = (int16_t)floating_val;
	//to get a fixed 16-bit decimal, work on the decimal part along; multiply by (2^8 * 2) which moves the decimal over 16 bits to create our int16_t
	//now while the degrees can be negative (requiring a int16_6), the decimal portion is always positive (and thus requiring a uint16_t)
	uint16_t long_decimal = (int16_t) ((floating_val - long_integer) * (double)(65536) ); 
	fixedpoint_16bit = long_integer * 65536 + long_decimal; //same as bitshifting, less headache doing it
	return fixedpoint_16bit;
}

double fixed_point_16x16bit_to_double(uint32_t fixed_point) {
	double return_val = 0.0;
	int16_t long_integer = fixed_point / 65536;
	uint16_t long_decimal = fixed_point - (long_integer * 65536) ;
	return_val = long_integer + ( (double)long_decimal / 65536);
	
	if (return_val < 0.0) {
		return_val-=1.0;
	}
	
	return return_val;
}

uint32_t widechar_len(char* instring, uint32_t _bytes_) {
	uint32_t wstring_len = 0;
	for (uint32_t i = 0; i <= _bytes_/2 ; i++) {
		if ( instring[0] == 0 && instring[1] == 0) {
			break;
		} else {
			instring+=2;
			wstring_len++;
		}
	}
	return wstring_len;
}

bool APar_assert(bool expression, int error_msg, char* supplemental_info) {
	bool force_break = true;
	if (!expression) {
		force_break = false;
		switch (error_msg) {
			case 1 : { //trying to set an iTunes-style metadata tag on an 3GP/MobileMPEG-4
				fprintf(stdout, "AP warning:\n\tSetting the %s tag is for ordinary MPEG-4 files.\n\tIt is not supported on 3gp/amc files.\nSkipping\n", supplemental_info);
				break;
			}
			
			case 2 : { //trying to set a 3gp asset on an mpeg-4 file with the improper brand
				fprintf(stdout, "AP warning:\n\tSetting the %s asset is only available on 3GPP files branded 3gp6 or later.\nSkipping\n", supplemental_info);
				break;
			}
			
			case 3 : { //trying to set 'meta' on a file without a iso2 or mp42 compatible brand.
				fprintf(stdout, "AtomicParsley warning: ID3 tags requires a v2 compatible file, which was not found.\nSkipping.\n");
				break;
			}
			case 4 : { //trying to set a 3gp album asset on an early 3gp file that only came into being with 3gp6
				fprintf(stdout, "Major brand of given file: %s\n", supplemental_info);
				break;
			}
			case 5 : { //trying to set metadata on track 33 when there are only 3 tracks
				fprintf(stdout, "AP warning: skipping non-existing track number setting user data atom: %s.\n", supplemental_info);
				break;
			}
			case 6 : { //trying to set id3 metadata on track 33 when there are only 3 tracks
				fprintf(stdout, "AP error: skipping non-existing track number setting frame %s for ID32 atom.\n", supplemental_info);
				break;
			}
			case 7 : { //trying to set id3 metadata on track 33 when there are only 3 tracks
				fprintf(stdout, "AP warning: the 'meta' atom is being hangled by a %s handler.\n  Remove the 'meta' atom and its contents and try again.\n", supplemental_info);
				break;
			}
			case 8 : { //trying to create an ID32 atom when there is a primary item atom present signaling referenced data (local or external)
				fprintf(stdout, "AP warning: unsupported external or referenced items were detected. Skipping this frame: %s\n", supplemental_info);
				break;
			}
			case 9 : { //trying to eliminate an id3 frame that doesn't exist
				fprintf(stdout, "AP warning: id3 frame %s cannot be deleted because it does not exist.\n", supplemental_info);
				break;
			}
			case 10 : { //trying to eliminate an id3 frame that doesn't exist
				fprintf(stdout, "AP warning: skipping setting unknown %s frame\n", supplemental_info);
				break;
			}
			case 11 : { //insuffient memory to malloc an id3 field (probably picture or encapuslated object)
				fprintf(stdout, "AP error: memory was not alloctated for frame %s. Exiting.\n", supplemental_info);
				break;
			}
		}
	}
	return force_break;
}

/* http://wwwmaths.anu.edu.au/~brent/random.html  */
/* xorgens.c version 3.04, R. P. Brent, 20060628. */

/* For type definitions see xorgens.h */

unsigned long xor4096i() {
  /* 32-bit or 64-bit integer random number generator 
     with period at least 2**4096-1.
     
     It is assumed that "UINT" is a 32-bit or 64-bit integer 
     (see typedef statements in xorgens.h).
     
     xor4096i should be called exactly once with nonzero seed, and
     thereafter with zero seed.  
     
     One random number uniformly distributed in [0..2**wlen) is returned,
     where wlen = 8*sizeof(UINT) = 32 or 64.

     R. P. Brent, 20060628.
  */

  /* UINT64 is TRUE if 64-bit UINT,
     UINT32 is TRUE otherwise (assumed to be 32-bit UINT). */
     
#define UINT64 (sizeof(UINT)>>3)
#define UINT32 (1 - UINT64) 

#define wlen (64*UINT64 +  32*UINT32)
#define r    (64*UINT64 + 128*UINT32)
#define s    (53*UINT64 +  95*UINT32)
#define a    (33*UINT64 +  17*UINT32)
#define b    (26*UINT64 +  12*UINT32)
#define c    (27*UINT64 +  13*UINT32)
#define d    (29*UINT64 +  15*UINT32)
#define ws   (27*UINT64 +  16*UINT32) 

	UINT seed = 0;
	
  static UINT w, weyl, zero = 0, x[r];
  UINT t, v;
  static int i = -1 ;              /* i < 0 indicates first call */
  int k;
	
	if (i < 0) {
#if defined HAVE_SRANDDEV
		sranddev();
#else
		srand((int) time(NULL));
#endif
		double doubleseed = ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) );
		seed = (UINT)(doubleseed*rand());
	}
  
  if ((i < 0) || (seed != zero)) { /* Initialisation necessary */
  
  /* weyl = odd approximation to 2**wlen*(sqrt(5)-1)/2. */

    if (UINT32) 
      weyl = 0x61c88647;
    else 
      weyl = ((((UINT)0x61c88646)<<16)<<16) + (UINT)0x80b583eb;
                 
    v = (seed!=zero)? seed:~seed;  /* v must be nonzero */

    for (k = wlen; k > 0; k--) {   /* Avoid correlations for close seeds */
      v ^= v<<10; v ^= v>>15;      /* Recurrence has period 2**wlen-1 */ 
      v ^= v<<4;  v ^= v>>13;      /* for wlen = 32 or 64 */
      }
    for (w = v, k = 0; (UINT)k < r; k++) { /* Initialise circular array */
      v ^= v<<10; v ^= v>>15; 
      v ^= v<<4;  v ^= v>>13;
      x[k] = v + (w+=weyl);                
      }
    for (i = r-1, k = 4*r; k > 0; k--) { /* Discard first 4*r results */ 
      t = x[i = (i+1)&(r-1)];   t ^= t<<a;  t ^= t>>b; 
      v = x[(i+(r-s))&(r-1)];   v ^= v<<c;  v ^= v>>d;          
      x[i] = t^v;       
      }
    }
    
  /* Apart from initialisation (above), this is the generator */

  t = x[i = (i+1)&(r-1)];            /* Assumes that r is a power of two */
  v = x[(i+(r-s))&(r-1)];            /* Index is (i-s) mod r */
  t ^= t<<a;  t ^= t>>b;             /* (I + L^a)(I + R^b) */
  v ^= v<<c;  v ^= v>>d;             /* (I + L^c)(I + R^d) */
  x[i] = (v ^= t);                   /* Update circular array */
  w += weyl;                         /* Update Weyl generator */
  return (v + (w^(w>>ws)));          /* Return combination */

#undef UINT64
#undef UINT32
#undef wlen
#undef r
#undef s
#undef a
#undef b
#undef c
#undef d
#undef ws 
}
