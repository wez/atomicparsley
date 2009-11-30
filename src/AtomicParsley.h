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
                                                                   */
//==================================================================//

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


#if defined (_WIN32) || defined (_MSC_VER)
#define MAXPATHLEN 255
#endif

#if defined (_MSC_VER)
#define _UNICODE
#define strncasecmp strnicmp
#endif

#include "AP_commons.h"
#include "AP_ID3v2_Definitions.h"

const uint32_t AtomFlags_Data_Binary = 0;           // Atom version 1byte/ Atom flags 3 bytes; 0x00 00 00 00
const uint32_t AtomFlags_Data_Text = 1;             // UTF-8, no termination
const uint32_t AtomFlags_Data_JPEGBinary = 13;      // \x0D
const uint32_t AtomFlags_Data_PNGBinary = 14;       // \x0E
const uint32_t AtomFlags_Data_UInt = 21;            // \x15 for cpil, tmpo, rtng; iTMS atoms: cnID, atID, plID, geID, sfID, akID
const uint32_t AtomFlags_Data_uuid_binary = 88;     // 0x58 for uuid atoms that contain files

enum {
  UTF8_iTunesStyle_256glyphLimited        = 0, //no NULL termination
  UTF8_iTunesStyle_Unlimited              = 1, //no NULL termination
	UTF8_iTunesStyle_Binary                 = 3, //no NULL termination, used in purl & egid
  UTF8_3GP_Style                          = 8, //terminated with a NULL uint8_t
  UTF16_3GP_Style                         = 16 //terminated with a NULL uint16_t
};

enum {
	UNDEFINED_STYLE                         = 0,
	ITUNES_STYLE                            = 100,
	THIRD_GEN_PARTNER                       = 300, //3gpp files prior to 3gp6
	THIRD_GEN_PARTNER_VER1_REL6             = 306, //3GPP Release6 the first spec to contain the complement of assets
	THIRD_GEN_PARTNER_VER1_REL7             = 307, //3GPP Release7 introduces ID32 atoms
	THIRD_GEN_PARTNER_VER2                  = 320, //3gp2 files
	THIRD_GEN_PARTNER_VER2_REL_A            = 321, //3gp2 files, 3GPP2 C.S0050-A introduces 'gadi'
	MOTIONJPEG2000                          = 400
};

struct AtomicInfo  {
	short     AtomicNumber;
	uint64_t  AtomicStart;
	uint64_t  AtomicLength;
	uint64_t  AtomicLengthExtended;
	char*     AtomicName;
	char*			ReverseDNSname;
	char*			ReverseDNSdomain;
	uint8_t   AtomicContainerState;
	uint8_t   AtomicClassification; 
	uint32_t	AtomicVerFlags;					//used by versioned atoms and derivatives
	uint16_t  AtomicLanguage;					//used by 3gp assets & ID32 atoms only
	uint8_t   AtomicLevel;
	char*     AtomicData;
	int       NextAtomNumber;					//our first atom is numbered 0; the last points back to it - so watch it!
	uint32_t  ancillary_data;         //just contains a simple number for atoms that contains some interesting info (like stsd codec used)
	uint8_t   uuid_style;
	char*     uuid_ap_atomname;
	ID3v2Tag* ID32_TagInfo;
};

//currently this is only used on Mac OS X to set type/creator for generic '.mp4' file extension files. The Finder 4 character code TYPE is what determines whether a file appears as a video or an audio file in a broad sense.
struct EmployedCodecs {
	bool has_avc1;
	bool has_mp4v;
	bool has_drmi;
	bool has_alac;
	bool has_mp4a;
	bool has_drms;
	bool has_timed_text; //carries the URL - in the mdat stream at a specific time - thus it too is timed.
	bool has_timed_jpeg; //no idea of podcasts support 'png ' or 'tiff'
	bool has_timed_tx3g; //this IS true timed text stream
	bool has_mp4s; //MPEG-4 Systems
	bool has_rtp_hint; //'rtp '; implies hinting
};

enum {
	MEDIADATA__PRECEDES__MOOV = 2,
  ROOT_META__PRECEDES__MOOV = 4,
	MOOV_META__PRECEDES__TRACKS = 8,
	MOOV_UDTA__PRECEDES__TRACKS = 16,
	
	PADDING_AT_EOF = 0x1000000
};

struct FreeAtomListing {
	AtomicInfo* free_atom;
	FreeAtomListing* next_free_listing;
};

struct DynamicUpdateStat  {
	bool updage_by_padding;
	bool reorder_moov;
	bool moov_was_mooved;
	bool prevent_dynamic_update;

	uint32_t optimization_flags;
	
	uint64_t padding_bytes;
	short consolidated_padding_insertion;

	AtomicInfo* last_trak_child_atom;
	AtomicInfo* moov_atom;
	AtomicInfo* moov_udta_atom;
	AtomicInfo* iTunes_list_handler_atom;
	AtomicInfo* moov_meta_atom;
	AtomicInfo* file_meta_atom;
	AtomicInfo* first_mdat_atom;
	AtomicInfo* first_movielevel_metadata_tagging_atom;
	AtomicInfo* initial_update_atom;
	AtomicInfo* first_otiose_freespace_atom;
	AtomicInfo* padding_store;
	AtomicInfo* padding_resevoir;
	FreeAtomListing* first_padding_atom;
	FreeAtomListing* last_padding_atom;
};

struct padding_preferences {
	uint32_t default_padding_size;
	uint32_t minimum_required_padding_size;
	uint32_t maximum_present_padding_size;
};

// Structure that defines the known atoms used by mpeg-4 family of specifications.
typedef struct {
  const char*         known_atom_name;
  const char*					known_parent_atoms[5]; //max known to be tested
  uint32_t			container_state;
  int						presence_requirements;
  uint32_t			box_type;
} atomDefinition;

typedef struct {
	uint8_t uuid_form;
  char* binary_uuid;
	char* uuid_AP_atom_name;
} uuid_vitals;

enum {
  PARENT_ATOM         = 0, //container atom
	SIMPLE_PARENT_ATOM  = 1,
	DUAL_STATE_ATOM     = 2, //acts as both parent (contains other atoms) & child (carries data)
	CHILD_ATOM          = 3, //atom that does NOT contain any children
	UNKNOWN_ATOM_TYPE   = 4
};

enum {
	REQUIRED_ONCE  = 30, //means total of 1 atom per file  (or total of 1 if parent atom is required to be present)
	REQUIRED_ONE = 31, //means 1 atom per container atom; totalling many per file  (or required present if optional parent atom is present)
	REQUIRED_VARIABLE = 32, //means 1 or more atoms per container atom are required to be present
	PARENT_SPECIFIC = 33, //means (iTunes-style metadata) the atom defines how many are present; most are MAX 1 'data' atoms; 'covr' is ?unlimited?
	OPTIONAL_ONCE = 34, //means total of 1 atom per file, but not required
	OPTIONAL_ONE = 35, //means 1 atom per container atom but not required; many may be present in a file
	OPTIONAL_MANY = 36, //means more than 1 occurrence per container atom
	REQ_FAMILIAL_ONE = OPTIONAL_ONE, //means that one of the family of atoms defined by the spec is required by the parent atom
	UKNOWN_REQUIREMENTS= 38
};

enum {
	SIMPLE_ATOM = 50,
	VERSIONED_ATOM = 51,
	EXTENDED_ATOM = 52,
	PACKED_LANG_ATOM = 53,
	UNKNOWN_ATOM = 59
};

enum {
	PRINT_DATA = 1,
	PRINT_FREE_SPACE = 2,
	PRINT_PADDING_SPACE = 4,
	PRINT_USER_DATA_SPACE = 8,
	PRINT_MEDIA_SPACE = 16,
	EXTRACT_ARTWORK = 20,
	EXTRACT_ALL_UUID_BINARYS = 21
};

typedef struct {
  const char*         stik_string;
  uint8_t       stik_number;
} stiks;

typedef struct {
  const char*         storefront_string;
  uint32_t      storefront_number;
} sfIDs;

typedef struct {
	const char* iso639_2_code;
	const char* iso639_1_code;
	const char* language_in_english;
} iso639_lang;

typedef struct {
  const char* media_rating;
  const char* media_rating_cli_str;
} m_ratings;

enum {
	UNIVERSAL_UTF8,
	WIN32_UTF16
};

enum {
 FORCE_M4B_TYPE = 85,
 NO_TYPE_FORCING = 90
};

enum {
	FILE_LEVEL_ATOM,
	MOVIE_LEVEL_ATOM,
	ALL_TRACKS_ATOM,
	SINGLE_TRACK_ATOM
};

enum {
	UUID_DEPRECATED_FORM,
	UUID_SHA1_NAMESPACE,
	UUID_AP_SHA1_NAMESPACE,
	UUID_OTHER
};

//==========================================================//

extern bool parsedfile;
extern bool file_opened;
extern bool modified_atoms;
extern bool alter_original;
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

//==========================================================//

#ifdef AP_VER
# define AtomicParsley_version AP_VER
#else
# define AtomicParsley_version	"0.9.3"
#endif
#define MAX_ATOMS 1024
#define MAXDATA_PAYLOAD 1256

#define DEFAULT_PADDING_LENGTH          2048;
#define MINIMUM_REQUIRED_PADDING_LENGTH 0;
#define MAXIMUM_REQUIRED_PADDING_LENGTH 5000;

//--------------------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------------//

void ShowVersionInfo();

void APar_FreeMemory();

short APar_FindParentAtom(int order_in_tree, uint8_t this_atom_level);
AtomicInfo* APar_FindAtomInTrack(uint8_t &total_tracks, uint8_t &track_num, char* search_atom_str);
AtomicInfo* APar_FindAtom(const char* atom_name, bool createMissing, uint8_t atom_type, uint16_t atom_lang,
                                                 bool match_full_uuids = false, const char* reverseDNSdomain = NULL);

int APar_MatchToKnownAtom(const char* atom_name, const char* atom_container, bool fromFile, const char* find_atom_path);
void APar_ScanAtoms(const char *path, bool deepscan_REQ = false);
void APar_IdentifyBrand(char* file_brand);

AtomicInfo* APar_CreateSparseAtom(AtomicInfo* surrogate_atom, AtomicInfo* parent_atom, short preceding_atom);
void APar_Unified_atom_Put(AtomicInfo* target_atom, const char* unicode_data, uint8_t text_tag_style, uint64_t ancillary_data, uint8_t anc_bit_width);
void APar_atom_Binary_Put(AtomicInfo* target_atom, const char* binary_data, uint32_t bytecount, uint64_t atomic_data_offset);

/* iTunes-style metadata */
void APar_MetaData_atomArtwork_Set(const char* artworkPath, char* env_PicOptions);
void APar_MetaData_atomGenre_Set(const char* atomPayload);
void APar_MetaData_atom_QuickInit(short atom_num, const uint32_t atomFlags, uint32_t supplemental_length, uint32_t allotment = MAXDATA_PAYLOAD + 1);
AtomicInfo* APar_MetaData_atom_Init(const char* atom_path, const char* MD_Payload, const uint32_t atomFlags);

AtomicInfo* APar_reverseDNS_atom_Init(const char* rDNS_atom_name, const char* rDNS_payload, const uint32_t* atomFlags, const char* rDNS_domain);

/* uuid user extension metadata; made to look much like iTunes-style metadata with a 4byte NULL */
AtomicInfo* APar_uuid_atom_Init(const char* atom_path, const char* uuidName, const uint32_t dataType, const char* uuidValue, bool shellAtom);

uint16_t APar_TestVideoDescription(AtomicInfo* video_desc_atom, FILE* ISObmff_file); //test whether the ipod uuid can be added for a video track
void APar_Generate_iPod_uuid(char* atom_path);

/* 3GP-style metadata */
uint32_t APar_3GP_Keyword_atom_Format(char* keywords_globbed, uint8_t keyword_count, bool set_UTF16_text, char* &formed_keyword_struct);
AtomicInfo* APar_UserData_atom_Init(const char* userdata_atom_name, const char* atom_payload, uint8_t udta_container, uint8_t track_idx, uint16_t userdata_lang);

/* ID3v2 (2.4) style metadata, non-external form */
AtomicInfo* APar_ID32_atom_Init(const char* frameID_str, char meta_area, const char* lang_str, uint16_t id32_lang);

void APar_RemoveAtom(const char* atom_path, uint8_t atom_type, uint16_t UD_lang, const char* rDNS_domain = NULL);
void APar_freefree(int purge_level);

void APar_MetadataFileDump(const char* ISObasemediafile);

void APar_Optimize(bool mdat_test_only);
void APar_DetermineAtomLengths();
void APar_WriteFile(const char* ISObasemediafile, const char* outfile, bool rewrite_original);

#endif

