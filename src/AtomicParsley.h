#ifndef ATOMIC_PARSLEY_H
#define ATOMIC_PARSLEY_H

//==================================================================//
/*
    AtomicParsley - AtomicParsley.h

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

    Copyright ©2005-2007 puck_lock
    with contributions from others; see the CREDITS file
                                                                   */
//==================================================================//

#ifdef _WIN32
#ifndef _UNICODE
#define _UNICODE
#endif
#if defined (_MSC_VER)
#define strncasecmp _strnicmp
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4244) // int64_t assignments to int32_t etc.
#endif
#endif

#include "config.h"

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <sys/types.h>
#ifdef __GLIBC__
# define HAVE_LROUNDF 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>
#include <math.h>
#include <errno.h>

#if HAVE_STDDEF_H
# include <stddef.h>
#endif
#if HAVE_STDINT_H
# include <stdint.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif
#if HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#if HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#if HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#if HAVE_WINDOWS_H
# include <windows.h>
#endif
#if HAVE_WCHAR_H
# include <wchar.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_IO_H
# include <io.h>
#endif
#if HAVE_SIGNAL_H
# include <signal.h>
#endif
#if HAVE_GETOPT_H
# include <getopt.h>
#else
# include "extras/getopt.h"
#endif

#ifndef PRIu64
# define PRIu64 "llu"
#endif
#ifndef PRIu16
# define PRIu16 "hu"
#endif
#ifndef PRIu8
# define PRIu8 "hhu"
#endif
#ifndef SCNu32
# define SCNu32 "lu"
#endif
#ifndef SCNu16
# define SCNu16 "u"
#endif

#ifndef MIN
//#define MIN(X,Y) ((X) < (Y) ? : (X) : (Y))
#define MIN min
#endif


#ifndef MAXPATHLEN
# define MAXPATHLEN 255
#endif

#include "util.h"

#define MAX_ATOMS 1024
#define MAXDATA_PAYLOAD 1256
#define DEFAULT_PADDING_LENGTH          2048;
#define MINIMUM_REQUIRED_PADDING_LENGTH 0;
#define MAXIMUM_REQUIRED_PADDING_LENGTH 5000;

#include "ap_types.h"

extern atomDefinition KnownAtoms[];
extern bool parsedfile;
extern bool file_opened;
extern bool modified_atoms;
extern bool alter_original;
extern bool preserve_timestamps;
extern bool deep_atom_scan;
extern bool cvs_build;
extern bool force_existing_hierarchy;
extern bool move_moov_atom;
extern bool moov_atom_was_mooved;

extern int metadata_style;
extern uint32_t brand;
extern uint64_t mdatData;
extern uint64_t file_size;
extern uint64_t gapless_void_padding;

extern EmployedCodecs track_codecs;

extern AtomicInfo parsedAtoms[];
extern short atom_number;
extern char* ISObasemediafile;
extern FILE* source_file;

extern padding_preferences pad_prefs;

extern uint8_t UnicodeOutputStatus;

extern uint8_t forced_suffix_type;

extern char* twenty_byte_buffer;
extern DynamicUpdateStat dynUpd;

extern ID3FrameDefinition KnownFrames[];
extern ID3v2FieldDefinition FrameTypeConstructionList[];
extern ImageFileFormatDefinition ImageList[];
extern ID3ImageType ImageTypeList[];

void ShowVersionInfo();
void APar_FreeMemory();

short APar_FindParentAtom(int order_in_tree, uint8_t this_atom_level);

AtomicInfo* APar_FindAtomInTrack(uint8_t &total_tracks, uint8_t &track_num,
  const char* search_atom_str);

AtomicInfo* APar_FindAtom(const char* atom_name, bool createMissing,
  uint8_t atom_type, uint16_t atom_lang, bool match_full_uuids = false,
  const char* reverseDNSdomain = NULL);

int APar_MatchToKnownAtom(const char* atom_name, const char* atom_container,
  bool fromFile, const char* find_atom_path);

void APar_ScanAtoms(const char *path, bool deepscan_REQ = false);
void APar_IdentifyBrand(char* file_brand);

