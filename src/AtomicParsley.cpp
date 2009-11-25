//==================================================================//
/*
    AtomicParsley - AtomicParsley.cpp

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

    ----------------------
    Code Contributions by:
		
    * Mike Brancato - Debian patches & build support
    * Lowell Stewart - null-termination bugfix for Apple compliance
		* Brian Story - native Win32 patches; memset/framing/leaks fixes
                                                                   */
//==================================================================//

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <wchar.h>

#if defined (HAVE_UNISTD_H)
#include <unistd.h>
#elif defined (_MSC_VER)
#include <io.h>
#endif

#include "AtomicParsley.h"
#include "AP_AtomDefinitions.h"
#include "AP_iconv.h"
#include "AP_arrays.h"
#include "APar_uuid.h"
#include "AP_ID3v2_tags.h"
#include "AP_MetadataListings.h"

#if defined (DARWIN_PLATFORM)
#include "AP_NSImage.h"
#include "AP_NSFile_utils.h"
#endif

//#define DEBUG_V

///////////////////////////////////////////////////////////////////////////////////////
//                               Global Variables                                    //
///////////////////////////////////////////////////////////////////////////////////////

bool modified_atoms = false;
bool alter_original = false;

bool svn_build = true; //controls which type of versioning - svn build-time stamp
//bool svn_build = false; //controls which type of versioning - release number

FILE* source_file = NULL;
uint32_t file_size;

struct AtomicInfo parsedAtoms[MAX_ATOMS];
short atom_number = 0;
uint8_t generalAtomicLevel = 1;

bool file_opened = false;
bool parsedfile = false;
bool move_moov_atom = true;
bool moov_atom_was_mooved = false;
AtomicInfo* hdlrAtom = NULL;
AtomicInfo* movie_header_atom = NULL;
bool complete_free_space_erasure = false;
bool psp_brand = false;
bool force_existing_hierarchy = false;
int metadata_style = UNDEFINED_STYLE;
bool deep_atom_scan = false;

uint32_t max_buffer = 4096*125; // increased to 512KB

uint32_t bytes_before_mdat=0;
uint32_t bytes_into_mdat = 0;
uint64_t mdat_supplemental_offset = 0;
uint32_t removed_bytes_tally = 0;
uint32_t new_file_size = 0; //used for the progressbar
uint32_t brand = 0;
uint32_t mdatData = 0; //now global, used in bitrate calcs

uint32_t gapless_void_padding = 0; //possibly used in the context of gapless playback support by Apple

struct DynamicUpdateStat dynUpd;
struct padding_preferences pad_prefs;

bool contains_unsupported_64_bit_atom = false; //reminder that there are some 64-bit files that aren't yet supported (and where that limit is set)

#if defined (WIN32) || defined (__CYGWIN__)
short max_display_width = 45;
#else
short max_display_width = 75; //ah, figured out grub - vga=773 makes a whole new world open up
#endif
char* file_progress_buffer=(char*)calloc(1, sizeof(char)* (max_display_width+50) ); //+50 for any overflow in "%100", or "|"

#if defined (DARWIN_PLATFORM)
struct PicPrefs myPicturePrefs;
#endif
bool parsed_prefs = false;
char* twenty_byte_buffer = (char *)malloc(sizeof(char)*20);

EmployedCodecs track_codecs = {false, false, false, false, false, false, false, false, false, false};

uint8_t UnicodeOutputStatus = UNIVERSAL_UTF8; //on windows, controls whether input/output strings are utf16 or raw utf8; reset in wmain()

uint8_t forced_suffix_type = NO_TYPE_FORCING;

///////////////////////////////////////////////////////////////////////////////////////
//                                Versioning                                         //
///////////////////////////////////////////////////////////////////////////////////////

