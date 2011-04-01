//==================================================================//
/*
    AtomicParsley - id3v2defs.h

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
    with contributions from others; see the CREDITS file
                                                                   */
//==================================================================//

ID3FrameDefinition KnownFrames[] = {
	{ "",    "",     "",     "Unknown frame",                            "",                ID3v2_UNKNOWN_FRAME,               ID3_UNKNOWN_FRAME },
	{ "TAL", "TALB", "TALB", "Album/Movie/Show title",                   "album",           ID3v2_FRAME_ALBUM,                 ID3_TEXT_FRAME },
	{ "TBP", "TBPM", "TBPM", "BPM (beats per minute)",                   "bpm",             ID3v2_FRAME_BPM,                   ID3_TEXT_FRAME },
	{ "TCM", "TCOM", "TCOM", "Composer",                                 "composer",        ID3v2_FRAME_COMPOSER,              ID3_TEXT_FRAME },
	{ "TCO", "TCON", "TCON", "Content Type/Genre",                       "genre",           ID3v2_FRAME_CONTENTTYPE,           ID3_TEXT_FRAME },
	{ "TCP", "TCOP", "TCOP", "Copyright message",                        "copyright",       ID3v2_FRAME_COPYRIGHT,             ID3_TEXT_FRAME },
	{ "",    "",     "TDEN", "Encoding time",                            "",                ID3v2_FRAME_ENCODINGTIME,          ID3_TEXT_FRAME },
	{ "TDY", "TDLY", "TDLY", "Playlist delay",                           "",                ID3v2_FRAME_PLAYLISTDELAY,         ID3_TEXT_FRAME },
	{ "",    "",     "TDOR", "Original release time",                    "",                ID3v2_FRAME_ORIGRELTIME,           ID3_TEXT_FRAME },
	{ "",    "",     "TDRC", "Recording time",                           "date",            ID3v2_FRAME_RECORDINGTIME,         ID3_TEXT_FRAME },
	{ "",    "",     "TDRL", "Release time",                             "released",        ID3v2_FRAME_RELEASETIME,           ID3_TEXT_FRAME },
	{ "",    "",     "TDTG", "Tagging time",                             "tagged",          ID3v2_FRAME_TAGGINGTIME,           ID3_TEXT_FRAME },
	{ "TEN", "TENC", "TENC", "Encoded by",                               "encoder",         ID3v2_FRAME_ENCODER,               ID3_TEXT_FRAME },
	{ "TXT", "TEXT", "TEXT", "Lyricist/Text writer",                     "writer",          ID3v2_FRAME_LYRICIST,              ID3_TEXT_FRAME },
	{ "TFT", "TFLT", "TFLT", "File type",                                "",                ID3v2_FRAME_FILETYPE,              ID3_TEXT_FRAME },
	{ "",    "",     "TIPL", "Involved people list",                     "",                ID3v2_FRAME_INVOLVEDPEOPLE,        ID3_TEXT_FRAME },
	{ "TT1", "TIT1", "TIT1", "Content group description",                "grouping",        ID3v2_FRAME_GROUP_DESC,            ID3_TEXT_FRAME },
	{ "TT2", "TIT2", "TIT2", "Title/songname/content description",       "title",           ID3v2_FRAME_TITLE,                 ID3_TEXT_FRAME },
	{ "TT3", "TIT3", "TIT3", "Subtitle/Description refinement",          "subtitle",        ID3v2_FRAME_SUBTITLE,              ID3_TEXT_FRAME },
	{ "TKE", "TKEY", "TKEY", "Initial key",                              "",                ID3v2_FRAME_INITIALKEY,            ID3_TEXT_FRAME },
	{ "TLA", "TLAN", "TLAN", "Language(s)",                              "",                ID3v2_FRAME_LANGUAGE,              ID3_TEXT_FRAME },
	{ "TLE", "TLEN", "TLEN", "Length",                                   "",                ID3v2_FRAME_TIMELENGTH,            ID3_TEXT_FRAME },
	{ "",    "",     "TMCL", "Musician credits list",                    "credits",         ID3v2_FRAME_MUSICIANLIST,          ID3_TEXT_FRAME },
	{ "TMT", "TMED", "TMED", "Media type",                               "media",           ID3v2_FRAME_MEDIATYPE,             ID3_TEXT_FRAME },
	{ "",    "",     "TMOO", "Mood",                                     "mood",            ID3v2_FRAME_MOOD,                  ID3_TEXT_FRAME },
	{ "TOT", "TOAL", "TOAL", "Original album/movie/show title",          "",                ID3v2_FRAME_ORIGALBUM,             ID3_TEXT_FRAME },
	{ "TOF", "TOFN", "TOFN", "Original filename",                        "",                ID3v2_FRAME_ORIGFILENAME,          ID3_TEXT_FRAME },
	{ "TOL", "TOLY", "TOLY", "Original lyricist(s)/text writer(s)",      "",                ID3v2_FRAME_ORIGWRITER,            ID3_TEXT_FRAME },
	{ "TOA", "TOPE", "TOPE", "Original artist(s)/performer(s)",          "",                ID3v2_FRAME_ORIGARTIST,            ID3_TEXT_FRAME },
	{ "",    "TOWN", "TOWN", "File owner/licensee",                      "",                ID3v2_FRAME_FILEOWNER,             ID3_TEXT_FRAME },
	{ "TP1", "TPE1", "TPE1", "Artist/Lead performer(s)/Soloist(s)",      "artist",          ID3v2_FRAME_ARTIST,                ID3_TEXT_FRAME },
	{ "TP2", "TPE2", "TPE2", "Album artist/Band/orchestra/accompaniment", "album artist",   ID3v2_FRAME_ALBUMARTIST,           ID3_TEXT_FRAME },
	{ "TP3", "TPE3", "TPE3", "Conductor/performer refinement",           "conductor",       ID3v2_FRAME_CONDUCTOR,             ID3_TEXT_FRAME },
	{ "TP4", "TPE4", "TPE4", "Interpreted or remixed by",                "remixer",         ID3v2_FRAME_REMIXER,               ID3_TEXT_FRAME },
	{ "TPA", "TPOS", "TPOS", "Part of a set",                            "",                ID3v2_FRAME_PART_O_SET,            ID3_TEXT_FRAME },
	{ "",    "",     "TPRO", "Produced notice",                          "",                ID3v2_FRAME_PRODNOTICE,            ID3_TEXT_FRAME },
	{ "TPB", "TPUB", "TPUB", "Publisher",                                "publisher",       ID3v2_FRAME_PUBLISHER,             ID3_TEXT_FRAME },
	{ "TRK", "TRCK", "TRCK", "Track number/Position in set",             "trk#",            ID3v2_FRAME_TRACKNUM,              ID3_TEXT_FRAME },
	{ "",    "TRSN", "TRSN", "Internet radio station name",              "",                ID3v2_FRAME_IRADIONAME,            ID3_TEXT_FRAME },
	{ "",    "TRSO", "TRSO", "Internet radio station owner",             "",                ID3v2_FRAME_IRADIOOWNER,           ID3_TEXT_FRAME },
	{ "",    "",     "TSOA", "Album sort order",                         "",                ID3v2_FRAME_ALBUMSORT,             ID3_TEXT_FRAME },
	{ "",    "",     "TSOP", "Performer sort order",                     "",                ID3v2_FRAME_PERFORMERSORT,         ID3_TEXT_FRAME },
	{ "",    "",     "TSOT", "Title sort order",                         "",                ID3v2_FRAME_TITLESORT,             ID3_TEXT_FRAME },
	{ "TRC", "TSRC", "TSRC", "ISRC",                                     "",                ID3v2_FRAME_ISRC,                  ID3_TEXT_FRAME },
	{ "TSS", "TSSE", "TSSE", "Software/Hardware and settings used for encoding", "",        ID3v2_FRAME_ENCODINGSETTINGS,      ID3_TEXT_FRAME },
	{ "",    "",     "TSST", "Set subtitle",                             "",                ID3v2_FRAME_SETSUBTITLE,           ID3_TEXT_FRAME },
	
	{ "TDA", "TDAT", "",     "Date",                                     "",                ID3v2_DATE,                        ID3_TEXT_FRAME },
	{ "TIM", "TIME", "",     "TIME",                                     "",                ID3v2_TIME,                        ID3_TEXT_FRAME },
	{ "TOR", "TORY", "",     "Original Release Year",                    "",                ID3v2_ORIGRELYEAR,                 ID3_TEXT_FRAME },
	{ "TRD", "TRDA", "",     "Recording dates",                          "",                ID3v2_RECORDINGDATE,               ID3_TEXT_FRAME },
	{ "TSI", "TSIZ", "",     "Size",                                     "",                ID3v2_FRAME_SIZE,                  ID3_TEXT_FRAME },
	{ "TYE", "TYER", "",     "YEAR",                                     "",                ID3v2_FRAME_YEAR,                  ID3_TEXT_FRAME },
	
	{ "TXX", "TXXX", "TXXX", "User defined text information frame",      "",                ID3v2_FRAME_USERDEF_TEXT,          ID3_TEXT_FRAME_USERDEF },
	
	//some of these (like WCOM, WOAF) allow for muliple frames - but (sigh) alas, such is not the case in AP. 
	{ "WCM", "WCOM", "WCOM", "Commercial information",                   "",               ID3v2_FRAME_URLCOMMINFO,           ID3_URL_FRAME },
	{ "WCP", "WCOP", "WCOP", "Copyright/Legal information",              "",               ID3v2_FRAME_URLCOPYRIGHT,          ID3_URL_FRAME },
	{ "WAF", "WOAF", "WOAF", "Official audio file webpage",              "",               ID3v2_FRAME_URLAUDIOFILE,          ID3_URL_FRAME },
	{ "WAR", "WOAR", "WOAR", "Official artist/performer webpage",        "",               ID3v2_FRAME_URLARTIST,             ID3_URL_FRAME },
	{ "WAS", "WOAS", "WOAS", "Official audio source webpage",            "",               ID3v2_FRAME_URLAUDIOSOURCE,        ID3_URL_FRAME },
	{ "",    "WORS", "WORS", "Official Internet radio station homepage", "",               ID3v2_FRAME_URLIRADIO,             ID3_URL_FRAME },
	{ "",    "WPAY", "WPAY", "Payment",                                  "",               ID3v2_FRAME_URLPAYMENT,            ID3_URL_FRAME },
	{ "WPB", "WPUB", "WPUB", "Publishers official webpage",              "",               ID3v2_FRAME_URLPUBLISHER,          ID3_URL_FRAME },
	{ "WXX", "WXXX", "WXXX", "User defined URL link frame",              "",               ID3v2_FRAME_USERDEF_URL,           ID3_URL_FRAME_USERDEF },
	
	{ "UFI", "UFID", "UFID", "Unique file identifier",                   "",               ID3v2_FRAME_UFID,                  ID3_UNIQUE_FILE_ID_FRAME },
	{ "MCI", "MCID", "MCDI", "Music CD Identifier",                      "",               ID3v2_FRAME_MUSIC_CD_ID,           ID3_CD_ID_FRAME },
	
	{ "COM", "COMM", "COMM", "Comment",                                  "comment",        ID3v2_FRAME_COMMENT,               ID3_DESCRIBED_TEXT_FRAME },
	{ "ULT", "USLT", "USLT", "Unsynchronised lyrics",                    "lyrics",         ID3v2_FRAME_UNSYNCLYRICS,          ID3_DESCRIBED_TEXT_FRAME },
	
	{ "",    "APIC", "APIC", "Attached picture",                         "",               ID3v2_EMBEDDED_PICTURE,            ID3_ATTACHED_PICTURE_FRAME },
	{ "PIC", "",     "",     "Attached picture",                         "",               ID3v2_EMBEDDED_PICTURE_V2P2,       ID3_OLD_V2P2_PICTURE_FRAME },
	{ "GEO", "GEOB", "GEOB", "Attached object",                          "",               ID3v2_EMBEDDED_OBJECT,             ID3_ATTACHED_OBJECT_FRAME },
	
	{ "",    "GRID", "GRID", "Group ID registration",                    "",               ID3v2_FRAME_GRID,                  ID3_GROUP_ID_FRAME },
	{ "",    "",     "SIGN", "Signature",                                "",               ID3v2_FRAME_SIGNATURE,             ID3_SIGNATURE_FRAME },
	{ "",    "PRIV", "PRIV", "Private frame",                            "",               ID3v2_FRAME_PRIVATE,               ID3_PRIVATE_FRAME },
	{ "CNT", "PCNT", "PCNT", "Play counter",                             "",               ID3v2_FRAME_PLAYCOUNTER,           ID3_PLAYCOUNTER_FRAME },
	{ "POP", "POPM", "POPM", "Popularimeter",                            "",               ID3v2_FRAME_POPULARITY,            ID3_POPULAR_FRAME }

};

