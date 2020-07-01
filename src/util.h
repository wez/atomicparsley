//==================================================================//
/*
    AtomicParsley - util.h

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

    Copyright �2006-2007 puck_lock
    with contributions from others; see the CREDITS file
																																		*/
//==================================================================//
#include "ap_types.h"

#if defined (__ppc__) || defined (__ppc64__)
#define SWAP16(x) (x)
#define SWAP32(x) (x)
#else
#define SWAP16(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define SWAP32(x) ((((x)&0xFF)<<24) | (((x)>>24)&0xFF) | (((x)&0x0000FF00)<<8) | (((x)&0x00FF0000)>>8) )
#endif

#if defined (_WIN32) && defined (_MSC_VER)
#undef HAVE_GETOPT_H
#undef HAVE_LROUNDF
#undef HAVE_STRSEP
//#undef HAVE_ZLIB_H //comment this IN when compiling on win32 withOUT zlib present
//#define HAVE_ZLIB_H 1 //and comment this OUT
#undef HAVE_SRANDDEV
#endif

#define MAXTIME_32 6377812095ULL

uint64_t findFileSize(const char *utf8_filepath);
FILE* APar_OpenFile(const char* utf8_filepath, const char* file_flags);
FILE* APar_OpenISOBaseMediaFile(const char* file, bool open); //openSomeFile
void TestFileExistence(const char *filePath, bool errorOut);

#if defined (_WIN32)
#ifndef HAVE_FSEEKO
int fseeko(FILE *stream, uint64_t pos, int whence);
#endif
HANDLE APar_OpenFileWin32(const char* utf8_filepath, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#endif
bool IsUnicodeWinOS();

const char *APar_strferror(FILE *f);
uint8_t APar_read8(FILE* ISObasemediafile, uint64_t pos);
uint16_t APar_read16(char* buffer, FILE* ISObasemediafile, uint64_t pos);
uint32_t APar_read32(char* buffer, FILE* ISObasemediafile, uint64_t pos);
uint64_t APar_read64(char* buffer, FILE* ISObasemediafile, uint64_t pos);
void APar_readX_noseek(char* buffer, FILE* ISObasemediafile, uint32_t length);
void APar_readX(char* buffer, FILE* ISObasemediafile, uint64_t pos, uint32_t length);
uint32_t APar_ReadFile(char* destination_buffer, FILE* a_file, uint32_t bytes_to_read);
uint32_t APar_FindValueInAtom(char* uint32_buffer, FILE* ISObasemediafile, short an_atom, uint64_t start_position, uint32_t eval_number);

void APar_UnpackLanguage(unsigned char lang_code[], uint16_t packed_language);
uint16_t PackLanguage(const char* language_code, uint8_t lang_offset);

#ifndef HAVE_STRSEP
char *strsep (char **stringp, const char *delim);
#endif

char* APar_extract_UTC(uint64_t total_secs);
uint32_t APar_get_mpeg4_time();
void APar_StandardTime(char* &formed_time);

wchar_t* Convert_multibyteUTF16_to_wchar(char* input_unicode, size_t glyph_length, bool skip_BOM);
unsigned char* Convert_multibyteUTF16_to_UTF8(char* input_utf8, size_t glyph_length, size_t byte_count);
wchar_t* Convert_multibyteUTF8_to_wchar(const char* input_utf8);
uint32_t findstringNULLterm(char* in_string, uint8_t encodingFlag, uint32_t max_len);
uint32_t skipNULLterm(char* in_string, uint8_t encodingFlag, uint32_t max_len);


uint16_t UInt16FromBigEndian(const char *string);
uint32_t UInt32FromBigEndian(const char *string);
uint64_t UInt64FromBigEndian(const char *string);
void UInt16_TO_String2(uint16_t snum, char* data);
void UInt32_TO_String4(uint32_t lnum, char* data);
void UInt64_TO_String8(uint64_t ullnum, char* data);

uint32_t float_to_16x16bit_fixed_point(double floating_val);
double fixed_point_16x16bit_to_double(uint32_t fixed_point);

uint32_t widechar_len(char* instring, uint32_t _bytes_);

bool APar_assert(bool expression, int error_msg, const char* supplemental_info);

unsigned long xor4096i();