void ShowVersionInfo() {

#if defined (_MSC_VER)
	char unicode_enabled[12];
	memset(unicode_enabled, 0, 12);
	if (UnicodeOutputStatus == WIN32_UTF16) {
		memcpy(unicode_enabled, "(utf16)", 7);

//its utf16 in the sense that any text entering on a modern Win32 system enters as utf16le - but gets converted immediately after AP.exe starts to utf8
//all arguments, strings, filenames, options are sent around as utf8. For modern Win32 systems, filenames get converted to utf16 for output as needed.
//Any strings to be set as utf16 in 3gp assets are converted to utf16be as needed (true for all OS implementations).
//Printing out to the console should be utf8.
	} else if (UnicodeOutputStatus == UNIVERSAL_UTF8) {
		memcpy(unicode_enabled, "(raw utf8)", 10);
		
//utf8 in the sense that any text entered had its utf16 upper byte stripped and reduced to (unchecked) raw utf8 for utilities that work in utf8. Any
//unicode (utf16) filenames were clobbered in that processes are invalid now. Any intermediate folder with unicode in it will now likely cause an error of
//some sort.
	}

#else
#define unicode_enabled	"(utf8)"
#endif

	if (svn_build) {  //below is the versioning from svn if used; remember to switch to AtomicParsley_version for a release

		fprintf(stdout, "AtomicParsley from svn built on %s %s\n", __DATE__, unicode_enabled);
	
	} else {  //below is the release versioning

		fprintf(stdout, "AtomicParsley version: %s %s\n", AtomicParsley_version, unicode_enabled); //release version
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                               Generic Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

int APar_TestArtworkBinaryData(const char* artworkPath) {
	int artwork_dataType = 0;
	FILE *artfile = APar_OpenFile(artworkPath, "rb");
	if (artfile != NULL) {		
		fread(twenty_byte_buffer, 1, 8, artfile);
		if ( strncmp(twenty_byte_buffer, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0 ) {
			artwork_dataType = AtomFlags_Data_PNGBinary;
		} else if ( strncmp(twenty_byte_buffer, "\xFF\xD8\xFF\xE0", 4) == 0 || memcmp(twenty_byte_buffer, "\xFF\xD8\xFF\xE1", 4) == 0 ) {
			artwork_dataType = AtomFlags_Data_JPEGBinary;
		} else {
			fprintf(stdout, "AtomicParsley error: %s\n\t image file is not jpg/png and cannot be embedded.\n", artworkPath);
			exit(1);
		}
		fclose(artfile);
		
	} else {
		fprintf(stdout, "AtomicParsley error: %s\n\t image file could not be opened.\n", artworkPath);
		exit(1);
	}
	return artwork_dataType;
}

#if defined (DARWIN_PLATFORM)
// enables writing out the contents of a single memory-resident atom out to a text file; for in-house testing purposes only - and unused in some time
void APar_AtomicWriteTest(short AtomicNumber, bool binary) {
	AtomicInfo anAtom = parsedAtoms[AtomicNumber];
	
	char* indy_atom_path = (char *)malloc(sizeof(char)*MAXPATHLEN); //this malloc can escape memset because its only for in-house testing
	strcat(indy_atom_path, "/Users/");
	strcat(indy_atom_path, getenv("USER") );
	strcat(indy_atom_path, "/Desktop/singleton_atom.txt");

	FILE* single_atom_file;
	single_atom_file = fopen(indy_atom_path, "wb");
	if (single_atom_file != NULL) {
		
		if (binary) {
			fwrite(anAtom.AtomicData, (size_t)(anAtom.AtomicLength - 12), 1, single_atom_file);
		} else {
			char* data = (char*)malloc(sizeof(char)*4);
			UInt32_TO_String4(anAtom.AtomicLength, data);
		
			fwrite(data, 4, 1, single_atom_file);
			fwrite(anAtom.AtomicName, 4, 1, single_atom_file);
		
			UInt32_TO_String4((uint32_t)anAtom.AtomicVerFlags, data);
			fwrite(data, 4, 1, single_atom_file);

			fwrite(anAtom.AtomicData, strlen(anAtom.AtomicData)+1, 1, single_atom_file);
			free(data);
		}
	}
	fclose(single_atom_file);
	free(indy_atom_path);
	return;
}
#endif

void APar_FreeMemory() {
	for(int iter=0; iter < atom_number; iter++) {
		if (parsedAtoms[iter].AtomicData != NULL) {
			free(parsedAtoms[iter].AtomicData);
			parsedAtoms[iter].AtomicData = NULL;
		}
		if (parsedAtoms[iter].ReverseDNSname != NULL) {
			free(parsedAtoms[iter].ReverseDNSname);
			parsedAtoms[iter].ReverseDNSname = NULL;
		}
		if (parsedAtoms[iter].ReverseDNSdomain != NULL) {
			free(parsedAtoms[iter].ReverseDNSdomain);
			parsedAtoms[iter].ReverseDNSdomain = NULL;
		}
		if (parsedAtoms[iter].uuid_ap_atomname != NULL) {
			free(parsedAtoms[iter].uuid_ap_atomname);
			parsedAtoms[iter].uuid_ap_atomname = NULL;
		}
		if (parsedAtoms[iter].ID32_TagInfo != NULL) {
			//a cascade of tests & free-ing of all the accrued ID3v2 tag/frame/field data
			APar_FreeID32Memory(parsedAtoms[iter].ID32_TagInfo);
						
			free(parsedAtoms[iter].ID32_TagInfo);
			parsedAtoms[iter].ID32_TagInfo = NULL;
		}
	}
	free(twenty_byte_buffer);
	twenty_byte_buffer=NULL;
	free(file_progress_buffer);
	file_progress_buffer=NULL;
	
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        Picture Preferences Functions                              //
///////////////////////////////////////////////////////////////////////////////////////

#if defined (DARWIN_PLATFORM)
PicPrefs APar_ExtractPicPrefs(char* env_PicOptions) {
	if (!parsed_prefs) {
		
		parsed_prefs = true; //only set default values & parse once
		
		myPicturePrefs.max_dimension=0; //dimensions won't be used to alter image
		myPicturePrefs.dpi = 72;
		myPicturePrefs.max_Kbytes = 0; //no target size to shoot for
		myPicturePrefs.allJPEG = false;
		myPicturePrefs.allPNG = false;
		myPicturePrefs.addBOTHpix = false;
		myPicturePrefs.force_dimensions = false;
		myPicturePrefs.force_height = 0;
		myPicturePrefs.force_width = 0;
		myPicturePrefs.removeTempPix = true; //we'll just make this the default
		
		char* unparsed_opts = env_PicOptions;
		if (env_PicOptions == NULL) return myPicturePrefs;
		
		while (unparsed_opts[0] != 0) {
			if (memcmp(unparsed_opts,"MaxDimensions=",14) == 0) {
				unparsed_opts+=14;
				myPicturePrefs.max_dimension = (int)strtol(unparsed_opts, NULL, 10);

			} else if (memcmp(unparsed_opts,"DPI=",4) == 0) {
				unparsed_opts+=4;
				myPicturePrefs.dpi = (int)strtol(unparsed_opts, NULL, 10);

			} else if (memcmp(unparsed_opts,"MaxKBytes=",10) == 0) {
				unparsed_opts+=10;
				myPicturePrefs.max_Kbytes = (int)strtol(unparsed_opts, NULL, 10)*1024;
				
			} else if (memcmp(unparsed_opts,"AllPixJPEG=",11) == 0) {
				unparsed_opts+=11;
				if (memcmp(unparsed_opts, "true", 4) == 0) {
					myPicturePrefs.allJPEG = true;
				}
				
			} else if (memcmp(unparsed_opts,"AllPixPNG=",10) == 0) {
				unparsed_opts+=10;
				if (memcmp(unparsed_opts, "true", 4) == 0) {
					myPicturePrefs.allPNG = true;
				}
				
			} else if (memcmp(unparsed_opts,"AddBothPix=",11) == 0) {
				unparsed_opts+=11;
				if (memcmp(unparsed_opts, "true", 4) == 0) {
					myPicturePrefs.addBOTHpix = true;
				}
				
			} else if (memcmp(unparsed_opts,"SquareUp",7) == 0) {
				unparsed_opts+=7;
				myPicturePrefs.squareUp = true;
				
			} else if (strncmp(unparsed_opts,"removeTempPix",13) == 0) {
				unparsed_opts+=13;
				myPicturePrefs.removeTempPix = true;
			
			} else if (memcmp(unparsed_opts,"keepTempPix",11) == 0) { //NEW
				unparsed_opts+=11;
				myPicturePrefs.removeTempPix = false;
			
			} else if (memcmp(unparsed_opts,"ForceHeight=",12) == 0) {
				unparsed_opts+=12;
				myPicturePrefs.force_height = strtol(unparsed_opts, NULL, 10);

			} else if (memcmp(unparsed_opts,"ForceWidth=",11) == 0) {
				unparsed_opts+=11;
				myPicturePrefs.force_width = strtol(unparsed_opts, NULL, 10);
				
			} else {
				unparsed_opts++;
			}
		}
	}

	if (myPicturePrefs.force_height > 0 && myPicturePrefs.force_width > 0) myPicturePrefs.force_dimensions = true;
	return myPicturePrefs;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////
//                            Locating/Finding Atoms                                 //
///////////////////////////////////////////////////////////////////////////////////////

AtomicInfo* APar_FindAtomInTrack(uint8_t &total_tracks, uint8_t &track_num, char* search_atom_str) {
	uint8_t track_tally = 0;
	short iter = 0;
	
	while (parsedAtoms[iter].NextAtomNumber != 0) {
		if ( memcmp(parsedAtoms[iter].AtomicName, "trak", 4) == 0 && parsedAtoms[iter].AtomicLevel == 2) {
			track_tally += 1;
			if (track_num == 0) {
				total_tracks += 1;
				
			} else if (track_num == track_tally) {
				//drill down into stsd
				short next_atom = parsedAtoms[iter].NextAtomNumber;
				while (parsedAtoms[next_atom].AtomicLevel > parsedAtoms[iter].AtomicLevel) {
					
					if (strncmp(parsedAtoms[next_atom].AtomicName, search_atom_str, 4) == 0) {
						return &parsedAtoms[next_atom];
					} else {
						next_atom = parsedAtoms[next_atom].NextAtomNumber;
					}
				}
			}
		}
		iter=parsedAtoms[iter].NextAtomNumber;
	}
	return NULL;
}

short APar_FindPrecedingAtom(short an_atom_num) {
	short precedingAtom = 0;
	short iter = 0;
	while (parsedAtoms[iter].NextAtomNumber != 0) {
		if (parsedAtoms[iter].NextAtomNumber == parsedAtoms[an_atom_num].NextAtomNumber) {
			break;
		} else {
			precedingAtom = iter;
			iter=parsedAtoms[iter].NextAtomNumber;
		}
	}
	return precedingAtom;
}

short APar_FindParentAtom(int order_in_tree, uint8_t this_atom_level) {
	short thisAtom = 0;
	short iter = order_in_tree;
	while (parsedAtoms[iter].AtomicNumber != 0) {
		iter = APar_FindPrecedingAtom(iter);
		if (parsedAtoms[iter].AtomicLevel == this_atom_level-1) {
			thisAtom = iter;
			break;
		}
	}
	return thisAtom;
}

/*----------------------
APar_ProvideAtomPath
  this_atom - index into array of parsedAtoms for the wanted path of an atom
  atom_path - string into which the path will be placed (working backwards)
  fromFile - controls the manner of extracting parents (atom sizes from file, or a simpler atomic level if from memory)

    First, determine exactly how many atoms will constitute the full path and calculate where into the string to first start placing atom names. Start by
		working off the current atom. Using fromFile, either use a more stringent atom start/length from a file, or a more relaxed atom level if from memory.
		The array in memory won't have proper atom sizes except for the last child atom typically ('data' will have a proper size, but its parent and all
		other parents will not have sizing automatically updated - which happens only at writeout time).
----------------------*/
void APar_ProvideAtomPath(short this_atom, char* &atom_path, bool fromFile) {
	short preceding_atom = this_atom;
	uint8_t	current_atomic_level = parsedAtoms[this_atom].AtomicLevel;
	int str_offset = (parsedAtoms[this_atom].AtomicLevel-1) * 5; //5 = 'atom" + '.'
	if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
		str_offset+=5; //include a "uuid=" string;
	}
	
	memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
	str_offset -=5;
	if (parsedAtoms[preceding_atom].AtomicLevel != 1) {
		memcpy(atom_path + str_offset +4, ".", 1);
	}
	if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
		memcpy(atom_path + str_offset, "uuid=", 5);
		str_offset-=5;
	}
	
	while (parsedAtoms[preceding_atom].AtomicNumber != 0) {
		
		if (fromFile) {
			if (parsedAtoms[preceding_atom].AtomicStart < parsedAtoms[this_atom].AtomicStart && 
					parsedAtoms[preceding_atom].AtomicLength > parsedAtoms[this_atom].AtomicLength && 
					parsedAtoms[preceding_atom].AtomicStart + parsedAtoms[preceding_atom].AtomicLength >= parsedAtoms[this_atom].AtomicStart + parsedAtoms[this_atom].AtomicLength &&
					parsedAtoms[preceding_atom].AtomicContainerState <= DUAL_STATE_ATOM ) {
				memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
				str_offset -=5;
				if (str_offset >= 0) {
					memcpy(atom_path + str_offset +4, ".", 1);
				}
				
				preceding_atom = APar_FindPrecedingAtom(preceding_atom);

			} else {
				preceding_atom = APar_FindPrecedingAtom(preceding_atom);
			}
		} else {
			if (parsedAtoms[preceding_atom].AtomicLevel < current_atomic_level) {
				memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
				str_offset -=5;
				if (str_offset >= 0) {
					memcpy(atom_path + str_offset +4, ".", 1);
				}
				
				current_atomic_level = parsedAtoms[preceding_atom].AtomicLevel;
				preceding_atom = APar_FindPrecedingAtom(preceding_atom);
			} else {
				preceding_atom = APar_FindPrecedingAtom(preceding_atom);
			}
		}
		if (preceding_atom == 0 || str_offset < 0) {
			break;
		}
	}
	
	return;
}

bool APar_Eval_ChunkOffsetImpact(short an_atom_num) {
	bool impact_calculations_directly = false;
	short iter = 0;
	uint8_t found_desired_atom = 0;
	
	while (true) {
		if ( strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0) {
			if (found_desired_atom) {
				impact_calculations_directly = true;
			}
			break;
		} else {
			iter=parsedAtoms[iter].NextAtomNumber;
		}
		if (iter == 0) {
			break;
		}
		if (iter == an_atom_num) {
			found_desired_atom = 1;
		}
	}
	return impact_calculations_directly;
}

short APar_FindLastAtom() {
	short this_atom_num = 0; //start our search with the first atom
	while (parsedAtoms[this_atom_num].NextAtomNumber != 0) {
		this_atom_num = parsedAtoms[this_atom_num].NextAtomNumber;
	}
	return this_atom_num;
}

short APar_FindLastChild_of_ParentAtom(short thisAtom) {
	short child_atom = parsedAtoms[thisAtom].NextAtomNumber;
	short last_atom = thisAtom; //if there are no children, this will be the first and last atom in the hiearchy
	while (true) {
		if (parsedAtoms[ child_atom ].AtomicLevel > parsedAtoms[thisAtom].AtomicLevel) {
			last_atom = child_atom;
		}
		child_atom = parsedAtoms[child_atom].NextAtomNumber;
		if (child_atom == 0 || parsedAtoms[ child_atom ].AtomicLevel <= parsedAtoms[thisAtom].AtomicLevel) {
			break;
		}
	}
	return last_atom;
}

/*----------------------
APar_ReturnChildrenAtoms
  this_atom - the parent atom that contains any number of children atoms (that are currenly unknown)
  atom_index - the index of the desired child. Passing 0 will return a count of the *total* number of children atoms under this_atom

    Working off of AtomicLevel, test the atoms that follow this_atom to see if they are immediately below this_atom. Increment total_children if is - if
		total_children should match our index, return that desired child at index atom.
----------------------*/
short APar_ReturnChildrenAtoms(short this_atom, uint8_t atom_index) {
	short child_atom = 0;
	uint8_t total_children = 0;
	short iter = parsedAtoms[this_atom].NextAtomNumber;
	
	while (true) {
		if ( (parsedAtoms[iter].AtomicLevel == parsedAtoms[this_atom].AtomicLevel + 1  && this_atom > 0) || 
				 (this_atom == 0 && parsedAtoms[iter].AtomicLevel == 1) ) {
			total_children++;
			
			if (atom_index == total_children) {
				child_atom = iter;
				break;
			}
		}
		if (parsedAtoms[iter].AtomicLevel <= parsedAtoms[this_atom].AtomicLevel && this_atom != 0) {
			break;
		} else {
			iter = parsedAtoms[iter].NextAtomNumber;
		}
		if (iter == 0) {
			break;
		}
	}
	if (atom_index == 0) {
		child_atom = (short)total_children;
	}
	return child_atom;
}

/*----------------------
APar_FindChildAtom
  parent_atom - the parent atom that contains any number of children atoms (that are currenly unknown)
	child_name - the name of the atom to search for under the parent_atom

    Given an atom, search its children for child_name
----------------------*/
AtomicInfo* APar_FindChildAtom(short parent_atom, char* child_name, uint8_t child_name_len = 4, uint16_t desired_index = 1) {
	AtomicInfo* found_child = NULL;
	short test_child_idx = parsedAtoms[parent_atom].NextAtomNumber;
	uint16_t current_count = 0;
	
	while (parsedAtoms[test_child_idx].AtomicLevel > parsedAtoms[parent_atom].AtomicLevel) {
		if ( (memcmp(parsedAtoms[test_child_idx].AtomicName, child_name, child_name_len) == 0  || memcmp(child_name, "any", 4) == 0) &&
		    parsedAtoms[test_child_idx].AtomicLevel == parsedAtoms[parent_atom].AtomicLevel +1) {
			current_count++;
			if (desired_index == current_count) {
				found_child = &parsedAtoms[test_child_idx];
				break;
			}
		}
		test_child_idx = parsedAtoms[test_child_idx].NextAtomNumber;
	}
	
	return found_child;
}

/*----------------------
APar_AtomicComparison
	proto_atom - the temporary atom structure to run the tests on
	test_atom - the exising atom to compare the proto_atom against
	match_full_uuids - selects whether to match by atom names (4 bytes) or uuids(16 bytes) which are stored on AtomicName
	reverseDNSdomain - the reverse DNS like com.foo.thing (only used with reverseDNS atoms: ----, mean, name)

    Test if proto_atom matches a single atom (test_atom) by name, level & classification (packed_lang_atom, extended atom...); for certain types of data 
		(like packed_lang & reverseDNS 'moov.udta.meta.ilst.----.name:[iTunNORM] atoms currently) add finer grained tests. The return result will be NULL
		if not matched, or returns the atom it matches.
----------------------*/
AtomicInfo* APar_AtomicComparison(AtomicInfo* proto_atom, short test_atom, bool match_full_uuids, const char* reverseDNSdomain) {
	AtomicInfo* return_atom = NULL;
	size_t ATOM_TEST_LEN = (match_full_uuids ? 16 : 4);
	
	if (parsedAtoms[test_atom].AtomicClassification == EXTENDED_ATOM && parsedAtoms[test_atom].uuid_style == UUID_DEPRECATED_FORM) {   //accommodate deprecated form
		if (memcmp(parsedAtoms[test_atom].uuid_ap_atomname, proto_atom->AtomicName, 4) == 0) {
			return &parsedAtoms[test_atom];
		}
	}
	
	//can't do AtomicVerFlags because lots of utilities don't write the proper iTunes flags for iTunes metadata
	if ( memcmp(proto_atom->AtomicName, parsedAtoms[test_atom].AtomicName, ATOM_TEST_LEN) == 0 &&
				proto_atom->AtomicLevel == parsedAtoms[test_atom].AtomicLevel && 
				(proto_atom->AtomicClassification == parsedAtoms[test_atom].AtomicClassification || proto_atom->AtomicClassification == UNKNOWN_ATOM) ) {
				
		if (proto_atom->AtomicClassification == PACKED_LANG_ATOM) {
			//0x05D9 = 'any' and will be used (internally) to match on name,class,container state alone, disregarding AtomicLanguage
			if (proto_atom->AtomicLanguage == parsedAtoms[test_atom].AtomicLanguage || proto_atom->AtomicLanguage == 0x05D9) {
				return_atom = &parsedAtoms[test_atom];
			}
			
		} else if (proto_atom->ReverseDNSname != NULL && parsedAtoms[test_atom].ReverseDNSname != NULL) {
			//match on moov.udta.meta.ilst.----.name:[something] (reverse DNS atom)
			size_t proto_rdns_len = strlen(proto_atom->ReverseDNSname) + 1;
			size_t test_rdns_len = strlen(parsedAtoms[test_atom].ReverseDNSname) + 1;
			size_t rdns_strlen = (proto_rdns_len > test_rdns_len? proto_rdns_len : test_rdns_len);
			if (memcmp(proto_atom->ReverseDNSname, parsedAtoms[test_atom].ReverseDNSname, rdns_strlen) == 0) {
				if (reverseDNSdomain == NULL) { //lock onto the first reverseDNS form irrespective of domain (TODO: manualAtomRemove will cause this to be NULL)
					return_atom = &parsedAtoms[test_atom];
				} else {
#if defined(DEBUG_V)
					fprintf(stdout, "AP_AtomicComparison   testing wanted rDNS %s domain against atom '%s' %s rDNS domain\n", reverseDNSdomain, parsedAtoms[test_atom].AtomicName,
					                                                              parsedAtoms[APar_FindPrecedingAtom(test_atom)].ReverseDNSdomain);
#endif
					if ( memcmp(reverseDNSdomain, parsedAtoms[APar_FindPrecedingAtom(test_atom)].ReverseDNSdomain, strlen(reverseDNSdomain)+1) == 0 ) {
						return_atom = &parsedAtoms[test_atom];
					}
				}
			}
		} else {
			return_atom = &parsedAtoms[test_atom];
		}
	}
	return return_atom;
}

/*----------------------
APar_FindLastLikeNamedAtom
	atom_name - the name of the atom to search for; the string itself may have more than 4 bytes
	containing_hierarchy - the parent hierarchy that is expected to carry multiply named atoms differing (in language for example)

    Follow through the atom tree; if a test atom is matched by name, and is a child to the container atom, remember that atom. If nothing matches, the index
		of the container atom is returned; otherwise the last like named atom is returned.
----------------------*/
short APar_FindLastLikeNamedAtom(char* atom_name, short containing_hierarchy) {
	short last_identically_named_atom = APar_FindLastChild_of_ParentAtom(containing_hierarchy); //default returns the last atom in the parent, not the parent
	short eval_atom = parsedAtoms[containing_hierarchy].NextAtomNumber;
	
	while (true) {
		if (parsedAtoms[eval_atom].AtomicLevel < parsedAtoms[containing_hierarchy].AtomicLevel + 1 || eval_atom == 0) {
			break;
		} else {
			if (memcmp(parsedAtoms[eval_atom].AtomicName, atom_name, 4) == 0 && 
			     parsedAtoms[eval_atom].AtomicLevel == parsedAtoms[containing_hierarchy].AtomicLevel + 1) {
				last_identically_named_atom = eval_atom;
			}
			eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
		}
	}
	return last_identically_named_atom;
}

void APar_FreeSurrogateAtom(AtomicInfo* surrogate_atom) {
	if (surrogate_atom->ReverseDNSname != NULL) {
		free(surrogate_atom->ReverseDNSname);
		surrogate_atom->ReverseDNSname = NULL;
	}
	return;
}

/*----------------------
APar_CreateSurrogateAtom

    Make a temporary AtomicInfo structure to run comparisons against; currently comparisons are done on name, level, classification (versioned...), langauge
		(3gp assets), and iTunes-style reverse dns 'name' carrying a string describing the purpose of the data (iTunNORM). This atom exists outside of a file's
		atom hieararchy that resides in the parsedAtoms[] array.
----------------------*/
void APar_CreateSurrogateAtom(AtomicInfo* surrogate_atom, const char* atom_name, uint8_t atom_level, uint8_t atom_class, uint16_t atom_lang,
                               char* revdns_name, uint8_t revdns_name_len) {
	surrogate_atom->AtomicName = (char*)atom_name;
	surrogate_atom->AtomicLevel = atom_level;
	
	if (revdns_name != NULL && revdns_name_len) {
		surrogate_atom->ReverseDNSname = (char *)malloc(sizeof(char)*revdns_name_len > 8 ? revdns_name_len+1 : 9);
		memset(surrogate_atom->ReverseDNSname, 0, sizeof(char)*revdns_name_len > 8 ? revdns_name_len+1 : 9);
		memcpy(surrogate_atom->ReverseDNSname, revdns_name, revdns_name_len);
		
	} else {
		APar_FreeSurrogateAtom(surrogate_atom);
	}
	surrogate_atom->AtomicClassification = atom_class;
	surrogate_atom->AtomicLanguage = atom_lang;
	return;
}

/*----------------------
APar_FindAtom
	atom_name - the full path describing the hiearchy the desired atom can be found in
	createMissing - either create the missing interim atoms as required, or return a NULL if not found
	atom_type - the classification of the last atom (packed language, uuid extended atom...)
	atom_lang - the language of the 3gp asset used when atom_type is packed language type
	match_full_uuids - match 16byte full uuids (typically removing ( possibly non-AP) uuids via --manualAtomRemoval; AP uuids (the new ones) still work on 4bytes**
	reverseDNSdomain - the reverse DNS like com.foo.thing (only used with reverseDNS atoms: ----, mean, name)

    Follow through the atom tree starting with the atom following 'ftyp'. Testing occurs on an atom level basis; a stand-in temporary skeletal atom
		is created to evaluate. If they atoms are deemed matching, atom_name is advanced forward (it still contains the full path, but only 4bytes are
		typically used at a time) and testing occurs until either the desired atom is found, or the last containing hiearchy with an exising atom is
		exhausted without making new atoms.
		
		NOTE: atom_name can come in these forms:
			classic/vanilla/ordinary atoms:		moov.udta.meta.ilst.cprt.data
			iTunes reverseDNS atoms:          moov.udta.meta.ilst.----.name:[iTunNORM]
			uuid user-extension atoms:        moov.udta.meta.uuid=tdtg (the deprecated form)
			uuid user-extension atoms:        moov.udta.meta.uuid=ba45fcaa-7ef5-5201-8a63-78886495ab1f
			index-based atoms:                moov.trak[2].mdia.minf
			
		NOTE: On my computer it takes about .04 second to scan the file, .1 second to add about 2 dozen tags, and 1.0 second to copy a file. Updating a file
		from start to finish takes 0.21 seconds. As many loops as this new APar_FindAtom eliminates, it is only marginally faster than the old code.
		
		** the reason why the old deprecated uuid form & the new uuid full 16byte form work off of a 4byte value (the atom name) is that because we are using a version 5
		sha1 hashed uuid of a name in a given namespace, the identical name in the identical namespace will yield identical an identical uuid (if corrected for endianness).
		This means that that matching by 4 bytes of atom name is the functional equvalent of matching by 16byte uuids.
----------------------*/
AtomicInfo* APar_FindAtom(const char* atom_name, bool createMissing, uint8_t atom_type, uint16_t atom_lang, bool match_full_uuids, const char* reverseDNSdomain) {
	AtomicInfo* thisAtom = NULL;
	char* search_atom_name = (char*)atom_name;
	char* reverse_dns_name = NULL;
	uint8_t revdns_name_len = 0;
	uint8_t atom_index = 0; // if there are atoms mutliple identically named at the same level, this is where to store the count as it occurs
	uint8_t desired_index = 1;
	uint8_t search_atom_type = UNKNOWN_ATOM;
	int known_atom = -1;
	short search_atom_start_num = parsedAtoms[0].NextAtomNumber; //don't test 'ftyp'; its atom_number[0] & will be used to know when we have hit the end of the tree; can't hardcode it to '1' because ftyp's following atom can change; only ftyp as parsedAtoms[0] is guaranteed.
	uint8_t present_atomic_level = 1;
	AtomicInfo* last_known_present_parent = NULL;
	AtomicInfo atom_surrogate = { 0 };
	
#if defined(DEBUG_V)
	fprintf(stdout, "debug: AP_FindAtom entry trying to find '%s'; create missing: %u\n", atom_name, createMissing);
#endif
	
	while (search_atom_name != NULL) {
		desired_index = 1; //reset the index
		
		if (atom_type == EXTENDED_ATOM && memcmp(search_atom_name, "uuid=", 5) == 0 ) {
			search_atom_name+=5;
			search_atom_type = atom_type;
 		}
		
#if defined(DEBUG_V)
		fprintf(stdout, "debug: AP_FindAtom   loop evaluate test  %s (index=%u)\n", search_atom_name, atom_index);
#endif
		
		size_t portion_len = strlen(search_atom_name);
		if (memcmp(search_atom_name+4, ":[", 2) == 0 && memcmp(search_atom_name + portion_len -1, "]", 1) == 0) {
			reverse_dns_name = search_atom_name + 4+2; //4bytes atom name 2bytes ":["
			revdns_name_len = portion_len-7; //4bytes atom name, 2 bytes ":[", 1 byte "]"
			search_atom_type = atom_type;
			
		} else if (memcmp(search_atom_name+4, "[", 1) == 0) {
			sscanf(search_atom_name+5, "%hhu", &desired_index);
			
#if defined(DEBUG_V)
			fprintf(stdout, "debug: AP_FindAtom     >#<indexed atom>#< '%s' at index=%u\n", search_atom_name, desired_index);
#endif
		}
		
		if (strlen(search_atom_name) == 4) {
			if (atom_type == UNKNOWN_ATOM) {
				known_atom = APar_MatchToKnownAtom(search_atom_name, last_known_present_parent->AtomicName, false, atom_name);
				search_atom_type = KnownAtoms[known_atom].box_type;
			} else {
				search_atom_type = atom_type;
			}
		}
		
		APar_CreateSurrogateAtom(&atom_surrogate, search_atom_name, present_atomic_level, search_atom_type, atom_lang, reverse_dns_name, revdns_name_len);
		atom_index = 0;
		
		short iter = search_atom_start_num;
		while (true) {
			AtomicInfo* result = NULL;
			
			//if iter == 0, that means test against 'ftyp' - and since its always 0, don't test it; its to know that the end of the tree is reached
			if (iter != 0 && (parsedAtoms[iter].AtomicLevel == present_atomic_level || reverse_dns_name != NULL) ) {
				result = APar_AtomicComparison(&atom_surrogate, iter, (search_atom_type == EXTENDED_ATOM ? match_full_uuids : false), reverseDNSdomain );
#if defined(DEBUG_V)
				fprintf(stdout, "debug: AP_FindAtom     compare  %s(%u)  against %s (wanted index=%u)\n", search_atom_name, atom_index, parsedAtoms[iter].AtomicName, desired_index);
			} else {
				fprintf(stdout, "debug: AP_FindAtom       %s  rejected against %s\n", search_atom_name, parsedAtoms[iter].AtomicName);
#endif
			}
			if (result != NULL) { //something matched
				atom_index++;
#if defined(DEBUG_V)
		fprintf(stdout, "debug: AP_FindAtom       ***matched*** current index=%u (want %u)\n", atom_index, desired_index);
#endif
				if (search_atom_type != UNKNOWN_ATOM || (search_atom_type == UNKNOWN_ATOM && known_atom != -1) ) {
					thisAtom = result;
#if defined(DEBUG_V)
					fprintf(stdout, "debug: AP_FindAtom         perfect match: %s(%u) == existing %s(%u)\n", search_atom_name, desired_index, parsedAtoms[iter].AtomicName, atom_index);
#endif
				} else {
					last_known_present_parent = result;   //if not, then it isn't the last atom, and must be some form of parent
				}
				if (desired_index == atom_index) {
					search_atom_start_num = parsedAtoms[iter].NextAtomNumber;
					break;
				}
			}
			
			if (parsedAtoms[iter].AtomicLevel < present_atomic_level && reverse_dns_name == NULL) {
				iter = 0; //force the ending determination of whether to make new atoms or not;
			}
			
			if (iter == 0 && createMissing) {
				//create that atom
				if (last_known_present_parent != NULL) {					
					short last_hierarchical_atom = 0;
#if defined(DEBUG_V)
					fprintf(stdout, "debug: AP_FindAtom-------missing atom, need to create '%s'\n", search_atom_name);
#endif					
					if (search_atom_type == PACKED_LANG_ATOM) {
						last_hierarchical_atom = APar_FindLastLikeNamedAtom(atom_surrogate.AtomicName, last_known_present_parent->AtomicNumber);
					} else {
						last_hierarchical_atom = APar_FindLastChild_of_ParentAtom(last_known_present_parent->AtomicNumber);
					}
					thisAtom = APar_CreateSparseAtom(&atom_surrogate, last_known_present_parent, last_hierarchical_atom);
					search_atom_start_num = thisAtom->AtomicNumber;
					if (strlen(search_atom_name) >= 4) {
						last_known_present_parent = thisAtom;
					}
				} else {
					//its a file-level atom that needs to be created, so it won't have a last_known_present_parent
					if (strlen(atom_name) == 4) {
						short total_root_level_atoms = APar_ReturnChildrenAtoms (0, 0);
						short test_root_atom = 0;
						
						
						//scan through all top level atoms
						for(uint8_t root_atom_i = 1; root_atom_i <= total_root_level_atoms; root_atom_i++) {
							test_root_atom = APar_ReturnChildrenAtoms (0, root_atom_i);
							if (memcmp(parsedAtoms[test_root_atom].AtomicName, "moov", 4) == 0) {
								break;
							}
						}
						if (test_root_atom != 0) {
							thisAtom = APar_CreateSparseAtom(&atom_surrogate, NULL, APar_FindLastChild_of_ParentAtom(test_root_atom));
						}
					}
				}
				break;
				
			} else if (iter == 0 && !createMissing) {
					search_atom_name = NULL; //force the break;
					break;
			}
			//fprintf(stdout, "while loop %s %u %u\n", parsedAtoms[iter].AtomicName, atom_index, desired_index);
			iter = parsedAtoms[iter].NextAtomNumber;
		}
		
		if (iter == 0 && (search_atom_name == NULL || search_atom_type == EXTENDED_ATOM) ) {
			break;
		} else {
			uint8_t periodicity = 0; //allow atoms with periods in their names
			while (true) { // search_atom_name = strsep(&atom_name,".") equivalent
				if (search_atom_name[0] == 0) {
					search_atom_name = NULL;
					break;
				} else if (memcmp(search_atom_name, ".", 1) == 0 && periodicity > 3) {
					search_atom_name++;
					periodicity++;
					break;
				} else {
					search_atom_name++;
					periodicity++;
				}
			}
			present_atomic_level++;
		}
	}
	//APar_PrintAtomicTree(); //because PrintAtomicTree calls DetermineDynamicUpdate (which calls this FindAtom function) to print out padding space, an infinite loop occurs
	APar_FreeSurrogateAtom(&atom_surrogate);
	return thisAtom;
}

///////////////////////////////////////////////////////////////////////////////////////
//                      File scanning & atom parsing                                 //
///////////////////////////////////////////////////////////////////////////////////////

void APar_AtomizeFileInfo(uint32_t Astart, uint32_t Alength, uint64_t Aextendedlength, char* Astring,
													uint8_t Alevel, uint8_t Acon_state, uint8_t Aclass, uint32_t Averflags, uint16_t Alang, uuid_vitals* uuid_info ) {
	static bool passed_mdat = false;
	AtomicInfo* thisAtom = &parsedAtoms[atom_number];
	
	thisAtom->AtomicStart = Astart;
	thisAtom->AtomicLength = Alength;
	thisAtom->AtomicLengthExtended = Aextendedlength;
	thisAtom->AtomicNumber = atom_number;
	thisAtom->AtomicLevel = Alevel;
	thisAtom->AtomicContainerState = Acon_state;
	thisAtom->AtomicClassification = Aclass;
	
	thisAtom->AtomicName = (char*)malloc(sizeof(char)*20);
	memset(thisAtom->AtomicName, 0, sizeof(char)*20);
	
	if (Aclass == EXTENDED_ATOM) {
		thisAtom->uuid_style = uuid_info->uuid_form;
		if (uuid_info->uuid_form == UUID_DEPRECATED_FORM) {
			memcpy(thisAtom->AtomicName, Astring, 4);
			thisAtom->uuid_ap_atomname = (char*)calloc(1, sizeof(char)*16);
			memcpy(thisAtom->uuid_ap_atomname, Astring, 4);
		} else {
			memcpy(thisAtom->AtomicName, uuid_info->binary_uuid, 16);
			if (uuid_info->uuid_form == UUID_AP_SHA1_NAMESPACE) {
				thisAtom->uuid_ap_atomname = (char*)calloc(1, sizeof(char)*16);
				memcpy(thisAtom->uuid_ap_atomname, uuid_info->uuid_AP_atom_name, 4);
			}
		}
	} else {
		memcpy(thisAtom->AtomicName, Astring, 4);
	}

	thisAtom->AtomicVerFlags = Averflags;
	thisAtom->AtomicLanguage = Alang;
	
	thisAtom->ancillary_data = 0;
	
	//set the next atom number of the PREVIOUS atom (we didn't know there would be one until now); this is our default normal mode
	parsedAtoms[atom_number-1].NextAtomNumber = atom_number;
	thisAtom->NextAtomNumber=0; //this could be the end... (we just can't quite say until we find another atom)
		
	if (strncmp(Astring, "mdat", 4) == 0) {
		passed_mdat = true;
	}

	if (!passed_mdat && Alevel == 1) {
		bytes_before_mdat += Alength; //this value gets used during FreeFree (for removed_bytes_tally) & chunk offset calculations
	}
	thisAtom->ID32_TagInfo = NULL;
			
	atom_number++; //increment to the next AtomicInfo array
	
	return;
}

uint8_t APar_GetCurrentAtomDepth(uint32_t atom_start, uint32_t atom_length) {
	short level = 1;
	for (int i = 0; i < atom_number; i++) {
		AtomicInfo* thisAtom = &parsedAtoms[i];
		if (atom_start == (thisAtom->AtomicStart + thisAtom->AtomicLength) ) {
			return thisAtom->AtomicLevel;
		} else {
			if ( (atom_start < thisAtom->AtomicStart + thisAtom->AtomicLength) && (atom_start > thisAtom->AtomicStart) ) {
				level++;
			}
		}
	}
	return level;
}

void APar_IdentifyBrand(char* file_brand ) {
	brand = UInt32FromBigEndian(file_brand);
	switch (brand) {
		//what ISN'T supported
		case 0x71742020 : //'qt  '  --this is listed at mp4ra, but there are features of the file that aren't supported (like the 4 NULL bytes after the last udta child atom
			fprintf(stdout, "AtomicParsley error: Quicktime movie files are not supported.\n");
			exit(2);
			break;
		
		//
		//3GPP2 specification documents brands
		//
		
		case 0x33673262 : //'3g2b' 3GPP2 release A
			metadata_style = THIRD_GEN_PARTNER_VER2_REL_A;    //3GPP2 C.S0050-A_v1.0_060403, Annex A.2 lists differences between 3GPP & 3GPP2 - assets are not listed
			break;

		case 0x33673261 : //'3g2a'                          //3GPP2 release 0
			metadata_style = THIRD_GEN_PARTNER_VER2;
			break;

		//
		//3GPP specification documents brands, not all are listed at mp4ra
		//
		
		case 0x33677037 : //'3gp7'                          //Release 7 introduces ID32; though it doesn't list a iso bmffv2 compatible brand. Technically, ID32
		                                                    //could be used on older 3gp brands, but iso2 would have to be added to the compatible brand list.
		case 0x33677337 : //'3gs7'                          //I don't feel the need to do that, since other things might have to be done. And I'm not looking into it.
		case 0x33677237 : //'3gr7'
		case 0x33676537 : //'3ge7'
		case 0x33676737 : //'3gg7'
			metadata_style = THIRD_GEN_PARTNER_VER1_REL7;
			break;
		
		case 0x33677036 : //'3gp6'                          //3gp assets which were introducted by NTT DoCoMo to the Rel6 workgroup on January 16, 2003
																												//with S4-030005.zip from http://www.3gpp.org/ftp/tsg_sa/WG4_CODEC/TSGS4_25/Docs/ (! albm, loci)
		case 0x33677236 : //'3gr6' progressive
		case 0x33677336 : //'3gs6' streaming
		case 0x33676536 : //'3ge6' extended presentations (jpeg images)
		case 0x33676736 : //'3gg6' general (not yet suitable; superset)		
			metadata_style = THIRD_GEN_PARTNER_VER1_REL6;
			break;
		
		case 0x33677034 : //'3gp4'                          //3gp assets (the full complement) are available: source clause is S5.5 of TS26.244 (Rel6.4 & later):
		case 0x33677035 : //'3gp5'                          //"that the file conforms to the specification; it includes everything required by,
			metadata_style = THIRD_GEN_PARTNER;               //and nothing contrary to the specification (though there may be other material)"
			break;                                            //it stands to reason that 3gp assets aren't contrary since 'udta' is defined by iso bmffv1
		
		//
		//other brands that are have compatible brands relating to 3GPP/3GPP2
		//
		
		case 0x6B646469 : //'kddi'                          //3GPP2 EZmovie (optionally restricted) media; these have a 3GPP2 compatible brand
			metadata_style = THIRD_GEN_PARTNER_VER2;
			break;
		case 0x6D6D7034 : //'mmp4'
			metadata_style = THIRD_GEN_PARTNER;
			break;
		
		//
		//what IS supported for iTunes-style metadata
		//
		
		case 0x4D534E56 : //'MSNV'  (PSP) - this isn't actually listed at mp4ra, but since they are popular...
			metadata_style = ITUNES_STYLE;
			psp_brand = true;
			break;
		case 0x4D344120 : //'M4A '  -- these are all listed at http://www.mp4ra.org/filetype.html as registered brands
		case 0x4D344220 : //'M4B '
		case 0x4D345020 : //'M4P '
		case 0x4D345620 : //'M4V '
		case 0x6D703432 : //'mp42'
		case 0x6D703431 : //'mp41'
		case 0x69736F6D : //'isom'
		case 0x69736F32 : //'iso2'
		case 0x61766331 : //'avc1'
		
		case 0x4D345648 : //'MV4H'

			metadata_style = ITUNES_STYLE;
			break;
		
		//
		//other brands that are derivatives of the ISO Base Media File Format
		//
		case 0x6D6A7032 : //'mjp2'
		case 0x6D6A3273 : //'mj2s'
			metadata_style = MOTIONJPEG2000;
			break;
			
		//other lesser unsupported brands; http://www.mp4ra.org/filetype.html like dv, mp21 & ... whatever mpeg7 brand is
		default :
			fprintf(stdout, "AtomicParsley error: unsupported MPEG-4 file brand found '%s'\n", file_brand);
			exit(2);
			break;
	}
	return;
}

void APar_TestCompatibleBrand(FILE* file, uint32_t atom_start, uint32_t atom_length) {
	if (atom_length <= 16) return;
	uint32_t compatible_brand = 0;

	for (uint32_t brand = 16; brand < atom_length; brand+=4) {
		compatible_brand = APar_read32(twenty_byte_buffer, file, atom_start+brand);
		if (compatible_brand == 0x6D703432 || compatible_brand == 0x69736F32) {
			parsedAtoms[atom_number-1].ancillary_data = compatible_brand;
		}
	}	
	return;
}

void APar_Extract_stsd_codec(FILE* file, uint32_t midJump) {
	memset(twenty_byte_buffer, 0, 12);
	fseeko(file, midJump, SEEK_SET);
	fread(twenty_byte_buffer, 1, 12, file);
	parsedAtoms[atom_number-1].ancillary_data = UInt32FromBigEndian( twenty_byte_buffer +4 );
  return;
}

/*----------------------
APar_LocateDataReference
  fill
----------------------*/
void APar_LocateDataReference(short chunk_offset_idx, FILE* file) {
	uint32_t data_ref_idx = 0;
	short sampletable_atom_idx = 0;
	short minf_atom_idx = 0;
	AtomicInfo *stsd_atom, *dinf_atom, *dref_atom, *target_reference_atom = NULL;
	
	sampletable_atom_idx = APar_FindParentAtom(chunk_offset_idx, parsedAtoms[chunk_offset_idx].AtomicLevel);
	stsd_atom = APar_FindChildAtom(sampletable_atom_idx, "stsd");
	if (stsd_atom == NULL) {
		return;
	}
	data_ref_idx = APar_read32(twenty_byte_buffer, file, stsd_atom->AtomicStart+28);
	
	minf_atom_idx = APar_FindParentAtom(sampletable_atom_idx, parsedAtoms[sampletable_atom_idx].AtomicLevel);
	dinf_atom = APar_FindChildAtom(minf_atom_idx, "dinf");
	dref_atom = APar_FindChildAtom(dinf_atom->AtomicNumber, "dref");
	
	target_reference_atom = APar_FindChildAtom(dref_atom->AtomicNumber, "any", 4, (uint16_t)data_ref_idx);
	
	if (target_reference_atom != NULL) {
		parsedAtoms[chunk_offset_idx].ancillary_data = target_reference_atom->AtomicVerFlags;
	}
	return;
}

void APar_SampleTableIterator(FILE *file) {
	uint8_t total_tracks = 0;
	uint8_t a_track = 0;
	char track_path[36];
	memset(track_path, 0, 36);
	AtomicInfo* samples_parent = NULL;
	AtomicInfo* chunk_offset_atom = NULL;
	
	APar_FindAtomInTrack(total_tracks, a_track, NULL); //gets the number of tracks
	for (uint8_t trk_idx=1; trk_idx <= total_tracks; trk_idx++) {
		sprintf(track_path, "moov.trak[%hhu].mdia.minf.stbl", trk_idx);
		samples_parent = APar_FindAtom(track_path, false, SIMPLE_ATOM, 0, false);
		if (samples_parent != NULL) {
			chunk_offset_atom = APar_FindChildAtom(samples_parent->AtomicNumber, "stco");
			if (chunk_offset_atom == NULL) chunk_offset_atom = APar_FindChildAtom(samples_parent->AtomicNumber, "co64");
			if (chunk_offset_atom != NULL) {
				APar_LocateDataReference(chunk_offset_atom->AtomicNumber, file);
			}
		}
	}
	return;
}

uint64_t APar_64bitAtomRead(FILE *file, uint32_t jump_point) {
	uint64_t extended_dataSize = APar_read64(twenty_byte_buffer, file, jump_point+8);
	
	//here's the problem: there can be some HUGE MPEG-4 files. They get specified by a 64-bit value. The specification says only use them when necessary - I've seen them on files about 700MB. So this will artificially place a limit on the maximum file size that would be supported under a 32-bit only AtomicParsley (but could still see & use smaller 64-bit values). For my 700MB file, moov was (rounded up) 4MB. So say 4MB x 6 +1MB give or take, and the biggest moov atom I would support is.... a heart stopping 30MB (rounded up). GADZOOKS!!! So, since I have no need to go greater than that EVER, I'm going to stik with uin32_t for filesizes and offsets & that sort. But a smaller 64-bit mdat (essentially a pseudo 32-bit traditional mdat) can be supported as long as its less than UINT32_T_MAX (minus our big fat moov allowance).
	
	if ( extended_dataSize > 4294967295UL - 30000000) {
		contains_unsupported_64_bit_atom = true;
		fprintf(stdout, "You must be off your block thinking I'm going to tag a file that is at LEAST %llu bytes long.\n", extended_dataSize);
		fprintf(stdout, "AtomicParsley doesn't have full 64-bit support");
		exit (2);
	}
	return extended_dataSize;
}

/*----------------------
APar_MatchToKnownAtom
  atom_name - the name of our newly found atom
  atom_container - the name of the parent container atom
  fromFile - controls the manner of extracting parents (passed thu to another function)

    Using the atom_name of this new atom, search through KnownAtoms, testing that the names match. If they do, move onto a finer grained sieve.
		If the parent can be at any level (like "free"), just let it through; if the parent is "ilst" (iTunes-style metadata), or a uuid, return a generic match
		The final test is the one most atoms will go through. Some atoms can have different parents - up to 5 different parents are allowed by this version of AP
		Iterate through the known parents, and test it against atom_container. If they match, return the properties of the known atom
----------------------*/
int APar_MatchToKnownAtom(const char* atom_name, const char* atom_container, bool fromFile, const char* find_atom_path) {
	uint32_t total_known_atoms = (uint32_t)(sizeof(KnownAtoms)/sizeof(*KnownAtoms));
	uint32_t return_known_atom = 0;
	
	//if this atom is contained by 'ilst', then it is *highly* likely an iTunes-style metadata parent atom
	if ( memcmp(atom_container, "ilst", 4) == 0 && memcmp(atom_name, "uuid", 4) != 0) {
		return_known_atom = total_known_atoms-2; //2nd to last KnowAtoms is a generic placeholder iTunes-parent atom
		//fprintf(stdout, "found iTunes parent %s = atom %s\n", KnownAtoms[return_known_atom].known_atom_name, atom_name);
	
	//if this atom is "data" get the full path to it; we will take any atom under 'ilst' and consider it an iTunes metadata parent atom
	} else if (memcmp(atom_name, "data", 4) == 0 && find_atom_path != NULL) {
		if (memcmp(find_atom_path, "moov.udta.meta.ilst.", 20) == 0) {
			return_known_atom = total_known_atoms-1; //last KnowAtoms is a generic placeholder iTunes-data atom
			//fprintf(stdout, "found iTunes data child\n");
		}
	
	} else if (memcmp(atom_name, "data", 4) == 0) {
		char* fullpath = (char *)malloc(sizeof(char) * 200);
		memset(fullpath, 0, sizeof(char) * 200);
		
		if (fromFile) {
			APar_ProvideAtomPath(parsedAtoms[atom_number-1].AtomicNumber, fullpath, fromFile);
		} else { //find_atom_path only is NULL in APar_ScanAtoms (where fromFile is true) and in APar_CreateSparseAtom, where atom_number was just filled
			APar_ProvideAtomPath(parsedAtoms[atom_number].AtomicNumber, fullpath, fromFile);
		}
		
		//fprintf(stdout, "APar_ProvideAtomPath gives %s (%s-%s)\n", fullpath, atom_name, atom_container);
		if (memcmp(fullpath, "moov.udta.meta.ilst.", 20) == 0) {
			return_known_atom = total_known_atoms-1; //last KnowAtoms is a generic placeholder iTunes-data atom
			//fprintf(stdout, "found iTunes data child\n");
		}
		free(fullpath);
		fullpath = NULL;
	
	//if this atom is "esds" get the full path to it; take any atom under 'stsd' as a parent to esds (that parent would be a 4CC codec; not all do have 'esds'...)
	} else if (memcmp(atom_name, "esds", 4) == 0 ) {
		char* fullpath = (char *)malloc(sizeof(char) * 300);
		memset(fullpath, 0, sizeof(char) * 200);
		
		APar_ProvideAtomPath(parsedAtoms[atom_number-1].AtomicNumber, fullpath, fromFile);
		
		if (memcmp(fullpath, "moov.trak.mdia.minf.stbl.stsd.", 30) == 0) {
			return_known_atom = total_known_atoms-3; //manually return the esds atom
		}
		free(fullpath);
		fullpath = NULL;

		
	} else {
	//try matching the name of the atom
		for(uint32_t i = 1; i < total_known_atoms; i++) {
			if (memcmp(atom_name, KnownAtoms[i].known_atom_name, 4) == 0) {
				//name matches, now see if the container atom matches any known container for that atom
				if ( memcmp(KnownAtoms[i].known_parent_atoms[0], "_ANY_LEVEL", 10) == 0 ) {
					return_known_atom = i; //the list starts at 0; the unknown atom is at 0; first known atom (ftyp) is at 1
					break;
					
				} else {
					uint8_t total_known_containers = (uint8_t)(sizeof(KnownAtoms[i].known_parent_atoms)/sizeof(*KnownAtoms[i].known_parent_atoms)); //always 5
					for (uint8_t iii = 0; iii < total_known_containers; iii++) {
						if (KnownAtoms[i].known_parent_atoms[iii] != NULL) {
							if ( memcmp(atom_container, KnownAtoms[i].known_parent_atoms[iii], strlen(atom_container) ) == 0) { //strlen(atom_container)
								return_known_atom = i; //the list starts at 0; the unknown atom is at 0; first known atom (ftyp) is at 1
								break;
							}
						}
					}
				}
				if (return_known_atom) {
					break;
				}
			}
		}
	}
	if ( return_known_atom > total_known_atoms ) {
		return_known_atom = 0;
	}
	//accommodate any future child to dref; force to being versioned
	if (return_known_atom == 0 && memcmp(atom_container, "dref", 4) == 0) {
		return_known_atom = total_known_atoms-4; //return a generic *VERSIONED* child atom; otherwise an atom without flags will be present & chunk offsets will not update
	}
	return return_known_atom;
}

/*----------------------
APar_Manually_Determine_Parent
  atom_start - the place in the file where the atom begins
  atom_length - length of the eval atom
  container - a string of the last known container atom (or FILE_LEVEL)

    given the next atom (unknown string at this point), run some tests using its starting point and length. Iterate backwards through the already parsed
		atoms, and for each test if it could contain this atom. Tests include if the container starts before ours (which it would to contain the new atom),
		that its length is longer than our length (any parent would need to be longer than us if even by 8 bytes), the start + length sum of the parent atom
		(where it ends), needs to be greater than or equal to where this new atom ends, and finally, that the eval containing atom be some form of parent as
		defined in KnownAtoms
----------------------*/
void APar_Manually_Determine_Parent(uint32_t atom_start, uint32_t atom_length, char* container) {
	short preceding_atom = atom_number-1;
	while (parsedAtoms[preceding_atom].AtomicNumber != 0) {

		if (parsedAtoms[preceding_atom].AtomicStart < atom_start && 
				parsedAtoms[preceding_atom].AtomicLength > atom_length && 
				parsedAtoms[preceding_atom].AtomicStart + parsedAtoms[preceding_atom].AtomicLength >= atom_start + atom_length &&
				parsedAtoms[preceding_atom].AtomicContainerState <= DUAL_STATE_ATOM ) {
			memcpy(container, parsedAtoms[preceding_atom].AtomicName, 5);
			break;

		} else {
			preceding_atom--;
		}
		if (preceding_atom == 0) {
			memcpy(container, "FILE_LEVEL", 11);
		}
	}
	return;
}

/*----------------------
APar_ScanAtoms
  path - the complete path to the originating file to be tested
  deepscan_REQ - controls whether we go into 'stsd' or just a superficial scan

    if the file has not yet been scanned (this gets called by nearly every cli option), then open the file and start scanning. Read in the first 12 bytes
		and see if bytes 4-8 are 'ftyp' as any modern MPEG-4 file will have 'ftyp' first. Accommodations are also in place for the jpeg2000 signature, but the sig.
		must be followed by 'ftyp' and have an 'mjp2' or 'mj2s' brand. If it does, start scanning the rest of the file. An MPEG-4 file is logically organized
		into discrete hierarchies called "atoms" or "boxes". Each atom is at minimum 8 bytes long. Bytes 1-4 make an unsigned 32-bit integer that denotes how
		long this atom is (ie: 8 would mean this atom is 8 bytes long). The next 4 bytes (bytes 5-8) make the atom name. If the atom presents longer than 8 bytes,
		then that supplemental data would be what the atom carries. Atoms are broadly separated into 2 categories: parents & children (or container & leaf).
		Typically, a parent can hold other atoms, but not data; a child can hold data but not other atoms. This 'rule' is broken sometimes (the atoms listed
		as DUAL_STATE_ATOM), but largely holds.
		
		Each atom is read in as 12 bytes (to accommodate flags & versioning). The atom name is extracted, and using the last known container (either FILE_LEVEL
		or an actual atom name), the new atom's hierarchy is found based on its length & position. Using its containing atom, the KnownAtoms table is searched to
		locate the properties of that atom (parent/child, versioned/simple), and jumping around in the file is based off that known atom's type. Atoms that
		fall into a hybrid category (DUAL_STATE_ATOMs) are explicitly handled. If an atom is listed has having a language attribute, it is read to support
		multiple langauges (as most 3GP assets do).
----------------------*/
void APar_ScanAtoms(const char *path, bool deepscan_REQ) {
	if (!parsedfile) {
		file_size = findFileSize(path);
		
		FILE *file = APar_OpenFile(path, "rb");
		if (file != NULL) {
			char *data = (char *)calloc(1, 13);
			char *container = (char *)calloc(1, 20);
			memcpy(container, "FILE_LEVEL", 10);
			bool corrupted_data_atom = false;
			bool jpeg2000signature = false;
			
			uuid_vitals uuid_info = {0};
			uuid_info.binary_uuid=(char*)malloc(sizeof(char)*16 + 1);  //this will hold any potential 16byte uuids
			uuid_info.uuid_AP_atom_name=(char*)malloc(sizeof(char)*5); //this will hold any atom name that is written after the uuid written by AP
			
			if (data == NULL) return;
			uint32_t dataSize = 0;
			uint32_t jump = 0;
			
			fread(data, 1, 12, file);
			char *atom = data+4;
			
			if ( memcmp(data, "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A ", 12) == 0 ) {
				jpeg2000signature = true;
			}

			if ( memcmp(atom, "ftyp", 4) == 0 || jpeg2000signature) {
							
				dataSize = UInt32FromBigEndian(data);
				jump = dataSize;
				
				APar_AtomizeFileInfo(0, jump, 0, atom, generalAtomicLevel, CHILD_ATOM, SIMPLE_ATOM, 0, 0 , &uuid_info);
				
				if (!jpeg2000signature) {
					APar_IdentifyBrand( data + 8 );
					APar_TestCompatibleBrand(file, 0, dataSize);
				}
				
				fseek(file, jump, SEEK_SET);
				
				while (jump < (uint32_t)file_size) {
					
					uuid_info.uuid_form = UUID_DEPRECATED_FORM; //start with the assumption that any found atom is in the depracted uuid form
					
					fread(data, 1, 12, file);
					char *atom = data+4;
					dataSize = UInt32FromBigEndian(data);
					
					if (jpeg2000signature) {
						if (memcmp(atom, "ftyp", 4) == 0) {
							APar_IdentifyBrand( data + 8 );
						} else {
							exit(0); //the atom right after the jpeg2000/mjpeg2000 signature is *supposed* to be 'ftyp'
						}
						jpeg2000signature = false;
					}
					
					if ( dataSize > (uint64_t)file_size) {
						dataSize = (uint32_t)(file_size - jump);
					}
					
					if (dataSize == 0 && (atom[0] == 0 && atom[1] == 0 && atom[2] == 0 && atom[3] == 0) ) {
						gapless_void_padding = file_size-jump; //Apple has decided to add around 2k of NULL space outside of any atom structure starting with iTunes 7.0.0
						break; //its possible this is part of gapless playback - but then why would it come after the 'free' at the end of a file like gpac writes?
					} //after actual tested its elimination, it doesn't seem to be required for gapless playback
					
					//diagnose damage to 'cprt' by libmp4v2 in 1.4.1 & 1.5.0.1
					//typically, the length of this atom (dataSize) will exceeed it parent (which is reported as 17)
					//true length ot this data will be 9 - impossible for iTunes-style 'data' atom.
					if (memcmp(atom, "data", 4) == 0 && parsedAtoms[ atom_number-1].AtomicContainerState == PARENT_ATOM) {
						if (dataSize > parsedAtoms[ atom_number-1].AtomicLength) {
							dataSize = parsedAtoms[ atom_number-1].AtomicLength -8; //force its length to its true length
							fprintf(stdout, "AtomicParsley warning: the 'data' child of the '%s' atom seems to be corrupted.\n", parsedAtoms[ atom_number-1].AtomicName);
							corrupted_data_atom = true;
						}
					}
					//end diagnosis; APar_Manually_Determine_Parent will still determine it to be a versioned atom (it tests by names), but at file write out,
					//it will write with a length of 9 bytes
					
					APar_Manually_Determine_Parent(jump, dataSize, container);
					int filtered_known_atom = APar_MatchToKnownAtom(atom, container, true, NULL);
					
					uint32_t atom_verflags = 0;
					uint16_t atom_language = 0;

					if (memcmp(atom, "uuid", 4) == 0) {
						memset(uuid_info.binary_uuid, 0, 20);
						
						APar_readX(uuid_info.binary_uuid, file, jump+8, 16);
						
						if (UInt32FromBigEndian(uuid_info.binary_uuid+8) == 0) { //the deperacted uuid form
							atom = data+8;
							atom_verflags = APar_read32(uuid_info.binary_uuid, file, jump+12);
							if (atom_verflags > (uint32_t)AtomFlags_Data_UInt) {
								atom_verflags = 0;
							}
						} else {
							uint8_t uuid_version = APar_extract_uuid_version(NULL, uuid_info.binary_uuid);
							APar_endian_uuid_bin_str_conversion(uuid_info.binary_uuid);
							if (uuid_version == 5) {
								uuid_info.uuid_form = UUID_SHA1_NAMESPACE;
								//read in what AP would set the atom name to. The new uuid form is:
								// 4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
								APar_readX(uuid_info.uuid_AP_atom_name, file, jump+24, 4);
								
								char uuid_of_foundname_in_AP_namesapce[20];
								APar_generate_uuid_from_atomname(uuid_info.uuid_AP_atom_name, uuid_of_foundname_in_AP_namesapce);
								if (memcmp(uuid_info.binary_uuid, uuid_of_foundname_in_AP_namesapce, 16) == 0) {
									uuid_info.uuid_form = UUID_AP_SHA1_NAMESPACE; //our own uuid ver5 atoms in the AtomicParsley.sf.net namespace
									atom_verflags = APar_read32(twenty_byte_buffer, file, jump+28);
								}
							} else {
								uuid_info.uuid_form = UUID_OTHER;
							}
						}
					}
					
					if (KnownAtoms[filtered_known_atom].box_type == VERSIONED_ATOM && !corrupted_data_atom) {
						atom_verflags = UInt32FromBigEndian(data+8); //flags & versioning were already read in with the original 12 bytes
					}
					
					if (KnownAtoms[filtered_known_atom].box_type == PACKED_LANG_ATOM) {
						//the problem with storing the language is that the uint16_t 2 bytes that carry the actual language are in different places on different atoms
						//some atoms have it right after the atom version/flags; some like rating/classification have it 8 bytes later; yrrc doesn't have it at all
						char bitpacked_lang[4];
						memset(bitpacked_lang, 0, 4);
						
						uint32_t userdata_box = UInt32FromBigEndian(atom);
						
						switch (userdata_box) {
							case 0x7469746C : //'titl'
							case 0x64736370 : //'dscp'
							case 0x63707274 : //'cprt'
							case 0x70657266 : //'perf'
							case 0x61757468 : //'auth'
							case 0x676E7265 : //'gnre'
							case 0x616C626D : //'albm'
							case 0x6B797764 : //'kywd'
							case 0x6C6F6369 : //'loci'
							case 0x49443332 : //'ID32' ; technically not a 'user data box', but this only extracts the packed language (which ID32 does have)
							{
								atom_language = APar_read16(bitpacked_lang, file, jump + 12);
								break;
							}
							case 0x636C7366 : //'clsf'
							{
								atom_language = APar_read16(bitpacked_lang, file, jump + 18);
								break;
							}
							case 0x72746E67 : //'rtng'
							{
								atom_language = APar_read16(bitpacked_lang, file, jump + 20);
								break;
							}
							//case 0x79727263 : //'yrrc' is the only 3gp tag that doesn't support multiple languages; won't even get here because != PACKED_LANG_ATOM
							default : 
							{
								break; //which means that any new/unknown packed language atoms will have their language of 0; AP will only support 1 of this atom name then
							}
						}
					}
					
					//mdat.length=1; and ONLY supported for mdat atoms - no idea if the spec says "only mdat", but that's what I'm doing for now
					if ( (strncmp(atom, "mdat", 4) == 0) && (generalAtomicLevel == 1) && (dataSize == 1) ) {
						uint64_t extended_dataSize = APar_64bitAtomRead(file, jump);
						APar_AtomizeFileInfo(jump, 1, extended_dataSize, atom, generalAtomicLevel, KnownAtoms[filtered_known_atom].container_state,
                                  KnownAtoms[filtered_known_atom].box_type, atom_verflags, atom_language, &uuid_info );
						
					} else {
						APar_AtomizeFileInfo(jump, dataSize, 0, atom, generalAtomicLevel, KnownAtoms[filtered_known_atom].container_state, 
                                  corrupted_data_atom ? SIMPLE_ATOM : KnownAtoms[filtered_known_atom].box_type, atom_verflags, atom_language, &uuid_info );
					}
					corrupted_data_atom = false;
					
					//read in the name of an iTunes-style internal reverseDNS directly into parsedAtoms
					if (memcmp(atom, "mean", 4) == 0 && memcmp(parsedAtoms[atom_number-2].AtomicName, "----", 4) == 0) {
							parsedAtoms[atom_number-1].ReverseDNSdomain = (char *)calloc(1, sizeof(char) * dataSize);
							
							fseeko(file, jump + 12, SEEK_SET); //'name' atom is the 2nd child
							fread(parsedAtoms[atom_number-1].ReverseDNSdomain, 1, dataSize - 12, file);
					}
					if (memcmp(atom, "name", 4) == 0 && 
					    memcmp(parsedAtoms[atom_number-2].AtomicName, "mean", 4) == 0 &&
							memcmp(parsedAtoms[atom_number-3].AtomicName, "----", 4) == 0) {
							
							parsedAtoms[atom_number-1].ReverseDNSname = (char *)calloc(1, sizeof(char) * dataSize);
							
							fseeko(file, jump + 12, SEEK_SET); //'name' atom is the 2nd child
							fread(parsedAtoms[atom_number-1].ReverseDNSname, 1, dataSize - 12, file);
					}
					
					if (dataSize == 0) { // length = 0 means it reaches to EOF
						break;
					}
					
					switch (KnownAtoms[filtered_known_atom].container_state) {
						case PARENT_ATOM : {
							jump += 8;
							break;
						}
						case CHILD_ATOM : {
							if (memcmp(atom, "hdlr", 4) == 0) {
								APar_readX(twenty_byte_buffer, file, jump+16, 4);
								parsedAtoms[atom_number-1].ancillary_data = UInt32FromBigEndian(twenty_byte_buffer);
							}

							if ((generalAtomicLevel == 1) && (dataSize == 1)) { //mdat.length =1 64-bit length that is more of a cludge.
								jump += parsedAtoms[atom_number-1].AtomicLengthExtended;
							} else {
								jump += dataSize;
							}
							break;
						}
						case DUAL_STATE_ATOM : {
							if (memcmp(atom, "meta", 4) == 0) {
								jump += 12;
								
							} else if (memcmp(atom, "dref", 4) == 0) {
								jump += 16;
								
							} else if (memcmp(atom, "iinf", 4) == 0) {
								jump += 14;
							
							} else if (memcmp(atom, "stsd", 4) == 0) {
								if (deepscan_REQ) {
									//for a tree ONLY, we go all the way, parsing everything; for any other option, we leave this atom as a monolithic entity
									jump += 16;
								} else {
									APar_Extract_stsd_codec(file, jump+16); //just get the codec used for this track
									jump += dataSize;
								}
								
							} else if (memcmp(atom, "schi", 4) == 0) {
								if (memcmp(container, "sinf", 4) == 0) { //seems for iTMS drm files, schi is a simple parent atom, and 'user' comes right after it
									jump += 8;
								} else {
									 jump += dataSize; //no idea what it would be under srpp, so just skip over it
								}
							
							} else if (memcmp(container, "stsd", 4) == 0) {
								//each one is different, so list its size manually
								//the beauty of this is that even if there is an error here or a new codec shows up, it only affects SHOWING the tree.
								//Getting a tree for display ONLY purposes is different from when setting a tag - display ONLY goes into stsd; tagging makes 'stsd' monolithic.
								//so setting metadata on unknown or improperly enumerated codecs (which might have different lengths) don't affect tagging.
								uint32_t named_atom = UInt32FromBigEndian(atom);
								switch(named_atom) {
									case 0x6D703473 : { //mp4s
										jump+= 16;
										break;
									}
									case 0x73727470 :		//srtp
									case 0x72747020 : { //'rtp '
										jump+= 24;
										break;
									}
									case 0x616C6163 :		//alac
									case 0x6D703461 :		//mp4a
									case 0x73616D72 :		//samr
									case 0x73617762 :		//sawb
									case 0x73617770 :		//sawp
									case 0x73657663 :		//sevc
									case 0x73716370 :		//sqcp
									case 0x73736D76 :		//ssmv
									case 0x64726D73 : { //drms
										jump+= 36;
										break;
									}
									case 0x74783367  : { //tx3g
										jump+= 46;
										break;
									}
									case 0x6D6A7032 :		//mjp2
									case 0x6D703476 :		//mp4v
									case 0x61766331 :		//avc1
									case 0x6A706567 :		//jpeg
									case 0x73323633 :		//s263
									case 0x64726D69 : { //drmi
										jump+= 86;
										break;
									}
									default :	{					//anything else that isn't covered here will just jump past any child atoms (avcp, text, enc*)
										jump += dataSize;
									}
								}
							}
							break;
						}
						case UNKNOWN_ATOM_TYPE : {
							short parent_atom = APar_FindParentAtom(atom_number-1, generalAtomicLevel);
							//to accommodate the retarted utility that keeps putting in 'prjp' atoms in mpeg-4 files written QTstyle
							if (parsedAtoms[parent_atom].AtomicContainerState == DUAL_STATE_ATOM) {
								jump = parsedAtoms[parent_atom].AtomicStart + parsedAtoms[parent_atom].AtomicLength;
							} else {
								jump += dataSize;
							}
							break;
						}
					} //end swtich
					
					generalAtomicLevel = APar_GetCurrentAtomDepth(jump, dataSize);
					
					if ( (jump > 8 ? jump : 8) >= (uint32_t)file_size) { //prevents jumping past EOF for the smallest of atoms
						break;
					}
					
					fseeko(file, jump, SEEK_SET); 
				}
				
			} else {
				fprintf(stderr, "\nAtomicParsley error: bad mpeg4 file (ftyp atom missing or alignment error).\n\n");
				data = NULL;
				exit(1); //return;
			}
			APar_SampleTableIterator(file);
			
			free(data);
			data=NULL;
			free(container);
			container=NULL;
			free(uuid_info.binary_uuid);
			free(uuid_info.uuid_AP_atom_name);
			fclose(file);
		}
		if (brand == 0x69736F6D) { //'isom' test for amc files & its (?always present?) uuid 0x63706764A88C11D48197009027087703
			char EZ_movie_uuid[100];
			memset(EZ_movie_uuid, 0, 100);
			memcpy(EZ_movie_uuid, "uuid=\x63\x70\x67\x64\xA8\x8C\x11\xD4\x81\x97\x00\x90\x27\x08\x77\x03", 21); //this is in an endian form, so it needs to be converted
			APar_endian_uuid_bin_str_conversion(EZ_movie_uuid+5);
			if ( APar_FindAtom(EZ_movie_uuid, false, EXTENDED_ATOM, 0, true) != NULL) {
				metadata_style = UNDEFINED_STYLE;
			}
		}
		parsedfile = true;
	}
	if (!deep_atom_scan && !parsedfile && APar_FindAtom("moov", false, SIMPLE_ATOM, 0) == NULL) {
		fprintf(stderr, "\nAtomicParsley error: bad mpeg4 file (no 'moov' atom).\n\n");
		exit(1);
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            mod time functions                                     //
///////////////////////////////////////////////////////////////////////////////////////

void APar_FlagMovieHeader() {
	if (movie_header_atom == NULL) movie_header_atom = APar_FindAtom("moov.mvhd", false, VERSIONED_ATOM, 0);
	if (movie_header_atom == NULL) return;
	if (movie_header_atom != NULL) movie_header_atom->ancillary_data = 0x666C6167;
	return;
}

void APar_FlagTrackHeader(AtomicInfo* thisAtom) {
	AtomicInfo* trak_atom = NULL;
	AtomicInfo* track_header_atom = NULL;
	short current_atom_idx = thisAtom->AtomicNumber;
	short current_level = thisAtom->AtomicLevel;

	if (thisAtom->AtomicLevel >= 3) {
		while (true) {
			short parent_atom = APar_FindParentAtom(current_atom_idx, current_level);
			current_atom_idx = parent_atom;
			current_level = parsedAtoms[parent_atom].AtomicLevel;

			if (current_level == 2) {
				if (memcmp(parsedAtoms[parent_atom].AtomicName, "trak", 4) == 0) {
					trak_atom = &parsedAtoms[parent_atom];
				}
				break;
			} else if (current_level == 1) {
				break;
			}
		}
		if (trak_atom != NULL) {
			track_header_atom = APar_FindChildAtom(trak_atom->AtomicNumber, "tkhd");
			if (track_header_atom != NULL) {
				track_header_atom->ancillary_data = 0x666C6167;
			}
		}
	}
	APar_FlagMovieHeader();
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Removal Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_EliminateAtom
  this_atom_number - the index into parsedAtoms[] of the atom to be erased
	resume_atom_number - the point in parsedAtoms[] where the tree will be picked up

    This manually removes the atoms from being used. The atom is still in parsedAtoms[] at the same location it was previously, but because parsedAtoms is used
		as a linked list & followed by NextAtomNumber, effectively, this atom (and atoms leading to resume_atom_number) are no longer considered part of the tree.
----------------------*/
void APar_EliminateAtom(short this_atom_number, int resume_atom_number) {
	if ( this_atom_number > 0 && this_atom_number < atom_number && resume_atom_number >= 0 && resume_atom_number < atom_number) {
		APar_FlagTrackHeader(&parsedAtoms[this_atom_number]);

		short preceding_atom_pos = APar_FindPrecedingAtom(this_atom_number);
		if ( APar_Eval_ChunkOffsetImpact(this_atom_number) ) {
			removed_bytes_tally+=parsedAtoms[this_atom_number].AtomicLength; //used in validation routine
		}
		parsedAtoms[preceding_atom_pos].NextAtomNumber = resume_atom_number;
		
		memset(parsedAtoms[this_atom_number].AtomicName, 0, 4); //blank out the name of the parent atom name
		parsedAtoms[this_atom_number].AtomicNumber = -1;
		parsedAtoms[this_atom_number].NextAtomNumber = -1;
	}
	return;
}

/*----------------------
APar_RemoveAtom
  atom_path - the "peri.od_d.elim.inat.ed__.atom.path" string that represents the target atom
	atom_type - the type of atom to be eliminated (packed language, extended...) of the target atom
	UD_lang - the language code for a packed language atom (ignored for non-packed language atoms)
	rDNS_domain - the reverse DNS domain (com.foo.thing) of the atom (ignored for non reverse DNS '----' atoms)

    APar_RemoveAtom tries to find the atom in the string. If it exists, then depending on its atom_type, it or its last child will get passed along for elimination.
		TODO: the last child part could use some more intelligence at some point; its relatively hardcoded.
----------------------*/
void APar_RemoveAtom(const char* atom_path, uint8_t atom_type, uint16_t UD_lang, const char* rDNS_domain) {
	
	AtomicInfo* desiredAtom = APar_FindAtom(atom_path, false, atom_type, UD_lang, (atom_type == EXTENDED_ATOM ? true : false), rDNS_domain );
	
	if (desiredAtom == NULL) return; //the atom didn't exist or wasn't found
	if (desiredAtom->AtomicNumber == 0) return; //we got the default atom, ftyp - and since that can't be removed, it must not exist (or it was missed)
	
	modified_atoms = true;
	if (atom_type != EXTENDED_ATOM) {
		if (atom_type == PACKED_LANG_ATOM || desiredAtom->AtomicClassification == UNKNOWN_ATOM) {
			APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);
			
		//reverseDNS atom
		} else if (desiredAtom->ReverseDNSname != NULL) {
			short parent_atom = APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel);
			short last_elim_atom = APar_FindLastChild_of_ParentAtom(parent_atom);
			APar_EliminateAtom( parent_atom, parsedAtoms[last_elim_atom].NextAtomNumber );
			
		} else if (memcmp(desiredAtom->AtomicName, "data", 4) == 0 && desiredAtom->AtomicLevel == 6){
			short parent_atom = APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel);
			short last_elim_atom = APar_FindLastChild_of_ParentAtom(parent_atom);
			APar_EliminateAtom( parent_atom, parsedAtoms[last_elim_atom].NextAtomNumber );
			
		} else if (desiredAtom->AtomicContainerState <= DUAL_STATE_ATOM) {
			short last_elim_atom = APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber);
			APar_EliminateAtom( desiredAtom->AtomicNumber, parsedAtoms[last_elim_atom].NextAtomNumber );
		
		} else if (UD_lang == 1) { //yrrc
			APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);
			
		} else {
			short last_elim_atom = APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber);
			APar_EliminateAtom(desiredAtom->AtomicNumber, last_elim_atom );
		}
		
	//this will only work for AtomicParsley created uuid atoms that don't have children, but since all uuid atoms are treaded as non-parent atoms... no problems
	} else if (atom_type == EXTENDED_ATOM) {
		APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);
	}
	return;
}