//the field listing array is mostly used for mental clarification instead of internal use - frames are parsed/rendered hardcoded irrespective of ordering here
ID3v2FieldDefinition FrameTypeConstructionList[] = {
	{ ID3_UNKNOWN_FRAME,            1, { ID3_UNKNOWN_FIELD } },
	{ ID3_TEXT_FRAME,               2, { ID3_TEXT_ENCODING_FIELD, ID3_TEXT_FIELD } },
	{ ID3_TEXT_FRAME_USERDEF,       3, { ID3_TEXT_ENCODING_FIELD, ID3_DESCRIPTION_FIELD, ID3_TEXT_FIELD } },
	{ ID3_URL_FRAME,                1, { ID3_URL_FIELD } },
	{ ID3_URL_FRAME_USERDEF,        3, { ID3_TEXT_ENCODING_FIELD, ID3_DESCRIPTION_FIELD, ID3_URL_FIELD } },
	{ ID3_UNIQUE_FILE_ID_FRAME,     2, { ID3_OWNER_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_CD_ID_FRAME,              1, { ID3_BINARY_DATA_FIELD } },
	{ ID3_DESCRIBED_TEXT_FRAME,     4, { ID3_TEXT_ENCODING_FIELD, ID3_LANGUAGE_FIELD, ID3_DESCRIPTION_FIELD, ID3_TEXT_FIELD } },
	{ ID3_ATTACHED_PICTURE_FRAME,   5, { ID3_TEXT_ENCODING_FIELD, ID3_MIME_TYPE_FIELD, ID3_PIC_TYPE_FIELD, ID3_DESCRIPTION_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_ATTACHED_OBJECT_FRAME,    5, { ID3_TEXT_ENCODING_FIELD, ID3_MIME_TYPE_FIELD, ID3_FILENAME_FIELD, ID3_DESCRIPTION_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_GROUP_ID_FRAME,           3, { ID3_OWNER_FIELD, ID3_GROUPSYMBOL_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_SIGNATURE_FRAME,          2, { ID3_GROUPSYMBOL_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_PRIVATE_FRAME,            2, { ID3_OWNER_FIELD, ID3_BINARY_DATA_FIELD } },
	{ ID3_PLAYCOUNTER_FRAME,        1, { ID3_COUNTER_FIELD } },
	{ ID3_POPULAR_FRAME,            3, { ID3_OWNER_FIELD, ID3_BINARY_DATA_FIELD, ID3_COUNTER_FIELD } },
	{ ID3_OLD_V2P2_PICTURE_FRAME,   5, { ID3_TEXT_ENCODING_FIELD, ID3_IMAGEFORMAT_FIELD, ID3_PIC_TYPE_FIELD, ID3_DESCRIPTION_FIELD, ID3_BINARY_DATA_FIELD } }
};

//used to determine mimetype for APIC image writing
ImageFileFormatDefinition ImageList[] = {
	{ "image/jpeg",				".jpg",  4,  "\xFF\xD8\xFF\xE0" },
	{ "image/jpeg",				".jpg",  4,  "\xFF\xD8\xFF\xE1" },
	{ "image/png",				".png",  8,  "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A" },
	{ "image/pdf",				".pdf",  7,  "%PDF-1." },
	{ "image/jp2",				".jp2",  12, "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A\x00\x00\x00\x14\x66\x74\x79\x70\x6A\x70\x32\x20" },
	{ "image/gif",				".gif",  6,  "GIF89a" },
	{ "image/tiff",				".tiff", 4,  "\x4D\x4D\x00\x2A" },
	{ "image/tiff",				".tiff", 4,  "\x49\x49\x2A\x00" },
	{ "image/bmp",				".bmp",  2,  "\x42\x4D" },
	{ "image/bmp",				".bmp",  2,  "\x42\x41" },
	{ "image/photoshop",  ".psd",  4,  "8BPS" },
	{ "image/other",			".img",  0,  "" }
};

ID3ImageType ImageTypeList[] = {
	{ 0x00,  "0x00", "Other" },
	{ 0x01,  "0x01", "32x32 pixels 'file icon' (PNG only)" },
	{ 0x02,  "0x02", "Other file icon" },
	{ 0x03,  "0x03", "Cover (front)" },
	{ 0x04,  "0x04", "Cover (back)" },
	{ 0x05,  "0x05", "Leaflet page" },
	{ 0x06,  "0x06", "Media (e.g. label side of CD)" },
	{ 0x07,  "0x07", "Lead artist/lead performer/soloist" },
	{ 0x08,  "0x08", "Artist/performer" },
	{ 0x09,  "0x09", "Conductor" },
	{ 0x0A,  "0x0A", "Band/Orchestra" },
	{ 0x0B,  "0x0B", "Composer" },
	{ 0x0C,  "0x0C", "Lyricist/text writer" },
	{ 0x0D,  "0x0D", "Recording Location" },
	{ 0x0E,  "0x0E", "During recording" },
	{ 0x0F,  "0x0F", "During performance" },
	{ 0x10,  "0x10", "Movie/video screen capture" },
	{ 0x11,  "0x11", "A bright coloured fish" },
	{ 0x12,  "0x12", "Illustration" },
	{ 0x13,  "0x13", "Band/artist logotype" },
	{ 0x14,  "0x14", "Publisher/Studio logotype" }
};
