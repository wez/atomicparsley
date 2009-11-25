//==================================================================//
/*
    AtomicParsley - AP_MetadataListings.h

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

void printBOM();
void APar_fprintf_UTF8_data(char* utf8_encoded_data);
#if defined (_MSC_VER)
void APar_unicode_win32Printout(wchar_t* unicode_out, char* utf8_out);
#endif

void APar_Extract_uuid_binary_file(AtomicInfo* uuid_atom, const char* originating_file, char* output_path);
void APar_Print_APuuid_atoms(const char *path, char* output_path, uint8_t target_information);

void APar_Print_iTunesData(const char *path, char* output_path, uint8_t supplemental_info, uint8_t target_information, AtomicInfo* ilstAtom = NULL);
void APar_PrintUserDataAssests(bool quantum_listing = false);

void APar_Extract_ID3v2_file(AtomicInfo* id32_atom, char* frame_str, char* originfile, char* destination_folder, AdjunctArgs* id3args);
void APar_Print_ID3v2_tags(AtomicInfo* id32_atom);

void APar_Print_ISO_UserData_per_track();
void APar_Mark_UserData_area(uint8_t track_num, short userdata_atom, bool quantum_listing);

//trees
void APar_PrintAtomicTree();
void APar_SimpleAtomPrintout();