AtomicInfo* APar_CreateSparseAtom(AtomicInfo* surrogate_atom,
  AtomicInfo* parent_atom, short preceding_atom);

void APar_Unified_atom_Put(AtomicInfo* target_atom, const char* unicode_data,
  uint8_t text_tag_style, uint64_t ancillary_data, uint8_t anc_bit_width);

void APar_atom_Binary_Put(AtomicInfo* target_atom, const char* binary_data,
  uint32_t bytecount, uint64_t atomic_data_offset);

/* iTunes-style metadata */
void APar_MetaData_atomArtwork_Set(const char* artworkPath,
  char* env_PicOptions);

void APar_MetaData_atomGenre_Set(const char* atomPayload);
void APar_MetaData_atom_QuickInit(short atom_num, const uint32_t atomFlags,
  uint32_t supplemental_length, uint32_t allotment = MAXDATA_PAYLOAD + 1);

AtomicInfo* APar_MetaData_atom_Init(const char* atom_path,
  const char* MD_Payload, const uint32_t atomFlags);

AtomicInfo* APar_reverseDNS_atom_Init(const char* rDNS_atom_name,
  const char* rDNS_payload, const uint32_t* atomFlags, const char* rDNS_domain);

/* uuid user extension metadata; made to look much like iTunes-style metadata
 * with a 4byte NULL */
AtomicInfo* APar_uuid_atom_Init(const char* atom_path, const char* uuidName,
  const uint32_t dataType, const char* uuidValue, bool shellAtom);

//test whether the ipod uuid can be added for a video track
uint16_t APar_TestVideoDescription(AtomicInfo* video_desc_atom,
  FILE* ISObmff_file);

void APar_Generate_iPod_uuid(char* atom_path);

/* 3GP-style metadata */
uint32_t APar_3GP_Keyword_atom_Format(char* keywords_globbed,
  uint8_t keyword_count, bool set_UTF16_text, char* &formed_keyword_struct);

AtomicInfo* APar_UserData_atom_Init(const char* userdata_atom_name,
  const char* atom_payload, uint8_t udta_container, uint8_t track_idx,
  uint16_t userdata_lang);

/* ID3v2 (2.4) style metadata, non-external form */
AtomicInfo* APar_ID32_atom_Init(const char* frameID_str, char meta_area,
  const char* lang_str, uint16_t id32_lang);

void APar_RemoveAtom(const char* atom_path, uint8_t atom_type,
  uint16_t UD_lang, const char* rDNS_domain = NULL);

void APar_freefree(int purge_level);

void APar_MetadataFileDump(const char* ISObasemediafile);

void APar_Optimize(bool mdat_test_only);
void APar_DetermineAtomLengths();
void APar_WriteFile(const char* ISObasemediafile, const char* outfile,
  bool rewrite_original);

void APar_zlib_inflate(char* in_buffer, uint32_t in_buf_len,
  char* out_buffer, uint32_t out_buf_len);

uint32_t APar_zlib_deflate(char* in_buffer, uint32_t in_buf_len,
  char* out_buffer, uint32_t out_buf_len);


void APar_print_uuid(ap_uuid_t* uuid, bool new_line = true);
void APar_sprintf_uuid(ap_uuid_t* uuid, char* destination);
uint8_t APar_uuid_scanf(char* in_formed_uuid, const char* raw_uuid);

void APar_endian_uuid_bin_str_conversion(char* raw_uuid);

uint8_t APar_extract_uuid_version(ap_uuid_t* uuid, char* binary_uuid_str);
void APar_generate_uuid_from_atomname(char* atom_name, char* uuid_binary_str);
void APar_generate_random_uuid(char* uuid_binary_str);

/* Initialize structure containing state of computation. */
extern void sha1_init_ctx (struct sha1_ctx *ctx);

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is necessary that LEN is a multiple of 64!!! */
extern void sha1_process_block (const void *buffer, size_t len,
        struct sha1_ctx *ctx);

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is NOT required that LEN is a multiple of 64.  */
extern void sha1_process_bytes (const void *buffer, size_t len,
        struct sha1_ctx *ctx);

