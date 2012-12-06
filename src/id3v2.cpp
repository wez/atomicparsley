//==================================================================//
/*
    AtomicParsley - id3v2.cpp

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

#include "AtomicParsley.h"
#include "CDtoc.h"
#include "id3v2defs.h"

ID3v2Tag* GlobalID3Tag = NULL;

//prefs
uint8_t AtomicParsley_ID3v2Tag_MajorVersion = 4;
uint8_t AtomicParsley_ID3v2Tag_RevisionVersion = 0;
uint8_t AtomicParsley_ID3v2Tag_Flags = 0;

bool ID3v2Tag_Flag_Footer = false; //bit4; MPEG-4 'ID32' requires this to be false
bool ID3v2Tag_Flag_Experimental = true; //bit5
bool ID3v2Tag_Flag_ExtendedHeader = true; //bit6
bool ID3v2Tag_Flag_Unsyncronization = false; //bit7

///////////////////////////////////////////////////////////////////////////////////////
//                       id3 number conversion functions                             //
///////////////////////////////////////////////////////////////////////////////////////

uint64_t syncsafeXX_to_UInt64(char* syncsafe_int, uint8_t syncsafe_len) {
	if (syncsafe_len == 5) {
		if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80 || syncsafe_int[4] & 0x80) return 0;
		return ((uint64_t)syncsafe_int[0] << 28) | (syncsafe_int[1] << 21) | (syncsafe_int[2] << 14) | (syncsafe_int[3] << 7) | syncsafe_int[4];
	} else if (syncsafe_len == 6) {
		if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80 || syncsafe_int[4] & 0x80 || syncsafe_int[5] & 0x80) return 0;
		return ((uint64_t)syncsafe_int[0] << 35) | ((uint64_t)syncsafe_int[1] << 28) | (syncsafe_int[2] << 21) | (syncsafe_int[3] << 14) |
		                                                                               (syncsafe_int[4] << 7) | syncsafe_int[5];
	} else if (syncsafe_len == 7) {
		if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80 ||
		    syncsafe_int[4] & 0x80 || syncsafe_int[5] & 0x80 || syncsafe_int[6] & 0x80) return 0;
		return ((uint64_t)syncsafe_int[0] << 42) | ((uint64_t)syncsafe_int[1] << 35) | ((uint64_t)syncsafe_int[2] << 28) | (syncsafe_int[3] << 21) | 
		       (syncsafe_int[3] << 14) | (syncsafe_int[5] << 7) | syncsafe_int[6];
	} else if (syncsafe_len == 8) {
		if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80 ||
		    syncsafe_int[4] & 0x80 || syncsafe_int[5] & 0x80 || syncsafe_int[6] & 0x80 || syncsafe_int[7] & 0x80) return 0;
		return ((uint64_t)syncsafe_int[0] << 49) | ((uint64_t)syncsafe_int[1] << 42) | ((uint64_t)syncsafe_int[2] << 35) | ((uint64_t)syncsafe_int[3] << 28) |
					(syncsafe_int[4] << 21) | (syncsafe_int[5] << 14) | (syncsafe_int[6] << 7) | syncsafe_int[7];
	} else if (syncsafe_len == 9) {
		if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80 || syncsafe_int[4] & 0x80 || 
		    syncsafe_int[5] & 0x80 || syncsafe_int[6] & 0x80 || syncsafe_int[7] & 0x80 || syncsafe_int[8] & 0x80) return 0;
		return ((uint64_t)syncsafe_int[0] << 56) | ((uint64_t)syncsafe_int[1] << 49) | ((uint64_t)syncsafe_int[2] << 42) | ((uint64_t)syncsafe_int[3] << 35) |
					  ((uint64_t)syncsafe_int[4] << 28) | (syncsafe_int[5] << 21) | (syncsafe_int[6] << 14) | (syncsafe_int[7] << 7) | syncsafe_int[8];
	}
	return 0;
}

uint32_t syncsafe32_to_UInt32(char* syncsafe_int) {
	if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80 || syncsafe_int[2] & 0x80 || syncsafe_int[3] & 0x80) return 0;
	return (syncsafe_int[0] << 21) | (syncsafe_int[1] << 14) | (syncsafe_int[2] << 7) | syncsafe_int[3];
}

uint16_t syncsafe16_to_UInt16(char* syncsafe_int) {
	if (syncsafe_int[0] & 0x80 || syncsafe_int[1] & 0x80) return 0;
	return (syncsafe_int[0] << 7) | syncsafe_int[1];
}

void convert_to_syncsafe32(uint32_t in_uint, char* buffer) {
	buffer[0] = (in_uint >> 21) & 0x7F;
	buffer[1] = (in_uint >> 14) & 0x7F;
	buffer[2] = (in_uint >>  7) & 0x7F;
	buffer[3] = (in_uint >>  0) & 0x7F;
	return;
}

uint8_t convert_to_syncsafeXX(uint64_t in_uint, char* buffer) {
	if 
#if defined (_MSC_VER)
	(in_uint <= (uint64_t)34359738367)
#else
	(in_uint <= 34359738367ULL)
#endif
	{
		buffer[0] = (in_uint >> 28) & 0x7F;
		buffer[1] = (in_uint >> 21) & 0x7F;
		buffer[2] = (in_uint >> 14) & 0x7F;
		buffer[3] = (in_uint >>  7) & 0x7F;
		buffer[4] = (in_uint >>  0) & 0x7F;
		return 5;
#if defined (_MSC_VER)
	} else if (in_uint <= (uint64_t)4398046511103) {
#else
	} else if (in_uint <= 4398046511103ULL) {
#endif
		buffer[0] = (in_uint >> 35) & 0x7F;
		buffer[1] = (in_uint >> 28) & 0x7F;
		buffer[2] = (in_uint >> 21) & 0x7F;
		buffer[3] = (in_uint >> 14) & 0x7F;
		buffer[4] = (in_uint >>  7) & 0x7F;
		buffer[5] = (in_uint >>  0) & 0x7F;
		return 6;
#if defined (_MSC_VER)
	} else if (in_uint <= (uint64_t)562949953421311) {
#else
	} else if (in_uint <= 562949953421311ULL) {
#endif
		buffer[0] = (in_uint >> 42) & 0x7F;
		buffer[1] = (in_uint >> 35) & 0x7F;
		buffer[2] = (in_uint >> 28) & 0x7F;
		buffer[3] = (in_uint >> 21) & 0x7F;
		buffer[4] = (in_uint >> 14) & 0x7F;
		buffer[5] = (in_uint >>  7) & 0x7F;
		buffer[6] = (in_uint >>  0) & 0x7F;
		return 7;
#if defined (_MSC_VER)
	} else if (in_uint <= (uint64_t)72057594037927935) {
#else
	} else if (in_uint <= 72057594037927935ULL) {
#endif
		buffer[0] = (in_uint >> 49) & 0x7F;
		buffer[1] = (in_uint >> 42) & 0x7F;
		buffer[2] = (in_uint >> 35) & 0x7F;
		buffer[3] = (in_uint >> 28) & 0x7F;
		buffer[4] = (in_uint >> 21) & 0x7F;
		buffer[5] = (in_uint >> 14) & 0x7F;
		buffer[6] = (in_uint >>  7) & 0x7F;
		buffer[7] = (in_uint >>  0) & 0x7F;
		return 8;
#if defined (_MSC_VER)
	} else if (in_uint <= (uint64_t)9223372036854775807) {
#else
	} else if (in_uint <= 9223372036854775807ULL) { //that is some hardcore lovin'
#endif
		buffer[0] = (in_uint >> 56) & 0x7F;
		buffer[1] = (in_uint >> 49) & 0x7F;
		buffer[2] = (in_uint >> 42) & 0x7F;
		buffer[3] = (in_uint >> 35) & 0x7F;
		buffer[4] = (in_uint >> 28) & 0x7F;
		buffer[5] = (in_uint >> 21) & 0x7F;
		buffer[6] = (in_uint >> 14) & 0x7F;
		buffer[7] = (in_uint >>  7) & 0x7F;
		buffer[8] = (in_uint >>  0) & 0x7F;
		return 9;
	}
	return 0;
}

uint32_t UInt24FromBigEndian(const char *string) { //v2.2 frame lengths
	return ((0 << 24) | ((string[0] & 0xff) << 16) | ((string[1] & 0xff) << 8) | (string[2] & 0xff) << 0);
}

uint32_t ID3v2_desynchronize(char* buffer, uint32_t bufferlen) {
	char* buf_ptr = buffer;
	uint32_t desync_count = 0;
	
	for (uint32_t i = 0; i < bufferlen; i++) {
		if ((unsigned char)buffer[i] == 0xFF && (unsigned char)buffer[i+1] == 0x00) {
			buf_ptr[desync_count] = buffer[i];
			i++;
		} else {
			buf_ptr[desync_count] = buffer[i];
		}
		desync_count++;
	}
	return desync_count;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        bit tests & generic functions                              //
///////////////////////////////////////////////////////////////////////////////////////

bool ID3v2_PaddingTest(char* buffer) {
	if (buffer[0] & 0x00 || buffer[1] & 0x00 || buffer[2] & 0x00 || buffer[3] & 0x00) return true;
	return false;
}

bool ID3v2_TestFrameID_NonConformance(char* frameid) {
	for (uint8_t i=0; i < 4; i++) {
		if ( !((frameid[i] >= '0' && frameid[i] <= '9') || ( frameid[i] >= 'A' && frameid[i] <= 'Z' )) ) {
			return true;
		}
	}
	return false;
}

bool ID3v2_TestTagFlag(uint8_t TagFlag, uint8_t TagBit) {
	if (TagFlag & TagBit) return true;
	return false;
}

bool ID3v2_TestFrameFlag(uint16_t FrameFlag, uint16_t FrameBit) {
	if (FrameFlag & FrameBit) return true;
	return false;
}

uint8_t TextField_TestBOM(char* astring) {
	if (((unsigned char*)astring)[0] == 0xFE && ((unsigned char*)astring)[1] == 0xFF) return 13; //13 looks like a B for BE
	if (((unsigned char*)astring)[0] == 0xFF && ((unsigned char*)astring)[1] == 0xFE) return 1; //1 looks like a l for LE
	return 0;
}

void APar_LimitBufferRange(uint32_t max_allowed, uint32_t target_amount) {
	if (target_amount > max_allowed) {
		fprintf(stderr, "AtomicParsley error: insufficient memory to process ID3 tags (%" PRIu32 ">%" PRIu32 "). Exiting.\n", target_amount, max_allowed);
		exit( target_amount - max_allowed );
	}
	return;
}

void APar_ValidateNULLTermination8bit(ID3v2Fields* this_field) {
	if (this_field->field_string[0] == 0) {
		this_field->field_length = 1;
	} else if (this_field->field_string[this_field->field_length-1] != 0) {
		this_field->field_length += 1;
	}
	return;
}

void APar_ValidateNULLTermination16bit(ID3v2Fields* this_field, uint8_t encoding) {
	if (this_field->field_string[0] == 0 && this_field->field_string[1] == 0) {
		this_field->field_length = 2;
		if (encoding == TE_UTF16LE_WITH_BOM) {
			if ( ((uint8_t)(this_field->field_string[0]) != 0xFF && (uint8_t)(this_field->field_string[1]) != 0xFE) || 
			     ((uint8_t)(this_field->field_string[0]) != 0xFE && (uint8_t)(this_field->field_string[1]) != 0xFF) ) {
				memcpy(this_field->field_string, "\xFF\xFE", 2);
				this_field->field_length = 4;
			}
		}
	} else if (this_field->field_string[this_field->field_length-2] != 0 && this_field->field_string[this_field->field_length-1] != 0) {
		this_field->field_length += 2;
	}
	return;
}

bool APar_EvalFrame_for_Field(int frametype, int fieldtype) {
	uint8_t frametype_idx = GetFrameCompositionDescription(frametype);
	
	for (uint8_t fld_i = 0; fld_i < FrameTypeConstructionList[frametype_idx].ID3_FieldCount; fld_i++) {
		if (FrameTypeConstructionList[frametype_idx].ID3_FieldComponents[fld_i] == fieldtype) {
			return true;
		}
	}
	return false;
}

uint8_t TestCharInRange(uint8_t testchar, uint8_t lowerlimit, uint8_t upperlimit) {
	if (testchar >= lowerlimit && testchar <= upperlimit) {
		return 1;
	}
	return 0;
}

uint8_t ImageListMembers() {
 return (uint8_t)(sizeof(ImageList)/sizeof(*ImageList));
}

///////////////////////////////////////////////////////////////////////////////////////
//                             test functions                                        //
///////////////////////////////////////////////////////////////////////////////////////

void WriteZlibData(char* buffer, uint32_t buff_len) {
	char* indy_atom_path = (char *)malloc(sizeof(char)*MAXPATHLEN); //this malloc can escape memset because its only for in-house testing
	strcat(indy_atom_path, "/Users/");
	strcat(indy_atom_path, getenv("USER") );
	strcat(indy_atom_path, "/Desktop/id3framedata.txt");

	FILE* test_file = fopen(indy_atom_path, "wb");
	if (test_file != NULL) {
		
		fwrite(buffer, (size_t)buff_len, 1, test_file);
		fclose(test_file);
	}
	free(indy_atom_path);
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                               cli functions                                       //
///////////////////////////////////////////////////////////////////////////////////////

static const char* ReturnFrameTypeStr(int frametype) {
	if (frametype == ID3_TEXT_FRAME) {
		return "text frame             ";
	} else if (frametype == ID3_TEXT_FRAME_USERDEF) {
		return "user defined text frame";
	} else if (frametype == ID3_URL_FRAME) {
		return "url frame              ";
	} else if (frametype == ID3_URL_FRAME_USERDEF) {
		return "user defined url frame ";
	} else if (frametype == ID3_UNIQUE_FILE_ID_FRAME) {
		return "file ID                ";
	} else if (frametype == ID3_CD_ID_FRAME ) {
		return "AudioCD ID frame       ";
	} else if (frametype == ID3_DESCRIBED_TEXT_FRAME) {
		return "described text frame   ";
	} else if (frametype == ID3_ATTACHED_PICTURE_FRAME) {
		return "picture frame          ";
	} else if (frametype == ID3_ATTACHED_OBJECT_FRAME) {
		return "encapuslated object frm";			
	} else if (frametype == ID3_GROUP_ID_FRAME) {
		return "group ID frame         ";
	} else if (frametype == ID3_SIGNATURE_FRAME) {
		return "signature frame        ";
	} else if (frametype == ID3_PRIVATE_FRAME) {
		return "private frame          ";
	} else if (frametype == ID3_PLAYCOUNTER_FRAME) {
		return "playcounter            ";
	} else if (frametype == ID3_POPULAR_FRAME) {
		return "popularimeter          ";
	}
	return "";
} 

void ListID3FrameIDstrings() {
	const char* frametypestr = NULL;
	const char* presetpadding = NULL;
	uint16_t total_known_frames = (uint16_t)(sizeof(KnownFrames)/sizeof(*KnownFrames));
	fprintf(stdout, "ID3v2.4 Implemented Frames:\nframeID    type                   alias       Description\n--------------------------------------------------------------------------\n");
	for (uint16_t i = 1; i < total_known_frames; i++) {
		if (strlen(KnownFrames[i].ID3V2p4_FrameID) != 4) continue;
		frametypestr = ReturnFrameTypeStr(KnownFrames[i].ID3v2_FrameType);

		int strpad = 12 - strlen(KnownFrames[i].CLI_frameIDpreset);
		if (strpad == 12) {
			presetpadding = "            ";
		} else if (strpad == 11) {
			presetpadding = "           ";
		} else if (strpad == 10) {
			presetpadding = "          ";
		} else if (strpad == 9) {
			presetpadding = "         ";
		} else if (strpad == 8) {
			presetpadding = "        ";
		} else if (strpad == 7) {
			presetpadding = "       ";
		} else if (strpad == 6) {
			presetpadding = "      ";
		} else if (strpad == 5) {
			presetpadding = "     ";
		} else if (strpad == 4) {
			presetpadding = "    ";
		} else if (strpad == 3) {
			presetpadding = "   ";
		} else if (strpad == 2) {
			presetpadding = "  ";
		} else if (strpad == 1) {
			presetpadding = " ";
		} else if (strpad <= 0) {
			presetpadding = "";
		}

		fprintf(stdout, "%s   %s    %s%s | %s\n", KnownFrames[i].ID3V2p4_FrameID, frametypestr, KnownFrames[i].CLI_frameIDpreset, presetpadding, KnownFrames[i].ID3V2_FrameDescription);
	}
	fprintf(stdout,
"--------------------------------------------------------------------------\n"
"For each frame type, these parameters are available:\n"
"  text frames:                 (str) [encoding]\n"
"  user defined text frame :    (str) [desc=(str)] [encoding]\n"
"  url frame :                  (url)\n"
"  user defined url frame :     (url) [desc=(str)] [encoding]\n"
"  file ID frame :              (owner) [uniqueID={\"randomUUIDstamp\",(str)}]\n"
#if defined (DARWIN_PLATFORM)
"  AudioCD ID frame :           disk(num)\n"
#elif defined (HAVE_LINUX_CDROM_H)
"  AudioCD ID frame :           (/path)\n"
#elif defined (_WIN32)
"  AudioCD ID frame :           (letter)\n"
#endif
"  described text frame :       (str) [desc=(str)] [encoding]\n"
"  picture frame :              (/path) [desc=(str)] [mimetype=(str)] [imagetype=(hex)] [encoding]\n"
"  encapuslated object frame :  (/path) [desc=(str)] [mimetype=(str)] [filename={\"FILENAMESTAMP\",(str)}] [encoding]\n"
"  group ID frame :             (owner) groupsymbol=(hex) [data=(str)]\n"
"  signature frame :            (str) groupsymbol=(hex)\n"
"  private frame :              (owner) data=(str)\n"
"  playcounter :                (num or \"+1\")\n"
"  popularimeter :              (owner) rating=(1...255) [counter=(num or \"+1\")]\n"
"\n"
"   Legend:\n"
"    parameters in brackets[] signal an optional parameter, parens() signal a required parameter\n"
"     [encoding] may be one either the default UTF8, or one of { LATIN1 UTF16BE UTF16LE }\n"
"     (str) signals a string - like \"Suzie\"\n"
"     (num) means a number; +1 will increment a counter by 1; (hex) means a hexadecimal number - like 0x11)\n"
"     (url) menas a url, in string form; (owner) means a url/email string\n"
"     uniqueID=randomUUIDstamp will create a high quality random uuid\n"
"     filename=FILENAMESTAMP will embed the name of the file given in the /path for GEOB\n"
"\n"
"   All frames also take additional parameters:\n"
"     [{root,track=(num)}] specifies file level, track level or (default) movie level for an ID32 atom\n"
"     [compress] compresses the given frame using zlib deflate compression\n"
"     [groupsymbol=(num)] associates a frame with a GRID frame of the same group symbol\n"
"     [lang=(3char)] (default='eng') sets the language/ID32 atom to which the frame belongs\n"
"                    use AP --languages-list to see a list of available languages\n"
);
	
	
	return;
}

void List_imagtype_strings() {
	uint8_t total_imgtyps = (uint8_t)(sizeof(ImageTypeList)/sizeof(*ImageTypeList));
	fprintf(stdout, "These 'image types' are used to identify pictures embedded in 'APIC' ID3 tags:\n      usage is \"AP --ID3Tag APIC /path.jpg --imagetype=\"str\"\n      str can be either the hex listing *or* the full string\n      default is 0x00 - meaning 'Other'\n   Hex       Full String\n  ----------------------------\n");
	for (uint8_t i=0; i < total_imgtyps; i++) {
		fprintf(stdout, "  %s      \"%s\"\n", ImageTypeList[i].hexstring, ImageTypeList[i].imagetype_str);
	}
	return;
}

const char* ConvertCLIFrameStr_TO_frameID(const char* frame_str) {
	const char* discovered_frameID = NULL;
	uint16_t total_known_frames = (uint16_t)(sizeof(KnownFrames)/sizeof(*KnownFrames));
	
	for (uint16_t i = 0; i < total_known_frames; i++) {
		if (strcmp(KnownFrames[i].CLI_frameIDpreset, frame_str) == 0) {
			if (AtomicParsley_ID3v2Tag_MajorVersion == 2) discovered_frameID = KnownFrames[i].ID3V2p2_FrameID;
			if (AtomicParsley_ID3v2Tag_MajorVersion == 3) discovered_frameID = KnownFrames[i].ID3V2p3_FrameID;
			if (AtomicParsley_ID3v2Tag_MajorVersion == 4) discovered_frameID = KnownFrames[i].ID3V2p4_FrameID;
			
			if (strlen(discovered_frameID) == 0) discovered_frameID = NULL;
			break;
		}
	}
	return discovered_frameID;
}

//0 = description
//1 = mimetype
//2 = type
bool TestCLI_for_FrameParams(int frametype, uint8_t testparam) {
	if (frametype == ID3_URL_FRAME_USERDEF && testparam == 0) return true;
	
	if (frametype == ID3_UNIQUE_FILE_ID_FRAME && testparam == 3) {
		return true;
	} else if (frametype == ID3_ATTACHED_OBJECT_FRAME && testparam == 4) {
		return true;
	} else if (frametype == ID3_POPULAR_FRAME && testparam == 5) {
		return true;
	} else if (frametype == ID3_POPULAR_FRAME && testparam == 6) {
		return true;
	} else if (frametype == ID3_GROUP_ID_FRAME && testparam == 7) {
		return true;
	} else if (frametype == ID3_PRIVATE_FRAME && testparam == 8) {
		return true;
	} else {
		uint8_t frametype_idx = GetFrameCompositionDescription(frametype);
	
		for (uint8_t fld_i = 0; fld_i < FrameTypeConstructionList[frametype_idx].ID3_FieldCount; fld_i++) {
			if (FrameTypeConstructionList[frametype_idx].ID3_FieldComponents[fld_i] == ID3_DESCRIPTION_FIELD && testparam == 0) {
				return true;
			}
			if (FrameTypeConstructionList[frametype_idx].ID3_FieldComponents[fld_i] == ID3_MIME_TYPE_FIELD && testparam == 1) {
				return true;
			}
			if (FrameTypeConstructionList[frametype_idx].ID3_FieldComponents[fld_i] == ID3_PIC_TYPE_FIELD && testparam == 2) {
				return true;
			}
			if (FrameTypeConstructionList[frametype_idx].ID3_FieldComponents[fld_i] == ID3_PIC_TYPE_FIELD && testparam == 3) {
				return true;
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////
//                         frame identity functions                                  //
///////////////////////////////////////////////////////////////////////////////////////

int MatchID3FrameIDstr(const char* foundFrameID, uint8_t tagVersion) {
	uint16_t total_known_frames = (uint16_t)(sizeof(KnownFrames)/sizeof(*KnownFrames));
	uint8_t frameLen = (tagVersion >= 3 ? 4 : 3) +1;
	
	for (int i = 0; i < total_known_frames; i++) {
		const char* testFrameID = NULL;
		if (tagVersion == 2) testFrameID = KnownFrames[i].ID3V2p2_FrameID;
		if (tagVersion == 3) testFrameID = KnownFrames[i].ID3V2p3_FrameID;
		if (tagVersion == 4) testFrameID = KnownFrames[i].ID3V2p4_FrameID;
		
		if (memcmp(foundFrameID, testFrameID, frameLen) == 0) {
			return KnownFrames[i].ID3v2_InternalFrameID;
		}
	}
	return ID3v2_UNKNOWN_FRAME; //return the UnknownFrame if it can't be found
}

uint8_t GetFrameCompositionDescription(int ID3v2_FrameTypeID) {
	uint8_t matchingFrameDescription = 0; //return the UnknownFrame/UnknownField if it can't be found
	uint8_t total_frame_descrips = (uint8_t)(sizeof(FrameTypeConstructionList)/sizeof(*FrameTypeConstructionList));
	
	for (uint8_t i = 0; i < total_frame_descrips; i++) {		
		if (FrameTypeConstructionList[i].ID3_FrameType == ID3v2_FrameTypeID) {
			matchingFrameDescription = i;
			break;
		}
	}
	return matchingFrameDescription;
}

int FrameStr_TO_FrameType(const char* frame_str) {
	const char* eval_framestr = NULL;
	int frame_type = 0;
	uint16_t total_known_frames = (uint16_t)(sizeof(KnownFrames)/sizeof(*KnownFrames));
	
	for (uint16_t i = 0; i < total_known_frames; i++) {
		if (AtomicParsley_ID3v2Tag_MajorVersion == 2) eval_framestr = KnownFrames[i].ID3V2p2_FrameID;
		if (AtomicParsley_ID3v2Tag_MajorVersion == 3) eval_framestr = KnownFrames[i].ID3V2p3_FrameID;
		if (AtomicParsley_ID3v2Tag_MajorVersion == 4) eval_framestr = KnownFrames[i].ID3V2p4_FrameID;
			
		if (strcmp(frame_str, eval_framestr) == 0) {
			frame_type = KnownFrames[i].ID3v2_FrameType;
			break;
		}
	}
	return frame_type;
}

ID3v2Fields* APar_FindLastTextField(ID3v2Frame* aFrame) {
	ID3v2Fields* lastusedtextfield = NULL;
	if (aFrame->textfield_tally > 0) {
		lastusedtextfield = aFrame->ID3v2_Frame_Fields+1;
		while (true) {
			if (lastusedtextfield->next_field == NULL) {
				break;
			}
			lastusedtextfield = lastusedtextfield->next_field;
		}
	}	
	return lastusedtextfield;
}

bool APar_ExtraTextFieldInit(ID3v2Fields* lastField, uint32_t utf8len, uint8_t textencoding) {
	ID3v2Fields* extraField = NULL;
	lastField->next_field = (ID3v2Fields*)calloc(1, sizeof(ID3v2Fields)*1);
	if (lastField->next_field == NULL) {
		fprintf(stdout, "There was insufficient memory to allocate another ID3 field\n");
		exit(12);
	}
	extraField = lastField->next_field;
	extraField->ID3v2_Field_Type = ID3_TEXT_FIELD;
	extraField->field_length = 0;
	
	if (textencoding == TE_UTF16LE_WITH_BOM || textencoding == TE_UTF16BE_NO_BOM) {
		extraField->alloc_length = 2 + (utf8len * 2);
	} else {
		extraField->alloc_length = utf8len + 1;
	}
	if (extraField->alloc_length > 0) {
		extraField->field_string = (char*)calloc(1, sizeof(char*) * extraField->alloc_length);
		if (!APar_assert((extraField->field_string != NULL), 11, "while setting an extra text field") ) exit(11);
		return true;
	}		
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            id3 parsing functions                                  //
///////////////////////////////////////////////////////////////////////////////////////

uint32_t APar_ExtractField(char* buffer, uint32_t maxFieldLen, ID3v2Frame* thisFrame, ID3v2Fields* thisField, int fieldType, uint8_t textEncoding) {
	uint32_t bytes_used = 0;
	thisField->next_field = NULL;
	switch(fieldType) {
		case ID3_UNKNOWN_FIELD : { //the difference between this unknown field & say a binary data field is the unknown field is always the first (and only) field
			thisField->ID3v2_Field_Type = ID3_UNKNOWN_FIELD;
			thisField->field_length = maxFieldLen;
			thisField->field_string = (char*)calloc(1, sizeof(char)*(maxFieldLen+1 > 16 ? maxFieldLen+1 : 16));
			thisField->alloc_length = sizeof(char)*(maxFieldLen+1 > 16 ? maxFieldLen+1 : 16);
			memcpy(thisField->field_string, buffer, maxFieldLen);
			
			bytes_used = maxFieldLen;
			break;
		}
		case ID3_PIC_TYPE_FIELD :
		case ID3_GROUPSYMBOL_FIELD :
		case ID3_TEXT_ENCODING_FIELD : {
			thisField->ID3v2_Field_Type = fieldType;
			thisField->field_length = 1;
			thisField->field_string = (char*)calloc(1, sizeof(char)*16);
			thisField->field_string[0] = buffer[0]; //memcpy(thisField->field_string, buffer, 1);
			thisField->alloc_length = sizeof(char)*16;
			
			bytes_used = 1;
			break;
		}
		case ID3_LANGUAGE_FIELD : {
			thisField->ID3v2_Field_Type = ID3_LANGUAGE_FIELD;
			thisField->field_length = 3;
			thisField->field_string = (char*)calloc(1, sizeof(char)*16);
			memcpy(thisField->field_string, buffer, 3);
			thisField->alloc_length = sizeof(char)*16;
			
			bytes_used = 3;
			break;
		}
		case ID3_TEXT_FIELD :
		case ID3_URL_FIELD :
		case ID3_COUNTER_FIELD :
		case ID3_BINARY_DATA_FIELD : { //this class of fields may contains NULLs but is *NOT* NULL terminated in any form
			thisField->ID3v2_Field_Type = fieldType;
			thisField->field_length = maxFieldLen;
			thisField->field_string = (char*)calloc(1, sizeof(char)*maxFieldLen+1 > 16 ? maxFieldLen+1 : 16);
			memcpy(thisField->field_string, buffer, maxFieldLen);
			thisField->alloc_length = (sizeof(char)*maxFieldLen+1 > 16 ? maxFieldLen+1 : 16);
			
			if (fieldType == ID3_TEXT_FIELD) {
				bytes_used = findstringNULLterm(buffer, textEncoding, maxFieldLen);			
			} else {
				bytes_used = maxFieldLen;
			}
			break;
		}
		case ID3_MIME_TYPE_FIELD :
		case ID3_OWNER_FIELD : { //difference between ID3_OWNER_FIELD & ID3_DESCRIPTION_FIELD field classes is the owner field is always 8859-1 encoded (single NULL term)
			thisField->ID3v2_Field_Type = fieldType;
			thisField->field_length = findstringNULLterm(buffer, 0, maxFieldLen);
			thisField->field_string = (char*)calloc(1, sizeof(char) * thisField->field_length +1 > 16 ? thisField->field_length +1 : 16);
			memcpy(thisField->field_string, buffer, thisField->field_length);
			thisField->alloc_length = (sizeof(char)*maxFieldLen+1 > 16 ? maxFieldLen+1 : 16);
						
			bytes_used = thisField->field_length;
			break;
		
		}
		case ID3_FILENAME_FIELD :
		case ID3_DESCRIPTION_FIELD : {
			thisField->ID3v2_Field_Type = fieldType;
			thisField->field_length = findstringNULLterm(buffer, textEncoding, maxFieldLen);
			thisField->field_string = (char*)calloc(1, sizeof(char) * thisField->field_length +1 > 16 ? thisField->field_length +1 : 16);
			memcpy(thisField->field_string, buffer, thisField->field_length);
			thisField->alloc_length = (sizeof(char) * thisField->field_length +1 > 16 ? thisField->field_length +1 : 16);
						
			bytes_used = thisField->field_length;
			break;
		}
	}
	//fprintf(stdout, "%" PRIu32 ", %s, %s\n", bytes_used, buffer, (thisFrame->ID3v2_Frame_Fields+fieldNum)->field_string);
	return bytes_used;
}

void APar_ScanID3Frame(ID3v2Frame* targetframe, char* frame_ptr, uint32_t frameLen) {
	uint64_t offset_into_frame = 0;
	
	switch(targetframe->ID3v2_FrameType) {
		case ID3_UNKNOWN_FRAME : {
			APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_UNKNOWN_FIELD, 0);
			break;
		}
		case ID3_TEXT_FRAME : {
			uint8_t textencoding = 0xFF;
			offset_into_frame += APar_ExtractField(frame_ptr, 1, targetframe, targetframe->ID3v2_Frame_Fields, ID3_TEXT_ENCODING_FIELD, 0);
			
			offset_into_frame += APar_ExtractField(frame_ptr + 1, frameLen - 1, targetframe, targetframe->ID3v2_Frame_Fields+1,
			                                        ID3_TEXT_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			targetframe->textfield_tally++;
			
			if (offset_into_frame >= frameLen) break;
			textencoding = targetframe->ID3v2_Frame_Fields->field_string[0];
			
			if (offset_into_frame < frameLen) {
				while (true) {
					if (offset_into_frame >= frameLen) break;
					
					//skip the required separator for multiple strings
					if (textencoding == TE_LATIN1 || textencoding == TE_UTF8) {
						offset_into_frame += 1;
					} else if (textencoding == TE_UTF16LE_WITH_BOM) {
						offset_into_frame += 2;
					}
					
					//multiple id3v2.4 strings should be separated with a single NULL byte; some implementations might terminate the string AND use a NULL separator
					if (textencoding == TE_LATIN1 || textencoding == TE_UTF8) {
						if ((frame_ptr + offset_into_frame)[0] == 0) offset_into_frame+=1;
					} else if (textencoding == TE_UTF16LE_WITH_BOM) {
						if ((frame_ptr + offset_into_frame)[0] == 0 && (frame_ptr + offset_into_frame)[1] == 0) offset_into_frame+=2;
					}
					
					//a 3rd NULL would not be good
					if (textencoding == TE_LATIN1 || textencoding == TE_UTF8) {
						if ((frame_ptr + offset_into_frame)[0] == 0) break;
					} else if (textencoding == TE_UTF16LE_WITH_BOM) {
						if ((frame_ptr + offset_into_frame)[0] == 0 && (frame_ptr + offset_into_frame)[1] == 0) break;
					}
					
					ID3v2Fields* last_textfield = APar_FindLastTextField(targetframe);
					if (APar_ExtraTextFieldInit(last_textfield, frameLen - offset_into_frame, textencoding)) {
						offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, last_textfield->next_field,
																									 ID3_TEXT_FIELD, textencoding);
						targetframe->textfield_tally++;
					}
					//copy the string to the new field
					break;
				}
			}
			break;
		}
		case ID3_URL_FRAME : {
			APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_URL_FIELD, 0);
			break;
		}
		case ID3_TEXT_FRAME_USERDEF :
		case ID3_URL_FRAME_USERDEF : {
			offset_into_frame += APar_ExtractField(frame_ptr, 1, targetframe, targetframe->ID3v2_Frame_Fields, ID3_TEXT_ENCODING_FIELD, 0);
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, 
			                                       targetframe->ID3v2_Frame_Fields+1, ID3_DESCRIPTION_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			
			offset_into_frame += skipNULLterm(frame_ptr + offset_into_frame, targetframe->ID3v2_Frame_Fields->field_string[0], frameLen - offset_into_frame);
			
			if (targetframe->ID3v2_FrameType == ID3_TEXT_FRAME_USERDEF) {
				APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe,
				                  targetframe->ID3v2_Frame_Fields+2, ID3_TEXT_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			} else if (targetframe->ID3v2_FrameType == ID3_URL_FRAME_USERDEF) {
				APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe,
				                  targetframe->ID3v2_Frame_Fields+2, ID3_URL_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			}
			break;
		}
		case ID3_UNIQUE_FILE_ID_FRAME : {
			offset_into_frame += APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_OWNER_FIELD, 0);
			offset_into_frame++; //iso-8859-1 owner field is NULL terminated
			
			APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_BINARY_DATA_FIELD, 0);
			break;
		}
		case ID3_CD_ID_FRAME : {
			APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_BINARY_DATA_FIELD, 0);
			break;
		}
		case ID3_DESCRIBED_TEXT_FRAME : {
			offset_into_frame += APar_ExtractField(frame_ptr, 1, targetframe, targetframe->ID3v2_Frame_Fields, ID3_TEXT_ENCODING_FIELD, 0);
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, 3, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_LANGUAGE_FIELD, 0);
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe,
			                                       targetframe->ID3v2_Frame_Fields+2, ID3_DESCRIPTION_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			
			offset_into_frame += skipNULLterm(frame_ptr + offset_into_frame, targetframe->ID3v2_Frame_Fields->field_string[0], frameLen - offset_into_frame);
			
			if (frameLen > offset_into_frame) {
				APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+3,
				                  ID3_TEXT_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			}
			break;
		}
		case ID3_ATTACHED_OBJECT_FRAME :
		case ID3_ATTACHED_PICTURE_FRAME : {
			offset_into_frame += APar_ExtractField(frame_ptr, 1, targetframe, targetframe->ID3v2_Frame_Fields, ID3_TEXT_ENCODING_FIELD, 0);
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - 1, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_MIME_TYPE_FIELD, 0);
			
			offset_into_frame += 1; //should only be 1 NULL
			
			if (targetframe->ID3v2_FrameType == ID3_ATTACHED_PICTURE_FRAME) {
				offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, 1, targetframe, targetframe->ID3v2_Frame_Fields+2, ID3_PIC_TYPE_FIELD, 0);
			} else {
				offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe,
				                                       targetframe->ID3v2_Frame_Fields+2, ID3_FILENAME_FIELD, 0);
			
				offset_into_frame+=skipNULLterm(frame_ptr + offset_into_frame, targetframe->ID3v2_Frame_Fields->field_string[0], frameLen - offset_into_frame);
			}
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+3, 
			                                       ID3_DESCRIPTION_FIELD, targetframe->ID3v2_Frame_Fields->field_string[0]);
			
			offset_into_frame += skipNULLterm(frame_ptr + offset_into_frame, targetframe->ID3v2_Frame_Fields->field_string[0], frameLen - offset_into_frame);
			
			if (frameLen > offset_into_frame) {
				offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+4,
				                                       ID3_BINARY_DATA_FIELD, 0);
			}
			break;
		}
		case ID3_PRIVATE_FRAME : { //the only difference between the 'priv' frame & the 'ufid' frame is ufid is limited to 64 bytes
			offset_into_frame += APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_OWNER_FIELD, 0);
			offset_into_frame++; //iso-8859-1 owner field is NULL terminated
			
			APar_ExtractField(frame_ptr + offset_into_frame, frameLen - 1, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_BINARY_DATA_FIELD, 0);
			break;
		}
		case ID3_GROUP_ID_FRAME : {
			offset_into_frame += APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_OWNER_FIELD, 0);
			offset_into_frame++; //iso-8859-1 owner field is NULL terminated
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, 1, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_GROUPSYMBOL_FIELD, 0);
			
			if (frameLen > offset_into_frame) {
				APar_ExtractField(frame_ptr + offset_into_frame, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+2, ID3_BINARY_DATA_FIELD, 0);
			}
			break;
		}
		case ID3_SIGNATURE_FRAME : {
			APar_ExtractField(frame_ptr, 1, targetframe, targetframe->ID3v2_Frame_Fields, ID3_GROUPSYMBOL_FIELD, 0);
			
			APar_ExtractField(frame_ptr + 1, frameLen - 1, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_BINARY_DATA_FIELD, 0);
			break;
		}
		case ID3_PLAYCOUNTER_FRAME : {
			APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_COUNTER_FIELD, 0);
			break;
		}
		case ID3_POPULAR_FRAME : {
			offset_into_frame += APar_ExtractField(frame_ptr, frameLen, targetframe, targetframe->ID3v2_Frame_Fields, ID3_OWNER_FIELD, 0); //surrogate for 'emai to user' field
			offset_into_frame++; //iso-8859-1 email address field is NULL terminated
			
			offset_into_frame += APar_ExtractField(frame_ptr + offset_into_frame, 1, targetframe, targetframe->ID3v2_Frame_Fields+1, ID3_BINARY_DATA_FIELD, 0);
			
			if (frameLen > offset_into_frame) {
				APar_ExtractField(frame_ptr, frameLen - offset_into_frame, targetframe, targetframe->ID3v2_Frame_Fields+2, ID3_COUNTER_FIELD, 0);
			}
			break;
		}
		case ID3_OLD_V2P2_PICTURE_FRAME : {
			break; //unimplemented
		}
	}
	return;
}

void APar_ID32_ScanID3Tag(FILE* source_file, AtomicInfo* id32_atom) {
	char* id32_fulltag = (char*)calloc(1, sizeof(char)*id32_atom->AtomicLength);
	char* fulltag_ptr = id32_fulltag;
	
	if (id32_atom->AtomicLength < 20) return;
	APar_readX(id32_fulltag, source_file, id32_atom->AtomicStart+14, id32_atom->AtomicLength-14); //+10 = 4bytes ID32 atom length + 4bytes ID32 atom name + 2 bytes packed lang
	
	if (memcmp(id32_fulltag, "ID3", 3) != 0) return;
	fulltag_ptr+=3;
	
	id32_atom->ID32_TagInfo = (ID3v2Tag*)calloc(1, sizeof(ID3v2Tag));
	id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion = *fulltag_ptr;
	fulltag_ptr++;
	id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion = *fulltag_ptr;
	fulltag_ptr++;
	id32_atom->ID32_TagInfo->ID3v2Tag_Flags = *fulltag_ptr;
	fulltag_ptr++;
	
	if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion != 4) {
		fprintf(stdout, "AtomicParsley warning: an ID32 atom was encountered using an unsupported ID3v2 tag version: %u. Skipping\n", id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion);
		return;
	}
	if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4 && id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion != 0) {
		fprintf(stdout, "AtomicParsley warning: an ID32 atom was encountered using an unsupported ID3v2.4 tag revision: %u. Skipping\n", id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion);
		return;
	}
	
	if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_BIT0+ID32_TAGFLAG_BIT1+ID32_TAGFLAG_BIT2+ID32_TAGFLAG_BIT3)) return;
	
	if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_FOOTER)) {
		fprintf(stdout, "AtomicParsley error: an ID32 atom was encountered with a forbidden footer flag. Exiting.\n");
		free(id32_fulltag);
		id32_fulltag = NULL;
		return;
	}
		
	if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_EXPERIMENTAL)) {
#if defined(DEBUG_V)
		fprintf(stdout, "AtomicParsley warning: an ID32 atom was encountered with an experimental flag set.\n");
#endif
	}
	
	if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
		id32_atom->ID32_TagInfo->ID3v2Tag_Length = syncsafe32_to_UInt32(fulltag_ptr);
		fulltag_ptr+=4;
	} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3) {
		id32_atom->ID32_TagInfo->ID3v2Tag_Length = UInt32FromBigEndian(fulltag_ptr); //TODO: when testing ends, this switches to syncsafe
		fulltag_ptr+=4;
	} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 2) {
		id32_atom->ID32_TagInfo->ID3v2Tag_Length = UInt24FromBigEndian(fulltag_ptr);
		fulltag_ptr+=3;
	}
	
	if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_UNSYNCRONIZATION)) {
		//uint32_t newtagsize = ID3v2_desynchronize(id32_fulltag, id32_atom->ID32_TagInfo->ID3v2Tag_Length);
		//fprintf(stdout, "New tag size is %" PRIu32 "\n", newtagsize);
		//WriteZlibData(id32_fulltag, newtagsize);
		//exit(0);
		fprintf(stdout, "AtomicParsley error: an ID3 tag with the unsynchronized flag set which is not supported. Skipping.\n");
		free(id32_fulltag);
		id32_fulltag = NULL;
		return;
	}

	if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_EXTENDEDHEADER)) {
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
			id32_atom->ID32_TagInfo->ID3v2_Tag_ExtendedHeader_Length = syncsafe32_to_UInt32(fulltag_ptr);
		} else {
			id32_atom->ID32_TagInfo->ID3v2_Tag_ExtendedHeader_Length = UInt32FromBigEndian(fulltag_ptr); //TODO: when testing ends, this switches to syncsafe; 2.2 doesn't have it
		}
		fulltag_ptr+= id32_atom->ID32_TagInfo->ID3v2_Tag_ExtendedHeader_Length;
	}
	
	id32_atom->ID32_TagInfo->ID3v2_FirstFrame = NULL;
	id32_atom->ID32_TagInfo->ID3v2_FrameList = NULL;
	
	//loop through parsing frames
	while (fulltag_ptr < id32_fulltag + (id32_atom->AtomicLength-14) ) {
		uint32_t fullframesize = 0;

		if (ID3v2_PaddingTest(fulltag_ptr)) break;
		if (ID3v2_TestFrameID_NonConformance(fulltag_ptr)) break;
		
		ID3v2Frame* target_list_frameinfo = (ID3v2Frame*)calloc(1, sizeof(ID3v2Frame));
		target_list_frameinfo->ID3v2_NextFrame = NULL;
		target_list_frameinfo->ID3v2_Frame_ID = MatchID3FrameIDstr(fulltag_ptr, id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion);
		target_list_frameinfo->ID3v2_FrameType = KnownFrames[target_list_frameinfo->ID3v2_Frame_ID+1].ID3v2_FrameType;
		
		uint8_t FrameCompositionList = GetFrameCompositionDescription(target_list_frameinfo->ID3v2_FrameType);
		target_list_frameinfo->ID3v2_FieldCount = FrameTypeConstructionList[FrameCompositionList].ID3_FieldCount;
		target_list_frameinfo->ID3v2_Frame_ExpandedLength = 0;
		target_list_frameinfo->textfield_tally = 0;
		target_list_frameinfo->eliminate_frame = false;
		uint8_t frame_offset = 0;

		if (id32_atom->ID32_TagInfo->ID3v2_FrameList != NULL) id32_atom->ID32_TagInfo->ID3v2_FrameList->ID3v2_NextFrame = target_list_frameinfo;

		//need to lookup how many components this Frame_ID is associated with. Do this by using the corresponding KnownFrames.ID3v2_FrameType
		//ID3v2_FrameType describes the general form this frame takes (text, text with description, attached object, attached picture)
		//the general form is composed of several fields; that number of fields needs to be malloced to target_list_frameinfo->ID3v2_Frame_Fields
		//and each target_list_frameinfo->ID3v2_Frame_Fields+num->field_string needs to be malloced and copied from id32_fulltag
		
		memset(target_list_frameinfo->ID3v2_Frame_Namestr, 0, 5);
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 2) {
			memcpy(target_list_frameinfo->ID3v2_Frame_Namestr, fulltag_ptr, 3);
			fulltag_ptr+= 3;
		} else {
			memcpy(target_list_frameinfo->ID3v2_Frame_Namestr, fulltag_ptr, 4);
			fulltag_ptr+= 4;
		}
		
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
			target_list_frameinfo->ID3v2_Frame_Length = syncsafe32_to_UInt32(fulltag_ptr);
			fulltag_ptr+=4;
		} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3) {
			target_list_frameinfo->ID3v2_Frame_Length = UInt32FromBigEndian(fulltag_ptr); //TODO: when testing ends, this switches to syncsafe
			fulltag_ptr+=4;
		} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 2) {
			target_list_frameinfo->ID3v2_Frame_Length = UInt24FromBigEndian(fulltag_ptr);
			fulltag_ptr+=3;
		}
		
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion >= 3) {
			target_list_frameinfo->ID3v2_Frame_Flags = UInt16FromBigEndian(fulltag_ptr); //v2.2 doesn't have frame level flags (but it does have field level flags)
			fulltag_ptr+=2;
			
			if (ID3v2_TestFrameFlag(target_list_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_UNSYNCED)) {
				//DE-UNSYNC frame
				fullframesize = target_list_frameinfo->ID3v2_Frame_Length;
				target_list_frameinfo->ID3v2_Frame_Length = ID3v2_desynchronize(fulltag_ptr+frame_offset, target_list_frameinfo->ID3v2_Frame_Length);
				target_list_frameinfo->ID3v2_Frame_Flags -= ID32_FRAMEFLAG_UNSYNCED;
			}
			
			//info based on frame flags (order based on the order of flags defined by the frame flags
			if (ID3v2_TestFrameFlag(target_list_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_GROUPING)) {
#if defined(DEBUG_V)
				fprintf(stdout, "Frame %s has a grouping flag set\n", target_list_frameinfo->ID3v2_Frame_Namestr);
#endif
				target_list_frameinfo->ID3v2_Frame_GroupingSymbol = *fulltag_ptr; //er, uh... wouldn't this also require ID32_FRAMEFLAG_LENINDICATED to be set???
				frame_offset++;
			}
			
			if (ID3v2_TestFrameFlag(target_list_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED)) { // technically ID32_FRAMEFLAG_LENINDICATED should also be tested
#if defined(DEBUG_V)
				fprintf(stdout, "Frame %s has a compressed flag set\n", target_list_frameinfo->ID3v2_Frame_Namestr);
#endif
				if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
					target_list_frameinfo->ID3v2_Frame_ExpandedLength = syncsafe32_to_UInt32(fulltag_ptr+frame_offset);
					frame_offset+=4;
				} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3) {
					target_list_frameinfo->ID3v2_Frame_ExpandedLength = UInt32FromBigEndian(fulltag_ptr+frame_offset); //TODO: when testing ends, switch this to syncsafe
					frame_offset+=4;
				}
			}
		}
		
		
		target_list_frameinfo->ID3v2_Frame_Fields = (ID3v2Fields*)calloc(1, sizeof(ID3v2Fields)*target_list_frameinfo->ID3v2_FieldCount);
		char* expanded_frame = NULL;
		char* frame_ptr = NULL;
		uint32_t frameLen = 0;
		
		if (target_list_frameinfo->ID3v2_Frame_ExpandedLength != 0) {
#ifdef HAVE_ZLIB_H
			expanded_frame = (char*)calloc(1, sizeof(char)*target_list_frameinfo->ID3v2_Frame_ExpandedLength + 1);
			APar_zlib_inflate(fulltag_ptr+frame_offset, target_list_frameinfo->ID3v2_Frame_Length, expanded_frame, target_list_frameinfo->ID3v2_Frame_ExpandedLength);
			
			WriteZlibData(expanded_frame, target_list_frameinfo->ID3v2_Frame_ExpandedLength);
			
			frame_ptr = expanded_frame;
			frameLen = target_list_frameinfo->ID3v2_Frame_ExpandedLength;
#else
			target_list_frameinfo->ID3v2_FrameType = ID3_UNKNOWN_FRAME;
			frame_ptr = fulltag_ptr+frame_offset;
			frameLen = target_list_frameinfo->ID3v2_Frame_ExpandedLength;
#endif
		} else {

			frame_ptr = fulltag_ptr+frame_offset;
			frameLen = target_list_frameinfo->ID3v2_Frame_Length; 
		}
		
		APar_ScanID3Frame(target_list_frameinfo, frame_ptr, frameLen);
		
		if (expanded_frame != NULL) {
			free(expanded_frame);
			expanded_frame = NULL;
		}

		if (target_list_frameinfo != NULL) {
			if (id32_atom->ID32_TagInfo->ID3v2_FrameCount == 0) {
				id32_atom->ID32_TagInfo->ID3v2_FirstFrame = target_list_frameinfo; //entrance to the linked list
			}
			id32_atom->ID32_TagInfo->ID3v2_FrameList = target_list_frameinfo; //this always points to the last frame that had the scan completed
		}
		
		if (fullframesize != 0) {
			fulltag_ptr+= fullframesize;
		} else {
			fulltag_ptr+= target_list_frameinfo->ID3v2_Frame_Length;
		}
		if (ID3v2_TestFrameFlag(target_list_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_GROUPING)) {
			fulltag_ptr++;
		}
		id32_atom->ID32_TagInfo->ID3v2_FrameCount++;
	}
	
	id32_atom->ID32_TagInfo->modified_tag = false; //if a frame is altered/added/removed, change this to true and render the tag & fill id32_atom-AtomicData with the tag
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                         id3 rendering functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

bool APar_LocateFrameSymbol(AtomicInfo* id32_atom, ID3v2Frame* targetFrame, uint8_t groupsymbol) {
	ID3v2Frame* testFrame = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
	while (testFrame != NULL) {
		if (targetFrame->ID3v2_Frame_ID == ID3v2_FRAME_GRID && testFrame->ID3v2_Frame_ID != ID3v2_FRAME_GRID) {
			if (testFrame->ID3v2_Frame_GroupingSymbol == groupsymbol) {
				return true;
			}
		} else if (targetFrame->ID3v2_Frame_ID != ID3v2_FRAME_GRID) {
			if (testFrame->ID3v2_Frame_ID == ID3v2_FRAME_GRID && groupsymbol == (uint8_t)(testFrame->ID3v2_Frame_Fields+1)->field_string[0]) {
				return true;
			}
		}
		testFrame = testFrame->ID3v2_NextFrame;
	}
	return false;
}

void APar_FrameFilter(AtomicInfo* id32_atom) {
	ID3v2Frame* MCDI_frame = NULL;
	ID3v2Frame* TRCK_frame = NULL;
	ID3v2Frame* thisFrame = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
	while (thisFrame != NULL) {
		if (!thisFrame->eliminate_frame) {
			if (thisFrame->ID3v2_FrameType == ID3_CD_ID_FRAME) {
				MCDI_frame = thisFrame;
			}
			if (thisFrame->ID3v2_Frame_ID == ID3v2_FRAME_TRACKNUM) {
				TRCK_frame = thisFrame;
			}
			if (thisFrame->ID3v2_Frame_ID == ID3v2_FRAME_GRID) { //find any frames containing this symbol; if none are present this frame will be discarded
				thisFrame->eliminate_frame = !APar_LocateFrameSymbol(id32_atom, thisFrame, (uint8_t)(thisFrame->ID3v2_Frame_Fields+1)->field_string[0]);
				if (!thisFrame->eliminate_frame) {
					thisFrame->ID3v2_Frame_Flags |= ID32_FRAMEFLAG_GROUPING;
				}
				
			} else if (thisFrame->ID3v2_Frame_ID == ID3v2_FRAME_SIGNATURE) { //find a GRID frame that contains this symbol (@ field_string, not ID3v2_Frame_GroupingSymbol)
				thisFrame->eliminate_frame = !APar_LocateFrameSymbol(id32_atom, thisFrame, (uint8_t)thisFrame->ID3v2_Frame_Fields->field_string[0]);
				//since the group symbol is carried as a field for SIGN, no need to set the frame's grouping bit in the frame flags
			
			} else if (thisFrame->ID3v2_Frame_GroupingSymbol > 0) { //find a GRID frame that contains this symbol, otherwise discard it
				thisFrame->eliminate_frame = !APar_LocateFrameSymbol(id32_atom, thisFrame, thisFrame->ID3v2_Frame_GroupingSymbol);
				if (!thisFrame->eliminate_frame) {
					thisFrame->ID3v2_Frame_Flags |= ID32_FRAMEFLAG_GROUPING;
				}
				
			}
		}
		thisFrame = thisFrame->ID3v2_NextFrame;
	}
	
	if (MCDI_frame != NULL && TRCK_frame == NULL) {
		fprintf(stderr, "AP warning: the MCDI frame was skipped due to a missing TRCK frame\n");
		MCDI_frame->eliminate_frame = true;
	}
	return;
}

uint32_t APar_GetTagSize(AtomicInfo* id32_atom) { // a rough approximation of how much to malloc; this will be larger than will be ultimately required
	uint32_t tag_len = 0;
	uint16_t surviving_frame_count = 0;
	if (id32_atom->ID32_TagInfo->modified_tag == false) return tag_len;
	if (id32_atom->ID32_TagInfo->ID3v2_FrameCount == 0) return tag_len; //but a frame isn't removed by AP; its just marked for elimination
	if (id32_atom->ID32_TagInfo->ID3v2_FrameList == NULL) return tag_len; //something went wrong somewhere if this wasn't an entry to a linked list of frames
	if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion != 4) return tag_len; //only id3 version 2.4 tags are written
	
	ID3v2Frame* eval_frame = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
	while (eval_frame != NULL) {
		if (eval_frame->eliminate_frame == true)  {
			eval_frame = eval_frame->ID3v2_NextFrame;
			continue;
		}
		tag_len += 15; //4bytes frameID 'TCON', 4bytes frame length (syncsafe int), 2 bytes frame flags; optional group symbol: 1byte + decompressed length 4bytes
		tag_len += 2*eval_frame->ID3v2_FieldCount; //excess amount to ensure that text fields have utf16 BOMs & 2 byte NULL terminations as required
		if (ID3v2_TestFrameFlag(eval_frame->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED)) {
			tag_len += eval_frame->ID3v2_Frame_ExpandedLength;
		} else {
			tag_len += eval_frame->ID3v2_Frame_Length;
		}
		surviving_frame_count++;
		eval_frame = eval_frame->ID3v2_NextFrame;
		if (surviving_frame_count == 0 && eval_frame == NULL) {
		}
	}
	if (surviving_frame_count == 0) return 0; //the 'ID3' header alone isn't going to be written with 0 existing frames
	return tag_len;
}

void APar_RenderFields(char* dest_buffer, uint32_t max_alloc, ID3v2Tag* id3_tag, ID3v2Frame* id3v2_frame, uint32_t* frame_header_len, uint32_t* frame_length) {
	uint8_t encoding_val = 0;
	if (id3v2_frame->ID3v2_Frame_Fields == NULL) {
		*frame_header_len = 0;
		*frame_length = 0;
		return;
	}
	
	for (uint8_t fld_idx = 0; fld_idx < id3v2_frame->ID3v2_FieldCount; fld_idx++) {
		ID3v2Fields* this_field = id3v2_frame->ID3v2_Frame_Fields+fld_idx;
		//fprintf(stdout, "Total Fields for %s: %u (this is %u, %u)\n", id3v2_frame->ID3v2_Frame_Namestr, id3v2_frame->ID3v2_FieldCount, fld_idx, this_field->ID3v2_Field_Type);
		switch(this_field->ID3v2_Field_Type) {
			
			//these are raw data fields of variable/fixed length and are not NULL terminated
			case ID3_UNKNOWN_FIELD : 
			case ID3_PIC_TYPE_FIELD :
			case ID3_GROUPSYMBOL_FIELD :
			case ID3_TEXT_ENCODING_FIELD :
			case ID3_LANGUAGE_FIELD :
			case ID3_COUNTER_FIELD :
			case ID3_IMAGEFORMAT_FIELD :
			case ID3_URL_FIELD :
			case ID3_BINARY_DATA_FIELD : {
				APar_LimitBufferRange(max_alloc, *frame_header_len + *frame_length);
				if (this_field->field_string != NULL) {
					memcpy(dest_buffer + *frame_length, this_field->field_string, this_field->field_length);
					*frame_length += this_field->field_length;
					//fprintf(stdout, "Field idx %u(%d) is now %" PRIu32 " bytes long (+%" PRIu32 ")\n", fld_idx, this_field->ID3v2_Field_Type, *frame_length, this_field->field_length);
				}
				break;
			}
			
			//these fields are subject to NULL byte termination - based on what the text encoding field says the encoding of this string is
			case ID3_TEXT_FIELD :
			case ID3_FILENAME_FIELD :
			case ID3_DESCRIPTION_FIELD : {
				if (this_field->field_string == NULL) {
					*frame_header_len = 0;
					*frame_length = 0;
					return;
				} else {
					APar_LimitBufferRange(max_alloc, *frame_header_len + *frame_length +2); //+2 for a possible extra NULLs
					encoding_val = id3v2_frame->ID3v2_Frame_Fields->field_string[0]; //ID3_TEXT_ENCODING_FIELD is always the first field, and should have an encoding
					if ( (id3_tag->ID3v2Tag_MajorVersion == 4 && encoding_val == TE_UTF8) || encoding_val == TE_LATIN1 ) {
						if (this_field->ID3v2_Field_Type != ID3_TEXT_FIELD) APar_ValidateNULLTermination8bit(this_field);
						
						memcpy(dest_buffer + *frame_length, this_field->field_string, this_field->field_length);
						*frame_length += this_field->field_length;

					} else if ((id3_tag->ID3v2Tag_MajorVersion == 4 && encoding_val == TE_UTF16LE_WITH_BOM) || encoding_val == TE_UTF16BE_NO_BOM) {
						APar_ValidateNULLTermination16bit(this_field, encoding_val); //TODO: shouldn't this also exclude ID3_TEXT_FIELDs?
						
						memcpy(dest_buffer + *frame_length, this_field->field_string, this_field->field_length);
						*frame_length += this_field->field_length;
					
					} else { //well, AP didn't set this frame, so just duplicate it.
						memcpy(dest_buffer + *frame_length, this_field->field_string, this_field->field_length);
						*frame_length += this_field->field_length;
					}
				}
				//fprintf(stdout, "Field idx %u(%d) is now %" PRIu32 " bytes long\n", fld_idx, this_field->ID3v2_Field_Type, *frame_length);
				break;
			}
			
			//these are iso 8859-1 encoded with a single NULL terminator
			//a 'LINK' url would also come here and be seperately enumerated (because it has a terminating NULL); but in 3gp assets, external references aren't allowed
			//an 'OWNE'/'COMR' price field would also be here because of single byte NULL termination
			case ID3_OWNER_FIELD :
			case ID3_MIME_TYPE_FIELD : {
				if (this_field->field_string == NULL) {
					*frame_header_len = 0;
					*frame_length = 0;
					return;
				} else {
					APar_LimitBufferRange(max_alloc, *frame_header_len + *frame_length +1); //+2 for a possible extra NULLs

					APar_ValidateNULLTermination8bit(this_field);
					memcpy(dest_buffer + *frame_length, this_field->field_string, this_field->field_length);
					*frame_length += this_field->field_length;

				}
				//fprintf(stdout, "Field idx %u(%d) is now %" PRIu32 " bytes long\n", fld_idx, this_field->ID3v2_Field_Type, *frame_length);
				break;
			}
			default : {
				//fprintf(stdout, "I was unable to determine the field class. I was provided with %u (i.e. text field: %u, text encoding: %u\n", this_field->ID3v2_Field_Type, ID3_TEXT_FIELD, ID3_TEXT_ENCODING_FIELD);
				break;
			}
		
		} //end switch
	}
	if (id3v2_frame->ID3v2_FrameType == ID3_TEXT_FRAME && id3v2_frame->textfield_tally > 1 && id3_tag->ID3v2Tag_MajorVersion == 4) {
		ID3v2Fields* extra_textfield = (id3v2_frame->ID3v2_Frame_Fields+1)->next_field;
		while (true) {
			if (extra_textfield == NULL) break;
			
			if (encoding_val == TE_UTF8 || encoding_val == TE_LATIN1 ) {
				*frame_length+=1;
			} else if (encoding_val == TE_UTF16LE_WITH_BOM || encoding_val == TE_UTF16BE_NO_BOM) {
				*frame_length+=2;
			}
			
			memcpy(dest_buffer + *frame_length, extra_textfield->field_string, extra_textfield->field_length);
			*frame_length += extra_textfield->field_length;
			
			extra_textfield = extra_textfield->next_field;
		}
	}
	return;
}

uint32_t APar_Render_ID32_Tag(AtomicInfo* id32_atom, uint32_t max_alloc) {
	bool contains_rendered_frames = false;
	APar_FrameFilter(id32_atom);
	
	UInt16_TO_String2(id32_atom->AtomicLanguage, id32_atom->AtomicData); //parsedAtoms[atom_idx].AtomicLanguage
	uint64_t tag_offset = 2; //those first 2 bytes will hold the language
	uint32_t frame_length, frame_header_len; //the length in bytes this frame consumes in AtomicData as rendered
	uint64_t frame_length_pos, frame_compressed_length_pos;
	
	memcpy(id32_atom->AtomicData + tag_offset, "ID3", 3);
	tag_offset+=3;
	
	id32_atom->AtomicData[tag_offset] = id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion; //should be 4
	id32_atom->AtomicData[tag_offset+1] = id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion; //should be 0
	id32_atom->AtomicData[tag_offset+2] = id32_atom->ID32_TagInfo->ID3v2Tag_Flags;
	tag_offset+=3;
	
	//unknown full length; fill in later
	if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3 || id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
		tag_offset+= 4;
		if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_EXTENDEDHEADER)) { //currently unimplemented
			tag_offset+=10;
		}
	} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 2) {
		tag_offset+= 3;
	}
	
	id32_atom->ID32_TagInfo->ID3v2Tag_Length = tag_offset-2;
	
	ID3v2Frame* thisframe = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
	while (thisframe != NULL) {
		frame_header_len = 0;
		frame_length_pos = 0;
		frame_compressed_length_pos = 0;
		
		if (thisframe->eliminate_frame == true) {
			thisframe = thisframe->ID3v2_NextFrame;
			continue;
		}
		
		contains_rendered_frames = true;
		//this won't be able to convert from 1 tag version to another because it doesn't look up the frame id strings for the change
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3 || id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
			memcpy(id32_atom->AtomicData + tag_offset, thisframe->ID3v2_Frame_Namestr, 4);
			frame_header_len += 4;
			
			//the frame length won't be determined until the end of rendering this frame fully; for now just remember where its supposed to be:
			frame_length_pos = tag_offset + frame_header_len;
			frame_header_len+=4;
			
		} else if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 2) {
			memcpy(id32_atom->AtomicData + tag_offset, thisframe->ID3v2_Frame_Namestr, 3);
			frame_header_len += 3;
			
			//the frame length won't be determined until the end of rendering this frame fully; for now just remember where its supposed to be:
			frame_length_pos = tag_offset + frame_header_len;
			frame_header_len+=3;
		}
		
		//render frame flags //TODO: compression & group symbol are the only ones that can possibly be set here
		if (id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 3 || id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion == 4) {
			UInt16_TO_String2(thisframe->ID3v2_Frame_Flags, id32_atom->AtomicData + tag_offset + frame_header_len);
			frame_header_len+=2;
		}
		
		//grouping flag? 1 byte; technically, its outside the header and before the fields begin
		if (ID3v2_TestFrameFlag(thisframe->ID3v2_Frame_Flags, ID32_FRAMEFLAG_GROUPING)) {
			id32_atom->AtomicData[tag_offset + frame_header_len] = thisframe->ID3v2_Frame_GroupingSymbol;
			frame_header_len++;
		}
		
		//compression flag? 4bytes; technically, its outside the header and before the fields begin
		if (ID3v2_TestFrameFlag(thisframe->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED)) {
			frame_compressed_length_pos = tag_offset + frame_header_len; //fill in later; remember where it is supposed to go
			frame_header_len+=4;
		}
		
		frame_length = 0;
		APar_RenderFields(id32_atom->AtomicData + tag_offset+frame_header_len,
			max_alloc-tag_offset, id32_atom->ID32_TagInfo, thisframe,
			&frame_header_len, &frame_length);
		
#if defined HAVE_ZLIB_H
		//and now that we have rendered the frame, its time to turn to compression, if set
		if (ID3v2_TestFrameFlag(thisframe->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED) ) {
			uint32_t compressed_len = 0;
			char* compress_buffer = (char*)calloc(1, sizeof(char)*frame_length + 20);
			
			compressed_len = APar_zlib_deflate(id32_atom->AtomicData + tag_offset+frame_header_len, frame_length, compress_buffer, frame_length + 20);
			
			if (compressed_len > 0) {
				memcpy(id32_atom->AtomicData + tag_offset+frame_header_len, compress_buffer, compressed_len + 1);
				convert_to_syncsafe32(frame_length, id32_atom->AtomicData + frame_compressed_length_pos);
				frame_length = compressed_len;

				//WriteZlibData(id32_atom->AtomicData + tag_offset+frame_header_len, compressed_len);
			}
		}
#endif
		
		convert_to_syncsafe32(frame_length, id32_atom->AtomicData + frame_length_pos);
		tag_offset += frame_header_len + frame_length; //advance
		id32_atom->ID32_TagInfo->ID3v2Tag_Length += frame_header_len + frame_length;
		thisframe = thisframe->ID3v2_NextFrame;
		
	}
	convert_to_syncsafe32(id32_atom->ID32_TagInfo->ID3v2Tag_Length - 10, id32_atom->AtomicData + 8); //-10 for a v2.4 tag with no extended header
	
	if (!contains_rendered_frames) id32_atom->ID32_TagInfo->ID3v2Tag_Length = 0;

	return id32_atom->ID32_TagInfo->ID3v2Tag_Length;
}

///////////////////////////////////////////////////////////////////////////////////////
//                       id3 initializing functions                                  //
///////////////////////////////////////////////////////////////////////////////////////

void APar_FieldInit(ID3v2Frame* aFrame, uint8_t a_field, uint8_t frame_comp_list, const char* frame_payload) {
	uint32_t byte_allocation = 0;
	ID3v2Fields* this_field = NULL;
	int field_type = FrameTypeConstructionList[frame_comp_list].ID3_FieldComponents[a_field];
	
	switch(field_type) {
		//case ID3_UNKNOWN_FIELD will not be handled
		
		//these are all 1 to less than 16 bytes.
		case ID3_GROUPSYMBOL_FIELD :
		case ID3_COUNTER_FIELD :
		case ID3_PIC_TYPE_FIELD :
		case ID3_LANGUAGE_FIELD :
		case ID3_IMAGEFORMAT_FIELD : //PIC in v2.2
		case ID3_TEXT_ENCODING_FIELD : {
			byte_allocation = 16;
			break;
		}
		
		//between 16 & 100 bytes.
		case ID3_MIME_TYPE_FIELD : {
			byte_allocation = 100;
			break;
		}
		
		//these are allocated with 2000 bytes
		case ID3_FILENAME_FIELD :
		case ID3_OWNER_FIELD :
		case ID3_DESCRIPTION_FIELD :
		case ID3_URL_FIELD :
		case ID3_TEXT_FIELD : {
			uint32_t string_len = strlen(frame_payload) + 1;
			if (string_len * 2 > 2000) {
				byte_allocation = string_len * 2;
			} else {
				byte_allocation = 2000;
			}
			break;
		}
		
		case ID3_BINARY_DATA_FIELD : {
			if (aFrame->ID3v2_Frame_ID == ID3v2_EMBEDDED_PICTURE || aFrame->ID3v2_Frame_ID == ID3v2_EMBEDDED_OBJECT ) {
				//this will be left NULL because it would would probably have to be realloced, so just do it later to the right size //byte_allocation = findFileSize(frame_payload) + 1; //this should be limited to max_sync_safe_uint28_t
			} else {
				byte_allocation = 2000;
			}
			break;
		}
	
		//default : {
		//	fprintf(stdout, "I am %d\n", FrameTypeConstructionList[frame_comp_list].ID3_FieldComponents[a_field]);
		//	break;
		//}
	}
	this_field = aFrame->ID3v2_Frame_Fields + a_field;
	this_field->ID3v2_Field_Type = field_type;
	if (byte_allocation > 0) {
		this_field->field_string = (char*)calloc(1, sizeof(char*)*byte_allocation);
		if (!APar_assert((this_field->field_string != NULL), 11, aFrame->ID3v2_Frame_Namestr) ) exit(11);
	} else {
		this_field->field_string = NULL;
	}
	this_field->field_length = 0;
	this_field->alloc_length = byte_allocation;
	this_field->next_field = NULL;
	//fprintf(stdout, "For %u field, %" PRIu32 " bytes were allocated.\n", this_field->ID3v2_Field_Type, byte_allocation);
	return;
}

void APar_FrameInit(ID3v2Frame* aFrame, const char* frame_str, int frameID, uint8_t frame_comp_list, const char* frame_payload) {
	aFrame->ID3v2_FieldCount = FrameTypeConstructionList[frame_comp_list].ID3_FieldCount;
	if (aFrame->ID3v2_FieldCount > 0) {
		aFrame->ID3v2_Frame_Fields = (ID3v2Fields*)calloc(1, sizeof(ID3v2Fields)*aFrame->ID3v2_FieldCount);
		aFrame->ID3v2_Frame_ID = frameID;
		aFrame->ID3v2_FrameType = FrameTypeConstructionList[frame_comp_list].ID3_FrameType;
		aFrame->ID3v2_Frame_ExpandedLength = 0;
		aFrame->ID3v2_Frame_GroupingSymbol = 0;
		aFrame->ID3v2_Frame_Flags = 0;
		aFrame->ID3v2_Frame_Length = 0;
		aFrame->textfield_tally = 0;
		aFrame->eliminate_frame = false;
		memcpy(aFrame->ID3v2_Frame_Namestr, frame_str, 5);
		
		for (uint8_t fld = 0; fld < aFrame->ID3v2_FieldCount; fld++) {
			APar_FieldInit(aFrame, fld, frame_comp_list, frame_payload);
		}
		
		//fprintf(stdout, "(%u = %d) Type %d\n", frameID, KnownFrames[frameID+1].ID3v2_InternalFrameID, aFrame->ID3v2_FrameType);
	}
	//fprintf(stdout, "Retrieved frame for '%s': %s (%u fields)\n", frame_str, KnownFrames[frameID].ID3V2p4_FrameID, aFrame->ID3v2_FieldCount);
	return;
}

void APar_ID3Tag_Init(AtomicInfo* id32_atom) {
	id32_atom->ID32_TagInfo = (ID3v2Tag*)calloc(1, sizeof(ID3v2Tag));
	id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion = AtomicParsley_ID3v2Tag_MajorVersion;
	id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion = AtomicParsley_ID3v2Tag_RevisionVersion;
	id32_atom->ID32_TagInfo->ID3v2Tag_Flags = AtomicParsley_ID3v2Tag_Flags;
	id32_atom->ID32_TagInfo->ID3v2Tag_Length = 10; //this would be 9 for v2.2
	id32_atom->ID32_TagInfo->ID3v2_Tag_ExtendedHeader_Length = 0;
	id32_atom->ID32_TagInfo->ID3v2_FrameCount = 0;
	id32_atom->ID32_TagInfo->modified_tag = false; //this will have to change when a frame is added/modified/removed because this id3 header won't be written with 0 frames
	
	id32_atom->ID32_TagInfo->ID3v2_FirstFrame = NULL;
	id32_atom->ID32_TagInfo->ID3v2_FrameList = NULL;
	return;
}

void APar_realloc_memcpy(ID3v2Fields* thisField, uint32_t new_size) {
	if (new_size > thisField->alloc_length) {
		char* new_alloc = (char*)calloc(1, sizeof(char*)*new_size + 1);
		//memcpy(new_alloc, thisField->field_string, thisField->field_length);
		thisField->field_length = 0;
		free(thisField->field_string);
		thisField->field_string = new_alloc;
		thisField->alloc_length = new_size;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                    id3 frame setting/finding functions                            //
///////////////////////////////////////////////////////////////////////////////////////

uint32_t APar_TextFieldDataPut(ID3v2Fields* thisField, const char* this_payload, uint8_t str_encoding, bool multistringtext = false) {
	uint32_t bytes_used = 0;

	if (multistringtext == false) {
		thisField->field_length = 0;
	}
	
	if (str_encoding == TE_UTF8) {
		bytes_used = strlen(this_payload); //no NULL termination is provided until render time
		if (bytes_used + thisField->field_length > thisField->alloc_length) {
			APar_realloc_memcpy(thisField, (bytes_used > 2000 ? bytes_used : 2000) );
		}
		memcpy(thisField->field_string + thisField->field_length, this_payload, bytes_used);
		thisField->field_length += bytes_used;
		
	} else if (str_encoding == TE_LATIN1) {
		int string_length = strlen(this_payload);
		if (string_length + thisField->field_length > thisField->alloc_length) {
			APar_realloc_memcpy(thisField, (string_length > 2000 ? string_length : 2000) );
		}
		int converted_bytes = UTF8Toisolat1((unsigned char*)thisField->field_string + thisField->field_length, (int)thisField->alloc_length,
		                                   (unsigned char*)this_payload, string_length);
		if (converted_bytes > 0) {
			thisField->field_length += converted_bytes;
			bytes_used = converted_bytes;
			//fprintf(stdout, "string %s, %" PRIu32 "=%" PRIu32 "\n", thisField->field_string, thisField->field_length, bytes_used);
		}
		
	} else if (str_encoding == TE_UTF16BE_NO_BOM) {
		int string_length = (int)utf8_length(this_payload, strlen(this_payload)) + 1;
		if (2 * string_length + thisField->field_length > thisField->alloc_length) {
			APar_realloc_memcpy(thisField, (2 * string_length + thisField->field_length > 2000 ? 2 * string_length + thisField->field_length : 2000) );
		}
		int converted_bytes = UTF8ToUTF16BE((unsigned char*)thisField->field_string + thisField->field_length, (int)thisField->alloc_length,
		                                   (unsigned char*)this_payload, string_length);
		if (converted_bytes > 0) {
			thisField->field_length += converted_bytes;
			bytes_used = converted_bytes;
		}
	
	} else if (str_encoding == TE_UTF16LE_WITH_BOM) {
		int string_length = (int)utf8_length(this_payload, strlen(this_payload)) + 1;
		uint64_t bom_offset = 0;
		
		if (2 * string_length + thisField->field_length > thisField->alloc_length) { //important: realloc before BOM testing!!!
			APar_realloc_memcpy(thisField, (2 * string_length + thisField->field_length > 2000 ? 2 * string_length + thisField->field_length : 2000) );
		}
		if (thisField->field_length == 0 && multistringtext == false) {
			memcpy(thisField->field_string, "\xFF\xFE", 2);
		}
		
		uint8_t field_encoding = TextField_TestBOM(thisField->field_string);
		if (field_encoding > 0) {
			bom_offset = 2;
		}
		int converted_bytes = UTF8ToUTF16LE((unsigned char*)thisField->field_string + thisField->field_length + bom_offset, (int)thisField->alloc_length,
		                                   (unsigned char*)this_payload, string_length);
		if (converted_bytes > 0) {
			thisField->field_length += converted_bytes + bom_offset;
			bytes_used = converted_bytes;
		}
	}
	
	if (multistringtext != false) {
		if (str_encoding == TE_UTF16LE_WITH_BOM || str_encoding == TE_UTF16BE_NO_BOM) {
			bytes_used += 2;
		} else {
			bytes_used += 1;
		}
	}
	return bytes_used;
}

uint32_t APar_BinaryFieldPut(ID3v2Fields* thisField, uint32_t a_number, const char* this_payload, uint32_t payload_len) {
	if (thisField->ID3v2_Field_Type == ID3_TEXT_ENCODING_FIELD || thisField->ID3v2_Field_Type == ID3_PIC_TYPE_FIELD || thisField->ID3v2_Field_Type == ID3_GROUPSYMBOL_FIELD) {
		thisField->field_string[0] = (unsigned char)a_number;
		thisField->field_length = 1;
		//fprintf(stdout, "My (TE/PT) content is 0x%02X\n", thisField->field_string[0]);
		return 1;
		
	} else if (thisField->ID3v2_Field_Type == ID3_BINARY_DATA_FIELD && payload_len == 0) { //contents of a file
		uint64_t file_length = findFileSize(this_payload);
		thisField->field_string = (char*)calloc(1, sizeof(char*)*file_length+16);
		
		FILE* binfile = APar_OpenFile(this_payload, "rb");
		APar_ReadFile(thisField->field_string, binfile, file_length);
		fclose(binfile);
		
		thisField->field_length = file_length;
		thisField->alloc_length = file_length+16;
		thisField->ID3v2_Field_Type = ID3_BINARY_DATA_FIELD;
		return file_length;
	
	} else if (thisField->ID3v2_Field_Type == ID3_BINARY_DATA_FIELD || thisField->ID3v2_Field_Type == ID3_COUNTER_FIELD) {
		thisField->field_string = (char*)calloc(1, sizeof(char*)*payload_len+16);
		memcpy(thisField->field_string, this_payload, payload_len);
		
		thisField->field_length = payload_len;
		thisField->alloc_length = payload_len+16;
		thisField->ID3v2_Field_Type = ID3_BINARY_DATA_FIELD;
		return payload_len;

	}
	return 0;
}

void APar_FrameDataPut(ID3v2Frame* thisFrame, const char* frame_payload, AdjunctArgs* adjunct_payload, uint8_t str_encoding) {
	if (adjunct_payload->multistringtext == false && !APar_EvalFrame_for_Field(thisFrame->ID3v2_FrameType, ID3_COUNTER_FIELD) ) thisFrame->ID3v2_Frame_Length = 0;
	switch(thisFrame->ID3v2_FrameType) {
		case ID3_TEXT_FRAME : {
			if (adjunct_payload->multistringtext && thisFrame->textfield_tally >= 1) {
				ID3v2Fields* last_textfield = APar_FindLastTextField (thisFrame);
				if (APar_ExtraTextFieldInit(last_textfield, strlen(frame_payload), (uint8_t)thisFrame->ID3v2_Frame_Fields->field_string[0])) {
					thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(last_textfield->next_field, frame_payload, (uint8_t)thisFrame->ID3v2_Frame_Fields->field_string[0], true);
				}
			} else {
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
				thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, frame_payload, str_encoding, false); //text field
				GlobalID3Tag->ID3v2_FrameCount++;
			}
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			//GlobalID3Tag->ID3v2_FrameCount++; //don't do this for all text frames because the multiple text field support of id3v2.4; only when the frame is initially set
			thisFrame->textfield_tally++;
			break;
		}
		case ID3_TEXT_FRAME_USERDEF : {
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->descripArg, str_encoding); //language
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+2, frame_payload, str_encoding); //text field
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_DESCRIBED_TEXT_FRAME : {
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->targetLang, 0); //language
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+2, adjunct_payload->descripArg, str_encoding); //description
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+3, frame_payload, str_encoding, adjunct_payload->multistringtext); //text field
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_URL_FRAME : {
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields, frame_payload, TE_LATIN1); //url field
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_URL_FRAME_USERDEF : {
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->descripArg, str_encoding); //language
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+2, frame_payload, TE_LATIN1); //url field
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_UNIQUE_FILE_ID_FRAME : {
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields, frame_payload, TE_LATIN1); //owner field
			
			if (memcmp(adjunct_payload->dataArg, "randomUUIDstamp", 16) == 0) {
				char uuid_binary_str[25]; memset(uuid_binary_str, 0, 25);
				APar_generate_random_uuid(uuid_binary_str);
				(thisFrame->ID3v2_Frame_Fields+1)->field_string = (char*)calloc(1, sizeof(char*)*40);
				APar_sprintf_uuid((ap_uuid_t*)uuid_binary_str, (thisFrame->ID3v2_Frame_Fields+1)->field_string);
		
				(thisFrame->ID3v2_Frame_Fields+1)->field_length = 36;
				(thisFrame->ID3v2_Frame_Fields+1)->alloc_length = 40;
				(thisFrame->ID3v2_Frame_Fields+1)->ID3v2_Field_Type = ID3_BINARY_DATA_FIELD;
				thisFrame->ID3v2_Frame_Length += 36;
			} else {
				uint8_t uniqueIDlen = strlen(adjunct_payload->dataArg);
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, 0, adjunct_payload->dataArg, (uniqueIDlen > 64 ? 64 : uniqueIDlen)); //unique file ID
			}

			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_CD_ID_FRAME : {
			thisFrame->ID3v2_Frame_Fields->field_length = GenerateMCDIfromCD(frame_payload, thisFrame->ID3v2_Frame_Fields->field_string);
			thisFrame->ID3v2_Frame_Length = thisFrame->ID3v2_Frame_Fields->field_length;
			
			if (thisFrame->ID3v2_Frame_Length < 12) {
				free(thisFrame->ID3v2_Frame_Fields->field_string);
				thisFrame->ID3v2_Frame_Fields->field_string = NULL;
				thisFrame->ID3v2_Frame_Fields->alloc_length = 0;
				thisFrame->ID3v2_Frame_Length = 0;
			} else {
				modified_atoms = true;
				GlobalID3Tag->modified_tag = true;
				GlobalID3Tag->ID3v2_FrameCount++;
			}
			break;
		}
		case ID3_ATTACHED_PICTURE_FRAME : {
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->mimeArg, TE_LATIN1); //mimetype
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+2, adjunct_payload->pictype_uint8, NULL, 1); //picturetype
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+3, adjunct_payload->descripArg, str_encoding); //description
			//(thisFrame->ID3v2_Frame_Fields+4)->ID3v2_Field_Type = ID3_BINARY_DATA_FIELD; //because it wasn't malloced, this needs to be set now
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+4, 0, frame_payload, 0); //binary file (path)
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_ATTACHED_OBJECT_FRAME : {
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, str_encoding, NULL, 1); //encoding
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->mimeArg, TE_LATIN1); //mimetype
			if (memcmp(adjunct_payload->filenameArg, "FILENAMESTAMP", 13) == 0) {
				const char* derived_filename = NULL;
#if defined (_WIN32)
				derived_filename = strrchr(frame_payload, '\\');
#if defined (__CYGWIN__)
				const char* derived_filename2 = strrchr(frame_payload, '/');
				if (derived_filename2 > derived_filename) {
					derived_filename = derived_filename2;
				}
#endif
#else
				derived_filename = strrchr(frame_payload, '/');
#endif
				if (derived_filename == NULL) {
					derived_filename = frame_payload;
				} else {
					derived_filename++; //get rid of the preceding slash
				}
				thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+2, derived_filename, str_encoding); //filename
			} else {
				thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+2, adjunct_payload->filenameArg, str_encoding); //filename
			}
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields+3, adjunct_payload->descripArg, str_encoding); //description
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+4, 0, frame_payload, 0); //binary file (path)
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_GROUP_ID_FRAME : {
			uint32_t groupdatalen = strlen(adjunct_payload->dataArg);
			if (adjunct_payload->groupSymbol > 0) {
				thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields, frame_payload, TE_LATIN1); //owner field
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+1, adjunct_payload->groupSymbol, NULL, 1); //group symbol
				if (groupdatalen > 0) { //not quite binary (unless it were entered as hex & converted), but it will do
					thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+2, 0, adjunct_payload->dataArg, groupdatalen); //group symbol
				}
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			}
			break;
		}
		case ID3_SIGNATURE_FRAME : {
			if (adjunct_payload->groupSymbol > 0) {
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, adjunct_payload->groupSymbol, NULL, 1); //group symbol
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+1, 0, frame_payload, strlen(frame_payload)); //signature
				modified_atoms = true;
				GlobalID3Tag->modified_tag = true;
				GlobalID3Tag->ID3v2_FrameCount++;
			}
			break;
		}
		case ID3_PRIVATE_FRAME : {
			uint32_t datalen = strlen(adjunct_payload->dataArg); //kinda precludes a true "binary" sense, but whatever...
			if (datalen > 0) {
				thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields, frame_payload, TE_LATIN1); //owner field
				thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+1, 0, adjunct_payload->dataArg, datalen); //data
				modified_atoms = true;
				GlobalID3Tag->modified_tag = true;
				GlobalID3Tag->ID3v2_FrameCount++;
			}
			break;
		}
		case ID3_PLAYCOUNTER_FRAME : {
			uint64_t playcount = 0;
			char play_count_syncsafe[16];

			memset(play_count_syncsafe, 0, sizeof(play_count_syncsafe));

			if (strcmp(frame_payload, "+1") == 0) {
				if (thisFrame->ID3v2_Frame_Length == 4) {
					playcount = (uint64_t)syncsafe32_to_UInt32(thisFrame->ID3v2_Frame_Fields->field_string) + 1;
				} else if (thisFrame->ID3v2_Frame_Length > 4) {
					playcount = syncsafeXX_to_UInt64(thisFrame->ID3v2_Frame_Fields->field_string, thisFrame->ID3v2_Frame_Fields->field_length) +1;
				} else {
					playcount = 1;
				}
			} else {
				sscanf(frame_payload, "%" SCNu64, &playcount);
			}
			
			if (playcount < 268435455) {
				convert_to_syncsafe32(playcount, play_count_syncsafe);
				thisFrame->ID3v2_Frame_Length = APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, 0, play_count_syncsafe, 4);
			} else {
				uint8_t conversion_len = convert_to_syncsafeXX(playcount, play_count_syncsafe);
				thisFrame->ID3v2_Frame_Length = APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, 0, play_count_syncsafe, conversion_len);
			}
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
		case ID3_POPULAR_FRAME : {
			unsigned char popm_rating = 0;
			uint64_t popm_playcount = 0;
			char popm_play_count_syncsafe[16];

			memset(popm_play_count_syncsafe, 0,
				sizeof(popm_play_count_syncsafe));
			
			if (adjunct_payload->ratingArg != NULL) {
				popm_rating = strtoul(adjunct_payload->ratingArg, NULL, 10);
			}
			thisFrame->ID3v2_Frame_Length += APar_TextFieldDataPut(thisFrame->ID3v2_Frame_Fields, frame_payload, TE_LATIN1); //owner field
			thisFrame->ID3v2_Frame_Length += APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields+1, 0, (char*)&popm_rating, 1); //rating
			
			if (adjunct_payload->dataArg != NULL) {
				if (strlen(adjunct_payload->dataArg) > 0) {
					if (memcmp(adjunct_payload->dataArg, "+1", 3) == 0) {
						if ((thisFrame->ID3v2_Frame_Fields+2)->field_length == 4) {
							popm_playcount = (uint64_t)syncsafe32_to_UInt32((thisFrame->ID3v2_Frame_Fields+2)->field_string) + 1;
						} else if ((thisFrame->ID3v2_Frame_Fields+2)->field_length > 4) {
							popm_playcount = syncsafeXX_to_UInt64((thisFrame->ID3v2_Frame_Fields+2)->field_string, (thisFrame->ID3v2_Frame_Fields+2)->field_length) +1;
						} else {
							popm_playcount = 1;
						}
					} else {
						sscanf(adjunct_payload->dataArg, "%" SCNu64, &popm_playcount);
					}
				}
			}
			if (popm_playcount > 0) {
				if (popm_playcount < 268435455) {
					convert_to_syncsafe32(popm_playcount, popm_play_count_syncsafe);
					thisFrame->ID3v2_Frame_Length = APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, 0, popm_play_count_syncsafe, 4); //syncsafe32 counter
				} else {
					uint8_t conversion_len = convert_to_syncsafeXX(popm_playcount, popm_play_count_syncsafe);
					thisFrame->ID3v2_Frame_Length = APar_BinaryFieldPut(thisFrame->ID3v2_Frame_Fields, 0, popm_play_count_syncsafe, conversion_len); //BIGsyncsafe counter
				}
			}
			modified_atoms = true;
			GlobalID3Tag->modified_tag = true;
			GlobalID3Tag->ID3v2_FrameCount++;
			break;
		}
	} //end switch
	return;
}

void APar_EmbeddedFileTests(const char* filepath, int frameType, AdjunctArgs* adjunct_payloads) {
	if (frameType == ID3_ATTACHED_PICTURE_FRAME) {
		
		//get cli imagetype
		uint8_t total_image_types = (uint8_t)(sizeof(ImageTypeList)/sizeof(*ImageTypeList));
		uint8_t img_typlen = strlen(adjunct_payloads->pictypeArg) + 1;
		const char* img_comparison_str = NULL;
		
		for (uint8_t itest = 0; itest < total_image_types; itest++) {
			if (img_typlen == 5) {
				img_comparison_str = ImageTypeList[itest].hexstring;
			} else {
				img_comparison_str = ImageTypeList[itest].imagetype_str;
			}
			if (strcmp(adjunct_payloads->pictypeArg, img_comparison_str) == 0) {
				adjunct_payloads->pictype_uint8 = ImageTypeList[itest].hexcode;
			}
		}

		if (strlen(filepath) > 0) {
			//see if file even exists
			TestFileExistence(filepath, true);
			
			char* image_headerbytes = (char*)calloc(1, (sizeof(char)*25));
			FILE* imagefile =  APar_OpenFile(filepath, "rb");
			APar_ReadFile(image_headerbytes, imagefile, 24);
			fclose(imagefile);
			//test mimetype
			if (strlen(adjunct_payloads->mimeArg) == 0 || memcmp(adjunct_payloads->mimeArg, "-->", 3) == 0) {
				uint8_t total_image_tests = (uint8_t)(sizeof(ImageList)/sizeof(*ImageList));
				for (uint8_t itest = 0; itest < total_image_tests; itest++) {
					if (ImageList[itest].image_testbytes == 0) {
						adjunct_payloads->mimeArg = ImageList[itest].image_mimetype;
						break;
					} else if (memcmp(image_headerbytes, ImageList[itest].image_binaryheader, ImageList[itest].image_testbytes) == 0) {
						adjunct_payloads->mimeArg = ImageList[itest].image_mimetype;
						if (adjunct_payloads->pictype_uint8 == 0x01) {
							if (memcmp(image_headerbytes+16, "\x00\x00\x00\x20\x00\x00\x00\x20", 8) != 0 && itest != 2) {
								adjunct_payloads->pictype_uint8 = 0x02;
							}
						}
						break;
					}
				}
			}
			free(image_headerbytes);
			image_headerbytes = NULL;
		}
		
	} else if (frameType == ID3_ATTACHED_OBJECT_FRAME) {
		if (strlen(filepath) > 0) {
			TestFileExistence(filepath, true);			
			FILE* embedfile =  APar_OpenFile(filepath, "rb");
			fclose(embedfile);
		}
	}
	return;
}

char* APar_ConvertField_to_UTF8(ID3v2Frame* targetframe, int fieldtype) {
	char* utf8str = NULL;
	uint8_t targetfield = 0xFF;
	uint8_t textencoding = 0;
	
	for (uint8_t frm_field = 0; frm_field < targetframe->ID3v2_FieldCount; frm_field++) {
		if ( (targetframe->ID3v2_Frame_Fields+frm_field)->ID3v2_Field_Type == fieldtype) {
			targetfield = frm_field;
			break;
		}
	}
	
	if (targetfield != 0xFF) {
		if (targetframe->ID3v2_Frame_Fields->ID3v2_Field_Type == ID3_TEXT_ENCODING_FIELD) {
			textencoding = targetframe->ID3v2_Frame_Fields->field_string[0];
		}
		
		if (textencoding == TE_LATIN1) {
			utf8str = (char*)calloc(1, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *2) +16);
			isolat1ToUTF8((unsigned char*)utf8str, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *2) +16,
			              (unsigned char*)((targetframe->ID3v2_Frame_Fields+targetfield)->field_string), (targetframe->ID3v2_Frame_Fields+targetfield)->field_length);

		} else if (textencoding == TE_UTF8) { //just so things can be free()'d with testing; a small price to pay
			utf8str = (char*)calloc(1, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length) +16);
			memcpy(utf8str, (targetframe->ID3v2_Frame_Fields+targetfield)->field_string, (targetframe->ID3v2_Frame_Fields+targetfield)->field_length);
			
		} else if (textencoding == TE_UTF16BE_NO_BOM) {
			utf8str = (char*)calloc(1, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *4) +16);
			UTF16BEToUTF8((unsigned char*)utf8str, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *4) +16,
			              (unsigned char*)((targetframe->ID3v2_Frame_Fields+targetfield)->field_string), (targetframe->ID3v2_Frame_Fields+targetfield)->field_length);
			
		} else if (textencoding == TE_UTF16LE_WITH_BOM) {
			utf8str = (char*)calloc(1, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *4) +16);
			if (memcmp( (targetframe->ID3v2_Frame_Fields+targetfield)->field_string, "\xFF\xFE", 2) == 0) {
				UTF16LEToUTF8((unsigned char*)utf8str, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *4) +16,
			                (unsigned char*)((targetframe->ID3v2_Frame_Fields+targetfield)->field_string+2), (targetframe->ID3v2_Frame_Fields+targetfield)->field_length-2);
			} else {
				UTF16BEToUTF8((unsigned char*)utf8str, sizeof(char*)*( (targetframe->ID3v2_Frame_Fields+targetfield)->field_length *4) +16,
			                (unsigned char*)((targetframe->ID3v2_Frame_Fields+targetfield)->field_string+2), (targetframe->ID3v2_Frame_Fields+targetfield)->field_length-2);
			}
		}
	}
	
	return utf8str;
}

/*----------------------
APar_FindFrame
	id3v2tag - an already initialized ID3 tag (contained by an ID32 atom) with 0 or more frames as a linked list
	frame_str - target frame string (like "TIT2")
	frameID - a known frame in listed in AP_ID3v2_FrameDefinitions & enumerated in AP_ID3v2_Definitions.h
	frametype - the type of frame (text, described text, picture, object...) to search for
	adjunct_payloads - holds optional/required args for supplementary matching; example: described text matching on frame name & description; TODO more criteria
	createframe - create the frame if not found to be existing; this initialzes the frame only - not its fields.

    this provides 2 functions: actually searching while looping through the frames & creation of a frame at the end of the frame list.
----------------------*/
ID3v2Frame* APar_FindFrame(ID3v2Tag* id3v2tag, const char* frame_str, int frameID, int frametype, AdjunctArgs* adjunct_payloads, bool createframe) {
	ID3v2Frame* returnframe = NULL;
	ID3v2Frame* evalframe = id3v2tag->ID3v2_FirstFrame;
	uint8_t supplemental_matching = 0;
	
	if (createframe) {
		ID3v2Frame* newframe = (ID3v2Frame*)calloc(1, sizeof(ID3v2Frame));
		newframe->ID3v2_NextFrame = NULL;
		if (id3v2tag->ID3v2_FirstFrame == NULL) id3v2tag->ID3v2_FirstFrame = newframe;
		if (id3v2tag->ID3v2_FrameList != NULL) id3v2tag->ID3v2_FrameList->ID3v2_NextFrame = newframe;
		id3v2tag->ID3v2_FrameList = newframe;
		return newframe;
	}
	
	if (APar_EvalFrame_for_Field(frametype, ID3_DESCRIPTION_FIELD)) {
		supplemental_matching = 0x01;
	}

	while (evalframe != NULL) {
		//if (trametype is a type containing a modifer like description or image type or symbol or such things
		if (supplemental_matching != 0) {
		
			//match on description + frame name
			if (supplemental_matching && 0x01 && evalframe->ID3v2_Frame_ID == frameID) {
				char* utf8_descrip = APar_ConvertField_to_UTF8(evalframe, ID3_DESCRIPTION_FIELD);
				if (utf8_descrip != NULL) {
					if (strcmp(adjunct_payloads->descripArg, utf8_descrip) == 0) {
						returnframe = evalframe;
						free(utf8_descrip);
						break;
					}
					free(utf8_descrip);
				}
			}
		
		} else if (evalframe->ID3v2_Frame_ID == ID3_UNKNOWN_FRAME) {
			if (memcmp(frame_str, evalframe->ID3v2_Frame_Namestr, 4) == 0) {
				returnframe = evalframe;
				break;
			}
		
		} else {
			//fprintf(stdout, "frame is %s; eval frameID is %d ?= %d\n", frame_str, evalframe->ID3v2_Frame_ID, frameID);
			if (evalframe->ID3v2_Frame_ID == frameID) {
				returnframe = evalframe;
				break;
			}
		}
		evalframe = evalframe->ID3v2_NextFrame;
	}
	return returnframe;
}