/*----------------------
APar_freefree
  purge_level - an integer ranging from -1 to some positive number of the level a 'free' atom must be on for it to be erased.

    Some tagging utilities (things based on libmp4v2 & faac irrespective of which tagging system used) have a dirty little secret. They way the work is to copy the
		'moov' atom - in its entirety - to the end of the file, and make the changes there. The original 'moov' file is nulled out, but the file only increases in size.
		Even if you eliminate the tag, the file grows. Only when the 'moov' atom is last do these taggers work efficiently - and they are blazingly fast, no doubt about
		that - but they are incredibly wasteful. It is possible to switch between using AP and mp4tags and build a file with dozens of megabytes wasted just be changing
		a single letter.
		This function can be used to iterate through the atoms in the file, and selectively eliminate 'free' atoms. Pass a -1 and every last 'free' atom that exists will
		be eliminated (but another will appear later as padding - to completely eliminate any resulting 'free' atoms, the environmental variable "AP_PADDING" needs to be
		set with MID_PAD to 0). Pass a 0, and all 'free' atoms preceding 'moov' or after 'mdat' (including gpac's pesky "Place Your Ad Here" free-at-the-end) will be
		removed. A value of >= 1 will eliminate 'free' atoms between those levels and level 1 (or file level).
----------------------*/
void APar_freefree(int purge_level) {
	modified_atoms = true;
	short eval_atom = 0;
	short prev_atom = 0;
	short moov_atom = 0; //a moov atom has yet to be seen
	short mdat_atom = 0; //any ol' mdat
	
	if (purge_level == -1) {
		complete_free_space_erasure = true; //prevent any in situ dynamic updating when trying to remove all free atoms. Also triggers a more efficient means of forcing padding
	}
	
	while (true) {
		prev_atom = eval_atom;
		eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
		if (eval_atom == 0) { //we've hit the last atom
			break;
		}

		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0 || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0 ) {
			//fprintf(stdout, "i am of size %u purge level %i (%u) -> %i\n", parsedAtoms[eval_atom].AtomicLength, purge_level, parsedAtoms[eval_atom].AtomicLevel, eval_atom);
			if ( purge_level == -1 || purge_level >= parsedAtoms[eval_atom].AtomicLevel || 
			     (purge_level == 0 && parsedAtoms[eval_atom].AtomicLevel == 1 && (moov_atom == 0 || mdat_atom != 0)) ) {
				short prev_atom = APar_FindPrecedingAtom(eval_atom);
				if (parsedAtoms[eval_atom].NextAtomNumber == 0) { //we've hit the last atom
					APar_EliminateAtom(eval_atom, parsedAtoms[eval_atom].NextAtomNumber);
					parsedAtoms[prev_atom].NextAtomNumber = 0;
				} else {
					APar_EliminateAtom(eval_atom, parsedAtoms[eval_atom].NextAtomNumber);
				}
				eval_atom = prev_atom; //go back to the previous atom and continue the search
			}
		}
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "moov", 4) == 0 ) {
			moov_atom = eval_atom;
		}
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0 ) {
			mdat_atom = eval_atom;
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Moving Functions                                    //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_MoveAtom
  this_atom_number - the atom that will follow the new_position atom
  new_position - the atom that the atoms at that level will precede, followed by this_atom_number (including its hierarchy of child atoms)

    first find which atoms lead to both atoms (precedingAtom flows to this_atom_number, lastStationaryAtom flows to new_position). Depending on whether there
		are children, find the last atom in the atoms that will be moving. Shunt as required; reordering occurs by following NextAtomNumber (linked list)
----------------------*/
void APar_MoveAtom(short this_atom_number, short new_position) {
	short precedingAtom = 0;
	short lastStationaryAtom = 0;
	short iter = 0;
	
	//look for the preceding atom (either directly before of the same level, or moov's last nth level child
	while (parsedAtoms[iter].NextAtomNumber != 0) {
		if (parsedAtoms[iter].NextAtomNumber == this_atom_number) {
			precedingAtom = iter;
			break;
		} else {
			if (parsedAtoms[iter].NextAtomNumber == 0) { //we found the last atom (which we end our search on)
				break;
			}
		}
		iter=parsedAtoms[iter].NextAtomNumber;
	}
	
	iter = 0;
	
	//search where to insert our new atom
	while (parsedAtoms[iter].NextAtomNumber != 0) {
		if (parsedAtoms[iter].NextAtomNumber == new_position) {
			lastStationaryAtom = iter;
			break;
		}
		iter=parsedAtoms[iter].NextAtomNumber;
		if (parsedAtoms[iter].NextAtomNumber == 0) { //we found the last atom
				lastStationaryAtom = iter;
				break;
		}

	}
	//fprintf(stdout, "%s preceded by %s, last would be %s\n", parsedAtoms[this_atom_number].AtomicName, parsedAtoms[precedingAtom].AtomicName, parsedAtoms[lastStationaryAtom].AtomicName);
	
	if (parsedAtoms[this_atom_number].AtomicContainerState <= DUAL_STATE_ATOM) {
		if (parsedAtoms[new_position].AtomicContainerState <= DUAL_STATE_ATOM) {
			short last_SwapChild = APar_FindLastChild_of_ParentAtom(this_atom_number);
			short last_WiredChild = APar_FindLastChild_of_ParentAtom(new_position);
			//fprintf(stdout, "moving %s, last child atom %s\n", parsedAtoms[this_atom_number].AtomicName, parsedAtoms[last_SwapChild].AtomicName);
			//fprintf(stdout, "wired %s, last child atom %s\n", parsedAtoms[new_position].AtomicName, parsedAtoms[last_WiredChild].AtomicName);
			//fprintf(stdout, "stationary atom %s , preceding atom %s\n", parsedAtoms[lastStationaryAtom].AtomicName, parsedAtoms[precedingAtom].AtomicName);
			
			short swap_resume = parsedAtoms[last_SwapChild].NextAtomNumber;
			short wired_resume = parsedAtoms[last_WiredChild].NextAtomNumber;
			
			parsedAtoms[precedingAtom].NextAtomNumber = swap_resume;         //shunt the main tree (over the [this_atom_number] atom to be move) to other tween atoms,
			parsedAtoms[lastStationaryAtom].NextAtomNumber = new_position;   //pick up with the 2nd to last hierarchy
			parsedAtoms[last_WiredChild].NextAtomNumber = this_atom_number;  //and route the 2nd to last hierarchy to wrap around to the this_atom_number atom
			parsedAtoms[last_SwapChild].NextAtomNumber = wired_resume;       //and continue with whatever was after the [new_position] atom
			
			
		} else {
			short last_child = APar_FindLastChild_of_ParentAtom(this_atom_number);
			parsedAtoms[lastStationaryAtom].NextAtomNumber = this_atom_number;
			parsedAtoms[precedingAtom].NextAtomNumber = parsedAtoms[last_child].NextAtomNumber;
			parsedAtoms[last_child].NextAtomNumber = new_position;
		}
		
	} else {
		parsedAtoms[lastStationaryAtom].NextAtomNumber = this_atom_number;
		parsedAtoms[precedingAtom].NextAtomNumber = parsedAtoms[this_atom_number].NextAtomNumber;
		parsedAtoms[this_atom_number].NextAtomNumber = new_position;
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Creation Functions                                  //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_InterjectNewAtom
  atom_name - the 4 character name of the atom
	cntr_state - the type of container it will be (child, parent, dual state)
	atom_class - the atom type it will be (simple, versioned, uuid, versioned with packed language)
	atom_length - the forced length of this atom (undefined beyond intrinsic length of the container type except for 'free' atoms)
	atom_verflags - the 1 byte atom version & 3 bytes atom flags for the atom if versioned
	packed_lang - the 2 byte packed language for the atom if versioned with packed language type
	atom_level - the level of this atom (1 denotes file level, anything else denotes a child of the last preceding parent or dual state atom)
	preceding_atom - the atom that precedes this newly created interjected atom

    Creates a single new atom (carrying NULLed data) inserted after preceding_atom
----------------------*/
short APar_InterjectNewAtom(char* atom_name, uint8_t cntr_state, uint8_t atom_class, uint32_t atom_length, 
														uint32_t atom_verflags, uint16_t packed_lang, uint8_t atom_level,
														short preceding_atom) {
														
	if (deep_atom_scan && !modified_atoms) {
		return 0;
	}

	AtomicInfo* new_atom = &parsedAtoms[atom_number];
	new_atom->AtomicNumber = atom_number;
	new_atom->AtomicName = (char*)malloc(sizeof(char)*6);
	memset(new_atom->AtomicName, 0, sizeof(char)*6);
	memcpy(new_atom->AtomicName, atom_name, 4);
				
	new_atom->AtomicContainerState = cntr_state;
	new_atom->AtomicClassification = atom_class;
	new_atom->AtomicVerFlags = atom_verflags;
	new_atom->AtomicLevel = atom_level;
	new_atom->AtomicLength = atom_length;
	new_atom->AtomicLanguage = packed_lang;
	
	new_atom->AtomicData = (char*)calloc(1, sizeof(char)* (atom_length > 16 ? atom_length : 16) ); //puts a hard limit on the length of strings (the spec doesn't)
	
	new_atom->ID32_TagInfo = NULL;

	new_atom->NextAtomNumber = parsedAtoms[ preceding_atom ].NextAtomNumber;
	parsedAtoms[ preceding_atom ].NextAtomNumber = atom_number;

	atom_number++;
	return new_atom->AtomicNumber;
}

/*----------------------
APar_CreateSparseAtom
	
	surrogate_atom - an skeletal template of the atom to be created; currently name, level, lang, (if uuid/extended: container & class) are copied; other
	                 stats should be filled in routines that called for their creation and know things like flags & if is to carry an data (this doesn't malloc)
	parent_atom - the stats for the parent atom (used to match through the KnownAtoms array and get things like container & class (for most atoms)
	preceding_atom - the new atom will follow this atom

    Create a single new atom (not carrying any data) copied from a template to follow preceding_atom 
----------------------*/
AtomicInfo* APar_CreateSparseAtom(AtomicInfo* surrogate_atom, AtomicInfo* parent_atom, short preceding_atom) {
	AtomicInfo* new_atom = &parsedAtoms[atom_number];
	new_atom->AtomicNumber = atom_number;
	new_atom->AtomicStart = 0;
	int known_atom = 0;
	
	new_atom->AtomicName = (char*)malloc(sizeof(char)*20);
	memset(new_atom->AtomicName, 0, sizeof(char)*20);
	size_t copy_bytes_len = (surrogate_atom->AtomicClassification == EXTENDED_ATOM ? 16 : 4);
	memcpy(new_atom->AtomicName, surrogate_atom->AtomicName, copy_bytes_len);
	new_atom->AtomicLevel = surrogate_atom->AtomicLevel;
	new_atom->AtomicLanguage = surrogate_atom->AtomicLanguage;
	
	//this is almost assuredly wrong for everything except a simple parent atom & needs to be handled properly afterwards; Note the use of 'Sparse'
	new_atom->AtomicVerFlags = 0;
	new_atom->AtomicLength = 8;

	new_atom->NextAtomNumber = parsedAtoms[ preceding_atom ].NextAtomNumber;
	parsedAtoms[ preceding_atom ].NextAtomNumber = atom_number;

	//if 'uuid' atom, copy the info directly, otherwise use KnownAtoms to get the info
	if (surrogate_atom->AtomicClassification == EXTENDED_ATOM) {
		new_atom->AtomicContainerState = CHILD_ATOM;
		new_atom->AtomicClassification = surrogate_atom->AtomicClassification;
		
		new_atom->uuid_style = UUID_AP_SHA1_NAMESPACE;
		
	} else {
		//determine the type of atom from our array of KnownAtoms; this is a worst case scenario; it should be handled properly afterwards
		//make sure the level & the atom gets integrated into NextAtomNumber before APar_MatchToKnownAtom because getting the fullpath will rely on that
		known_atom = APar_MatchToKnownAtom(surrogate_atom->AtomicName, (parent_atom == NULL ? "FILE_LEVEL" : parent_atom->AtomicName), false, NULL);
		
		new_atom->AtomicContainerState = KnownAtoms[known_atom].container_state;
		new_atom->AtomicClassification = KnownAtoms[known_atom].box_type;
	}
	new_atom->ID32_TagInfo = NULL;

	atom_number++;
	
	return new_atom;
}

/*----------------------
APar_Unified_atom_Put
  target_atom - pointer to the structure describing the atom we are setting
  unicode_data - a pointer to a string (possibly utf8 already); may go onto conversion to utf16 prior to the put
  text_tag_style - flag to denote that unicode_data is to become utf-16, or stay the flavors of utf8 (iTunes style, 3gp style...)
  ancillary_data - a (possibly cast) 32-bit number of any type of supplemental data to be set
	anc_bit_width - controls the number of bytes to set for ancillary data [0 to skip, 8 (1byte) - 32 (4 bytes)]

    take any variety of data & tack it onto the malloced AtomicData at the next available spot (determined by its length)
    priority is given to the numerical ancillary_data so that language can be set prior to setting whatever unicode data. Finally, advance
	  the length of the atom so that we can tack onto the end repeated times (up to the max malloced amount - which isn't checked [blush])
		if unicode_data is NULL itself, then only ancillary_data will be set - which is endian safe cuz o' bitshifting (or set 1 byte at a time)
		
		works on iTunes-style & 3GP asset style but NOT binary safe (use APar_atom_Binary_Put)
		TODO: work past the max malloced amount onto a new larger array
----------------------*/
void APar_Unified_atom_Put(AtomicInfo* target_atom, const char* unicode_data, uint8_t text_tag_style, uint32_t ancillary_data, uint8_t anc_bit_width) {
	uint32_t atom_data_pos = 0;
	if (target_atom == NULL) {
		return;
	}
	if (target_atom->AtomicClassification == EXTENDED_ATOM) {
		if (target_atom->uuid_style == UUID_SHA1_NAMESPACE) atom_data_pos = target_atom->AtomicLength - 32;
		else if (target_atom->uuid_style == UUID_OTHER) atom_data_pos = target_atom->AtomicLength - 24;
	} else {
		atom_data_pos = target_atom->AtomicLength - 12;
	}
	
	switch (anc_bit_width) {
		case 0 : { //aye, 'twas a false alarm; arg (I'm a pirate), we just wanted to set a text string
			break;
		}
		
		case 8 : { //compilation, podcast flag, advisory
			target_atom->AtomicData[atom_data_pos] = (uint8_t)ancillary_data;
			target_atom->AtomicLength++;
			atom_data_pos++;
			break;
		}
		
		case 16 : { //lang & its ilk
			target_atom->AtomicData[atom_data_pos] = (ancillary_data & 0xff00) >> 8;
			target_atom->AtomicData[atom_data_pos + 1] = (ancillary_data & 0xff) << 0;
			target_atom->AtomicLength+= 2;
			atom_data_pos+=2;
			break;
		}
		
		case 32 : { //things like coordinates and.... stuff (ah, the prose)
			target_atom->AtomicData[atom_data_pos] = (ancillary_data & 0xff000000) >> 24;
			target_atom->AtomicData[atom_data_pos + 1] = (ancillary_data & 0xff0000) >> 16;
			target_atom->AtomicData[atom_data_pos + 2] = (ancillary_data & 0xff00) >> 8;
			target_atom->AtomicData[atom_data_pos + 3] = (ancillary_data & 0xff) << 0;
			target_atom->AtomicLength+= 4;
			atom_data_pos+=4;
			break;
		}
			
		default : {
			break;
		}		
	}

	if (unicode_data != NULL) {
		if (text_tag_style == UTF16_3GP_Style) {
			uint32_t string_length = strlen(unicode_data) + 1;
			uint32_t glyphs_req_bytes = mbstowcs(NULL, unicode_data, string_length) * 2; //passing NULL pre-calculates the size of wchar_t needed;
			
			unsigned char* utf16_conversion = (unsigned char*)calloc(1, sizeof(unsigned char)* string_length * 2 );
			
			UTF8ToUTF16BE(utf16_conversion, glyphs_req_bytes, (unsigned char*)unicode_data, string_length);
			
			target_atom->AtomicData[atom_data_pos] = 0xFE; //BOM
			target_atom->AtomicData[atom_data_pos+1] = 0xFF; //BOM
			atom_data_pos +=2; //BOM
			
			/* copy the string directly onto AtomicData at the address of the start of AtomicData + the current length in atom_data_pos */
			/* in marked contrast to iTunes-style metadata where a string is a single string, 3gp tags like keyword & classification are more complex */
			/* directly putting the text into memory and being able to tack on more becomes a necessary accommodation */
			memcpy(target_atom->AtomicData + atom_data_pos, utf16_conversion, glyphs_req_bytes );
			target_atom->AtomicLength += glyphs_req_bytes;
			
			//double check terminating NULL (don't want to double add them - blush.... or have them missing - blushing on the.... other side)
			if (target_atom->AtomicData[atom_data_pos + (glyphs_req_bytes -1)] + target_atom->AtomicData[atom_data_pos + glyphs_req_bytes] != 0) {
				target_atom->AtomicLength += 4; //+4 because add 2 bytes for the character we just found + 2bytes for the req. NULL
			}
			free(utf16_conversion); utf16_conversion = NULL;
		
		} else if (text_tag_style == UTF8_iTunesStyle_Binary) { //because this will be 'binary' data (a misnomer for purl & egid), memcpy 4 bytes into AtomicData, not at the start of it
			uint32_t binary_bytes = strlen(unicode_data);
			memcpy(target_atom->AtomicData + atom_data_pos, unicode_data, binary_bytes + 1 );
			target_atom->AtomicLength += binary_bytes;
		
		} else {
			uint32_t total_bytes = 0;
			
			if (text_tag_style == UTF8_3GP_Style) {
				total_bytes = strlen(unicode_data);
				total_bytes++;  //include the terminating NULL
				
			} else if (text_tag_style == UTF8_iTunesStyle_256glyphLimited) {

				uint32_t raw_bytes = strlen(unicode_data);
				total_bytes = utf8_length(unicode_data, 255); //counts the number of characters, not bytes
				
				if (raw_bytes > total_bytes && total_bytes > 255) {
					
					fprintf(stdout, "AtomicParsley warning: %s was trimmed to 255 characters (%u characters over)\n", 
					        parsedAtoms[ APar_FindParentAtom(target_atom->AtomicNumber, target_atom->AtomicLevel) ].AtomicName,
									                                 utf8_length(unicode_data+total_bytes, 0) );
				} else {
					total_bytes = raw_bytes;
				}
						
			} else if (text_tag_style == UTF8_iTunesStyle_Unlimited) {
				total_bytes = strlen(unicode_data);
				
				if (total_bytes > MAXDATA_PAYLOAD) {
					free(target_atom->AtomicData);
					target_atom->AtomicData = NULL;
					
					target_atom->AtomicData = (char*)malloc( sizeof(char)* (total_bytes +1) );
					memset(target_atom->AtomicData + atom_data_pos, 0, total_bytes +1);
					
				}
			}
			
			//if we are setting iTunes-style metadata, add 0 to the pointer; for 3gp user data atoms - add in the (length-default bare atom lenth): account for language uint16_t (plus any other crap we will set); unicodeWin32 with wchar_t was converted right after program started, so do a direct copy

			memcpy(target_atom->AtomicData + atom_data_pos, unicode_data, total_bytes + 1 );
			target_atom->AtomicLength += total_bytes;
		}	
	}	
	return;
}

/*----------------------
APar_atom_Binary_Put
  target_atom - pointer to the structure describing the atom we are setting
  binary_data - a pointer to a string of binary data
  bytecount - number of bytes to copy
  atomic_data_offset - place binary data some bytes offset from the start of AtomicData

    Simple placement of binary data (perhaps containing NULLs) onto AtomicData.
		TODO: if over MAXDATA_PAYLOAD malloc a new char string
----------------------*/
void APar_atom_Binary_Put(AtomicInfo* target_atom, const char* binary_data, uint32_t bytecount, uint32_t atomic_data_offset) {
	if (target_atom == NULL) return;
	
	if (atomic_data_offset + bytecount + target_atom->AtomicLength <= MAXDATA_PAYLOAD) {
		memcpy(target_atom->AtomicData + atomic_data_offset, binary_data, bytecount );
		target_atom->AtomicLength += bytecount;
	} else {
		fprintf(stdout, "AtomicParsley warning: some data was longer than the allotted space and was skipped\n");
	}
	return;
}

/*----------------------
APar_Verify__udta_meta_hdlr__atom

    only test if the atom is present for now, it will be created just before writeout time - to insure it only happens once.
----------------------*/
void APar_Verify__udta_meta_hdlr__atom() {
	bool Create__udta_meta_hdlr__atom = false;
	
	if (metadata_style == ITUNES_STYLE && hdlrAtom == NULL) {
		hdlrAtom = APar_FindAtom("moov.udta.meta.hdlr", false, VERSIONED_ATOM, 0);
		if (hdlrAtom == NULL) {
			Create__udta_meta_hdlr__atom = true;
		}
	}
	if (Create__udta_meta_hdlr__atom ) {
		
		//if Quicktime (Player at the least) is used to create any type of mp4 file, the entire udta hierarchy is missing. If iTunes doesn't find
		//this "moov.udta.meta.hdlr" atom (and its data), it refuses to let any information be changed & the dreaded "Album Artwork Not Modifiable"
		//shows up. It's because this atom is missing. Oddly, QT Player can see the info, but this only works for mp4/m4a files.
		
		hdlrAtom = APar_FindAtom("moov.udta.meta.hdlr", true, VERSIONED_ATOM, 0);
		
		APar_MetaData_atom_QuickInit(hdlrAtom->AtomicNumber, 0, 0);
		APar_Unified_atom_Put(hdlrAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0x6D646972, 32); //'mdir'
		APar_Unified_atom_Put(hdlrAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0x6170706C, 32); //'appl'
		APar_Unified_atom_Put(hdlrAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
		APar_Unified_atom_Put(hdlrAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
		APar_Unified_atom_Put(hdlrAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
	}
	return;
}

/*----------------------
APar_MetaData_atomGenre_Set
	atomPayload - the desired string value of the genre

    genre is special in that it gets carried on 2 atoms. A standard genre (as listed in ID3v1GenreList) is represented as a number on a 'gnre' atom
		any value other than those, and the genre is placed as a string onto a '©gen' atom. Only one or the other can be present. So if atomPayload is a
		non-NULL value, first try and match the genre into the ID3v1GenreList standard genres. Try to remove the other type of genre atom, then find or
		create the new genre atom and put the data manually onto the atom.
----------------------*/
void APar_MetaData_atomGenre_Set(const char* atomPayload) {
	if (metadata_style == ITUNES_STYLE) {
		const char* standard_genre_atom = "moov.udta.meta.ilst.gnre";
		const char* std_genre_data_atom = "moov.udta.meta.ilst.gnre.data";
		const char* custom_genre_atom = "moov.udta.meta.ilst.©gen";
		const char* cstm_genre_data_atom = "moov.udta.meta.ilst.©gen.data";
		
		if ( strlen(atomPayload) == 0) {
			APar_RemoveAtom(std_genre_data_atom, VERSIONED_ATOM, 0); //find the atom; don't create if it's "" to remove
			APar_RemoveAtom(cstm_genre_data_atom, VERSIONED_ATOM, 0); //find the atom; don't create if it's "" to remove
		} else {
		
			uint8_t genre_number = StringGenreToInt(atomPayload);
			AtomicInfo* genreAtom;
			
			APar_Verify__udta_meta_hdlr__atom();
			modified_atoms = true;
			
			if (genre_number != 0) {
				//first find if a custom genre atom ("©gen") exists; erase the custom-string genre atom in favor of the standard genre atom
				
				AtomicInfo* verboten_genre_atom = APar_FindAtom(custom_genre_atom, false, SIMPLE_ATOM, 0);
				
				if (verboten_genre_atom != NULL) {
					if (strlen(verboten_genre_atom->AtomicName) > 0) {
						if (strncmp(verboten_genre_atom->AtomicName, "©gen", 4) == 0) {
							APar_RemoveAtom(cstm_genre_data_atom, VERSIONED_ATOM, 0);
						}
					}
				}
				
				genreAtom = APar_FindAtom(std_genre_data_atom, true, VERSIONED_ATOM, 0);
				APar_MetaData_atom_QuickInit(genreAtom->AtomicNumber, AtomFlags_Data_Binary, 0);
				APar_Unified_atom_Put(genreAtom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 8);
				APar_Unified_atom_Put(genreAtom, NULL, UTF8_iTunesStyle_256glyphLimited, (uint32_t)genre_number, 8);

			} else {
				
				AtomicInfo* verboten_genre_atom = APar_FindAtom(standard_genre_atom, false, SIMPLE_ATOM, 0);
				
				if (verboten_genre_atom != NULL) {
					if (verboten_genre_atom->AtomicNumber > 5 && verboten_genre_atom->AtomicNumber < atom_number) {
						if (strncmp(verboten_genre_atom->AtomicName, "gnre", 4) == 0) {
							APar_RemoveAtom(std_genre_data_atom, VERSIONED_ATOM, 0);
						}		
					}
				}
				genreAtom = APar_FindAtom(cstm_genre_data_atom, true, VERSIONED_ATOM, 0);
				APar_MetaData_atom_QuickInit(genreAtom->AtomicNumber, AtomFlags_Data_Text, 0);
				APar_Unified_atom_Put(genreAtom, atomPayload, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			}
		}
		APar_FlagMovieHeader();
	} //end if (metadata_style == ITUNES_STYLE)
	return;
}

/*----------------------
APar_MetaData_atomArtwork_Init
	atom_num - the AtomicNumber of the atom in the parsedAtoms array (probably newly created)
	artworkPath - the path that was provided on a (hopefully) existant jpg/png file

    artwork will be inited differently because we need to test a) that the file exists and b) get its size in bytes. This info will be used at the size
		of the 'data' atom under 'covr' - and the path will be carried on AtomicData until write-out time, when the binary contents of the original will be
		copied onto the atom.
----------------------*/
void APar_MetaData_atomArtwork_Init(short atom_num, const char* artworkPath) {
	TestFileExistence(artworkPath, false);
	off_t picture_size = findFileSize(artworkPath);
	
	if (picture_size > 0) {
		APar_MetaData_atom_QuickInit(atom_num, APar_TestArtworkBinaryData(artworkPath), 0, (uint32_t)picture_size );
		FILE* artfile = APar_OpenFile(artworkPath, "rb");
		uint32_t bytes_read = APar_ReadFile(parsedAtoms[atom_num].AtomicData + 4, artfile, (uint32_t)picture_size); //+4 for the 4 null bytes
		if (bytes_read > 0) parsedAtoms[atom_num].AtomicLength += bytes_read;
		fclose(artfile);
	}	
	return;
}

/*----------------------
APar_MetaData_atomArtwork_Set
	artworkPath - the path that was provided on a (hopefully) existant jpg/png file
	env_PicOptions - picture embedding preferences from a 'export PIC_OPTIONS=foo' setting

    artwork gets stored under a single 'covr' atom, but with many 'data' atoms - each 'data' atom contains the binary data for each picture.
		When the 'covr' atom is found, we create a sparse atom at the end of the existing 'data' atoms, and then perform any of the image manipulation
		features on the image. The path of the file (either original, modified artwork, or both) are returned to use for possible atom creation
----------------------*/
void APar_MetaData_atomArtwork_Set(const char* artworkPath, char* env_PicOptions) {
	if (metadata_style == ITUNES_STYLE) {
		const char* artwork_atom = "moov.udta.meta.ilst.covr";
		if (memcmp(artworkPath, "REMOVE_ALL", 10) == 0) {
			APar_RemoveAtom(artwork_atom, SIMPLE_ATOM, 0);
			
		} else {
			APar_Verify__udta_meta_hdlr__atom();
			
			modified_atoms = true;
			AtomicInfo* desiredAtom = APar_FindAtom(artwork_atom, true, SIMPLE_ATOM, 0);
			AtomicInfo sample_data_atom = { 0 };
			short parent_atom = desiredAtom->AtomicNumber; //used on Darwin adding a 2nd image (the original)
			APar_CreateSurrogateAtom(&sample_data_atom, "data", 6, VERSIONED_ATOM, 0, NULL, 0);
			desiredAtom = APar_CreateSparseAtom(&sample_data_atom, desiredAtom, APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber) );

	#if defined (DARWIN_PLATFORM)			
			//determine if any picture preferences will impact the picture file in any way
			myPicturePrefs = APar_ExtractPicPrefs(env_PicOptions);

			char* resized_filepath = (char*)calloc(1, sizeof(char)*MAXPATHLEN+1);

			if ( ResizeGivenImage(artworkPath , myPicturePrefs, resized_filepath) ) {
				APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, resized_filepath);
			
				if (myPicturePrefs.addBOTHpix) {
					//create another sparse atom to hold the new image data
					desiredAtom = APar_CreateSparseAtom(&sample_data_atom, desiredAtom, APar_FindLastChild_of_ParentAtom(parent_atom) );
					APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
					if (myPicturePrefs.removeTempPix) remove(resized_filepath);
				}
			} else {
				APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
			}
			free(resized_filepath);
			resized_filepath=NULL;
	#else
			//perhaps some libjpeg based resizing/modification for non-Mac OS X based platforms
			APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
	#endif
		}
		APar_FlagMovieHeader();
	} ////end if (metadata_style == ITUNES_STYLE)
	return;
}

/*----------------------
APar_sprintf_atompath
	dest_path - the destination string that will hold the final path
	target_atom_name - the name of the atom that will be used to fill the %s portion of the tokenized path
	track_index - the index into the trak[X] that needs to be found; fills the %u portion of the tokenized path
	udta_container - the area in which 'udta' will be: either movie level (the easiest) or track level (which requires an index into a specific 'trak' atom)
	dest_len - the amount of malloc'ed bytes for dest_path

    Fill a destinaiton path with the fully expressed atom path taking track indexes into consideration
----------------------*/
void APar_sprintf_atompath(char* dest_path, char* target_atom_name, uint8_t track_index, uint8_t udta_container, uint16_t dest_len) {
	memset(dest_path, 0, dest_len);
	if (udta_container == MOVIE_LEVEL_ATOM) {
		memcpy(dest_path, "moov.udta.", 10);
		memcpy(dest_path +10, target_atom_name, 4);
	} else {
		sprintf(dest_path, "moov.trak[%u].udta.%s", track_index, target_atom_name);
	}
	return;
}

/*----------------------
APar_3GP_Keyword_atom_Format
	keywords_globbed - the globbed string of keywords ('foo1,foo2,foo_you')
	keyword_count - count of keywords in the above globbed string
	set_UTF16_text - whether to encode as utf16
	formed_keyword_struct - the char string that will hold the converted keyword struct (manually formatted)

    3gp keywords are a little more complicated. Since they will be entered separated by some form of punctuation, they need to be separated out
		They also will possibly be converted to utf16 - and they NEED to start with the BOM then. Prefacing each keyword is the 8bit length of the string
		And each keyword needs to be NULL terminated. Technically it would be possible to even have mixed encodings (not supported here).
----------------------*/
uint32_t APar_3GP_Keyword_atom_Format(char* keywords_globbed, uint8_t keyword_count, bool set_UTF16_text, char* &formed_keyword_struct) {
	uint32_t formed_string_offset = 0;
	uint32_t string_len = 0;
	
	char* a_keyword = strsep(&keywords_globbed,",");
	
	for (uint8_t i=1; i <= keyword_count; i++) {
		string_len = strlen(a_keyword);
		if (set_UTF16_text) {

			uint32_t glyphs_req_bytes = mbstowcs(NULL, a_keyword, string_len+1) * 2; //passing NULL pre-calculates the size of wchar_t needed;
			
			formed_keyword_struct[formed_string_offset+1] = 0xFE; //BOM
			formed_keyword_struct[formed_string_offset+2] = 0xFF; //BOM
			formed_string_offset+=3; //BOM + keyword length that has yet to be set
			
			int bytes_converted = UTF8ToUTF16BE((unsigned char*)(formed_keyword_struct + formed_string_offset), glyphs_req_bytes, (unsigned char*)a_keyword, string_len);
			
			if (bytes_converted > 1) {
				formed_keyword_struct[formed_string_offset-3] = (uint8_t)bytes_converted + 4; //keyword length is NOW set
				formed_string_offset += bytes_converted + 2; //NULL terminator
			}						
		} else {

			uint32_t string_len = strlen(a_keyword);
			formed_keyword_struct[formed_string_offset] = (uint8_t)string_len + 1; //add the terminating NULL
			formed_string_offset++;
			memcpy(formed_keyword_struct + formed_string_offset, a_keyword, string_len );
			formed_string_offset+= (string_len +1);
		}
		a_keyword = strsep(&keywords_globbed,",");
	}
	return formed_string_offset;
}

/*----------------------
APar_uuid_atom_Init
	atom_path - the parent hierarchy of the desired atom (with the location of the specific uuid atom supplied as '=%s')
	uuidName - the name of the atom (possibly provided in a forbidden utf8 - only latin1 aka iso8859 is acceptable)
	dataType - for now text is only supported; really its atom version/flags as used by iTunes
	uuidValue - the string that will get embedded onto the atom
	shellAtom - flag to denote whether the atom may possibly come as utf8 encoded

    uuid atoms are user-supplied/user-defined atoms that allow for extended tagging support. Because a uuid atom is malleable, and defined by the utility
		that created it, any information carried by a uuid is arbitrary, and cannot be guaranteed by a non-originating utility. In AtomicParsley uuid atoms,
		the data is presented much like an iTunes-style atom - except that the information gets carried directly on the uuid atom - no 'data' child atom
		exists. A uuid atom is a special longer type of traditional atom. As a traditional atom, it name is 'uuid' - and the 4 bytes after that represent
		its uuid name. Because the data will be directly sitting on the atom, a different means of finding these atoms exists, as well as creating the
		acutal uuidpath itself. Once created however, placing information on it is very much like any other atom - done via APar_Unified_atom_Put
		
		//4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
----------------------*/
AtomicInfo* APar_uuid_atom_Init(const char* atom_path, char* uuidName, const uint32_t dataType, const char* uuidValue, bool shellAtom) {
	AtomicInfo* desiredAtom = NULL;
	char uuid_path[256];
	char uuid_binary_str[20];
	char uuid_4char_name[10];
	memset(uuid_path, 0, 20);
	memset(uuid_binary_str, 0, 20);
	memset(uuid_4char_name, 0, 10);
	uint16_t path_len = 0;
	
	if (shellAtom) {
		UTF8Toisolat1((unsigned char*)&uuid_4char_name, 4, (unsigned char*)uuidName, strlen(uuidName) );
	} else {
		memcpy(uuid_4char_name, uuidName, 4);
	}

	APar_generate_uuid_from_atomname(uuid_4char_name, uuid_binary_str);
	APar_endian_uuid_bin_str_conversion(uuid_binary_str);	
	
	//this will only append (and knock off) %s (anything) at the end of a string
	path_len = strlen(atom_path);
	memcpy(uuid_path, atom_path, path_len-2);
	memcpy(uuid_path + (path_len-2), uuid_binary_str, 16);
	
#if defined(DEBUG_V)
	fprintf(stdout, "debug: APar_uuid_atom_Init desired atom '%s' converted to uuidv5: ", uuidName);
	APar_print_uuid( (ap_uuid_t*)(uuid_path + (path_len-2)) );
#endif	

	if ( uuidValue == NULL || strlen(uuidValue) == 0) {
		APar_RemoveAtom(uuid_path, EXTENDED_ATOM, 0); //find the atom; don't create if it's "" to remove
		APar_FlagMovieHeader();
		return NULL;
		
	} else {
		if ( !(dataType == AtomFlags_Data_Text || dataType == AtomFlags_Data_uuid_binary) ) { //the only supported types
			fprintf(stdout, "AP warning: only text or file types are allowed on uuid atom %s (%u-%u); skipping\n", uuidName, dataType, AtomFlags_Data_Text);
			return NULL;
		}
		//uuid atoms won't have 'data' child atoms - they will carry the data directly as opposed to traditional iTunes-style metadata that does store the information on 'data' atoms. But user-defined is user-defined, so that is how it will be defined here.
		modified_atoms = true;

		desiredAtom = APar_FindAtom(uuid_path, true, EXTENDED_ATOM, 0, true);
		desiredAtom->uuid_ap_atomname = (char*)calloc(1, sizeof(char)*10);       //only useful to print out the atom tree midway through an operation
		memcpy(desiredAtom->uuid_ap_atomname, uuid_4char_name, 4);               //only useful to print out the atom tree midway through an operation
		
		if (dataType == AtomFlags_Data_Text) APar_MetaData_atom_QuickInit(desiredAtom->AtomicNumber, dataType, 20); //+20 including the 4 NULL bytes preceding any string we set
		//NOTE: setting a file into a uuid atom  (dataType == AtomFlags_Data_uuid_binary) is handled in main.cpp - the length of the file extension, description and file
		//all add up to the amount to malloc AtomicData to, so handle that separately.
		
		parsedAtoms[desiredAtom->AtomicNumber].AtomicClassification = EXTENDED_ATOM;
		APar_FlagMovieHeader();
	}
	return desiredAtom;
}

/*----------------------
APar_MetaData_atom_QuickInit
	atom_num - the position in the parsedAtoms array (either found in the file or a newly created sparse atom) so AtomicData can be initialized
	atomFlags - the AtomicVerFlags for the iTunes-style metadata atom
	supplemental_length - iTunes-style metadata for 'data' atoms is >= 16bytes long; AtomicParsley created uuid atoms will be +4bytes directly on that atom
	allotment - the bytes of AtomicData to malloc (defaults to MAXDATA_PAYLOAD + 1 (+50) unless changed - like uuids from file)

    Metadata_QuickInit will initialize a pre-found atom to MAXDATA_PAYLOAD so that it can carry info on AtomicData
----------------------*/
void APar_MetaData_atom_QuickInit(short atom_num, const uint32_t atomFlags, uint32_t supplemental_length, uint32_t allotment) {
	//this will skip the finding of atoms and just malloc the AtomicData; used by genre & artwork

	parsedAtoms[atom_num].AtomicData = (char*)calloc(1, sizeof(char)* allotment+50 );
	if (parsedAtoms[atom_num].AtomicData == NULL) {
		fprintf(stdout, "AP error: there was insufficient memory available for allocation. Exiting.%c\n", '\a');
		exit(1);
	}
	
	parsedAtoms[atom_num].AtomicLength = 16 + supplemental_length; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags, 4 bytes NULL
	parsedAtoms[atom_num].AtomicVerFlags = atomFlags;
	parsedAtoms[atom_num].AtomicContainerState = CHILD_ATOM;
	parsedAtoms[atom_num].AtomicClassification = VERSIONED_ATOM;
	
	return;
}

/*----------------------
APar_MetaData_atom_Init
	atom_path - the hierarchical path to the specific atom carrying iTunes-style metadata that will be found (and created if necessary)
	MD_Payload - the information to be carried (also used as a test if NULL to remove the atom)
	atomFlags - the AtomicVerFlags for the atom (text, integer or unsigned integer)

    Metadata_Init will search for and create the necessary hierarchy so that the atom can be initialized to carry the payload data on AtomicData.
		This will provide a shell of an atom with 4bytes length, 4bytes name, 4bytes version/flags, 4bytes NULL + any other data
----------------------*/
AtomicInfo* APar_MetaData_atom_Init(const char* atom_path, const char* MD_Payload, const uint32_t atomFlags) {
	//this will handle the vanilla iTunes-style metadata atoms; genre will be handled elsewehere because it gets carried on 2 different atoms, and artwork gets special treatment because it can have multiple child data atoms
	if (metadata_style != ITUNES_STYLE) return NULL;
	bool retain_atom = true;
		
	if ( strlen(MD_Payload) == 0 ) {
		retain_atom = false;
	}
	
	if (retain_atom) {
		APar_Verify__udta_meta_hdlr__atom();
	}
	
	AtomicInfo* desiredAtom = APar_FindAtom(atom_path, retain_atom, VERSIONED_ATOM, 0); //finds the atom; if not present, creates the atom
	if (desiredAtom == NULL) return NULL;
	modified_atoms = true;
		
	if (! retain_atom) {
		AtomicInfo* parent_atom = &parsedAtoms[ APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel) ];
		if (desiredAtom->AtomicNumber > 0 && parent_atom->AtomicNumber > 0) {
			APar_EliminateAtom(parent_atom->AtomicNumber, desiredAtom->NextAtomNumber);
			APar_FlagMovieHeader();
			return NULL;
		}
			
	} else {
		parsedAtoms[desiredAtom->AtomicNumber].AtomicData = (char*)malloc(sizeof(char)* MAXDATA_PAYLOAD + 1 ); //puts a hard limit on the length of strings (the spec doesn't)
		memset(parsedAtoms[desiredAtom->AtomicNumber].AtomicData, 0, sizeof(char)* MAXDATA_PAYLOAD + 1 );
		
		parsedAtoms[desiredAtom->AtomicNumber].AtomicLength = 16; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags, 4 bytes NULL
		parsedAtoms[desiredAtom->AtomicNumber].AtomicVerFlags = atomFlags;
		parsedAtoms[desiredAtom->AtomicNumber].AtomicContainerState = CHILD_ATOM;
		parsedAtoms[desiredAtom->AtomicNumber].AtomicClassification = VERSIONED_ATOM;
		APar_FlagMovieHeader();
	}
	return desiredAtom;
}

/*----------------------
APar_UserData_atom_Init
	userdata_atom_name - the name of the atom to be set ('titl', 'loci', 'cprt')
	atom_payload - the information to be carried (also used as a test if NULL to remove the atom)
	udta_container - determines whether to create an atom path at movie level or track level
	track_idx - provide the track for the target atom if at track level
	userdata_lang - the language for the tag (multiple tags with the same name, but different languages are supported for some of these atoms)

    UserData_Init can be called multiple times from a single cli invocation (if used to target at atom at track level for all tracks). Since it can be called
		repeatedly, the atom path is created here based on the container for 'udta': 'moov' for movie level or 'trak' at track level. If there is no payload that
		atom path is found & if found to exists is removed. If the payload does contain something, the created atom path is created, initialized & the language
		setting is set (for internal AP use) for that atom.
		
			NOTE: the language setting (if supported - yrrc doesn't) occurs in different places in 3GP atoms. Most occur right after atom flags/versioning - but
			in rtng/clsf they occur later. The language is instead stored in binary form amid the data for the atom, but is also put into the parsedAtoms[] array
			(that information is only used in finding atoms not the actual writing of atoms out). Both (storing the language in AtomicData in binary form & put in
			the parsedAtoms[] AtomicInfo array) forms are required to implement multiple language support for 3gp atoms.
			
			NOTE2: Perhaps there is something wrong with Apple's implementation of 3gp metadata, or I'm loosing my mind. The exact same utf8 string that shows up in a
			3gp file as ??? - ??? shows up *perfect* in an mp4 or mov container. Encoded as utf16 same problem a sample string using Polish glyphs in utf8 has some
			gylphs missing with lang=eng. The same string with 'lang=pol', and different glyphs are missing. The problem occurs using unicode.org's ConvertUTF8toUTF16
			or using libxmls's UTF8ToUTF16BE (when converting to utf16) in the same places - except for the copyright symbol which unicode.org's ConvertUTF16toUTF8 didn't
			properly convert - which was the reason why libxml's functions are now used. And at no point can I get the audio protected P-in-a-circle glyph to show up in
			utf8 or utf16. To summarize, either I am completely overlooking some interplay (of lang somehow changing the utf8 or utf16 standard), the unicode translations
			are off (which in the case of utf8 is embedded directly on Mac OS X, so that can't be it), or Apple's 3gp implementation is off.
			
			TODO NOTE: the track modification date should change if set at track level because of this
----------------------*/
AtomicInfo* APar_UserData_atom_Init(char* userdata_atom_name, const char* atom_payload, uint8_t udta_container, uint8_t track_idx, uint16_t userdata_lang) {
	uint8_t atom_type = PACKED_LANG_ATOM;
	uint8_t total_tracks = 0;
	uint8_t a_track = 0;//unused
	AtomicInfo* desiredAtom = NULL;
	char* userdata_atom_path = NULL;
	
	if (userdata_lang == 0)  atom_type = VERSIONED_ATOM;
		
	APar_FindAtomInTrack(total_tracks, a_track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.
	
	if (track_idx > total_tracks || (track_idx == 0 && udta_container == SINGLE_TRACK_ATOM) ) {
		APar_assert(false, 5, userdata_atom_name);
		return NULL;
	}
	
	userdata_atom_path = (char*)malloc(sizeof(char)* 400 );
	
	if (udta_container == MOVIE_LEVEL_ATOM) {
		APar_sprintf_atompath(userdata_atom_path, userdata_atom_name, 0xFF, MOVIE_LEVEL_ATOM, 400);
	} else if (udta_container == ALL_TRACKS_ATOM) {
		APar_sprintf_atompath(userdata_atom_path, userdata_atom_name, track_idx, ALL_TRACKS_ATOM, 400);
	} else {
		APar_sprintf_atompath(userdata_atom_path, userdata_atom_name, track_idx, SINGLE_TRACK_ATOM, 400);
	}
	
	if ( strlen(atom_payload) == 0) {
		
		APar_RemoveAtom(userdata_atom_path, atom_type, atom_type == VERSIONED_ATOM ? 1 : userdata_lang); //find the atom; don't create if it's "" to remove
		free(userdata_atom_path); userdata_atom_path = NULL;
		return NULL;
	} else {
		modified_atoms = true;
		
		desiredAtom = APar_FindAtom(userdata_atom_path, true, atom_type, userdata_lang);
	
		desiredAtom->AtomicData = (char*)calloc(1, sizeof(char)* MAXDATA_PAYLOAD ); //puts a hard limit on the length of strings (the spec doesn't)
	
		desiredAtom->AtomicLength = 12; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags (NULLs)
		desiredAtom->AtomicVerFlags = 0;
		desiredAtom->AtomicContainerState = CHILD_ATOM;
		desiredAtom->AtomicClassification = atom_type;
		desiredAtom->AtomicLanguage = userdata_lang;
		APar_FlagTrackHeader(desiredAtom);
	}
	free(userdata_atom_path); userdata_atom_path = NULL;
	return desiredAtom;
}

/*----------------------
APar_reverseDNS_atom_Init
	rDNS_atom_name - the name of the descriptor for the reverseDNS atom form (like iTunNORM)
	rDNS_payload - the information to be carried (also used as a test if NULL to remove the atom)
	atomFlags - text, integer, binary flag of the data payload
	rDNS_domain - the reverse domain itself (like net.sourceforge.atomicparsley or com.apple.iTunes)

    FILL IN
----------------------*/
AtomicInfo* APar_reverseDNS_atom_Init(const char* rDNS_atom_name, const char* rDNS_payload, const uint32_t* atomFlags, const char* rDNS_domain) {
	AtomicInfo* desiredAtom = NULL;
	char* reverseDNS_atompath = (char*)calloc(1, sizeof(char)*2001);
	
	if (metadata_style != ITUNES_STYLE) {
		free(reverseDNS_atompath); reverseDNS_atompath = NULL;
		return NULL;
	}
	
	sprintf(reverseDNS_atompath, "moov.udta.meta.ilst.----.name:[%s]", rDNS_atom_name); //moov.udta.meta.ilst.----.name:[iTunNORM]
		
	if ( rDNS_payload != NULL ) {
		if (strlen(rDNS_payload) == 0) {
			APar_RemoveAtom(reverseDNS_atompath, VERSIONED_ATOM, 0, rDNS_domain);
			free(reverseDNS_atompath); reverseDNS_atompath = NULL;
			return NULL;		
		}
		APar_Verify__udta_meta_hdlr__atom();
	} else {
		APar_RemoveAtom(reverseDNS_atompath, VERSIONED_ATOM, 0, rDNS_domain);
		APar_FlagMovieHeader();
		free(reverseDNS_atompath); reverseDNS_atompath = NULL;
		return NULL;
	}

	desiredAtom = APar_FindAtom(reverseDNS_atompath, false, VERSIONED_ATOM, 0, false, rDNS_domain); //finds the atom; do NOT create it if not found - manually create the hierarchy
	
	if (desiredAtom == NULL) {
		AtomicInfo* ilst_atom = APar_FindAtom("moov.udta.meta.ilst", true, SIMPLE_ATOM, 0);
		short last_iTunes_list_descriptor = APar_FindLastChild_of_ParentAtom(ilst_atom->AtomicNumber); //the *last* atom contained by ilst - even if its the 4th 'data' atom

		short rDNS_four_dash_parent = APar_InterjectNewAtom("----", PARENT_ATOM, SIMPLE_ATOM, 8, 0, 0, ilst_atom->AtomicLevel+1, last_iTunes_list_descriptor);
		
		short rDNS_mean_atom = APar_InterjectNewAtom("mean", CHILD_ATOM, VERSIONED_ATOM, 12, AtomFlags_Data_Binary, 0, ilst_atom->AtomicLevel+2, rDNS_four_dash_parent);
		uint32_t domain_len = strlen(rDNS_domain);
		parsedAtoms[rDNS_mean_atom].ReverseDNSdomain = (char*)calloc(1, sizeof(char)*101);
		memcpy( parsedAtoms[rDNS_mean_atom].ReverseDNSdomain, rDNS_domain, domain_len );
		APar_atom_Binary_Put(&parsedAtoms[rDNS_mean_atom], rDNS_domain, domain_len, 0);
		
		short rDNS_name_atom = APar_InterjectNewAtom("name", CHILD_ATOM, VERSIONED_ATOM, 12, AtomFlags_Data_Binary, 0, ilst_atom->AtomicLevel+2, rDNS_mean_atom);
		uint32_t name_len = strlen(rDNS_atom_name);
		parsedAtoms[rDNS_name_atom].ReverseDNSname = (char*)calloc(1, sizeof(char)*101);
		memcpy( parsedAtoms[rDNS_name_atom].ReverseDNSname, rDNS_atom_name, name_len );
		APar_atom_Binary_Put(&parsedAtoms[rDNS_name_atom], rDNS_atom_name, name_len, 0);
		
		AtomicInfo proto_rDNS_data_atom = { 0 };
		APar_CreateSurrogateAtom(&proto_rDNS_data_atom, "data", ilst_atom->AtomicLevel+2, VERSIONED_ATOM, 0, NULL, 0);
		desiredAtom = APar_CreateSparseAtom(&proto_rDNS_data_atom, ilst_atom, rDNS_name_atom);
		APar_MetaData_atom_QuickInit(desiredAtom->AtomicNumber, *atomFlags, 0, MAXDATA_PAYLOAD);
	}	else {
		if (memcmp(rDNS_domain, "com.apple.iTunes", 17) == 0) { //for the iTunes domain, only support 1 'data' entry
			APar_MetaData_atom_QuickInit(desiredAtom->NextAtomNumber, *atomFlags, 0, MAXDATA_PAYLOAD);
			desiredAtom = &parsedAtoms[desiredAtom->NextAtomNumber];
			
		} else { //now create a 'data' atom at the end of the hierarchy (allowing multiple entries)		
			short rDNSparent_idx = APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel);
			short last_child = APar_FindLastChild_of_ParentAtom(rDNSparent_idx);
			AtomicInfo proto_rDNS_data_atom = { 0 };
			APar_CreateSurrogateAtom(&proto_rDNS_data_atom, "data", parsedAtoms[last_child].AtomicLevel, VERSIONED_ATOM, 0, NULL, 0);
			desiredAtom = APar_CreateSparseAtom(&proto_rDNS_data_atom, &parsedAtoms[rDNSparent_idx], last_child);
			APar_MetaData_atom_QuickInit(desiredAtom->AtomicNumber, *atomFlags, 0, MAXDATA_PAYLOAD);
		}
	}
	APar_FlagMovieHeader();
	free(reverseDNS_atompath); reverseDNS_atompath = NULL;
	modified_atoms = true;
	return desiredAtom;
}

AtomicInfo* APar_ID32_atom_Init(char* frameID_str, char meta_area, char* lang_str, uint16_t id32_lang) {
	uint8_t total_tracks = 0;
	uint8_t a_track = 0;//unused
	AtomicInfo* meta_atom = NULL;
	AtomicInfo* hdlr_atom = NULL;
	char* id32_trackpath = NULL;
	AtomicInfo* ID32_atom = NULL;
	bool non_referenced_data = false;
	bool remove_ID32_atom = false;
	
	APar_FindAtomInTrack(total_tracks, a_track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.
	
	if (meta_area > 0) {
		if ((uint8_t)meta_area > total_tracks) {
			APar_assert(false, 6, frameID_str);
			return NULL;
		}
	}
	
	id32_trackpath = (char*)calloc(1, sizeof(char)*100);
	
	if (meta_area == 0-FILE_LEVEL_ATOM) {
		meta_atom = APar_FindAtom("meta", false, DUAL_STATE_ATOM, 0);
	} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
		meta_atom = APar_FindAtom("moov.meta", false, DUAL_STATE_ATOM, 0);
	//} else if (meta_area = 0) {
		//setting id3tags for all tracks will *not* be supported;
	} else if (meta_area > 0) {
		sprintf(id32_trackpath, "moov.trak[%u].meta", meta_area);
		meta_atom = APar_FindAtom(id32_trackpath, false, DUAL_STATE_ATOM, 0);
	}
	
	if (meta_atom != NULL) {
		hdlr_atom = APar_FindChildAtom(meta_atom->AtomicNumber, "hdlr");
		if (hdlr_atom != NULL) {
			if (hdlr_atom->ancillary_data != 0x49443332) {
				memset(id32_trackpath, 0, 5); //well, it won't be used for anything else since the handler type doesn't match, might as well convert the handler type using it
				UInt32_TO_String4(hdlr_atom->ancillary_data, id32_trackpath);
				APar_assert(false, 7, id32_trackpath);
				free(id32_trackpath);
				
				return NULL;
			}
		}
	}
	
	//its possible the ID32 atom targeted already exists - finding it in the traditional form (not external, and not locally referenced) is easiest. Locally referenced isn't.
	if (meta_area == 0-FILE_LEVEL_ATOM) {
		ID32_atom = APar_FindAtom("meta.ID32", false, PACKED_LANG_ATOM, id32_lang);
	} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
		ID32_atom = APar_FindAtom("moov.meta.ID32", false, PACKED_LANG_ATOM, id32_lang);
	//} else if (meta_area = 0) {
		//setting id3tags for all tracks will *not* be supported;
	} else if (meta_area > 0) {
		sprintf(id32_trackpath, "moov.trak[%u].meta.ID32", meta_area);
		ID32_atom = APar_FindAtom(id32_trackpath, false, PACKED_LANG_ATOM, id32_lang);
	}
	
	if (ID32_atom != NULL) {
		free(id32_trackpath);
		id32_trackpath = NULL;
		if (ID32_atom->ID32_TagInfo == NULL) {
			APar_ID32_ScanID3Tag(source_file, ID32_atom);
		}
		return ID32_atom; //and that completes finding the ID32 atom and verifying that it was local and in a traditionally represented form.

	} else {
		if (meta_atom != NULL) {
			//if the primary item atom is present, it points to either a local data reference (flag of 0x000001) or external data which is unsupported. Either way skip it.
			//..or probably another test would be if a data REFERENCE atom were present.... but you would have to match item_IDs - which are found in pitm (required for ID32).
			if (APar_FindChildAtom(meta_atom->AtomicNumber, "pitm") != NULL) {
				non_referenced_data = false;
				APar_assert(false, 8, frameID_str);
				free(id32_trackpath);
				return NULL;
			} else { //the inline/3gpp 'ID32' atom calls for referenced content to carry a 'pitm' atom. No worries - its just a 'meta'.
				non_referenced_data = true;
			}
		} else {
			//no 'meta' atom? Great - a blank slate. There won't be any jumping through a multitude of atoms to determine referencing
			non_referenced_data = true;
		}
	}
	
	if (frameID_str == NULL) {
		remove_ID32_atom = true;
	} else if (strlen(frameID_str) == 0) {
		remove_ID32_atom = true;
	}
	//this only gets executed if a pre-existing satisfactory ID32 atom was not found. Being able to find it by atom.path by definition means it was not referenced.
	if (non_referenced_data && !remove_ID32_atom) {
		if (meta_atom == NULL) {
			if (meta_area == 0-FILE_LEVEL_ATOM) {
				meta_atom = APar_FindAtom("meta", true, VERSIONED_ATOM, 0);
			} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
				meta_atom = APar_FindAtom("moov.meta", true, VERSIONED_ATOM, 0);
			//} else if (meta_area = 0) {
				//setting id3tags for all tracks will *not* be supported;
			} else if (meta_area > 0) {
				sprintf(id32_trackpath, "moov.trak[%u].meta", meta_area);
				meta_atom = APar_FindAtom(id32_trackpath, true, VERSIONED_ATOM, 0);
			}
		}
		
		//create the required hdlr atom
		if (hdlr_atom == NULL) {
			if (meta_area == 0-FILE_LEVEL_ATOM) {
				hdlr_atom = APar_FindAtom("meta.hdlr", true, VERSIONED_ATOM, 0);
			} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
				hdlr_atom = APar_FindAtom("moov.meta.hdlr", true, VERSIONED_ATOM, 0);
			//} else if (meta_area = 0) {
				//setting id3tags for all tracks will *not* be supported;
			} else if (meta_area > 0) {
				sprintf(id32_trackpath, "moov.trak[%u].meta.hdlr", meta_area);
				hdlr_atom = APar_FindAtom(id32_trackpath, true, VERSIONED_ATOM, 0);
			}
			if (hdlr_atom == NULL) {
				fprintf(stdout, "Uh, problem\n");
				exit(0);
			}
			APar_MetaData_atom_QuickInit(hdlr_atom->AtomicNumber, 0, 0);
			APar_Unified_atom_Put(hdlr_atom, "ID32", UTF8_iTunesStyle_256glyphLimited, 0, 0);
			APar_Unified_atom_Put(hdlr_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
			APar_Unified_atom_Put(hdlr_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
			APar_Unified_atom_Put(hdlr_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
			APar_Unified_atom_Put(hdlr_atom, "AtomicParsley ID3v2 Handler", UTF8_3GP_Style, 0, 0);
			hdlr_atom->ancillary_data = 0x49443332;
		}
		
		//and finally create the ID32 atom
		if (meta_area == 0-FILE_LEVEL_ATOM) {
			ID32_atom = APar_FindAtom("meta.ID32", true, PACKED_LANG_ATOM, id32_lang);
		} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
			ID32_atom = APar_FindAtom("moov.meta.ID32", true, PACKED_LANG_ATOM, id32_lang);
		//} else if (meta_area = 0) {
			//setting id3tags for all tracks will *not* be supported;
		} else if (meta_area > 0) {
			sprintf(id32_trackpath, "moov.trak[%u].meta.ID32", meta_area);
			ID32_atom = APar_FindAtom(id32_trackpath, true, PACKED_LANG_ATOM, id32_lang);
		}

		if (id32_trackpath != NULL) {
			free(id32_trackpath);
			id32_trackpath = NULL;
		}
		
		ID32_atom->AtomicLength = 12; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags (NULLs), 2 bytes lang
		ID32_atom->AtomicVerFlags = 0;
		ID32_atom->AtomicContainerState = CHILD_ATOM;
		ID32_atom->AtomicClassification = PACKED_LANG_ATOM;
		ID32_atom->AtomicLanguage = id32_lang;
		
		APar_ID3Tag_Init(ID32_atom);
		//search for the desired frame

		//add the frame to an empty frame, copy data onto the id32_atom structure as required
		//set modified _atoms to true

		return ID32_atom;
	} else if (remove_ID32_atom) {
		if (meta_area == 0-FILE_LEVEL_ATOM) {
			APar_RemoveAtom("meta.ID32", PACKED_LANG_ATOM, id32_lang);
		} else if (meta_area == 0-MOVIE_LEVEL_ATOM) {
			APar_RemoveAtom("moov.meta.ID32", PACKED_LANG_ATOM, id32_lang);
		//} else if (meta_area = 0) {
			//setting id3tags for all tracks will *not* be supported;
		} else if (meta_area > 0) {
			sprintf(id32_trackpath, "moov.trak[%u].meta.ID32", meta_area);
			APar_RemoveAtom(id32_trackpath, PACKED_LANG_ATOM, id32_lang);
		}
	}
	return NULL;
}

void APar_RenderAllID32Atoms() {
	short atom_idx = 0;
	short meta_idx = -1;
	//loop through each atom in the struct array (which holds the offset info/data)
 	while (true) {
		if (memcmp(parsedAtoms[atom_idx].AtomicName, "ID32", 4) == 0) {
			if (parsedAtoms[atom_idx].ID32_TagInfo != NULL) {
				uint32_t id32tag_max_length = APar_GetTagSize(&parsedAtoms[atom_idx]);
				if (id32tag_max_length > 0) {
					parsedAtoms[atom_idx].AtomicData = (char*)calloc(1, sizeof(char)* id32tag_max_length + (16*parsedAtoms[atom_idx].ID32_TagInfo->ID3v2_FrameCount) );
					APar_Unified_atom_Put(&parsedAtoms[atom_idx], NULL, 0, parsedAtoms[atom_idx].AtomicLanguage, 16);
					parsedAtoms[atom_idx].AtomicLength = APar_Render_ID32_Tag(&parsedAtoms[atom_idx], id32tag_max_length + (16*parsedAtoms[atom_idx].ID32_TagInfo->ID3v2_FrameCount) ) + 14;
					if (parsedAtoms[atom_idx].AtomicLength < 12+10) {
						meta_idx = APar_FindParentAtom(parsedAtoms[atom_idx].AtomicNumber, parsedAtoms[atom_idx].AtomicLevel);
						APar_EliminateAtom(parsedAtoms[atom_idx].AtomicNumber, parsedAtoms[atom_idx].NextAtomNumber);
					} else {
						APar_FlagTrackHeader(&parsedAtoms[atom_idx]);
					}
				} else {
					meta_idx = APar_FindParentAtom(parsedAtoms[atom_idx].AtomicNumber, parsedAtoms[atom_idx].AtomicLevel);
					APar_EliminateAtom(parsedAtoms[atom_idx].AtomicNumber, parsedAtoms[atom_idx].NextAtomNumber);
				}
			}
		}
		if (meta_idx > 0) {
			if (memcmp(parsedAtoms[meta_idx].AtomicName, "meta", 4) == 0) {
				if ( APar_ReturnChildrenAtoms(meta_idx, 0) == 1 ) {
					AtomicInfo* meta_handler = &parsedAtoms[ parsedAtoms[meta_idx].NextAtomNumber ];
					if (memcmp(meta_handler->AtomicName, "hdlr", 4) == 0 && meta_handler->ancillary_data == 0x49443332) {
						APar_EliminateAtom(meta_idx, meta_handler->NextAtomNumber);
					}
				}
			}
		}
		atom_idx = parsedAtoms[atom_idx].NextAtomNumber;
		if (parsedAtoms[atom_idx].AtomicNumber == 0) break;
		meta_idx = -1;
	}
	return;
}

/*----------------------
APar_TestVideoDescription
	video_desc_atom - the avc1 atom after stsd that contains the height/width
	ISObmff_file - the reopened source file

    read in the height, width, profile & level of an avc (non-drm) track. If the the macroblocks are between 300 & 1200, return a non-zero number to allow the ipod
		uuid to be written
		
    NOTE: this requires the deep scan cli flag to break out stsd from its normal monolithic form
----------------------*/
uint16_t APar_TestVideoDescription(AtomicInfo* video_desc_atom, FILE* ISObmff_file) {
	uint16_t video_width = 0;
	uint16_t video_height = 0;
	uint16_t video_macroblocks = 0;
	uint8_t video_profile = 0;
	uint8_t video_level = 0;	
	AtomicInfo* avcC_atom = NULL;
	
	if (ISObmff_file == NULL) return 0;
	
	char* avc1_contents = (char*)calloc(1, sizeof(char)* (size_t)video_desc_atom->AtomicLength);
	if (avc1_contents == NULL) {
		fclose(ISObmff_file);
		return 0;
	}
	
	APar_readX(avc1_contents, ISObmff_file, video_desc_atom->AtomicStart, video_desc_atom->AtomicLength); //actually reads in avcC as well, but is unused
	video_width = UInt16FromBigEndian(avc1_contents+32); // well, iTunes only allows 640 max but the avc wiki says it *could* go up to 720, so I won't bother to check it
	video_height = UInt16FromBigEndian(avc1_contents+34);
	video_macroblocks = (video_width / 16) * (video_height / 16);
	
	avcC_atom = APar_FindChildAtom(video_desc_atom->AtomicNumber, "avcC");
	if (avcC_atom != NULL) {
		uint32_t avcC_offset = avcC_atom->AtomicStart - video_desc_atom->AtomicStart;
		video_profile = *(avc1_contents+avcC_offset+9);
		video_level = *(avc1_contents+avcC_offset+11);
	}
	
	if (video_profile == 66 && video_level <= 30) {
		if (video_macroblocks > 300 && video_macroblocks <= 1200) {
		
			if (video_level <= 30 && avcC_atom != NULL) {
				avcC_atom->AtomicData = (char*)calloc(1, sizeof(char)* (size_t)avcC_atom->AtomicLength);
				APar_readX(avcC_atom->AtomicData, ISObmff_file, avcC_atom->AtomicStart+8, avcC_atom->AtomicLength-8);
				if (video_macroblocks > 396 && video_macroblocks <= 792) {
					*(avcC_atom->AtomicData + 3) = 21;
				} else if (video_macroblocks > 792) {
					*(avcC_atom->AtomicData + 3) = 22;
				}
			}

			fclose(ISObmff_file);
			return video_macroblocks;
		} else {
			fprintf(stdout, "AtomicParsley warning: the AVC track macroblocks were not in the required range (300-1200). Skipping.\n");
		}
	} else {
		fprintf(stdout, "AtomicParsley warning: the AVC track profile/level was too high. The ipod hi-res uuid was not added.\n");
	}
	
	fclose(ISObmff_file);
	return 0;
}

/*----------------------
APar_TestVideoDescription
	atom_path - pointer to the string containing the atom.path already targeted to the right track
		
		Find/Create the ipod hi-res (1200 macroblock) uuid for the avc1 track & set up its default parameters

    NOTE: this requires the deep scan cli flag to break out stsd from its normal monolithic form
----------------------*/
void APar_Generate_iPod_uuid(char* atom_path) {
	AtomicInfo* ipod_uuid_atom = NULL;
	
	ipod_uuid_atom = APar_FindAtom(atom_path, false, EXTENDED_ATOM, 0, true);
	if (ipod_uuid_atom == NULL) {
		ipod_uuid_atom = APar_FindAtom(atom_path, true, EXTENDED_ATOM, 0, true);
		if (ipod_uuid_atom == NULL) {
			fprintf(stdout, "An error occured trying to create the ipod uuid atom for the avc track\n");
			return;
		}
		ipod_uuid_atom->AtomicData = (char*)calloc(1, sizeof(char)* 60 );
		ipod_uuid_atom->AtomicContainerState = CHILD_ATOM;
		ipod_uuid_atom->AtomicClassification = EXTENDED_ATOM;
		ipod_uuid_atom->uuid_style = UUID_OTHER;
		ipod_uuid_atom->AtomicLength = 24;
		APar_Unified_atom_Put(ipod_uuid_atom, NULL, UTF8_iTunesStyle_Unlimited, 1, 32);
		modified_atoms = true;
		
		APar_FlagTrackHeader(ipod_uuid_atom);
		APar_FlagMovieHeader();
		
		track_codecs.has_avc1 = true; //only used on Mac OS X when setting the ipod uuid *only* (otherwise it gets set properly)
	} else {
		fprintf(stdout, "the ipod higher-resolution uuid is already present.\n");
	}
	
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            offset calculations                                    //
///////////////////////////////////////////////////////////////////////////////////////

//determine if our mdat atom has moved at all...
uint32_t APar_DetermineMediaData_AtomPosition() {
	uint32_t mdat_position = 0;
	short thisAtomNumber = 0;
	
	//loop through each atom in the struct array (which holds the offset info/data)
 	while (parsedAtoms[thisAtomNumber].NextAtomNumber != 0) {
		
		if ( strncmp(parsedAtoms[thisAtomNumber].AtomicName, "mdat", 4) == 0 && parsedAtoms[thisAtomNumber].AtomicLevel == 1 ) {
			if (parsedAtoms[thisAtomNumber].AtomicLength <= 1 || parsedAtoms[thisAtomNumber].AtomicLength > 75) {
				break;
			}
		} else if (parsedAtoms[thisAtomNumber].AtomicLevel == 1 && parsedAtoms[thisAtomNumber].AtomicLengthExtended == 0) {
			mdat_position +=parsedAtoms[thisAtomNumber].AtomicLength;
		} else {
			//part of the pseudo 64-bit support
			mdat_position +=parsedAtoms[thisAtomNumber].AtomicLengthExtended;
		}
		thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
	}
	return mdat_position;
}

uint32_t APar_SimpleSumAtoms(short stop_atom) {
	uint32_t byte_sum = 0;
	//first, find the first mdat after this initial 'tfhd' atom to get the sum relative to that atom
	while (true) {
		if ( strncmp(parsedAtoms[stop_atom].AtomicName, "mdat", 4) == 0) {
			stop_atom--; //don't include the fragment's mdat, just the atoms prior to it
			break;
		} else {
			if (parsedAtoms[stop_atom].NextAtomNumber != 0) {
				stop_atom = parsedAtoms[stop_atom].NextAtomNumber;
			} else {
				break;
			}
		}
	}
	byte_sum += 8; //the 'tfhd' points to the byte in mdat where the fragment data is - NOT the atom itself (should always be +8bytes with a fragment)
	while (true) {
		if (parsedAtoms[stop_atom].AtomicLevel == 1) {
			byte_sum+= (parsedAtoms[stop_atom].AtomicLength == 1 ? (uint32_t )parsedAtoms[stop_atom].AtomicLengthExtended : parsedAtoms[stop_atom].AtomicLength);
			//fprintf(stdout, "%i %s (%u)\n", stop_atom, parsedAtoms[stop_atom].AtomicName, parsedAtoms[stop_atom].AtomicLength);
		}
		if (stop_atom == 0) {
			break;
		} else {
			stop_atom = APar_FindPrecedingAtom(stop_atom);
		}
	}
	return byte_sum;
}

/*----------------------
APar_QuickSumAtomicLengths
	target_atom - pointer to the atom that will have its position within the *new* file determined; NOTE: only level 1 or level 2 atoms in moov

    fill
----------------------*/
uint32_t APar_QuickSumAtomicLengths(AtomicInfo* target_atom) {
	uint32_t atom_pos = 0;
	short atom_idx = 0;
	short current_level = 0;
	if (target_atom == NULL) return atom_pos;
	if (target_atom->AtomicLevel > 2) return atom_pos;
	
	atom_idx = APar_FindPrecedingAtom(target_atom->AtomicNumber);
	current_level = target_atom->AtomicLevel;
	
	while (true) {
		if (parsedAtoms[atom_idx].AtomicLevel <= target_atom->AtomicLevel) {
			if (parsedAtoms[atom_idx].AtomicContainerState >= DUAL_STATE_ATOM || parsedAtoms[atom_idx].AtomicLevel == 2) {
				atom_pos += (parsedAtoms[atom_idx].AtomicLength == 1 ? (uint32_t)parsedAtoms[atom_idx].AtomicLengthExtended : parsedAtoms[atom_idx].AtomicLength);
			} else if (parsedAtoms[atom_idx].AtomicContainerState <= SIMPLE_PARENT_ATOM) {
				if (target_atom->AtomicLevel == 1) {
					atom_pos += (parsedAtoms[atom_idx].AtomicLength == 1 ? (uint32_t)parsedAtoms[atom_idx].AtomicLengthExtended : parsedAtoms[atom_idx].AtomicLength);
				} else {
					atom_pos += 8;
				}
			}
		}
		if (atom_idx == 0) break;
		atom_idx = APar_FindPrecedingAtom(atom_idx);
	}
	return atom_pos;
}

/*----------------------
APar_Constituent_mdat_data
	desired_data_pos - the position in the file where the desired data begins
	desired_data_len - the length of the desired data contained within the file

    fill
----------------------*/
AtomicInfo* APar_Constituent_mdat_data(uint32_t desired_data_pos, uint32_t desired_data_len) {
	AtomicInfo* target_mdat = NULL;
	short eval_atom = 0;
	
 	while (parsedAtoms[eval_atom].NextAtomNumber != 0) {
		
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0 && parsedAtoms[eval_atom].AtomicLevel == 1 ) {
			if (parsedAtoms[eval_atom].AtomicLength == 1) {
				if ( (parsedAtoms[eval_atom].AtomicStart + (uint32_t)parsedAtoms[eval_atom].AtomicLengthExtended >= desired_data_pos + desired_data_len) && 
				      parsedAtoms[eval_atom].AtomicStart < desired_data_pos) {
					target_mdat = &parsedAtoms[eval_atom];
					break;
				}
			} else {
				if ( (parsedAtoms[eval_atom].AtomicStart + parsedAtoms[eval_atom].AtomicLength >= desired_data_pos + desired_data_len) && 
				     parsedAtoms[eval_atom].AtomicStart < desired_data_pos) {
					target_mdat = &parsedAtoms[eval_atom];
					break;
				}
			}
		}
		eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
	}
	return target_mdat;
}

/*----------------------
APar_Readjust_iloc_atom
	iloc_number - the target iloc atom index in parsedAtoms

    fill
----------------------*/
bool APar_Readjust_iloc_atom(short iloc_number) {
	bool iloc_changed = false;
	
	parsedAtoms[iloc_number].AtomicData = (char*)calloc(1, sizeof(char)* (size_t)(parsedAtoms[iloc_number].AtomicLength) );
	APar_readX(parsedAtoms[iloc_number].AtomicData, source_file, parsedAtoms[iloc_number].AtomicStart+12, parsedAtoms[iloc_number].AtomicLength-12);

	uint8_t offset_size = ( *parsedAtoms[iloc_number].AtomicData >> 4) & 0x0F;
	uint8_t length_size = *parsedAtoms[iloc_number].AtomicData & 0x0F;
	uint8_t base_offset_size = ( *(parsedAtoms[iloc_number].AtomicData+1) >> 4) & 0x0F;
	uint16_t item_count = UInt16FromBigEndian(parsedAtoms[iloc_number].AtomicData+2);
	uint32_t aggregate_offset = 4;
	char* base_offset_ptr = NULL;
	
#if defined(DEBUG_V)
	fprintf(stdout, "debug: AP_Readjust_iloc_atom   Offset %X, len %X, base %X, item count %u\n", offset_size, length_size, base_offset_size, item_count);
#endif
	
	for (uint16_t an_item=1; an_item <= item_count; an_item++) {
		uint16_t an_item_ID = UInt16FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
		uint16_t a_data_ref_idx = UInt16FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset+2);
		uint32_t base_offset = 0;
		uint32_t curr_container_pos = 0;
		uint32_t extent_len_sum = 0;
		
		aggregate_offset +=4;
		
		if (a_data_ref_idx != 0) {
			continue;
		}
		
		if (base_offset_size == 4 || base_offset_size == 8) {
			base_offset_ptr = parsedAtoms[iloc_number].AtomicData+aggregate_offset;
			if (base_offset_size == 4) {
				base_offset = UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
				aggregate_offset +=4;
			} else {
				base_offset = (uint32_t)UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
				aggregate_offset +=8;
			}
		}
		
		if (base_offset > 0) {
			uint16_t this_item_extent_count = UInt16FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
			aggregate_offset +=2;
			
			for (uint16_t an_extent=1; an_extent <= this_item_extent_count; an_extent++) {
				uint32_t this_extent_offset = 0;
				uint32_t this_extent_length = 0;
				
				if (offset_size == 4 || offset_size == 8) {
					if (offset_size == 4) {
						this_extent_offset = UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
						aggregate_offset +=4;
					} else {
						this_extent_offset = (uint32_t)UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
						aggregate_offset +=8;
					}
				}
				if (length_size == 4 || length_size == 8) {
					if (length_size == 4) {
						this_extent_length = UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
						aggregate_offset +=4;
					} else {
						this_extent_length = (uint32_t)UInt32FromBigEndian(parsedAtoms[iloc_number].AtomicData+aggregate_offset);
						aggregate_offset +=8;
					}
					extent_len_sum+= this_extent_length;
				}
			} //for loop extent
			
#if defined(DEBUG_V)
			fprintf(stdout, "debug: AP_Readjust_iloc_atom  iloc's %u index at base offset: %u, total bytes %u\n", an_item_ID, base_offset, extent_len_sum);
#endif
			AtomicInfo* container_atom = APar_Constituent_mdat_data(base_offset, 0x013077 );
			
			if (container_atom != NULL) {
				curr_container_pos = APar_QuickSumAtomicLengths(container_atom);
				uint32_t exisiting_offset_into_atom = base_offset - container_atom->AtomicStart;
				uint32_t new_item_offset = curr_container_pos + exisiting_offset_into_atom;
				
#if defined(DEBUG_V)
				fprintf(stdout, "debug: AP_Readjust_iloc_atom  item is contained on mdat started @ %u (now at %u)\n", container_atom->AtomicStart, curr_container_pos);
				fprintf(stdout, "debug: AP_Readjust_iloc_atom  item is %u bytes offset into atom (was %u, now %u)\n", exisiting_offset_into_atom, base_offset, new_item_offset);
#endif
				if (base_offset_size == 4) {
					UInt32_TO_String4(new_item_offset, base_offset_ptr);
				} else {
					UInt64_TO_String8((uint64_t)new_item_offset, base_offset_ptr);
				}
				iloc_changed = true;
			}
		}
	}
	
	return iloc_changed;
}

bool APar_Readjust_CO64_atom(uint32_t mdat_position, short co64_number) {
	bool co64_changed = false;
	
	if (parsedAtoms[co64_number].ancillary_data != 1) { 
		return co64_changed;
	}
	
	parsedAtoms[co64_number].AtomicData = (char*)calloc(1, sizeof(char)* (size_t)(parsedAtoms[co64_number].AtomicLength) );
	APar_readX(parsedAtoms[co64_number].AtomicData, source_file, parsedAtoms[co64_number].AtomicStart+12, parsedAtoms[co64_number].AtomicLength-12);

	parsedAtoms[co64_number].AtomicVerFlags = 0;
	bool deduct = false;
	//readjust
	
	char* co64_entries = (char *)malloc(sizeof(char)*4 + 1);
	memset(co64_entries, 0, sizeof(char)*4 + 1);
	
	memcpy(co64_entries, parsedAtoms[co64_number].AtomicData, 4);
	uint32_t entries = UInt32FromBigEndian(co64_entries);
	
	char* a_64bit_entry = (char *)malloc(sizeof(char)*8 + 1);
	memset(a_64bit_entry, 0, sizeof(char)*8 + 1);
	
	for(uint32_t i=1; i<=entries; i++) {
		//read 8 bytes of the atom into a 8 char uint64_t a_64bit_entry to eval it
		for (int c = 0; c <=7; c++ ) {
			//first co64 entry (32-bit uint32_t) is the number of entries; every other one is an actual offset value
			a_64bit_entry[c] = parsedAtoms[co64_number].AtomicData[4 + (i-1)*8 + c];
		}
		uint64_t this_entry = UInt64FromBigEndian(a_64bit_entry);
		
		if (i == 1 && mdat_supplemental_offset == 0) { //for the first chunk, and only for the first *ever* entry, make the global mdat supplemental offset
			if (this_entry - removed_bytes_tally > mdat_position) {
				mdat_supplemental_offset = (uint64_t)mdat_position - ((uint64_t)this_entry - (uint64_t)removed_bytes_tally);
				bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;
				deduct=true;
			} else {
				mdat_supplemental_offset = mdat_position - (this_entry - removed_bytes_tally);
				bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;
			}
			
			if (mdat_supplemental_offset == 0) {
				break;
			}
		}
		
		if (mdat_supplemental_offset != 0) {
			co64_changed = true;
		}
		
		if (deduct) { //crap, uint32_t's were so nice to flip over by themselves to subtract nicely. going from 32-bit to 64-bit prevents that flipping
			this_entry += mdat_supplemental_offset - (bytes_into_mdat * -1); // + bytes_into_mdat;
		} else {
			this_entry += mdat_supplemental_offset + bytes_into_mdat; //this is where we add our new mdat offset difference
		}
		UInt64_TO_String8(this_entry, a_64bit_entry);
		//and put the data back into AtomicData...
		for (int d = 0; d <=7; d++ ) {
			//first stco entry is the number of entries; every other one is an actual offset value
			parsedAtoms[co64_number].AtomicData[4 + (i-1)*8 + d] = a_64bit_entry[d];
		}
	}
	
	free(a_64bit_entry);
	free(co64_entries);
	a_64bit_entry=NULL;
	co64_entries=NULL;
	//end readjustment
	return co64_changed;
}

bool APar_Readjust_TFHD_fragment_atom(uint32_t mdat_position, short tfhd_number) {
	static bool tfhd_changed = false;
	static bool determined_offset = false;
	static uint64_t base_offset = 0;
	
	parsedAtoms[tfhd_number].AtomicData = (char*)calloc(1, sizeof(char)* (size_t)(parsedAtoms[tfhd_number].AtomicLength) );
	APar_readX(parsedAtoms[tfhd_number].AtomicData, source_file, parsedAtoms[tfhd_number].AtomicStart+12, parsedAtoms[tfhd_number].AtomicLength-12);
	
	char* tfhd_atomFlags_scrap = (char *)malloc(sizeof(char)*10);
	memset(tfhd_atomFlags_scrap, 0, 10);
	//parsedAtoms[tfhd_number].AtomicVerFlags = APar_read32(tfhd_atomFlags_scrap, source_file, parsedAtoms[tfhd_number].AtomicStart+8);
 
	if (parsedAtoms[tfhd_number].AtomicVerFlags & 0x01) { //seems the atomflags suggest bitpacking, but the spec doesn't specify it; if the 1st bit is set...
		memset(tfhd_atomFlags_scrap, 0, 10);	
		memcpy(tfhd_atomFlags_scrap, parsedAtoms[tfhd_number].AtomicData, 4);
		
		uint32_t track_ID = UInt32FromBigEndian(tfhd_atomFlags_scrap); //unused
		uint64_t tfhd_offset = UInt64FromBigEndian(parsedAtoms[tfhd_number].AtomicData +4);
		
		if (!determined_offset) {
			determined_offset = true;
			base_offset = APar_SimpleSumAtoms(tfhd_number) - tfhd_offset;
			if (base_offset != 0) {
				tfhd_changed = true;
			}
		}
		
		tfhd_offset += base_offset;
		UInt64_TO_String8(tfhd_offset, parsedAtoms[tfhd_number].AtomicData +4);
	}
	return tfhd_changed;
}

bool APar_Readjust_STCO_atom(uint32_t mdat_position, short stco_number) {
	bool stco_changed = false;
	
	if (parsedAtoms[stco_number].ancillary_data != 1) { 
		return stco_changed;
	}

	parsedAtoms[stco_number].AtomicData = (char*)calloc(1, sizeof(char)* (size_t)(parsedAtoms[stco_number].AtomicLength) );
	APar_readX(parsedAtoms[stco_number].AtomicData, source_file, parsedAtoms[stco_number].AtomicStart+12, parsedAtoms[stco_number].AtomicLength-12);
	
	parsedAtoms[stco_number].AtomicVerFlags = 0;
	//readjust
	
	char* stco_entries = (char *)malloc(sizeof(char)*4 + 1);
	memset(stco_entries, 0, sizeof(char)*4 + 1);
	
	memcpy(stco_entries, parsedAtoms[stco_number].AtomicData, 4);
	uint32_t entries = UInt32FromBigEndian(stco_entries);
	
	char* an_entry = (char *)malloc(sizeof(char)*4 + 1);
	memset(an_entry, 0, sizeof(char)*4 + 1);
	
	for(uint32_t i=1; i<=entries; i++) {
		//read 4 bytes of the atom into a 4 char uint32_t an_entry to eval it
		for (int c = 0; c <=3; c++ ) {
			//first stco entry is the number of entries; every other one is an actual offset value
			an_entry[c] = parsedAtoms[stco_number].AtomicData[i*4 + c];
		}
		
		uint32_t this_entry = UInt32FromBigEndian(an_entry);
						
		if (i == 1 && mdat_supplemental_offset == 0) { //for the first chunk, and only for the first *ever* entry, make the global mdat supplemental offset
		
			mdat_supplemental_offset = (uint64_t)(mdat_position - (this_entry - removed_bytes_tally) );
			bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;
			
			if (mdat_supplemental_offset == 0) {
				break;
			}
		}
		
		if (mdat_supplemental_offset != 0) {
			stco_changed = true;
		}

		this_entry += mdat_supplemental_offset + bytes_into_mdat;
		UInt32_TO_String4(this_entry, an_entry);
		//and put the data back into AtomicData...
		for (int d = 0; d <=3; d++ ) {
			//first stco entry is the number of entries; every other one is an actual offset value
			parsedAtoms[stco_number].AtomicData[i*4 + d] = an_entry[d];
		}
	}
	
	free(an_entry);
	free(stco_entries);
	an_entry=NULL;
	stco_entries=NULL;
	//end readjustment
	return stco_changed;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            Reorder / Padding                                      //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_CreatePadding
	padding_length - the new length of padding

    Create a 'free' atom at a pre-determined area of a given length & set that atom as the global padding store atom
----------------------*/
void APar_CreatePadding(uint32_t padding_length) {
	AtomicInfo* next_atom = &parsedAtoms[ parsedAtoms[dynUpd.consolidated_padding_insertion].NextAtomNumber ];
	if (padding_length > 2000 && next_atom->AtomicLevel > 1) {
		short padding_atom = APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, 2000, 0, 0,
		                       (next_atom->AtomicLevel == 1 ? 1 : parsedAtoms[dynUpd.consolidated_padding_insertion].AtomicLevel), dynUpd.consolidated_padding_insertion );
		dynUpd.padding_store = &parsedAtoms[padding_atom];
		
		if (dynUpd.first_mdat_atom != NULL && padding_length - 2000 >= 8) {
			short padding_res_atom = APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, padding_length - 2000, 0, 0, 1, 
			                                                APar_FindPrecedingAtom(dynUpd.first_mdat_atom->AtomicNumber) );
			dynUpd.padding_resevoir = &parsedAtoms[padding_res_atom];
		}
	} else {
		short padding_atom = APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, padding_length, 0, 0,
		                       (next_atom->AtomicLevel == 1 ? 1 : parsedAtoms[dynUpd.consolidated_padding_insertion].AtomicLevel), dynUpd.consolidated_padding_insertion );
		dynUpd.padding_store = &parsedAtoms[padding_atom];
	}
	return;
}

/*----------------------
APar_AdjustPadding
	new_padding_length - the new length of padding

    Adjust the consolidated padding store atom to the new size - creating&splitting if necessary.
----------------------*/
void APar_AdjustPadding(uint32_t new_padding_length) {
	uint32_t avail_padding = 0;
	
	if ( (psp_brand || force_existing_hierarchy) && (dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV) ) return;
	
	if (dynUpd.padding_store == NULL) {
		if (new_padding_length >= 8) {
			APar_CreatePadding(new_padding_length);
		} else {
			return;
		}
	}
	
	if (new_padding_length < 8) {
		APar_EliminateAtom(dynUpd.padding_store->AtomicNumber, dynUpd.padding_store->NextAtomNumber);
		dynUpd.updage_by_padding = false;
	}
	
	if (dynUpd.padding_store != NULL) avail_padding += dynUpd.padding_store->AtomicLength;
	if (dynUpd.padding_resevoir != NULL) avail_padding += dynUpd.padding_resevoir->AtomicLength;
	
	if ((new_padding_length > avail_padding && new_padding_length < 2000) || dynUpd.padding_store->AtomicLevel == 1 ) {
		free(dynUpd.padding_store->AtomicData);
		dynUpd.padding_store->AtomicData = (char*)calloc(1, sizeof(char)*new_padding_length );
		dynUpd.padding_store->AtomicLength = new_padding_length;
	} else {
		free(dynUpd.padding_store->AtomicData);
		dynUpd.padding_store->AtomicData = (char*)calloc(1, sizeof(char)*2007 );
		dynUpd.padding_store->AtomicLength = 2000;
		
		if (new_padding_length - 2000 < 8) {
			dynUpd.padding_store->AtomicLength = new_padding_length;
			return;
		}
		
		if (dynUpd.padding_resevoir == NULL && dynUpd.first_mdat_atom != NULL) {
			short pad_res_atom = APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, new_padding_length - 2000, 0, 0, 1,
			                                              APar_FindPrecedingAtom(dynUpd.first_mdat_atom->AtomicNumber) );
			dynUpd.padding_resevoir = &parsedAtoms[pad_res_atom];
			dynUpd.padding_resevoir->AtomicLength = new_padding_length - 2000;
		} else if (dynUpd.padding_resevoir != NULL) {
			free(dynUpd.padding_resevoir->AtomicData);
			dynUpd.padding_resevoir->AtomicData = (char*)calloc(1, sizeof(char)*(new_padding_length - 2000) );
			dynUpd.padding_resevoir->AtomicLength = new_padding_length - 2000;
		}
	}
	
	return;
}

/*----------------------
APar_PaddingAmount
	target_amount - the amount of padding that is requested
	limit_by_prefs - whether to limit the amount of padding to the environmental preferences

    When a file will completely rewritten, limit the amount of padding according to the environmental prefs: min & max with a default amount substitued when padding is
		found to fall outside those values. When a file will be updated by dynamic updating, the consolidated padding will be initially set to the amount that was present
		before any modifications to tags. This amount of padding will in all likelyhood change when the final determination of whether a dynamic update can occur.
----------------------*/
uint32_t APar_PaddingAmount(uint32_t target_amount, bool limit_by_prefs) {
	uint32_t padding_allowance = 0;
	if (limit_by_prefs) {
		if (pad_prefs.maximum_present_padding_size == 0) {
			return 0;
		} else if (target_amount >= pad_prefs.maximum_present_padding_size) {
			padding_allowance = pad_prefs.maximum_present_padding_size;
		} else if (target_amount <= pad_prefs.minimum_required_padding_size) {
			padding_allowance = pad_prefs.default_padding_size;
		} else {			
			padding_allowance = target_amount;
		}
	} else {
		padding_allowance = target_amount;
	}
	if (padding_allowance < 8 ) return 0;
	if (!alter_original) return pad_prefs.default_padding_size;
	return padding_allowance;
}

/*----------------------
APar_DetermineDynamicUpdate

    Find where the first 'mdat' atom will eventually wind up in any new file. If this movement is within the bounds of padding prefs, allow a dynamic update. Adjust
		the amount of padding to accommodate the difference in positions (persuant to padding prefs). Otherwise prevent it, and make sure the default amount of padding
		exists for the file for a full rewrite.
----------------------*/
void APar_DetermineDynamicUpdate() {
	if (!force_existing_hierarchy && moov_atom_was_mooved)  {
		dynUpd.prevent_dynamic_update = true;
		return;
	}
	uint32_t mdat_pos = 0;
	uint32_t moov_udta_pos = APar_QuickSumAtomicLengths(dynUpd.moov_udta_atom);
	uint32_t moov_last_trak_pos = APar_QuickSumAtomicLengths(dynUpd.last_trak_child_atom);
	uint32_t moov_meta_pos = APar_QuickSumAtomicLengths(dynUpd.moov_meta_atom);
	uint32_t moov_pos = APar_QuickSumAtomicLengths(dynUpd.moov_atom);
	uint32_t root_meta_pos = APar_QuickSumAtomicLengths(dynUpd.file_meta_atom);
	
	if (root_meta_pos > 0 && root_meta_pos != dynUpd.file_meta_atom->AtomicStart) {
	
	} else if (moov_pos > 0 && moov_pos != dynUpd.moov_atom->AtomicStart) { //this could reflect either a root meta being moved, or moov came after mdat & was rearranged
		dynUpd.initial_update_atom = &parsedAtoms[0];
	
	} else if (moov_pos > 0) {
		dynUpd.initial_update_atom = dynUpd.moov_atom;
	}
	
	
	if (dynUpd.first_mdat_atom == NULL && alter_original) {
		//work this out later
	
	} else if (dynUpd.first_mdat_atom != NULL) {
		mdat_pos = APar_QuickSumAtomicLengths(dynUpd.first_mdat_atom);
		if (mdat_pos >= dynUpd.first_mdat_atom->AtomicStart) {
			uint32_t offset_increase = mdat_pos - dynUpd.first_mdat_atom->AtomicStart;
			if (offset_increase > dynUpd.padding_bytes) {
				if ( (psp_brand || force_existing_hierarchy) && (dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV) ) {
					dynUpd.updage_by_padding = true;
				} else {
					dynUpd.prevent_dynamic_update = true;
				}
			} else {
				uint32_t padding_remaining = dynUpd.padding_bytes - offset_increase;
				uint32_t padding_allowed = APar_PaddingAmount(padding_remaining, true);
				
				if (padding_remaining == padding_allowed) {
					if (alter_original) {
						dynUpd.updage_by_padding = true;
					}
					APar_AdjustPadding(padding_allowed);
				} else {
					dynUpd.updage_by_padding = false;
					APar_AdjustPadding(padding_allowed);
				}
			}
			
		} else {
			uint32_t padding_replenishment = dynUpd.first_mdat_atom->AtomicStart - mdat_pos;
			uint32_t padding_allowed = APar_PaddingAmount(padding_replenishment, true);
			
			if (padding_replenishment == padding_allowed) {
				if (alter_original) {
					dynUpd.updage_by_padding = true;
				}
				APar_AdjustPadding(padding_allowed);
			} else {
				dynUpd.updage_by_padding = false;
				APar_AdjustPadding(padding_allowed);
			}			
		}
	}
	if (!dynUpd.updage_by_padding && dynUpd.padding_bytes < pad_prefs.default_padding_size) {
		APar_AdjustPadding(pad_prefs.default_padding_size); //if there is a full rewrite, add a default amount of padding
	}
	APar_DetermineAtomLengths();
	return;
}

/*----------------------
APar_ConsolidatePadding

    Work through all the atoms that were determined to be 'padding' in APar_FindPadding, sum up their lengths and remove them from the hieararchy. No bytes are added
		or removed from the file because the total length of the padding is reformed as a single consolidated 'free' atom. This free atom will either form after the iTunes
		handler or (probably) at level 1 (probably before mdat).
		When doing a dynamic update in situ, the length of this consolidated padding atom will change (later) if metadata is altered.
----------------------*/
void APar_ConsolidatePadding() {
	uint32_t bytes_o_padding = 0;
	if (dynUpd.consolidated_padding_insertion == 0) return;
	FreeAtomListing* padding_entry = dynUpd.first_padding_atom;
	
	while (true) {
		if (padding_entry == NULL) break;
		
		APar_EliminateAtom(padding_entry->free_atom->AtomicNumber, padding_entry->free_atom->NextAtomNumber);
		
		FreeAtomListing* next_entry = padding_entry->next_free_listing;
		free(padding_entry);
		padding_entry = next_entry;
	}
	
	bytes_o_padding = APar_PaddingAmount(dynUpd.padding_bytes, !alter_original);
		
	if (bytes_o_padding >= 8) {
		if ( !(psp_brand || force_existing_hierarchy) && (dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV) ) {
			APar_CreatePadding(bytes_o_padding);
		}
	}
	return;
}

/*----------------------
APar_FindPadding
	listing_purposes_only - controls whether to just track free/skip atoms, or actually consolidate

    This is called before all the lengths of all atoms gets redetermined. The atoms at this point are already moved via optimization, and so the location of where
		to place padding can be ascertained. It is not known howevever where the ultimate positions of these landmark atoms will be - which can change according to padding
		prefs (which gets applied to all the padding in APar_ConsolidatePadding(). Any free/skip atoms found to exist between 'moov' & the first 'mdat' atom will be
		be considered padding (excepting anything under 'stsd' which will be monolithing during *setting* of metadata tags). Each padding atom will be noted. A place where
		padding can reside will also be found - the presence of iTunes tags will ultimately deposit all accumulated padding in in the iTunes hierarchy.
----------------------*/
void APar_FindPadding(bool listing_purposes_only) {
	AtomicInfo* DynamicUpdateRangeStart = NULL;
	AtomicInfo* DynamicUpdateRangeEnd = dynUpd.first_mdat_atom; //could be NULL (a missing 'mdat' would mean externally referenced media that would not be self-contained)
	short eval_atom = 0;
	
	if (dynUpd.moov_atom != NULL) {
		DynamicUpdateRangeStart = &parsedAtoms[dynUpd.moov_atom->NextAtomNumber];
	}
	if (DynamicUpdateRangeStart == NULL) return;
	
	eval_atom = DynamicUpdateRangeStart->AtomicNumber;
	while (true) {
		if (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0 || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0) {
			if (dynUpd.first_padding_atom == NULL) {
				dynUpd.first_padding_atom = (FreeAtomListing*)malloc(sizeof(FreeAtomListing));
				dynUpd.first_padding_atom->free_atom = &parsedAtoms[eval_atom];
				dynUpd.first_padding_atom->next_free_listing = NULL;
			} else {
				FreeAtomListing* free_listing = (FreeAtomListing*)malloc(sizeof(FreeAtomListing));
				free_listing->free_atom = &parsedAtoms[eval_atom];
				free_listing->next_free_listing = NULL;
				if (dynUpd.first_padding_atom->next_free_listing == NULL) {
					dynUpd.first_padding_atom->next_free_listing = free_listing;
				} else if (dynUpd.last_padding_atom != NULL) {
					dynUpd.last_padding_atom->next_free_listing = free_listing;
				}
				dynUpd.last_padding_atom = free_listing;
			}
			dynUpd.padding_bytes += (parsedAtoms[eval_atom].AtomicLength == 1 ? (uint32_t)parsedAtoms[eval_atom].AtomicLengthExtended : parsedAtoms[eval_atom].AtomicLength);
		}
		eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
		if (eval_atom == 0) break;
		if (DynamicUpdateRangeEnd != NULL) {
			if (DynamicUpdateRangeEnd->AtomicNumber == eval_atom) break;
		}
	}
	
	//search for a suitable location where padding can accumulate
	if (dynUpd.moov_udta_atom != NULL) {
		if (dynUpd.iTunes_list_handler_atom == NULL) {
			dynUpd.iTunes_list_handler_atom = APar_FindAtom("moov.udta.meta.hdlr", false, VERSIONED_ATOM, 0);
			if (dynUpd.iTunes_list_handler_atom != NULL) {
				if (parsedAtoms[dynUpd.iTunes_list_handler_atom->NextAtomNumber].AtomicLevel >= dynUpd.iTunes_list_handler_atom->AtomicLevel) {
					dynUpd.consolidated_padding_insertion = dynUpd.iTunes_list_handler_atom->AtomicNumber;
				}
			}
		}
	}
	if (dynUpd.consolidated_padding_insertion == 0) {
		if (dynUpd.first_mdat_atom != NULL && !(dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV)) {
			dynUpd.consolidated_padding_insertion = APar_FindPrecedingAtom(dynUpd.first_mdat_atom->AtomicNumber);
		} else {
			dynUpd.consolidated_padding_insertion = APar_FindLastAtom();
			dynUpd.optimization_flags |= PADDING_AT_EOF;
		}		
	}
	
	//if the place where padding will eventually wind up is just before a padding atom, that padding atom will be erased by APar_ConsolidatePadding when its
	//AtomicNumber becomes -1; so work back through the hierarchy for the first non-padding atom
	while (true) {
		if (memcmp(parsedAtoms[dynUpd.consolidated_padding_insertion].AtomicName, "free", 4) == 0 ||
	      memcmp(parsedAtoms[dynUpd.consolidated_padding_insertion].AtomicName, "skip", 4) == 0) {
			dynUpd.consolidated_padding_insertion = APar_FindPrecedingAtom(dynUpd.consolidated_padding_insertion);
		} else {
			break;
		}
	}
	return;
}

void APar_LocateAtomLandmarks() {
	short total_file_level_atoms = APar_ReturnChildrenAtoms (0, 0);
	short eval_atom = 0;
	
	dynUpd.optimization_flags = 0;
	dynUpd.padding_bytes = 0;                  //this *won't* get filled here; it is only tracked for the purposes of dynamic updating
	dynUpd.consolidated_padding_insertion = 0; //this will eventually hold the point where to insert a new atom that will accumulate padding

	dynUpd.last_trak_child_atom = NULL;
	dynUpd.moov_atom = NULL;
	dynUpd.moov_udta_atom = NULL;
	dynUpd.iTunes_list_handler_atom = NULL;    //this *won't* get filled here; it is only tracked for the purposes of dynamic updating
	dynUpd.moov_meta_atom = NULL;
	dynUpd.file_meta_atom = NULL;
	dynUpd.first_mdat_atom = NULL;
	dynUpd.first_movielevel_metadata_tagging_atom = NULL;
	dynUpd.initial_update_atom = NULL;
	dynUpd.first_otiose_freespace_atom = NULL;
	dynUpd.first_padding_atom = NULL;          //this *won't* get filled here; it is only tracked for the purposes of dynamic updating
	dynUpd.last_padding_atom = NULL;           //this *won't* get filled here; it is only tracked for the purposes of dynamic updating
	dynUpd.padding_store = NULL;               //this *won't* get filled here; it gets filled in APar_ConsolidatePadding
	dynUpd.padding_resevoir = NULL;
	
	//scan through all top level atoms; fragmented files won't be optimized
	for(uint8_t iii = 1; iii <= total_file_level_atoms; iii++) {
		eval_atom = APar_ReturnChildrenAtoms (0, iii);
		//fprintf(stdout, "file level children - %s\n", parsedAtoms[eval_atom].AtomicName);
		
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "moof", 4) == 0 || memcmp(parsedAtoms[eval_atom].AtomicName, "mfra", 4) == 0) {
			move_moov_atom = false; //moov reordering won't be occurring on fragmented files, but it should have moov first anyway (QuickTime does at least)
		}

		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0 ) {
			if (dynUpd.first_mdat_atom == NULL) {
				dynUpd.first_mdat_atom = &parsedAtoms[eval_atom];
			}
		}
		
		if (dynUpd.first_otiose_freespace_atom == NULL) {
			if ( (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0 || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0) &&
		        dynUpd.first_mdat_atom == NULL && dynUpd.moov_atom == NULL ) {
				dynUpd.first_otiose_freespace_atom = &parsedAtoms[eval_atom]; //the scourge of libmp4v2
			}
		}
		
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "moov", 4) == 0 ) {
			dynUpd.moov_atom = &parsedAtoms[eval_atom];
			if (dynUpd.first_mdat_atom != NULL) {
				dynUpd.optimization_flags |= MEDIADATA__PRECEDES__MOOV; //or mdat could be entirely missing as well; check later
			}
		}
		
		if ( memcmp(parsedAtoms[eval_atom].AtomicName, "meta", 4) == 0 ) {
			dynUpd.file_meta_atom = &parsedAtoms[eval_atom];
			if (dynUpd.moov_atom != NULL) {
				dynUpd.optimization_flags |= ROOT_META__PRECEDES__MOOV;
			}//meta before moov would require more rewrite of the portion of the file than I want to do
		} //but it wouldn't be all that difficult to accommodate rewriting everything from ftyp down to the first mdat, but currently its limited to last 'trak' child to mdat
	}
	return;
}

/*----------------------
APar_Optimize
	mdat_test_only - the only info desired (if true, when printing the tree) is to know whether mdat precedes moov (and nullifing the concept of padding)

		The visual:
		ftyp
		moov
		    trak
				trak
				trak
				udta
				    meta
						    hdlr
								free (functions as padding store when there are iTunes tags present)
								ilst
				meta
				    hdlr
		meta
		    hdlr
		free (functions as padding store when there are *no* iTunes tags present)
		mdat
								
----------------------*/
void APar_Optimize(bool mdat_test_only) {
	short last_child_of_moov = 0;
	short eval_atom = 0;

	APar_LocateAtomLandmarks();
	
	/* -----------move moov to precede any media data (mdat)--------- */
	if (move_moov_atom && (dynUpd.first_mdat_atom != NULL && (dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV))) {    //first_mdat_atom > 0 && moov_atom > 0 && moov_atom > first_mdat_atom) {
		if (mdat_test_only) {
			moov_atom_was_mooved = true; //this is all the interesting info required (during APar_PrintAtomicTree)
			APar_FindPadding(mdat_test_only);
			return;
		} else {
			APar_MoveAtom(dynUpd.moov_atom->AtomicNumber, dynUpd.first_mdat_atom->AtomicNumber);
			moov_atom_was_mooved = true;
		}
	}
	
	/* -----------move a file/root level 'meta' to follow 'moov', but precede any media data(mdat)--------- */
	if (dynUpd.file_meta_atom != NULL && (dynUpd.optimization_flags & ROOT_META__PRECEDES__MOOV)) {
		last_child_of_moov = APar_FindLastChild_of_ParentAtom(dynUpd.moov_atom->AtomicNumber);
		APar_MoveAtom(dynUpd.file_meta_atom->AtomicNumber, parsedAtoms[last_child_of_moov].NextAtomNumber);
	}

	/* -----------optiizing the layout of movie-level atoms--------- */
	if (dynUpd.moov_atom != NULL) { //it can't be null, but just in case...
		uint8_t extra_atom_count = 0;
		AtomicInfo* last_trak_atom = NULL;
		short total_moov_child_atoms = APar_ReturnChildrenAtoms(dynUpd.moov_atom->AtomicNumber, 0);
		AtomicInfo** other_track_level_atom = (AtomicInfo**)malloc(total_moov_child_atoms * sizeof(AtomicInfo*));
		for (uint8_t xi = 0; xi < total_moov_child_atoms; xi++) {
			other_track_level_atom[xi] = NULL;
		}

		for(uint8_t moov_i = 1; moov_i <= total_moov_child_atoms; moov_i ++) {
			eval_atom = APar_ReturnChildrenAtoms(dynUpd.moov_atom->AtomicNumber, moov_i);
			if ( memcmp(parsedAtoms[eval_atom].AtomicName, "udta", 4) == 0 && parsedAtoms[eval_atom].AtomicLevel == 2) {
				dynUpd.moov_udta_atom = &parsedAtoms[eval_atom];
				if (dynUpd.first_movielevel_metadata_tagging_atom == NULL) dynUpd.first_movielevel_metadata_tagging_atom = &parsedAtoms[eval_atom];
			} else if ( memcmp(parsedAtoms[eval_atom].AtomicName, "meta", 4) == 0 && parsedAtoms[eval_atom].AtomicLevel == 2) {
				dynUpd.moov_meta_atom = &parsedAtoms[eval_atom];
				if (dynUpd.first_movielevel_metadata_tagging_atom == NULL) dynUpd.first_movielevel_metadata_tagging_atom = &parsedAtoms[eval_atom];
			} else if (memcmp(parsedAtoms[eval_atom].AtomicName, "trak", 4) == 0) {
				last_trak_atom = &parsedAtoms[eval_atom];
				if (dynUpd.first_movielevel_metadata_tagging_atom != NULL) {
					if (dynUpd.moov_meta_atom != NULL) dynUpd.optimization_flags |= MOOV_META__PRECEDES__TRACKS;
					else if (dynUpd.moov_udta_atom != NULL) dynUpd.optimization_flags |= MOOV_UDTA__PRECEDES__TRACKS;
				}
			} else if (!(memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0 || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0)) {
				if (dynUpd.moov_meta_atom != NULL || dynUpd.moov_udta_atom != NULL) {
					other_track_level_atom[extra_atom_count] = &parsedAtoms[eval_atom]; //anything else gets noted because it *follows* moov.meta or moov.udta and needs to precede it
					extra_atom_count++;
				}
			}
		}
		
		if (last_trak_atom != NULL) {
			short last_child_of_last_track = APar_FindLastChild_of_ParentAtom(last_trak_atom->AtomicNumber);
			if (last_child_of_last_track > 0) {
				dynUpd.last_trak_child_atom = &parsedAtoms[last_child_of_last_track];
			}
		}
		
		/* -----------moving extra movie-level atoms (![trak,free,skip,meta,udta]) to precede the first metadata tagging hierarchy (moov.meta or moov.udta)--------- */
		if (extra_atom_count > 0 && dynUpd.first_movielevel_metadata_tagging_atom != NULL) {
			for (uint8_t xxi = 0; xxi < extra_atom_count; xxi++) {
				APar_MoveAtom((*other_track_level_atom + xxi)->AtomicNumber, dynUpd.first_movielevel_metadata_tagging_atom->AtomicNumber);
			}
		}
		
		/* -----------moving udta or meta to follow the trak atom--------- */
		if (dynUpd.optimization_flags & MOOV_META__PRECEDES__TRACKS) {
			if (last_child_of_moov == 0) last_child_of_moov = APar_FindLastChild_of_ParentAtom(dynUpd.moov_atom->AtomicNumber);
			APar_MoveAtom(dynUpd.moov_meta_atom->AtomicNumber, parsedAtoms[last_child_of_moov].NextAtomNumber);
		} else if (dynUpd.optimization_flags & MOOV_UDTA__PRECEDES__TRACKS) {
			if (last_child_of_moov == 0) last_child_of_moov = APar_FindLastChild_of_ParentAtom(dynUpd.moov_atom->AtomicNumber);
			APar_MoveAtom(dynUpd.moov_udta_atom->AtomicNumber, parsedAtoms[last_child_of_moov].NextAtomNumber);
		}
		
		free(other_track_level_atom);
		other_track_level_atom = NULL;
	}
	
	if (mdat_test_only) {
		APar_FindPadding(mdat_test_only);
		return;
	}
	
	/* -----------delete free/skip atoms that precede media data or meta data--------- */
	if (dynUpd.first_otiose_freespace_atom != NULL && !alter_original) { //as a courtesty, for a full rewrite, eliminate L1 pre-mdat free/skip atoms 
		AtomicInfo* free_space = dynUpd.first_otiose_freespace_atom;
		while (true) {
			if (free_space->AtomicLevel > 1) break;
			if (memcmp(free_space->AtomicName, "free", 4) != 0) break; //only get the consecutive 'free' space
			AtomicInfo* nextatom = &parsedAtoms[free_space->NextAtomNumber];
			APar_EliminateAtom(free_space->AtomicNumber, free_space->NextAtomNumber);
			free_space = nextatom;
		}
	}	
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Determine Atom Length                                    //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_DetermineNewFileLength

    Sum up level 1 atoms (excludes any extranous EOF null bytes that iTunes occasionally writes - or used to)
----------------------*/
void APar_DetermineNewFileLength() {
	new_file_size = 0;
	short thisAtomNumber = 0;
	while (true) {		
		if (parsedAtoms[thisAtomNumber].AtomicLevel == 1) {
			if (parsedAtoms[thisAtomNumber].AtomicLengthExtended == 0) {
				//normal 32-bit number when AtomicLengthExtended == 0 (for run-o-the-mill mdat & mdat.length=0)
				new_file_size += parsedAtoms[thisAtomNumber].AtomicLength; //used in progressbar
			} else {
				//pseudo 64-bit mdat length
				new_file_size += parsedAtoms[thisAtomNumber].AtomicLengthExtended; //used in progressbar
			}
			if (parsedAtoms[thisAtomNumber].AtomicLength == 0) {				
				new_file_size += (uint32_t)file_size - parsedAtoms[thisAtomNumber].AtomicStart; //used in progressbar; mdat.length = 1
			}
		}
		if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
			break;
		}
		thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
	}
	return;
}

/*----------------------
APar_DetermineAtomLengths

    Working backwards from the last atom in the tree, for each parent atom, add the lengths of all the children atoms. All atom lengths for all parent atoms are
		recalculated from the lenghts of children atoms directly under the parent. Even atoms in hieararchies that were not touched are recalculated. Accommodations
		are made for certain dual-state atoms that are parents & contain data/versioning - the other dual-state atoms do not need to be listed because they exist
		within the stsd hierarchy which is only parsed when viewing the tree (for setting tags, it remains monolithic). 
----------------------*/
void APar_DetermineAtomLengths() {	
	short rev_atom_loop = APar_FindLastAtom();
	//fprintf(stdout, "Last atom is named %s, num:%i\n", parsedAtoms[last_atom].AtomicName, parsedAtoms[last_atom].AtomicNumber);
	
	while (true) {
		short next_atom = 0;
		uint32_t atom_size = 0;
		short previous_atom = 0; //only gets used in testing for atom under stsd

		//fprintf(stdout, "current atom is named %s, num:%i\n", parsedAtoms[rev_atom_loop].AtomicName, parsedAtoms[rev_atom_loop].AtomicNumber);
		
		if (rev_atom_loop == 0) {
			break; //we seem to have hit the first atom
		} else {
			previous_atom = APar_FindPrecedingAtom(rev_atom_loop);
		}
		
		uint32_t _atom_ = UInt32FromBigEndian(parsedAtoms[rev_atom_loop].AtomicName);
		switch (_atom_) {
			//the enumerated atoms here are all of DUAL_STATE_ATOM type
			case 0x6D657461 : //'meta'
				atom_size += 12;
				break;
			
			case 0x73747364 : //'stsd'
				atom_size += 16;
				break;
				
			case 0x64726566 : //'dref'
				atom_size += 16;
				break;
				
			case 0x69696E66 : //'iinf'
				atom_size += 14;
				break;
				
			//accommodate parsing of atoms under stsd when required
			case 0x6D703473 : { //mp4s
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM && deep_atom_scan) {
					atom_size += 16;
				} else {
					atom_size += 8;
				}
				break;
			}
			case 0x73727470 :		//srtp
			case 0x72747020 : { //'rtp '
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM && deep_atom_scan) {
					atom_size += 24;
				} else {
					atom_size += 8;
				}
				break;
			}
			case 0x616C6163 :		//alac
			case 0x6D703461 :		//mp4a
			case 0x73616D72 :		//samr
			case 0x73617762 :		//sawb
			case 0x73617770 :		//sawp
			case 0x73657663 :		//sevc
			case 0x73716370 :		//sqcp
			case 0x73736D76 :		//ssmv
			case 0x64726D73 : { //drms
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM && deep_atom_scan) {
					atom_size += 36;
				} else {
					atom_size += 8;
				}
				break;
			}
			case 0x74783367  : { //tx3g
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM && deep_atom_scan) {
					atom_size += 46;
				} else {
					atom_size += 8;
				}
				break;
			}
			case 0x6D6A7032 :		//mjp2
			case 0x6D703476 :		//mp4v
			case 0x61766331 :		//avc1
			case 0x6A706567 :		//jpeg
			case 0x73323633 :		//s263
			case 0x64726D69 : { //drmi
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM && deep_atom_scan) {
					atom_size += 86;
				} else {
					atom_size += 8;
				}
				break;
			}
			
			default :
				if (parsedAtoms[rev_atom_loop].AtomicLevel == 7 && parsedAtoms[rev_atom_loop].AtomicContainerState == DUAL_STATE_ATOM) {
					atom_size += parsedAtoms[rev_atom_loop].AtomicLength;
				} else {
					atom_size += 8; //all atoms have *at least* 4bytes length & 4 bytes name
				}
				break;
		}
		
		if (parsedAtoms[rev_atom_loop].NextAtomNumber != 0) {
			next_atom = parsedAtoms[rev_atom_loop].NextAtomNumber;
		}
		
		while (parsedAtoms[next_atom].AtomicLevel > parsedAtoms[rev_atom_loop].AtomicLevel) { // eval all child atoms....
			//fprintf(stdout, "\ttest child atom %s, level:%i (sum %u)\n", parsedAtoms[next_atom].AtomicName, parsedAtoms[next_atom].AtomicLevel, atom_size);
			if (parsedAtoms[rev_atom_loop].AtomicLevel == ( parsedAtoms[next_atom].AtomicLevel - 1) ) { // only child atoms 1 level down
				atom_size += parsedAtoms[next_atom].AtomicLength;
				//fprintf(stdout, "\t\teval child atom %s, level:%i (sum %u)\n", parsedAtoms[next_atom].AtomicName, parsedAtoms[next_atom].AtomicLevel, atom_size); 
				//fprintf(stdout, "\t\teval %s's child atom %s, level:%i (sum %u, added %u)\n", parsedAtoms[previous_atom].AtomicName, parsedAtoms[next_atom].AtomicName, parsedAtoms[next_atom].AtomicLevel, atom_size, parsedAtoms[next_atom].AtomicLength);
			} else if (parsedAtoms[next_atom].AtomicLevel < parsedAtoms[rev_atom_loop].AtomicLevel) {
				break;
			}
			next_atom = parsedAtoms[next_atom].NextAtomNumber; //increment to eval next atom
			parsedAtoms[rev_atom_loop].AtomicLength = atom_size;
		}
		
		if (_atom_ == 0x75647461 && parsedAtoms[rev_atom_loop].AtomicLevel > parsedAtoms[ parsedAtoms[rev_atom_loop].NextAtomNumber ].AtomicLevel) { //udta with no child atoms; get here by erasing the last asset in a 3gp file, and it won't quite erase because udta thinks its the former AtomicLength
			parsedAtoms[rev_atom_loop].AtomicLength = 8;
		}
		if (_atom_ == 0x6D657461 && parsedAtoms[rev_atom_loop].AtomicLevel != parsedAtoms[ parsedAtoms[rev_atom_loop].NextAtomNumber ].AtomicLevel - 1) { //meta with no child atoms; get here by erasing the last existing uuid atom.
			parsedAtoms[rev_atom_loop].AtomicLength = 12;
		}
		if (_atom_ == 0x696C7374 && parsedAtoms[rev_atom_loop].AtomicLevel != parsedAtoms[ parsedAtoms[rev_atom_loop].NextAtomNumber ].AtomicLevel - 1) { //ilst with no child atoms; get here by erasing the last piece of iTunes style metadata
			parsedAtoms[rev_atom_loop].AtomicLength = 8;
		}
		
		rev_atom_loop = APar_FindPrecedingAtom(parsedAtoms[rev_atom_loop].AtomicNumber);
		
	}
	APar_DetermineNewFileLength();
	//APar_SimpleAtomPrintout();
	//APar_PrintAtomicTree();
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Writing Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_ValidateAtoms

    A gaggle of tests go on here - to TRY to make sure that files are not corrupted.
		
		1. because there is a limit to the number of atoms, test to make sure we haven't hit MAX_ATOMS (probably only likely on a 300MB fragmented file ever 2 secs)
		2. test that the atom name is at least 4 letters long. So far, only quicktime atoms have NULLs in their names.
		3. For files over 300k, make sure that no atom can present larger than the filesize (which would be bad); handy for when the file isn't parsed correctly
		4. Test to make sure 'mdat' is at file-level. That is the only place it should ever be.
		5. If its is a child atom that was set (and resides in memory), then its AtomicData should != NULL.
		6. (A crude) Test to see if 'trak' atoms have only a 'udta' child. If setting a copyright notice on a track at index built with some compilers faux 'trak's are made
		7. If the file shunk below 90% (after accounting for additions or removals), error out - something went awry.
----------------------*/
void APar_ValidateAtoms() {
	bool atom_name_with_4_characters = true;
	short iter = 0;
	uint64_t simple_tally = 0;
	uint8_t atom_ftyp_count = 0;
	uint16_t external_data = 0;
	
	//test1
	if (atom_number > MAX_ATOMS) {
		fprintf(stderr, "AtomicParsley error: amount of atoms exceeds internal limit. Aborting.\n");
		exit(1);
	}
	
	while (true) {
		//test2
		// there are valid atom names that are 0x00000001 - but I haven't seen them in MPEG-4 files, but they could show up, so this isn't a hard error
		if ( strlen(parsedAtoms[iter].AtomicName) < 4 && parsedAtoms[iter].AtomicClassification != EXTENDED_ATOM) {
			atom_name_with_4_characters = false;
		}
		
		//test3
		//test for atoms that are going to be greater than out current file size; problem is we could be adding a 1MB pix to a 200k 3gp file; only fail for a file > 300k file; otherwise there would have to be more checks (like artwork present, but a zealous tagger could make moov.lengt > filzesize)
		if (parsedAtoms[iter].AtomicLength > (uint32_t)file_size && file_size > 300000) {
			if (parsedAtoms[iter].AtomicData == NULL) {
				fprintf(stderr, "AtomicParsley error: an atom was detected that presents as larger than filesize. Aborting. %c\n", '\a');
#if defined (_MSC_VER) /* apparently, long long is forbidden there*/
				fprintf(stderr, "atom %s is %u bytes long which is greater than the filesize of %llu\n", parsedAtoms[iter].AtomicName, parsedAtoms[iter].AtomicLength, (long unsigned int)file_size);
#else
				fprintf(stderr, "atom %s is %u bytes long which is greater than the filesize of %llu\n", parsedAtoms[iter].AtomicName, parsedAtoms[iter].AtomicLength, (long long unsigned int)file_size);
#endif
				exit(1); //its conceivable to repair such an off length by the surrounding atoms constrained by file_size - just not anytime soon; probly would catch a foobar2000 0.9 tagged file
			}
		}
		
		if (parsedAtoms[iter].AtomicLevel == 1) {
			if (parsedAtoms[iter].AtomicLength == 0 && strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0) {
				simple_tally = (uint64_t)file_size - parsedAtoms[iter].AtomicStart;
			} else {
				simple_tally += parsedAtoms[iter].AtomicLength == 1 ? parsedAtoms[iter].AtomicLengthExtended : parsedAtoms[iter].AtomicLength;
			}
		}
		
		//test4
		if (strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0 && parsedAtoms[iter].AtomicLevel != 1) {
			fprintf(stderr, "AtomicParsley error: mdat atom was found not at file level. Aborting. %c\n", '\a');
			exit(1); //the error which forced this was some bad atom length redetermination; probably won't be fixed
		}
		//test5
		if (parsedAtoms[iter].AtomicStart == 0 && parsedAtoms[iter].AtomicData == NULL &&
		    parsedAtoms[iter].AtomicNumber > 0 && parsedAtoms[iter].AtomicContainerState == CHILD_ATOM) {
			fprintf(stderr, "AtomicParsley error: a '%s' atom was rendered to NULL length. Aborting. %c\n", parsedAtoms[iter].AtomicName, '\a');
			exit(1); //data was not written to AtomicData for this new atom.
		}
		
		//test6
		if (memcmp(parsedAtoms[iter].AtomicName, "trak", 4) == 0 && parsedAtoms[iter+1].NextAtomNumber != 0) { //prevent writing any malformed tracks
			if (!(memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "tkhd", 4) == 0 || 
			    memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "tref", 4) == 0 ||
					memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "mdia", 4) == 0 ||
					memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "edts", 4) == 0 ||
					memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "meta", 4) == 0 ||
					memcmp(parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, "tapt", 4) == 0 )) {
				if (APar_ReturnChildrenAtoms(iter, 0) < 2) {
					//a 'trak' must contain 'tkhd' & 'mdia' at the very least
					fprintf(stderr, "AtomicParsley error: incorrect track structure containing atom %s. %c\n", parsedAtoms[ parsedAtoms[iter].NextAtomNumber ].AtomicName, '\a');
					exit(1);
				}
			}
		}
		
		if (memcmp(parsedAtoms[iter].AtomicName, "ftyp", 4) == 0) {
			atom_ftyp_count++;
		}
		
		if ( (memcmp(parsedAtoms[iter].AtomicName, "stco", 4) == 0 || memcmp(parsedAtoms[iter].AtomicName, "co64", 4) == 0) && parsedAtoms[iter].ancillary_data != 1) {
			external_data++;
		}
		iter=parsedAtoms[iter].NextAtomNumber;
		if (iter == 0) {
			break;
		}
	}
	
	//test7
	double perdiff = (float)((float)((uint32_t)simple_tally) * 100.0 / (double)(file_size-removed_bytes_tally) );
	int percentage_difference = (int)lroundf((float)perdiff);
	
	if (percentage_difference < 90 && file_size > 300000) { //only kick in when files are over 300k & 90% of the size
		fprintf(stderr, "AtomicParsley error: total existing atoms present as larger than filesize. Aborting. %c\n", '\a');
		//APar_PrintAtomicTree();
		fprintf(stdout, "%i %llu\n", percentage_difference, simple_tally);
		exit(1);
	}
	
	if (atom_ftyp_count != 1) {
		fprintf(stdout, "AtomicParsley error: unresolved looping of atoms. Aborting. %c\n", '\a');
		exit(1);
	}
	
	if (!atom_name_with_4_characters) {
		fprintf(stdout, "AtomicParsley warning: atom(s) were detected with atypical names containing NULLs\n");
	}
	
	if (external_data > 0) {
		fprintf(stdout, "AtomicParsley warning: externally referenced data found.");
	}
	
	return;
}

void APar_DeriveNewPath(const char *filePath, char* temp_path, int output_type, const char* file_kind, char* forced_suffix, bool random_filename = true) {
	char* suffix = NULL;
	char* file_name = NULL;
	size_t file_name_len = 0;
	bool relative_path = false;
	
	if (forced_suffix == NULL) {
		suffix = strrchr(filePath, '.');
	} else {
		suffix = forced_suffix;
	}
	
	size_t filepath_len = strlen(filePath);
	size_t base_len = filepath_len-strlen(suffix);
	if (output_type >= 0) {
		memcpy(temp_path, filePath, base_len);
		memcpy(temp_path + base_len, file_kind, strlen(file_kind));
		
	} else if (output_type == -1) { //make the output file invisible by prefacing the filename with '.'
#if defined (_MSC_VER)
		memcpy(temp_path, filePath, base_len);
		memcpy(temp_path + base_len, file_kind, strlen(file_kind));
#else
		file_name = strrchr(filePath, '/');
		if (file_name != NULL) {
			file_name_len = strlen(file_name);
			memcpy(temp_path, filePath, filepath_len-file_name_len+1);
		} else {
			getcwd(temp_path, MAXPATHLEN);
			file_name = (char*)filePath;
			file_name_len = strlen(file_name);
			memcpy(temp_path + strlen(temp_path), "/", 1);
			relative_path = true;
		}
		memcpy(temp_path + strlen(temp_path), ".", 1);
		memcpy(temp_path + strlen(temp_path), (relative_path ? file_name : file_name+1), file_name_len - strlen(suffix) -1);
		memcpy(temp_path + strlen(temp_path), file_kind, strlen(file_kind));
#endif
	}
	
	if (random_filename) {
		char randstring[6];
		srand((int) time(NULL)); //Seeds rand()
		int randNum = rand()%100000;
		sprintf(randstring, "%i", randNum);
		memcpy(temp_path + strlen(temp_path), randstring, strlen(randstring));
	}
	
	if (forced_suffix_type == FORCE_M4B_TYPE) {
		memcpy(temp_path + strlen(temp_path), ".m4b", 5);
	} else {
		memcpy(temp_path + strlen(temp_path), suffix, strlen(suffix) );
	}
	return;
}

void APar_MetadataFileDump(const char* ISObasemediafile) {
	char* dump_file_name=(char*)malloc( sizeof(char)* (strlen(ISObasemediafile) +12 +1) );
	memset(dump_file_name, 0, sizeof(char)* (strlen(ISObasemediafile) +12 +1) );
	
	FILE* dump_file;
	AtomicInfo* userdata_atom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);
	
	//make sure that the atom really exists
	if (userdata_atom != NULL) {
		char* dump_buffer=(char*)malloc( sizeof(char)* userdata_atom->AtomicLength +1 );
		memset(dump_buffer, 0, sizeof(char)* userdata_atom->AtomicLength +1 );
	
		APar_DeriveNewPath(ISObasemediafile, dump_file_name, 1, "-dump-", ".raw");
		dump_file = APar_OpenFile(dump_file_name, "wb");
		if (dump_file != NULL) {
			//body of atom writing here
			
			fseeko(source_file, userdata_atom->AtomicStart, SEEK_SET);
			fread(dump_buffer, 1, (size_t)userdata_atom->AtomicLength, source_file);
			
			fwrite(dump_buffer, (size_t)userdata_atom->AtomicLength, 1, dump_file);
			fclose(dump_file);
		
			fprintf(stdout, " Metadata dumped to %s\n", dump_file_name);
		}
		free(dump_buffer);
		dump_buffer=NULL;
		
	} else {
		fprintf(stdout, "AtomicParsley error: no moov.udta atom was found to dump out to file.\n");
	}
	
	return;
}

void APar_UpdateModTime(AtomicInfo* container_header_atom) {
	container_header_atom->AtomicData = (char*)calloc(1, sizeof(char)* (size_t)container_header_atom->AtomicLength );
	APar_readX(container_header_atom->AtomicData, source_file, container_header_atom->AtomicStart+12, container_header_atom->AtomicLength-12);

	uint32_t current_time = APar_get_mpeg4_time();
	if (container_header_atom->AtomicVerFlags & 0xFFFFFF == 1) {
		UInt64_TO_String8((uint64_t)current_time, container_header_atom->AtomicData+8);
	} else {
		UInt32_TO_String4(current_time, container_header_atom->AtomicData+4);
	}
	return;
}

void APar_ShellProgressBar(uint32_t bytes_written) {
	if (dynUpd.updage_by_padding) {
		return;
	}
	strcpy(file_progress_buffer, " Progress: ");
	
	double dispprog = (double)bytes_written/(double)new_file_size * 100.0 *( (double)max_display_width/100.0);
	int display_progress = (int)lroundf((float)dispprog);
	double percomp = 100.0 * (double)((double)bytes_written/ (double)new_file_size);
	int percentage_complete = (int)lroundf((float)percomp);
		
	for (int i = 0; i <= max_display_width; i++) {
		if (i < display_progress ) {
			strcat(file_progress_buffer, "=");
		} else if (i == display_progress) {
			sprintf(file_progress_buffer, "%s>%d%%", file_progress_buffer, percentage_complete);
		} else {
			strcat(file_progress_buffer, "-");
		}
	}
	if (percentage_complete < 100) {
		strcat(file_progress_buffer, "-");
		if (percentage_complete < 10) {
			strcat(file_progress_buffer, "-");
		}
	}
	
	strcat(file_progress_buffer, "|");
		
	fprintf(stdout, "%s\r", file_progress_buffer);
	fflush(stdout);
	return;
}

void APar_MergeTempFile(FILE* dest_file, FILE *src_file, uint32_t src_file_size, uint32_t dest_position, char* &buffer) {
	uint32_t file_pos = 0;
	while (file_pos <= src_file_size) {
		if (file_pos + max_buffer <= src_file_size ) {
			fseeko(src_file, file_pos, SEEK_SET);
			fread(buffer, 1, (size_t)max_buffer, src_file);
			
			//fseek(dest_file, dest_position + file_pos, SEEK_SET);
#if defined(_MSC_VER)
			fpos_t file_offset = dest_position + file_pos;
#elif defined(__GLIBC__)
      fpos_t file_offset = {0};
			file_offset.__pos = dest_position + file_pos;
#else
			off_t file_offset = dest_position + file_pos;
#endif
			fsetpos(dest_file, &file_offset);
			fwrite(buffer, (size_t)max_buffer, 1, dest_file);
			file_pos += max_buffer;
			
		} else {
			fseeko(src_file, file_pos, SEEK_SET);
			fread(buffer, 1, (size_t)(src_file_size - file_pos), src_file);
			//fprintf(stdout, "buff starts with %s\n", buffer+4);
#if defined(_MSC_VER)
			fpos_t file_offset = dest_position + file_pos;
#elif defined(__GLIBC__)
      fpos_t file_offset = {0};
			file_offset.__pos = dest_position + file_pos;
#else
			off_t file_offset = dest_position + file_pos;
#endif
			fsetpos(dest_file, &file_offset );
			fwrite(buffer, (size_t)(src_file_size - file_pos), 1, dest_file);
			file_pos += src_file_size - file_pos;
			break;
		}		
	}
	if (dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV) {
#if defined (WIN32)
		fflush(dest_file);
		SetEndOfFile((HANDLE)_get_osfhandle(fileno(dest_file)));
#else
		ftruncate(fileno(dest_file), src_file_size+dest_position);
#endif
	}
	return;
}

uint32_t APar_WriteAtomically(FILE* source_file, FILE* temp_file, bool from_file, char* &buffer, uint32_t bytes_written_tally, short this_atom) {
	uint32_t bytes_written = 0;
	
	if (parsedAtoms[this_atom].AtomicLength > 1 && parsedAtoms[this_atom].AtomicLength < 8) { //prevents any spurious atoms from appearing
		return bytes_written;
	}
	
	//write the length of the atom first... taken from our tree in memory
	UInt32_TO_String4(parsedAtoms[this_atom].AtomicLength, twenty_byte_buffer);
	fseeko(temp_file, bytes_written_tally, SEEK_SET);
	fwrite(twenty_byte_buffer, 4, 1, temp_file);
	bytes_written += 4;
	
	//since we have already writen the length out to the file, it can be changed now with impunity
	if (parsedAtoms[this_atom].AtomicLength == 0) { //the spec says if an atom has a length of 0, it extends to EOF
		parsedAtoms[this_atom].AtomicLength = (uint32_t)file_size - parsedAtoms[this_atom].AtomicLength;
	} else if (parsedAtoms[this_atom].AtomicLength == 1) {
		//part of the pseudo 64-bit support
		parsedAtoms[this_atom].AtomicLength = (uint32_t)parsedAtoms[this_atom].AtomicLengthExtended;
	} else if (parsedAtoms[this_atom].AtomicContainerState == DUAL_STATE_ATOM) {
		if (memcmp(parsedAtoms[this_atom].AtomicName, "dref", 4) == 0) {
			parsedAtoms[this_atom].AtomicLength = 16;
		} else if (memcmp(parsedAtoms[this_atom].AtomicName, "iinf", 4) == 0) {
			parsedAtoms[this_atom].AtomicLength = 14;
		}
	}
	
	if (deep_atom_scan && parsedAtoms[this_atom].AtomicContainerState == DUAL_STATE_ATOM) {
		uint32_t atom_val = UInt32FromBigEndian(parsedAtoms[this_atom].AtomicName);
		if (atom_val == 0x73747364 ) { //stsd
			parsedAtoms[this_atom].AtomicLength = 16;
		} else if (atom_val == 0x6D703473 ) { //mp4s
			parsedAtoms[this_atom].AtomicLength = 16;
			
		} else if (atom_val == 0x73727470 ) { //srtp
			parsedAtoms[this_atom].AtomicLength = 24;
		} else if (atom_val == 0x72747020 && parsedAtoms[this_atom].AtomicLevel == 7) { //'rtp '
			parsedAtoms[this_atom].AtomicLength = 24;
		
		} else if (atom_val == 0x616C6163 && parsedAtoms[this_atom].AtomicLevel == 7) { //alac
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x6D703461 ) { //mp4a
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73616D72 ) { //samr
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73617762 ) { //sawb
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73617770 ) { //sawp
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73657663 ) { //sevc
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73716370 ) { //sqcp
			parsedAtoms[this_atom].AtomicLength = 36;
		} else if (atom_val == 0x73736D76 ) { //ssmv
			parsedAtoms[this_atom].AtomicLength = 36;
			
		} else if (atom_val == 0x74783367 ) { //tx3g
			parsedAtoms[this_atom].AtomicLength = 46;
				
		} else if (atom_val == 0x6D6A7032 ) { //mjp2
			parsedAtoms[this_atom].AtomicLength = 86;
		} else if (atom_val == 0x6D703476 ) { //mp4v
			parsedAtoms[this_atom].AtomicLength = 86;
		} else if (atom_val == 0x61766331 ) { //avc1
			parsedAtoms[this_atom].AtomicLength = 86;
		} else if (atom_val == 0x6A706567 ) { //jpeg
			parsedAtoms[this_atom].AtomicLength = 86;
		} else if (atom_val == 0x73323633 ) { //s263
			parsedAtoms[this_atom].AtomicLength = 86;
		}
	}
	
	if (from_file) {
		// here we read in the original atom into the buffer. If the length is greater than our buffer length,
		// we loop, reading in chunks of the atom's data into the buffer, and immediately write it out, reusing the buffer.
		
		while (bytes_written <= parsedAtoms[this_atom].AtomicLength) {
			if (bytes_written + max_buffer <= parsedAtoms[this_atom].AtomicLength ) {
				//fprintf(stdout, "Writing atom %s from file looping into buffer\n", parsedAtoms[this_atom].AtomicName);
//fprintf(stdout, "Writing atom %s from file looping into buffer %u - %u | %u\n", parsedAtoms[this_atom].AtomicName, parsedAtoms[this_atom].AtomicLength, bytes_written_tally, bytes_written);
				//read&write occurs from & including atom name through end of atom
				fseeko(source_file, (bytes_written + parsedAtoms[this_atom].AtomicStart), SEEK_SET);
				fread(buffer, 1, (size_t)max_buffer, source_file);
				
				fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);
				fwrite(buffer, (size_t)max_buffer, 1, temp_file);
				bytes_written += max_buffer;
				
				APar_ShellProgressBar(bytes_written_tally + bytes_written);
				
			} else { //we either came up on a short atom (most are), or the last bit of a really long atom
				//fprintf(stdout, "Writing atom %s from file directly into buffer\n", parsedAtoms[this_atom].AtomicName);
				fseeko(source_file, (bytes_written + parsedAtoms[this_atom].AtomicStart), SEEK_SET);
				fread(buffer, 1, (size_t)(parsedAtoms[this_atom].AtomicLength - bytes_written), source_file);
				
				fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);
				fwrite(buffer, (size_t)(parsedAtoms[this_atom].AtomicLength - bytes_written), 1, temp_file);
				bytes_written += parsedAtoms[this_atom].AtomicLength - bytes_written;
				
				APar_ShellProgressBar(bytes_written_tally + bytes_written);
				
				break;
			}
		}
		
	} else { // we are going to be writing not from the file, but directly from the tree (in memory).
		uint32_t atom_name_len = 4;

		//fprintf(stdout, "Writing atom %s from memory %u\n", parsedAtoms[this_atom].AtomicName, parsedAtoms[this_atom].AtomicClassification);
		fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);
		
		if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
			fwrite("uuid", 4, 1, temp_file);
			atom_name_len = 16; //total of 20 bytes for a uuid atom
			//fprintf(stdout, "%u\n", parsedAtoms[this_atom].AtomicLength);
			if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM && parsedAtoms[this_atom].uuid_style == UUID_OTHER) bytes_written += 4;
		}
		
		fwrite(parsedAtoms[this_atom].AtomicName, atom_name_len, 1, temp_file);
		bytes_written += atom_name_len;
		if (parsedAtoms[this_atom].AtomicClassification == VERSIONED_ATOM || parsedAtoms[this_atom].AtomicClassification == PACKED_LANG_ATOM) {
			UInt32_TO_String4( (uint32_t)parsedAtoms[this_atom].AtomicVerFlags, twenty_byte_buffer);
			fwrite(twenty_byte_buffer, 4, 1, temp_file);
			bytes_written += 4;
		}
		
		uint32_t atom_data_size = 0;
		switch (parsedAtoms[this_atom].AtomicContainerState) {
			case PARENT_ATOM :
			case SIMPLE_PARENT_ATOM : {
				atom_data_size = 0;
				break;
			}
			case DUAL_STATE_ATOM : {
				switch ( UInt32FromBigEndian(parsedAtoms[this_atom].AtomicName) ) {
					case 0x6D657461 : { //meta
						break;
					}
					case 0x73747364 : { //stsd
						atom_data_size = parsedAtoms[this_atom].AtomicLength - 12;
						break;
					}
					case 0x73636869 : { //schi (code only executes when deep_atom_scan = true; otherwise schi is contained by the monolithic/unparsed 'stsd')
						atom_data_size = parsedAtoms[this_atom].AtomicLength - 12;
					}
				}
				break;
			}
			case UNKNOWN_ATOM_TYPE :
			case CHILD_ATOM : {
				if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM && parsedAtoms[this_atom].uuid_style == UUID_AP_SHA1_NAMESPACE) {
					//4bytes length, 4 bytes 'uuid', 4bytes name, 4bytes NULL (AP writes its own uuid atoms - not those copied - iTunes style with atom versioning)
					atom_data_size = parsedAtoms[this_atom].AtomicLength - (16 + 12); //16 uuid; 16 = 4bytes * ('uuid', ap_uuid_name, verflag, 4 NULL bytes)
				} else if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM && parsedAtoms[this_atom].uuid_style != UUID_DEPRECATED_FORM) {
					atom_data_size = parsedAtoms[this_atom].AtomicLength - (16 + 8);
				} else if (parsedAtoms[this_atom].AtomicClassification == VERSIONED_ATOM || parsedAtoms[this_atom].AtomicClassification == PACKED_LANG_ATOM) {
					//4bytes legnth, 4bytes name, 4bytes flag&versioning (language would be 2 bytes, but because its in different places, it gets stored as data)
					atom_data_size = parsedAtoms[this_atom].AtomicLength - 12;
				} else {
					//just 4bytes length, 4bytes name and then whatever data
					atom_data_size = parsedAtoms[this_atom].AtomicLength - 8;
				}
				break;
			}
		}
		
		if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM && parsedAtoms[this_atom].uuid_style == UUID_AP_SHA1_NAMESPACE) {
			//AP writes uuid atoms much like iTunes style metadata; with version/flags to connote what type of data is being carried
		  //4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
			fwrite(parsedAtoms[this_atom].uuid_ap_atomname, 4, 1, temp_file);
			bytes_written += 4;
			
			UInt32_TO_String4( (uint32_t)parsedAtoms[this_atom].AtomicVerFlags, twenty_byte_buffer);
			fwrite(twenty_byte_buffer, 4, 1, temp_file);
			bytes_written += 4;
		}
		
		if (atom_data_size > 0) {
			fwrite(parsedAtoms[this_atom].AtomicData, atom_data_size, 1, temp_file);
			bytes_written += atom_data_size;
			
			APar_ShellProgressBar(bytes_written_tally + bytes_written);
		}
	}
	return bytes_written;
}

/*----------------------
APar_copy_gapless_padding
	mp4file - destination file
	last_atom_pos - the last byte in the destination file that is contained by any atom (in parsedAtoms[] array)
	buffer - a buffer that will be used to set & write out from the NULLs used in gapless padding

    Add the discovered amount of already present gapless void padding at the end of the file (which is *not* contained by any atom at all) back into the destination
		file.
		
		Update: it would seem that this gapless void padding at the end of the file is not critical to gapless playback. In my 1 test of the thing, it seemed to work
		regardless of whether this NULL space was present or not, 'pgap' seemed to work. But, since Apple put it in for some reason, it will be left there unless explicity
		directed not to (via AP_PADDING). Although tying ordinary padding to this gapless padding may reduce flexibility - the assumption is that someone interested in
		squeezing out wasted space would want to eliminate this wasted space too (and so far, it does seem wasted).
		
		NOTE: Apple seems not to have seen this portion of the ISO 14496-12 Annex A, section A.2, para 6:		
		"All the data within a conforming file is encapsulated in boxes (called atoms in predecessors of this file format). There is no data outside the box structure."
		And yet, Apple (donators of the file format) has caused iTunes to create non-conforming files with iTunes 7.x because of this NULL data outside of any box/atom
		structure.
		
----------------------*/
void APar_copy_gapless_padding(FILE* mp4file, uint32_t last_atom_pos, char* buffer) {
	uint32_t gapless_padding_bytes_written = 0;
	while (gapless_padding_bytes_written < gapless_void_padding) {
		if (gapless_padding_bytes_written + max_buffer <= gapless_void_padding ) {
			memset(buffer, 0, max_buffer);
			
			fseeko(mp4file, (last_atom_pos + gapless_padding_bytes_written), SEEK_SET);
			fwrite(buffer, (size_t)max_buffer, 1, mp4file);
			gapless_padding_bytes_written += max_buffer;
			
		} else { //less then 512k of gapless padding (here's hoping we get here always)
			memset(buffer, 0, (gapless_void_padding - gapless_padding_bytes_written) );
			
			fseeko(mp4file, (last_atom_pos + gapless_padding_bytes_written), SEEK_SET);
			fwrite(buffer, (size_t)(gapless_void_padding - gapless_padding_bytes_written), 1, mp4file);
			gapless_padding_bytes_written+= (gapless_void_padding - gapless_padding_bytes_written);
			break;
		}
	}
	return;
}

void APar_WriteFile(const char* ISObasemediafile, const char* outfile, bool rewrite_original) {
	char* temp_file_name=(char*)calloc(1, sizeof(char)* 3500 );
	char* file_buffer=(char*)calloc(1, sizeof(char)* max_buffer + 1 );
	FILE* temp_file;
	uint32_t temp_file_bytes_written = 0;
	short thisAtomNumber = 0;
	char* originating_file = NULL;
	bool free_modified_name = false;
	
	APar_RenderAllID32Atoms();
	
	if (!(psp_brand || force_existing_hierarchy)) {
		APar_Optimize(false);
	} else {
		APar_LocateAtomLandmarks();
	}
	
	APar_FindPadding(false);
	APar_ConsolidatePadding();
	APar_DetermineAtomLengths();

	if (!complete_free_space_erasure) {
		APar_DetermineDynamicUpdate();
	}
	
	if (!rewrite_original || dynUpd.prevent_dynamic_update) {
		dynUpd.updage_by_padding = false;
	}
	
	APar_ValidateAtoms();
	
	//whatever atoms/space comes before mdat has to be added/removed before this point, or chunk offsets (in stco, co64, tfhd) won't be properly determined
	uint32_t mdat_position = APar_DetermineMediaData_AtomPosition(); 
	
	if (dynUpd.updage_by_padding) {
		APar_DeriveNewPath(ISObasemediafile, temp_file_name, 0, "-data-", NULL); //APar_DeriveNewPath(ISObasemediafile, temp_file_name, -1, "-data-", NULL);
		temp_file = APar_OpenFile(temp_file_name, "wb");
#if defined (_MSC_VER)
		char* invisi_command=(char*)malloc(sizeof(char)*2*MAXPATHLEN);
		memset( invisi_command, 0, 2*MAXPATHLEN+1);
		memcpy( invisi_command, "ATTRIB +S +H ", 13);
		memcpy( invisi_command + strlen(invisi_command), temp_file_name, strlen(temp_file_name) );
		
		if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
			wchar_t* invisi_command_long = Convert_multibyteUTF8_to_wchar(invisi_command);
		
			_wsystem(invisi_command_long);
		
			free(invisi_command_long);
			invisi_command_long = NULL;
		} else {
			system(invisi_command);
		}
		free(invisi_command);
		invisi_command = NULL;
#endif
	
	} else if (!outfile) {
		APar_DeriveNewPath(ISObasemediafile, temp_file_name, 0, "-temp-", NULL);
		temp_file = APar_OpenFile(temp_file_name, "wb");
		
#if defined (DARWIN_PLATFORM)
		APar_SupplySelectiveTypeCreatorCodes(ISObasemediafile, temp_file_name, forced_suffix_type); //provide type/creator codes for ".mp4" for randomly named temp files
#endif
		
	} else {
		//case-sensitive compare means "The.m4a" is different from "THe.m4a"; on certiain Mac OS X filesystems a case-preservative but case-insensitive FS exists &
		//AP probably will have a problem there. Output to a uniquely named file as I'm not going to poll the OS for the type of FS employed on the target drive.
		if (strncmp(ISObasemediafile,outfile,strlen(outfile)) == 0 && (strlen(outfile) == strlen(ISObasemediafile)) ) {
			//er, nice try but you were trying to ouput to the exactly named file of the original. Y'all ain't so slick
			APar_DeriveNewPath(ISObasemediafile, temp_file_name, 0, "-temp-", NULL);
			temp_file = APar_OpenFile(temp_file_name, "wb");
			
#if defined (DARWIN_PLATFORM)
			APar_SupplySelectiveTypeCreatorCodes(ISObasemediafile, temp_file_name, forced_suffix_type); //provide type/creator codes for ".mp4" for a fall-through randomly named temp files
#endif
		
		} else {
			temp_file = APar_OpenFile(outfile, "wb");
			
#if defined (DARWIN_PLATFORM)
			APar_SupplySelectiveTypeCreatorCodes(ISObasemediafile, outfile, forced_suffix_type); //provide type/creator codes for ".mp4" for a user-defined output file
#endif
			
			}
	}
	
	if (temp_file != NULL) { //body of atom writing here
		
		if (dynUpd.updage_by_padding) {
			thisAtomNumber = dynUpd.initial_update_atom->AtomicNumber;
			fprintf(stdout, "\n Updating metadata... ");
		} else {
			fprintf(stdout, "\n Started writing to temp file.\n");
		}
		
		while (true) {
			
			AtomicInfo* thisAtom = &parsedAtoms[thisAtomNumber];
			if (thisAtom->AtomicNumber == -1) break;
			
			//the loop where the critical determination is made
			if (memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && dynUpd.updage_by_padding) break;
			
			if (thisAtom->ancillary_data == 0x666C6167) { //'flag'
				APar_UpdateModTime(thisAtom);
			}
			
			if (memcmp(thisAtom->AtomicName, "stco", 4) == 0) {
				bool readjusted_stco = APar_Readjust_STCO_atom(mdat_position, thisAtomNumber);
				
				temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, !readjusted_stco, file_buffer, temp_file_bytes_written, thisAtomNumber);
					
			} else if (memcmp(thisAtom->AtomicName, "co64", 4) == 0) {
				bool readjusted_co64 = APar_Readjust_CO64_atom(mdat_position, thisAtomNumber);
				
				temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, !readjusted_co64, file_buffer, temp_file_bytes_written, thisAtomNumber);
				
			} else if (memcmp(thisAtom->AtomicName, "tfhd", 4) == 0) {
				bool readjusted_tfhd = APar_Readjust_TFHD_fragment_atom(mdat_position, thisAtomNumber);
				
				temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, !readjusted_tfhd, file_buffer, temp_file_bytes_written, thisAtomNumber);
				
			} else if (memcmp(thisAtom->AtomicName, "iloc", 4) == 0) {
				bool readjusted_iloc = APar_Readjust_iloc_atom(thisAtomNumber);
				
				temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, !readjusted_iloc, file_buffer, temp_file_bytes_written, thisAtomNumber);
				
			} else if (thisAtom->AtomicData != NULL || memcmp(thisAtom->AtomicName, "meta", 4) == 0) {
				temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, false, file_buffer, temp_file_bytes_written, thisAtomNumber);
				
			} else {
				//write out parent atoms (the standard kind that are only offset & name from the tree in memory (total: 8bytes)
				if ( thisAtom->AtomicContainerState <= SIMPLE_PARENT_ATOM ) {
					temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, false, file_buffer, temp_file_bytes_written, thisAtomNumber);
				//or its a child (they invariably contain some sort of data.
				} else {
					temp_file_bytes_written += APar_WriteAtomically(source_file, temp_file, true, file_buffer, temp_file_bytes_written, thisAtomNumber);
				}
			}
			if (thisAtom->NextAtomNumber == 0) { //if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
				//fprintf(stdout, "The loop is buh-rokin\n");
				break;
			}
			
			//prevent any looping back to atoms already written
			thisAtom->AtomicNumber = -1;
			thisAtomNumber = thisAtom->NextAtomNumber;
		}
		if (!dynUpd.updage_by_padding) {
			if (gapless_void_padding > 0 && pad_prefs.default_padding_size > 0) { //only when some sort of padding is wanted will the gapless null padding be copied
				APar_copy_gapless_padding(temp_file, temp_file_bytes_written, file_buffer);
			}
			fprintf(stdout, "\n Finished writing to temp file.\n");
			fclose(temp_file);
		}
		
	} else {
		fprintf(stdout, "AtomicParsley error: an error occurred while trying to create a temp file.\n");
		exit(1);
	}
	
	if (dynUpd.updage_by_padding && rewrite_original) {
		fclose(temp_file);
		uint32_t metadata_len = (uint32_t)findFileSize(temp_file_name);

		temp_file = APar_OpenFile(temp_file_name, "rb");
		fclose(source_file);
		source_file = APar_OpenFile(ISObasemediafile, "r+b");
		if (source_file == NULL) {
			fclose(temp_file);
			remove(temp_file_name);
			fprintf(stdout, "AtomicParsley error: the original file was no longer found.\nExiting.\n");
			exit(1);
		} else if (!(dynUpd.optimization_flags & MEDIADATA__PRECEDES__MOOV) && metadata_len != (dynUpd.first_mdat_atom->AtomicStart - dynUpd.initial_update_atom->AtomicStart)) {
			fclose(temp_file);
			remove(temp_file_name);
			fprintf(stdout, "AtomicParsley error: the insuffiecient space to retag the source file (%u!=%u).\nExiting.\n", metadata_len, dynUpd.first_mdat_atom->AtomicStart - dynUpd.initial_update_atom->AtomicStart);
			exit(1);		
		}
		
		APar_MergeTempFile(source_file, temp_file, temp_file_bytes_written, dynUpd.initial_update_atom->AtomicStart, file_buffer);
	
		fclose(source_file);
		fclose(temp_file);
		remove(temp_file_name);

	} else if (rewrite_original && !outfile) { //disable overWrite when writing out to a specifically named file; presumably the enumerated output file was meant to be the final destination
		fclose(source_file);

		if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER) /* native windows seems to require removing the file first; rename() on Mac OS X does the removing automatically as needed */
			wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(ISObasemediafile);
		
			_wremove(utf16_filepath);
		
			free(utf16_filepath);
			utf16_filepath = NULL;
#endif
		} else {
			remove(ISObasemediafile);
		}
		
		int err = 0;
		
		if (forced_suffix_type != NO_TYPE_FORCING) {
			originating_file = (char*)calloc( 1, sizeof(char)* 3500 );
			free_modified_name = true;
			if (forced_suffix_type == FORCE_M4B_TYPE) { //using --stik Audiobook with --overWrite will change the original file's extension
				uint16_t filename_len = strlen(ISObasemediafile);
				char* suffix = strrchr(ISObasemediafile, '.');
				memcpy(originating_file, ISObasemediafile, filename_len+1 );
				memcpy(originating_file + (filename_len - strlen(suffix) ), ".m4b", 5 );
			}
		} else {
			originating_file = (char*)ISObasemediafile;
		}
		
		if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
			wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(originating_file);
			wchar_t* temp_utf16_filepath = Convert_multibyteUTF8_to_wchar(temp_file_name);
		
			err = _wrename(temp_utf16_filepath, utf16_filepath);
		
			free(utf16_filepath);
			free(temp_utf16_filepath);
			utf16_filepath = NULL;
			temp_utf16_filepath = NULL;
#endif
		} else {
			err = rename(temp_file_name, originating_file);
		}
		
		if (err != 0) {
			switch (errno) {
				
				case ENAMETOOLONG: {
					fprintf (stdout, "Some or all of the orginal path was too long.");
					exit (-1);
				}
				case ENOENT: {
					fprintf (stdout, "Some part of the original path was missing.");
					exit (-1);
				}
				case EACCES: {
					fprintf (stdout, "Unable to write to a directory lacking write permission.");
					exit (-1);
				}
				case ENOSPC: {
					fprintf (stdout, "Out of space.");
					exit (-1);
				}
			}
		}
	}
	
	free(temp_file_name);
	if (free_modified_name) free(originating_file);
	temp_file_name=NULL;
	free(file_buffer);
	file_buffer = NULL;
	
	return;
}
