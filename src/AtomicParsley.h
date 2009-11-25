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

#if defined (_WIN32) || defined (_MSC_VER)
#define MAXPATHLEN 255
#else
#include <sys/param.h>
#endif

#if defined (_MSC_VER)
#define _UNICODE
#define strncasecmp strnicmp

#include <windows.h>  /* for WriteConsoleW */
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
	uint32_t  AtomicStart;
	uint32_t  AtomicLength;
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
typedef struct EmployedCodecs {
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
	
	uint32_t padding_bytes;
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
  char*         known_atom_name;
  char*					known_parent_atoms[5]; //max known to be tested
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
  char*         stik_string;
  uint8_t       stik_number;
} stiks;

typedef struct {
  char*         storefront_string;
  uint32_t      storefront_number;
} sfIDs;

typedef struct {
	char* iso639_2_code;
	char* iso639_1_code;
	char* language_in_english;
} iso639_lang;

typedef struct {
  char* media_rating;
  char* media_rating_cli_str;
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
extern uint32_t mdatData;
extern uint32_t file_size;
extern uint32_t gapless_void_padding;

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

#define AtomicParsley_version	"0.9.0"

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
void APar_Unified_atom_Put(AtomicInfo* target_atom, const char* unicode_data, uint8_t text_tag_style, uint32_t ancillary_data, uint8_t anc_bit_width);
void APar_atom_Binary_Put(AtomicInfo* target_atom, const char* binary_data, uint32_t bytecount, uint32_t atomic_data_offset);

/* iTunes-style metadata */
void APar_MetaData_atomArtwork_Set(const char* artworkPath, char* env_PicOptions);
void APar_MetaData_atomGenre_Set(const char* atomPayload);
void APar_MetaData_atom_QuickInit(short atom_num, const uint32_t atomFlags, uint32_t supplemental_length, uint32_t allotment = MAXDATA_PAYLOAD + 1);
AtomicInfo* APar_MetaData_atom_Init(const char* atom_path, const char* MD_Payload, const uint32_t atomFlags);

AtomicInfo* APar_reverseDNS_atom_Init(const char* rDNS_atom_name, const char* rDNS_payload, const uint32_t* atomFlags, const char* rDNS_domain);

/* uuid user extension metadata; made to look much like iTunes-style metadata with a 4byte NULL */
AtomicInfo* APar_uuid_atom_Init(const char* atom_path, char* uuidName, const uint32_t dataType, const char* uuidValue, bool shellAtom);

uint16_t APar_TestVideoDescription(AtomicInfo* video_desc_atom, FILE* ISObmff_file); //test whether the ipod uuid can be added for a video track
void APar_Generate_iPod_uuid(char* atom_path);

/* 3GP-style metadata */
uint32_t APar_3GP_Keyword_atom_Format(char* keywords_globbed, uint8_t keyword_count, bool set_UTF16_text, char* &formed_keyword_struct);
AtomicInfo* APar_UserData_atom_Init(char* userdata_atom_name, const char* atom_payload, uint8_t udta_container, uint8_t track_idx, uint16_t userdata_lang);

/* ID3v2 (2.4) style metadata, non-external form */
AtomicInfo* APar_ID32_atom_Init(char* frameID_str, char meta_area, char* lang_str, uint16_t id32_lang);

void APar_RemoveAtom(const char* atom_path, uint8_t atom_type, uint16_t UD_lang, const char* rDNS_domain = NULL);
void APar_freefree(int purge_level);

void APar_MetadataFileDump(const char* ISObasemediafile);

void APar_Optimize(bool mdat_test_only);
void APar_DetermineAtomLengths();
void APar_WriteFile(const char* ISObasemediafile, const char* outfile, bool rewrite_original);

//--------------------------------------------------------------------------------------------------------------------------------//
/*
v0.1   10/05/2005 Parsing of atoms; intial Tree printout; extraction of all "covr.data" atoms out to files
v0.2   11/10/2005 AtomicInfo.NextAtomNumber introduced to facilitate dynamic atom tree reorganization; CreateSparseAtom added
v0.5   11/22/2005 Writes artist properly of variable lengths properly into an iTMS m4p file properly (other files don't fare well due to the stsd atom non-standard nature); a number of code-uglifying workarounds were employed to get get that far;
v0.6   11/25/2005 Added genre string/numerical support, support for genre's dual-atom ©gen/gnre nature, genre string->integer; bug fixes to APar_LocateAtomInsertionPoint when an atom is missing; APar_CreateSparseAtom for ordinary non-data atoms are now type -1 (which means they aren't of any interest to us besides length & name); implemnted the Integer data class; char4short; verified iTunes standard genres only go up to "Hard Rock"; added jpg/png artwork embedding into "covr" atoms; slight bugfix for APar_FindAtom (created spurious trailing "covr" atoms).
v0.6    GPL'ed at sourceforge.net
v0.65   11/25/2005  bugfixes to newly introduced bugs in APar_FindAtom; metaEnema to remove all metadata (safe even for m4p drm files); year implemented properly (tagtime moved onto non-standard 'tdtg' atom ala id3v2.4 - because I like that tag); added setting compilation "cpil" tag (an annoying 5byte tag); added advisory setting (maybe it'll give me a kick one cold winter day-do a "Get Info" in iTunes & in the main "Summary" tab view will be a new little icon next to artwork)
v0.7    11/26/2005 added a writeBack flag to for a less beta-like future; integrated NSImage resizing of artwork; environmental preferences for artwork modifications; build system mods for Mac-specific compiling; 
v0.7.1  11/27/2005 modified parsing & writing to support Apple Lossless (alac) mp4 files. The lovely "alac.alac" non-standard atoms (parents & carry data) caused unplayable files to be written. Only QT ISMA files get screwed now (no idea about Nero)
v0.7.2  11/29/2005 creates iTunes-required meta.hdlr; all the tags now get spit back when reading them (--textdata); slight fix to how atoms are parsed; all known m4a files now tag properly: iTunes (m4a, m4b, chapterized, alac), Quicktime (ISMA & mpeg4 - change filename ext to .m4a to see art; all QT products require the meta.hdlr addition), faac, Helix Producer & Nero; slight change to how PrintDataAtoms called FindParentAtom; added tag time on "©ed1" (edit date-might only really belong directly under udta); added "©url" to hold url; fixes to APar_RemoveAtom; added cli ability to remove all artwork
v0.7.3  12/02/2005 handles stsd (and child) atoms better; modifies all stco offsets when needed (not just the first); new oddball iTMS video "drmi" atom handling; new "stik" atom support (sets iTunes GetInfo->options:Movie,TV Show, Music Video); writes iTMS video drm TV shows well now; diffs in a hex editor are moov atom length, and then into stco, so all is well
v0.7.4  12/03/2005 "desc", "tvnn", "tvsh", "tven" & "tves" setting
v0.7.5b 12/09/2005 forced 'mdat' into being childless (chapterized mpeg4 files have atoms scattered througout mdat, but they aren't children); fixed issues with ffmpeg created mpeg4 files (that have mdat as 2nd atom; moov & chilren as last atoms); moved ffmpeg mdat atoms around to end; better atom adding at the end; subbed getopt_long_only to getopt_long for pre-10.4 users; added progressbar
v0.7.5c 12/10/2005 funnguy0's linux patches (thanks so much for that)
v0.7.5d 12/11/2005 endian issues for x86 mostly resolved; setting genre's segfaults; stik doesn't get set in a multi-option command, but does as a single atom setting; Debian port added to binaries (compiled under debian-31r0a-i386 with g++4.02-2, libc6_2.3.5-8 & libstdc++6_4.0.2-2) - under VirtualPC - with the nano editor!
v0.7.5e 12/16/2005 ammends how atoms are added at the end of the hierarchy (notably this affects ffmpeg video files); writes "keyw", "catg", "pcst", "aART" atoms; read-only "purl" & "egid" added
v0.7.6  12/31/2005 ceased flawed null-termination (which was implemented more in my mind) of text 'data' atoms; UTF-8 output on Mac OS X & Linux - comment in DUSE_ICONV_CONVERSION in the build file to test it other platforms (maybe my win98Se isn't utf8 aware?); cygwin build accommodations; fix to the secondary "of" number for track/disk on non-PPC; implemented user-defined completely sanctioned 'uuid' atoms to hold.... anything (text only for now); "--tagtime", "--url" & "--information" now get set onto uuid atoms; allow creation of uuid atoms directly from the cli; cygwin-win98SE port added to binary releases; added '--freefree' to remove any&all 'free' atoms
v0.8    01/14/2006 switched over to uint8_t for former ADC_CPIL_TMPO & former ADC_Integer; added podcast stik setting & purl/egid; bugfixes to APar_RemoveAtom; bugfixes & optimizations to APar_FindAtom; changes to text output & set values for stik atom; increase in buffer size; limit non-uuid strings to 255bytes; fixed retreats in progress bar; added purd atom; support mdat.length=0 atom (length=1/64-bit isn't supported; I'll somehow cope with a < 4GB file); switch from long to uint32_t; better x86 bitshifting; added swtich to prevent moving mdat atoms (possible PSP requires mdat before moov); universal binary for Mac OS X release; no text limit on lyrics tag
v0.8.4  02/25/2006 fixed an imaging bug from preferences; fixed metaEnema screwing up the meta atom (APar_RemoveAtom bugfix to remove a direct_find atom); added --output, --overWrite; added --metaDump to dump ONLY metadata tags to a file; versioning for cvs builds; limited support for 64-bit mdat atoms (limited to a little less than a 32-bit atom; > 4GB); bugfixes to APar_RemoveAtom for removing uuid atoms or non-existing atoms & to delete all artwork, then add in 1 command ("--artwork REMOVE_ALL --artwork /path --artwork /path"); support 64-bit co64 atom; support MacOSX-style type/creator codes for tempfiles that end in ".mp4" (no need to change extn to ".m4v"/".m4a" anymore); moved purl/egid onto AtomicDataClass_UInteger (0x00 instead of 0x15) to mirror Apple's change on these tags; start incorporating Brian's Win32 fixes (if you malloc, memset is sure to follow; fopen); give the 'name' atom for '---' iTunes-internal tags for metadata printouts; allow --freefree remove 'free's up to a certain level (preserves iTunes padding); squash some memory leaks; change how CreateSparseAtom was matching atoms to accommodate EliminateAtom-ed atoms (facilitates the previous artwork amendments); exit on unsupported 'ftyp' file brands; anonymous 3rd party native win32 contributions; reworked APar_DetermineAtomLengths to accommodate proper tag setting with --mdatLock; parsing atoms under 'stsd' is no longer internally used - only for tree printing; reworked Mac OS X TYPE determination based on new stsd_codec structure member; revisit co64 offset calculations; start extracting track-level details (dates, language, encoder, channels); changed stco/co64 calculations to support non-muxed files; anonymous "Everyday is NOT like Sunday" contribution; changed unknown 0x15 flagged metadata atoms to hex printouts; move mdat only when moov precedes mdat; new flexible esds parsing
v0.8.8  05/21/2006 prevent libmp4v2 artwork from a hexdump; changed how short strings were set; win32 change for uuid atoms to avoid sprintf; skip parsing 'free' atoms; work around foobar2000 0.9 non-compliant tagging scheme & added cli switch to give 'tags' the GoLytely - aka '--foobar2000Enema'; ability to read/set completely separate 3gp tags subset (3GPP TS 26.444 version 6.4.0 Release 6 compliant & more like QuickTime-style tags); added libxml's utf8 & utf16 conversion functions; new windows (windows2000 & later) unicode (utf16) console output (literal utf8 bytes in win98 & earlier; memset standard means of initializing; simplified setting of arbitrary info uniformly onto parsedAtoms.AtomicData; win32 switch to CP_UTF8 codepage on redirected console output for better unicode output support; eliminate need for libiconv - use xml's utf8<->latin1 functions to supplant libiconv; properly display atoms like '©nam' under Windows for trees & atom printouts; support setting unicode on Windows CP_UTF8; added 3GP keyword; fixed bug removing last 3GP asset to reset the length of 'udta'; added 'manualAtomRemove' for manually removing iTunes-style atoms; improved tracking of filesize/percentage when large free atoms impinge on % of new filesize; added 3GP location 'loci' (El Loco) atom - all known 3GP assets can now be set/viewed (except support for multiple same atoms of different languages); ->forced<- elimination of Nero tagging scheme (their foobar2000 inspired 'tags' atom) on 3GP files; prevent iTunes-style tags on 3GP files or 3GP assets on MPEG-4 files; fix offsets in fragmented files ("moof.traf.tfhd"); up MAX_ATOMS to 1024; Windows support for full utf16 (unicode) for cli args & filenames
v0.9.0	09/15/2006 new file scanning method based on an array of known atoms/KnownAtoms struct added to list the gamut of known atoms & their basic properties; better atom versioning & flags support; allow negatives in 3gp asset coordinates (switch to high-bit ascii for getopt_long for assets); fixed minor bug that crept in on non-Win systems in removing files; switch from moving mdat(s) to moving moov to reorder atoms; mellow_flow's genre fix; SLarew's utf16 fix for printing 3gp assets on Win32; reorder moov's child atoms so that udta is last (as per ISO spec recommendations) in moov; enable use of 'free' atom padding for rapid updating, pad with a (user-defined) default amount of padding with a complete file rewrite; switch remaining AtomicInfo variables over to pointers; add support for multiple same atoms with differing languages (like 3gp assets); more flexible 'stik' setting/retrieving & added Audiobook; genre bugfix (again!!); added ability to list std genres & stik strings; switch output for rtng's "Lyrics" to "Content"; list file brands; bugfix for removing some cli metadata; prevent optimizing on PSP mpeg-4 files (but allow dynamic updating, and don't add padding to psp files); new APar_FindAtom routine eliminating some loops; updated routine to find 'moov.udta.meta.hdlr' or iTunes-style tagging; simplified APar_RemoveAtom; 3gp assets differing in language are grouped now instead of being fifo; simplified printing of non-string iTunes-style tags; work around 3rd party bug affecting 'cprt' corruption; switch to fseeko to support files between 2.5GB & 4GB (and ancillary routines off of filesize like progress bar); fix co64 reduction offsets; prevent optimizing when just getting a tree or tags (screwed up track level details); bashfulbladder's booklet stik, only allow dynamic updating with --overWrite & new "AP -t +" routine to show padding & supplemental info; changing win32 filename to '-utf8.exe' forces raw utf8 input/output; win32 longhelp is converted to utf16 (for atom names); new shorthelp added as default help page; bugfix removing non-existing atoms; an actual change (removal/addition/change) of an atom is now required for any type of write action; fix channel listing for 'esds' without sec5 info; added ability to force image dimensions on MacOSX; revamped track level details; 255 byte limit for strings changed to 255 utf8 *character* limit; --stik Audiobook now changes file extension to '.m4b' (for Mac OS X, finder Type code is changed to 'M4B ' too); fix --3gp-year "" in APar_RemoveAtom; bugfix setting string lengths in 3gp keyword; added ability to add ISO 'cprt' copyright at movie or track level; implemented v5 sha1 namepsace/name uuids; fixed crash on finding any atom with full uuids (like psp files); more extensive type/profiles/levels in track level details; add support for embedding files on uuid atoms; switch to reading artwork directly into memory (as opposed to copying from a->b) when setting artwork; modified ExtractPixPrefs for leaks - defaults now to deleting temp pic files; skip sprintf for uuid binary strings ('qlts' is why) & switch to (less flexible) memcpy; accommodate iTunes 7.0 adding aprox. 2k of NULL bytes outside of any atom structure; add 'pgap' atom; defaults to duplicating the gapless padding at the end of file now (but can be optionally skipped); fixed clipping when setting unicode characters
v0.9.X  ??/??/2007 now checks/lists 3 letter language codes; allow setting 3gp assets at track level; fix double fclose & relative paths with --overWrite; coalesce iso copyright notices into the new APar_UserData_atom_Init; initial support for setting iTunes reverseDNS atoms; fix validation test for 'trak' child atoms for atypical order; add mjpeg2000 (mjp2) major brand support (for copyright notices & uuid atoms); restyled listings of all text metadata tags (-t 1); fix multiple BOM prints on printouts; limit offset adjustments to local (non-external) data; added support for adjusting item location offsets; switch to a makefile/configure/config.h build system; start of ID3v2 2.4 implementation to go into ID32 atoms; limit chunk offset updates to local data; extend atom creation to file level (FL meta gets created after 'moov'); much of ID3v2 2.4 is completed: multiple text fields, counters, APIC/GEOB setting/extracting, group symbols & zlib compression; add 3gp7 brands; allow ID32 based on compatible ftyp branding; refactoring & splitting of metadata listings; allow multiple entries in reverseDNS atoms (excepting iTunes domain); initial (unfinished) revisit of file reorganizing/padding; update mvhd/tkhd modification timestamps
*/