/* Process the remaining bytes in the buffer and put result from CTX
   in first 20 bytes following RESBUF.  The result is always in little
   endian byte order, so that a byte-wise output yields to the wanted
   ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF be correctly
   aligned for a 32 bits value.  */
extern void *sha1_finish_ctx (struct sha1_ctx *ctx, void *resbuf);


/* Put result from CTX in first 20 bytes following RESBUF.  The result is
   always in little endian byte order, so that a byte-wise output yields
   to the wanted ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
extern void *sha1_read_ctx (const struct sha1_ctx *ctx, void *resbuf);


/* Compute SHA1 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 20 bytes
   beginning at RESBLOCK.  */
extern int sha1_stream (FILE *stream, void *resblock);

/* Compute SHA1 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
extern void *sha1_buffer (const char *buffer, size_t len, void *resblock);

int isolat1ToUTF8(unsigned char* out, int outlen,
  const unsigned char* in, int inlen);

int UTF8Toisolat1(unsigned char* out, int outlen,
  const unsigned char* in, int inlen);

int UTF16BEToUTF8(unsigned char* out, int outlen,
  const unsigned char* inb, int inlenb);

int UTF8ToUTF16BE(unsigned char* outb, int outlen,
  const unsigned char* in, int inlen);

int UTF16LEToUTF8(unsigned char* out, int outlen,
  const unsigned char* inb, int inlenb);

int UTF8ToUTF16LE(unsigned char* outb, int outlen,
  const unsigned char* in, int inlen);

int isUTF8(const char* in_string);

unsigned int utf8_length(const char *in_string, unsigned int char_limit);

int strip_bogusUTF16toRawUTF8(unsigned char* out, int inlen,
  wchar_t* in, int outlen);

int test_conforming_alpha_string(char* in_string);
bool test_limited_ascii(char* in_string, unsigned int str_len);

void APar_ExtractDetails(FILE* isofile, uint8_t optional_output);
void APar_ExtractBrands(char* filepath);

void printBOM();
void APar_fprintf_UTF8_data(const char* utf8_encoded_data);
void APar_unicode_win32Printout(wchar_t* unicode_out, char* utf8_out);

void APar_Extract_uuid_binary_file(AtomicInfo* uuid_atom,
  const char* originating_file, char* output_path);

void APar_Print_APuuid_atoms(const char *path, char* output_path,
  uint8_t target_information);

void APar_Print_iTunesData(const char *path, char* output_path,
  uint8_t supplemental_info, uint8_t target_information,
  AtomicInfo* ilstAtom = NULL);

void APar_PrintUserDataAssests(bool quantum_listing = false);

void APar_Extract_ID3v2_file(AtomicInfo* id32_atom, const char* frame_str,
  const char* originfile, const char* destination_folder, AdjunctArgs* id3args);

void APar_Print_ID3v2_tags(AtomicInfo* id32_atom);

void APar_Print_ISO_UserData_per_track();
void APar_Mark_UserData_area(uint8_t track_num, short userdata_atom,
  bool quantum_listing);

//trees
void APar_PrintAtomicTree();
void APar_SimpleAtomPrintout();

uint32_t APar_4CC_CreatorCode(const char* filepath, uint32_t new_type_code);
void APar_SupplySelectiveTypeCreatorCodes(const char *inputPath,
  const char *outputPath, uint8_t forced_type_code);

bool ResizeGivenImage(const char* filePath, PicPrefs myPicPrefs,
  char* resized_path);

char* GenreIntToString(int genre);
uint8_t StringGenreToInt(const char* genre_string);
void ListGenresValues();

stiks* MatchStikString(const char* stik_string);
stiks* MatchStikNumber(uint8_t in_stik_num);
void ListStikValues();

sfIDs* MatchStoreFrontNumber(uint32_t storefrontnum);

bool MatchLanguageCode(const char* in_code);
void ListLanguageCodes();

void ListMediaRatings();
const char* Expand_cli_mediastring(const char* cli_rating);

char* ID3GenreIntToString(int genre);
uint8_t ID3StringGenreToInt(const char* genre_string);

#endif /* ATOMIC_PARSLEY_H */

// vim:ts=2:sw=2:et:
