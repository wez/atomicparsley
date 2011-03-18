#ifndef AP_TYPES_H
#define AP_TYPES_H



// Atom version 1byte/ Atom flags 3 bytes; 0x00 00 00 00
#define AtomFlags_Data_Binary 0

// UTF-8, no termination
#define AtomFlags_Data_Text 1

// \x0D
#define AtomFlags_Data_JPEGBinary 13

// \x0E
#define AtomFlags_Data_PNGBinary 14

// \x15 for cpil, tmpo, rtng; iTMS atoms: cnID, atID, plID, geID, sfID, akID
#define AtomFlags_Data_UInt 21

// 0x58 for uuid atoms that contain files
#define AtomFlags_Data_uuid_binary 88


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

#include "id3v2types.h"
struct AtomicInfo  {
  short     AtomicNumber;
  uint64_t  AtomicStart;
  uint64_t  AtomicLength;
  uint64_t  AtomicLengthExtended;
  char*     AtomicName;
  char*     ReverseDNSname;
  char*     ReverseDNSdomain;
  uint8_t   AtomicContainerState;
  uint8_t   AtomicClassification; 
  uint32_t  AtomicVerFlags;         //used by versioned atoms and derivatives
  uint16_t  AtomicLanguage;         //used by 3gp assets & ID32 atoms only
  uint8_t   AtomicLevel;
  char*     AtomicData;
  int       NextAtomNumber;         //our first atom is numbered 0; the last points back to it - so watch it!
  uint32_t  ancillary_data;         //just contains a simple number for atoms that contains some interesting info (like stsd codec used)
  uint8_t   uuid_style;
  char*     uuid_ap_atomname;
  ID3v2Tag* ID32_TagInfo;
};
#include "id3v2.h"

// currently this is only used on Mac OS X to set type/creator for generic
// '.mp4' file extension files. The Finder 4 character code TYPE is what
// determines whether a file appears as a video or an audio file in a broad
// sense.
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
  const char*         known_parent_atoms[5]; //max known to be tested
  uint32_t      container_state;
  int           presence_requirements;
  uint32_t      box_type;
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
  const char*   stik_string;
  uint8_t       stik_number;
} stiks;

typedef struct {
  const char*   storefront_string;
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

typedef struct {
  const char* genre_id_movie_string;
  uint16_t genre_id_movie_value;
} geIDMovie;

typedef struct {
  const char* genre_id_tv_string;
  uint16_t genre_id_tv_value;
} geIDTV;

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


/* Declarations of functions and data types used for SHA1 sum
   library functions.
   Copyright (C) 2000, 2001, 2003, 2005 Free Software Foundation, Inc.
*/

typedef struct {
    uint32_t       time_low;
    uint16_t       time_mid;
    uint16_t       time_hi_and_version;
    uint8_t        clock_seq_hi_and_reserved;
    uint8_t        clock_seq_low;
    unsigned char  node[6];
} ap_uuid_t;

typedef uint32_t md5_uint32;

/* Structure to save state of computation between the single steps.  */
struct sha1_ctx
{
  md5_uint32 A;
  md5_uint32 B;
  md5_uint32 C;
  md5_uint32 D;
  md5_uint32 E;

  md5_uint32 total[2];
  md5_uint32 buflen;
  char buffer[128];   //char buffer[128] __attribute__ ((__aligned__ (__alignof__ (md5_uint32))));
};

typedef struct { //if any of these are unused, they are set to 0xFF
  uint8_t od_profile_level;
  uint8_t scene_profile_level;
  uint8_t audio_profile;
  uint8_t video_profile_level;
  uint8_t graphics_profile_level;
} iods_OD;

typedef struct {
  uint64_t creation_time;
  uint64_t modified_time;
  uint64_t duration;
  bool track_enabled;
  unsigned char unpacked_lang[4];
  char track_hdlr_name[100];
  char encoder_name[100];
  
  uint32_t track_type;
  uint32_t track_codec;
  uint32_t protected_codec;
  
  bool contains_esds;
  
  uint64_t section3_length;
  uint64_t section4_length;
  uint8_t ObjectTypeIndication;
  uint32_t max_bitrate;
  uint32_t avg_bitrate;
  uint64_t section5_length;
  uint8_t descriptor_object_typeID;
  uint16_t channels;
  uint64_t section6_length; //unused

  //specifics
  uint8_t m4v_profile;
  uint8_t avc_version;
  uint8_t profile;
  uint8_t level;
  uint16_t video_height;
  uint16_t video_width;
  uint32_t macroblocks;
  uint64_t sample_aggregate;
  uint16_t amr_modes;
  
  uint8_t type_of_track;
  
} TrackInfo;

typedef struct {
  uint64_t creation_time;
  uint64_t modified_time;
  uint32_t timescale;
  uint32_t duration;
  uint32_t playback_rate; //fixed point 16.16
  uint16_t volume; //fixed 8.8 point
  
  double seconds;
  double simple_bitrate_calc;
  
  bool contains_iods;
} MovieInfo;

typedef struct {
  uint8_t total_tracks;
  uint8_t track_num;
  short track_atom;
} Trackage;

typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  double rem_millisecs;
} ap_time;

enum {
  UNKNOWN_TRACK = 0,
  VIDEO_TRACK = 2,
  AUDIO_TRACK = 4,
  DRM_PROTECTED_TRACK = 8,
  OTHER_TRACK = 16
};

enum {
  MP4V_TRACK = 65,
  AVC1_TRACK = 66,
  S_AMR_TRACK = 67,
  S263_TRACK = 68,
  EVRC_TRACK = 69,
  QCELP_TRACK = 70,
  SMV_TRACK = 71
};

enum {
  SHOW_TRACK_INFO = 2,
  SHOW_DATE_INFO = 4
};

struct PicPrefs  {
  int max_dimension;
  int dpi;
  int max_Kbytes;
  bool squareUp;
  bool allJPEG;
  bool allPNG;
  bool addBOTHpix;
  bool removeTempPix;
  bool force_dimensions;
  int force_height;
  int force_width;
};




#endif

/* vim:ts=2:sw=2:et:
 */