/*----------------------
APar_ID3FrameAmmend
	id32_atom - the ID32 atom targeted to this language; the ID32 atom is already created, the ID3 tag is either created or containing already parsed ID3 frames
	frame_str - the string for the frame (like TCON) that is desired. This string must be a known frame string in AP_ID3v2_FrameDefinitions.h
	frame_payload - the major piece of metadata to be set (for APIC its the path, for MCDI its a device...), that can optionally be NULL (for removal of the frame)
	adjunct_payloads - a structure holding a number of optional/required parameters for the frame (compression...)
	str_encoding - the encoding to be used in the fields of the target frame when different encodings are allowed	

    lookup what frame_str is supposed to look like in the KnownFrames[] array in AP_ID3v2_FrameDefinitions.h. First see if this frame exists at all - if it does &
		the frame_str is NULL or blank (""), then mark this frame for elimination. if the frame is of a particular type (like TCON), run some tests on the frame_payload.
		If all is well after the tests, and the frame does not exists, create it via APar_FindFrame(... true) & initialize the frame to hold data. Send the frame, payload &
		adjunct payloads onto APar_FrameDataPut to actually place the data onto the frame
----------------------*/
void APar_ID3FrameAmmend(AtomicInfo* id32_atom, const char* frame_str, const char* frame_payload, AdjunctArgs* adjunct_payloads, uint8_t str_encoding) {
	ID3v2Frame* targetFrame = NULL;
	
	if (id32_atom == NULL) return;
	GlobalID3Tag = id32_atom->ID32_TagInfo;
	//fprintf(stdout, "frame is %s; payload is %s; %s %s\n", frame_str, frame_payload, adjunct_payloads->descripArg, adjunct_payloads->targetLang);
	
	int frameID = MatchID3FrameIDstr(frame_str, GlobalID3Tag->ID3v2Tag_MajorVersion);
	int frameType = KnownFrames[frameID+1].ID3v2_FrameType;
	uint8_t frameCompositionList = GetFrameCompositionDescription(frameType);
	
	if (frameType == ID3_ATTACHED_PICTURE_FRAME || frameType == ID3_ATTACHED_OBJECT_FRAME) {
		APar_EmbeddedFileTests(frame_payload, frameType, adjunct_payloads);
	}
	
	targetFrame = APar_FindFrame(id32_atom->ID32_TagInfo, frame_str, frameID, frameType, adjunct_payloads, false);
	
	if (frame_payload == NULL) {
		if (targetFrame != NULL) {
			targetFrame->eliminate_frame = true;
			modified_atoms = true;
			id32_atom->ID32_TagInfo->modified_tag = true;
		}
		return;
		
	} else if (strlen(frame_payload) == 0) {
		if (targetFrame != NULL) {
			targetFrame->eliminate_frame = true; //thats right, frames of empty text are removed - so be a doll and try to convey some info, eh?
			modified_atoms = true;
			id32_atom->ID32_TagInfo->modified_tag = true;
		}
		return;
	
	} else {
		if (frameType == ID3_UNKNOWN_FRAME) {
			APar_assert(false, 10, frame_str);
			return;
		}
		
		//check tags to be set so they conform to the id3v2 informal specification
		if (frameType == ID3_TEXT_FRAME) {
			if (targetFrame != NULL) {
				if (!targetFrame->eliminate_frame) adjunct_payloads->multistringtext = true; //if a frame already exists and isn't marked for elimination, append a new string
			}
			
			if (frameID == ID3v2_FRAME_COPYRIGHT || frameID == ID3v2_FRAME_PRODNOTICE) {
				if ((TestCharInRange(frame_payload[0], '0', '9') + TestCharInRange(frame_payload[1], '0', '9') + TestCharInRange(frame_payload[2], '0', '9') +
						 TestCharInRange(frame_payload[3], '0', '9') != 4) || frame_payload[4] != ' ') {
					fprintf(stderr, "AtomicParsley warning: frame %s was skipped because it did not start with a year followed by a space\n", KnownFrames[frameID].ID3V2p4_FrameID);
					return;
				}
			
			} else if (frameID == ID3v2_FRAME_PART_O_SET || frameID == ID3v2_FRAME_TRACKNUM) {
				uint8_t pos_len = strlen(frame_payload);
				for (uint8_t letter_idx = 0; letter_idx < pos_len; letter_idx++) {
					if (frame_payload[letter_idx] == '/') continue;
					if (TestCharInRange(frame_payload[letter_idx], '0', '9') != 1) {
						if (frameID-1 == ID3v2_FRAME_PART_O_SET) {
							fprintf(stderr, "AtomicParsley warning: frame %s was skipped because it had an extraneous character: %c\n", KnownFrames[frameID].ID3V2p4_FrameID, frame_payload[letter_idx]);
							return;
						} else { //okay this is to support the beloved vinyl
							if (!(TestCharInRange(frame_payload[letter_idx], 'A', 'F') && TestCharInRange(frame_payload[letter_idx+1], '0', '9'))) {
								fprintf(stderr, "AtomicParsley warning: frame %s was skipped because it had an extraneous character: %c\n", KnownFrames[frameID].ID3V2p4_FrameID, frame_payload[letter_idx]);
								return;
							}
						}
					}
				}			
			} else if (frameID == ID3v2_FRAME_ISRC) {
				uint8_t isrc_len = strlen(frame_payload);
				if (isrc_len != 12) {
					fprintf(stderr, "AtomicParsley warning: setting ISRC frame was skipped because it was not 12 characters long\n");
					return;
				}
				for (uint8_t isrc_ltr_idx = 0; isrc_ltr_idx < isrc_len; isrc_ltr_idx++) {
					if (TestCharInRange(frame_payload[isrc_ltr_idx], '0', '9') + TestCharInRange(frame_payload[isrc_ltr_idx], 'A', 'Z') == 0) {
						fprintf(stderr, "AtomicParsley warning: ISRC can only consist of A-Z & 0-9; letter %u was %c; skipping\n", isrc_ltr_idx+1, frame_payload[isrc_ltr_idx]);
						return;
					}
				}
			}
		}

		
		if (targetFrame == NULL) {
			targetFrame = APar_FindFrame(id32_atom->ID32_TagInfo, frame_str, frameID, frameType, adjunct_payloads, true);
			if (targetFrame == NULL) {
				fprintf(stdout, "NULL frame\n");
				exit(0);
			} else {
				APar_FrameInit(targetFrame, frame_str, frameID, frameCompositionList, frame_payload);			
			}
		}
	}
	
	if (targetFrame != NULL) {
		if (adjunct_payloads->zlibCompressed) {
			targetFrame->ID3v2_Frame_Flags |= (ID32_FRAMEFLAG_COMPRESSED + ID32_FRAMEFLAG_LENINDICATED);
		}
		
		if (targetFrame->ID3v2_Frame_ID == ID3v2_FRAME_LANGUAGE) {
			APar_FrameDataPut(targetFrame, adjunct_payloads->targetLang, adjunct_payloads, str_encoding);
			
		} else if (targetFrame->ID3v2_Frame_ID == ID3v2_FRAME_CONTENTTYPE) {
			uint8_t genre_idx = ID3StringGenreToInt(frame_payload);
			if (genre_idx != 0xFF) {
				char genre_str_idx[2];
				genre_str_idx[0] = 0; genre_str_idx[1] = 0; genre_str_idx[1] = 0;
				sprintf(genre_str_idx, "%u", genre_idx);
				APar_FrameDataPut(targetFrame, genre_str_idx, adjunct_payloads, str_encoding);
			} else {
				APar_FrameDataPut(targetFrame, frame_payload, adjunct_payloads, str_encoding);
			}
		
		} else {
			APar_FrameDataPut(targetFrame, frame_payload, adjunct_payloads, str_encoding);
		}
		
		if (adjunct_payloads->zlibCompressed) {
			targetFrame->ID3v2_Frame_ExpandedLength = targetFrame->ID3v2_Frame_Length;
		}
		targetFrame->ID3v2_Frame_GroupingSymbol = adjunct_payloads->groupSymbol;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                           id3 cleanup function                                    //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_FreeID32Memory

    free all the little bits of allocated memory. Follow the ID3v2Frame pointers by each frame's ID3v2_NextFrame. Each frame has ID3v2_FieldCount number of field
		strings (char*) that were malloced.
----------------------*/
void APar_FreeID32Memory(ID3v2Tag* id32tag) {
	ID3v2Frame* aframe = id32tag->ID3v2_FirstFrame;
	while (aframe != NULL) {
		
#if defined(DEBUG_V)
		fprintf(stdout, "freeing frame %s of %u fields\n", aframe->ID3v2_Frame_Namestr, aframe->ID3v2_FieldCount);
#endif
		for(uint8_t id3fld = 0; id3fld < aframe->ID3v2_FieldCount; id3fld++) {
#if defined(DEBUG_V)
			fprintf(stdout, "freeing field %s ; %u of %u fields\n", (aframe->ID3v2_Frame_Fields+id3fld)->field_string, id3fld+1, aframe->ID3v2_FieldCount);
#endif
			ID3v2Fields* afield = aframe->ID3v2_Frame_Fields+id3fld;
			ID3v2Fields* freefield = NULL;
			while (true) {
				if ( afield != NULL && afield->field_string != NULL ) {
					free( afield->field_string );
					afield->field_string = NULL;
				}
				freefield = afield;
				afield = afield->next_field;
				if (afield == NULL) break;
				if (aframe->ID3v2_Frame_Fields+id3fld != freefield) free(freefield);
			}
		}
		free( aframe->ID3v2_Frame_Fields );
		aframe->ID3v2_Frame_Fields = NULL;
		free(aframe);	
		aframe = aframe->ID3v2_NextFrame;
	}
	return;
}
