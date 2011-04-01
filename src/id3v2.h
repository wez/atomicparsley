//==================================================================//
/*
    AtomicParsley - id3v2.h

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

struct AdjunctArgs  {
	const char*     targetLang;
	const char*     descripArg;
	const char*     mimeArg;
	const char*     pictypeArg;
	const char*     filenameArg;
	const char*     ratingArg;
	const char*     dataArg; //multipurposed: PRIV's binary data, GRID's group data, UFID's binary data, POPM's counter field
	uint8_t   pictype_uint8;
	uint8_t   groupSymbol;
	bool      zlibCompressed;
	bool      multistringtext;
};

uint64_t syncsafeXX_to_UInt64(char* syncsafe_int, uint8_t syncsafe_len);
uint32_t syncsafe32_to_UInt32(char* syncsafe_int);

bool ID3v2_TestTagFlag(uint8_t TagFlag, uint8_t TagBit);
bool ID3v2_TestFrameFlag(uint16_t FrameFlag, uint16_t FrameBit);
uint8_t ImageListMembers();

void ListID3FrameIDstrings();
void List_imagtype_strings();
const char* ConvertCLIFrameStr_TO_frameID(const char* frame_str);
bool TestCLI_for_FrameParams(int frametype, uint8_t testparam);

int MatchID3FrameIDstr(const char* foundFrameID, uint8_t tagVersion);
uint8_t GetFrameCompositionDescription(int ID3v2_FrameTypeID);
int FrameStr_TO_FrameType(const char* frame_str);

void APar_ID32_ScanID3Tag(FILE* source_file, AtomicInfo* id32_atom);

uint32_t APar_GetTagSize(AtomicInfo* id32_atom);
uint32_t APar_Render_ID32_Tag(AtomicInfo* id32_atom, uint32_t max_alloc);

char* APar_ConvertField_to_UTF8(ID3v2Frame* targetframe, int fieldtype);

void APar_ID3Tag_Init(AtomicInfo* id32_atom);
void APar_ID3FrameAmmend(AtomicInfo* id32_atom, const char* frame_str, const char* frame_payload, AdjunctArgs* adjunct_payloads, uint8_t str_encoding);

void APar_FreeID32Memory(ID3v2Tag* id32tag);
