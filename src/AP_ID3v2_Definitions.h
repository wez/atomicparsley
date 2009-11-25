//==================================================================//
/*
    AtomicParsley - AP_ID3v2_Definitions.h

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
                                                                   */
//==================================================================//

enum ID3_FieldTypes {
	ID3_UNKNOWN_FIELD = -1,
	ID3_TEXT_FIELD,
	ID3_TEXT_ENCODING_FIELD,
	ID3_OWNER_FIELD, //UFID,PRIV
	ID3_DESCRIPTION_FIELD, //TXXX, WXXX
	ID3_URL_FIELD,
	ID3_LANGUAGE_FIELD, //USLT
	ID3_MIME_TYPE_FIELD, //APIC
	ID3_PIC_TYPE_FIELD, //APIC
	ID3_BINARY_DATA_FIELD, //APIC,GEOB
	ID3_FILENAME_FIELD, //GEOB
	ID3_GROUPSYMBOL_FIELD,
	ID3_COUNTER_FIELD,
	ID3_IMAGEFORMAT_FIELD //PIC in v2.2
};

//the order of these frame types must exactly match the order listed in the FrameTypeConstructionList[] array!!!
enum ID3v2FrameType {
	ID3_UNKNOWN_FRAME = -1,
	ID3_TEXT_FRAME,
	ID3_TEXT_FRAME_USERDEF,
	ID3_URL_FRAME,
	ID3_URL_FRAME_USERDEF,
	ID3_UNIQUE_FILE_ID_FRAME,
	ID3_CD_ID_FRAME,
	ID3_DESCRIBED_TEXT_FRAME, //oy... these frames (COMM, USLT) can differ by description
	ID3_ATTACHED_PICTURE_FRAME,
	ID3_ATTACHED_OBJECT_FRAME,
	ID3_GROUP_ID_FRAME,
	ID3_SIGNATURE_FRAME,
	ID3_PRIVATE_FRAME,
	ID3_PLAYCOUNTER_FRAME,
	ID3_POPULAR_FRAME,
	ID3_OLD_V2P2_PICTURE_FRAME
};

//the order of these frames must exactly match the order listed in the KnownFrames[] array!!!
enum ID3v2FrameIDs {
	ID3v2_UNKNOWN_FRAME = -1,
	ID3v2_FRAME_ALBUM,
	ID3v2_FRAME_BPM,
	ID3v2_FRAME_COMPOSER,
	ID3v2_FRAME_CONTENTTYPE,
	ID3v2_FRAME_COPYRIGHT,
	ID3v2_FRAME_ENCODINGTIME,
	ID3v2_FRAME_PLAYLISTDELAY,
	ID3v2_FRAME_ORIGRELTIME,
	ID3v2_FRAME_RECORDINGTIME,
	ID3v2_FRAME_RELEASETIME,
	ID3v2_FRAME_TAGGINGTIME,
	ID3v2_FRAME_ENCODER,
	ID3v2_FRAME_LYRICIST,
	ID3v2_FRAME_FILETYPE,
	ID3v2_FRAME_INVOLVEDPEOPLE,
	ID3v2_FRAME_GROUP_DESC,
	ID3v2_FRAME_TITLE,
	ID3v2_FRAME_SUBTITLE,
	ID3v2_FRAME_INITIALKEY,
	ID3v2_FRAME_LANGUAGE,
	ID3v2_FRAME_TIMELENGTH,
	ID3v2_FRAME_MUSICIANLIST,
	ID3v2_FRAME_MEDIATYPE,
	ID3v2_FRAME_MOOD,
	ID3v2_FRAME_ORIGALBUM,
	ID3v2_FRAME_ORIGFILENAME,
	ID3v2_FRAME_ORIGWRITER,
	ID3v2_FRAME_ORIGARTIST,
	ID3v2_FRAME_FILEOWNER,
	ID3v2_FRAME_ARTIST,
	ID3v2_FRAME_ALBUMARTIST,
	ID3v2_FRAME_CONDUCTOR,
	ID3v2_FRAME_REMIXER,
	ID3v2_FRAME_PART_O_SET,
	ID3v2_FRAME_PRODNOTICE,
	ID3v2_FRAME_PUBLISHER,
	ID3v2_FRAME_TRACKNUM,
	ID3v2_FRAME_IRADIONAME,
	ID3v2_FRAME_IRADIOOWNER,
	ID3v2_FRAME_ALBUMSORT,
	ID3v2_FRAME_PERFORMERSORT,
	ID3v2_FRAME_TITLESORT,
	ID3v2_FRAME_ISRC, 
	ID3v2_FRAME_ENCODINGSETTINGS,
	ID3v2_FRAME_SETSUBTITLE,
	ID3v2_DATE,
	ID3v2_TIME,
	ID3v2_ORIGRELYEAR,
	ID3v2_RECORDINGDATE,
	ID3v2_FRAME_SIZE,
	ID3v2_FRAME_YEAR,
	ID3v2_FRAME_USERDEF_TEXT,
	ID3v2_FRAME_URLCOMMINFO,
	ID3v2_FRAME_URLCOPYRIGHT,
	ID3v2_FRAME_URLAUDIOFILE,
	ID3v2_FRAME_URLARTIST,
	ID3v2_FRAME_URLAUDIOSOURCE,
	ID3v2_FRAME_URLIRADIO,
	ID3v2_FRAME_URLPAYMENT,
	ID3v2_FRAME_URLPUBLISHER,
	ID3v2_FRAME_USERDEF_URL,
	ID3v2_FRAME_UFID,
	ID3v2_FRAME_MUSIC_CD_ID,
	ID3v2_FRAME_COMMENT,
	ID3v2_FRAME_UNSYNCLYRICS,
	ID3v2_EMBEDDED_PICTURE,
	ID3v2_EMBEDDED_PICTURE_V2P2,
	ID3v2_EMBEDDED_OBJECT,
	ID3v2_FRAME_GRID,
	ID3v2_FRAME_SIGNATURE,
	ID3v2_FRAME_PRIVATE,
	ID3v2_FRAME_PLAYCOUNTER,
	ID3v2_FRAME_POPULARITY
};

enum ID3v2_TagFlags {
	ID32_TAGFLAG_BIT0 = 0x01,
	ID32_TAGFLAG_BIT1 = 0x02,
	ID32_TAGFLAG_BIT2 = 0x04,
	ID32_TAGFLAG_BIT3 = 0x08,
	ID32_TAGFLAG_FOOTER = 0x10,
	ID32_TAGFLAG_EXPERIMENTAL = 0x20,
	ID32_TAGFLAG_EXTENDEDHEADER = 0x40,
	ID32_TAGFLAG_UNSYNCRONIZATION = 0x80
};

enum ID3v2_FrameFlags {
	ID32_FRAMEFLAG_STATUS         =  0x4000,
	ID32_FRAMEFLAG_PRESERVE       =  0x2000,
	ID32_FRAMEFLAG_READONLY       =  0x1000,
	ID32_FRAMEFLAG_GROUPING       =  0x0040,
	ID32_FRAMEFLAG_COMPRESSED     =  0x0008,
	ID32_FRAMEFLAG_ENCRYPTED      =  0x0004,
	ID32_FRAMEFLAG_UNSYNCED       =  0x0002,
	ID32_FRAMEFLAG_LENINDICATED   =  0x0001
};

// the wording of the ID3 (v2.4 in this case) 'informal standard' is not always replete with clarity.
// text encodings are worded as having a NULL terminator (8or16bit), even for the body of text frames
// with that in hand, then a description field from COMM should look much like a utf8 text field
// and yet for TXXX, description is expressely worded as:
//
// "The frame body consists of a description of the string, represented as a terminated string, followed by the actual string."
//     Description       <text string according to encoding> $00 (00)
//     Value             <text string according to encoding>
//
// Note how description is expressly *worded* as having a NULL terminator, but the text field is not.
// GEOB text clarifies things better:
// "The first two strings [mime & filename] may be omitted, leaving only their terminations.
//     MIME type              <text string> $00
//     Filename               <text string according to encoding> $00 (00)
//
// so these trailing $00 (00) are the terminators for the strings - not separators between n-length string fields.
// If the string is devoid of content (not NULLed out, but *devoid* of info), then the only thing that should exist
// is for a utf16 BOM to exist on text encoding 0x01. The (required) terminator for mime & filename are specifically
// enumerated in the frame format, which matches the wording of the frame description.
// ...and so AP does not terminate text fields
//
// Further sealing the case is the reference implementation for id3v2.3 (id3lib) doesn't terminate text fields:
// http://sourceforge.net/project/showfiles.php?group_id=979&package_id=4679

enum text_encodings {
	TE_LATIN1 = 0,
	TE_UTF16LE_WITH_BOM = 1,
	TE_UTF16BE_NO_BOM = 2,
	TE_UTF8 = 3
};

// Structure that defines the (subset) known ID3 frames defined by id3 informal specification.
typedef struct {
  char*         ID3V2p2_FrameID;
  char*					ID3V2p3_FrameID;
  char*					ID3V2p4_FrameID;
	char*					ID3V2_FrameDescription;
	char*         CLI_frameIDpreset;
  int						ID3v2_InternalFrameID;
  int						ID3v2_FrameType;
} ID3FrameDefinition;

typedef struct {
  char*         image_mimetype;
	char*         image_fileextn;
  uint8_t				image_testbytes;
  char*					image_binaryheader;
} ImageFileFormatDefinition;

typedef struct {
	uint8_t       hexcode;
	char*         hexstring;
	char*         imagetype_str;
} ID3ImageType;

// Structure that defines how any ID3v2FrameType is constructed, listing an array of its constituent ID3_FieldTypes
typedef struct {
	ID3v2FrameType   ID3_FrameType;
	uint8_t          ID3_FieldCount;
	ID3_FieldTypes	 ID3_FieldComponents[5]; //max known to be tested
} ID3v2FieldDefinition;

struct ID3v2Fields {
	int          ID3v2_Field_Type;
	uint32_t     field_length;
	uint32_t     alloc_length;
	char*        field_string;
	ID3v2Fields* next_field;
};

struct ID3v2Frame {
	char          ID3v2_Frame_Namestr[5];
	uint32_t      ID3v2_Frame_Length; //this is the real length, not a syncsafe int; note: does not include frame ID (like 'TIT2', 'TCO' - 3or4 bytes) or frame flags (2bytes)
	uint16_t      ID3v2_Frame_Flags;
	
	//these next 2 values can be potentially be stored based on bitsetting in frame flags;
	uint8_t       ID3v2_Frame_GroupingSymbol;
	uint32_t      ID3v2_Frame_ExpandedLength;
	
	int           ID3v2_Frame_ID;
	int           ID3v2_FrameType;
	uint8_t       ID3v2_FieldCount;
	uint8_t       textfield_tally;
	ID3v2Fields*  ID3v2_Frame_Fields; //malloc
	ID3v2Frame*   ID3v2_NextFrame;
	bool          eliminate_frame;	
};

struct ID3v2Tag  {
	uint8_t       ID3v2Tag_MajorVersion;
	uint8_t       ID3v2Tag_RevisionVersion;
	uint8_t       ID3v2Tag_Flags;
	uint32_t      ID3v2Tag_Length; //this is a bonafide uint_32_t length, not a syncsafe int

	//this extended header section depends on a bitsetting in ID3v2Tag_Flags
	uint32_t      ID3v2_Tag_ExtendedHeader_Length; 	//the entire extended header section is unimplemented flags & flag frames

	ID3v2Frame*   ID3v2_FirstFrame;
	ID3v2Frame*   ID3v2_FrameList;
	uint16_t      ID3v2_FrameCount;
	bool          modified_tag;
};
