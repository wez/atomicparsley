//==================================================================//
/*
    AtomicParsley - metalist.cpp

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

bool BOM_printed = false;

uint32_t APar_ProvideTallyForAtom(const char* atom_name) {
	uint32_t tally_for_atom = 0;
	short iter = parsedAtoms[0].NextAtomNumber;
	while (true) {
		if (memcmp(parsedAtoms[iter].AtomicName, atom_name, 4) == 0) {
			if (parsedAtoms[iter].AtomicLength == 0) {
				tally_for_atom += file_size - parsedAtoms[iter].AtomicStart;
			} else if (parsedAtoms[iter].AtomicLength == 1) {
				tally_for_atom += parsedAtoms[iter].AtomicLengthExtended;
			} else {
				tally_for_atom += parsedAtoms[iter].AtomicLength;
			}
		}
		if (iter == 0) {
			break;
		} else {
			iter=parsedAtoms[iter].NextAtomNumber;
		}
	}
	return tally_for_atom;
}

void printBOM() {
	if (BOM_printed) return;
	
#if defined (_WIN32) && !defined (__CYGWIN__)
	if (UnicodeOutputStatus == WIN32_UTF16) {
		APar_unicode_win32Printout(L"\xEF\xBB\xBF", "\xEF\xBB\xBF");
	}
#else
	fprintf(stdout, "\xEF\xBB\xBF"); //Default to output of a UTF-8 BOM
#endif

	BOM_printed = true;
	return;
}

#if defined (_WIN32) && !defined (__CYGWIN__)
void APar_unicode_win32Printout(wchar_t* unicode_out, char* utf8_out) { //based on http://blogs.msdn.com/junfeng/archive/2004/02/25/79621.aspx
	//its possible that this isn't even available on windows95
	DWORD dwBytesWritten;
	DWORD fdwMode;
	HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// ThreadLocale adjustment, resource loading, etc. is skipped
	if ( (GetFileType(outHandle) & FILE_TYPE_CHAR) && GetConsoleMode( outHandle, &fdwMode) ) {
		if ( wcsncmp(unicode_out, L"\xEF\xBB\xBF", 3) != 0 ) { //skip BOM when writing directly to the console
			WriteConsoleW( outHandle, unicode_out, wcslen(unicode_out), &dwBytesWritten, 0);
		}
	} else {
		//writing out to a file. Everything will be written out in utf8 to the file.
		fprintf(stdout, "%s", utf8_out);
	}
	return;
}
#endif

void APar_fprintf_UTF8_data(const char* utf8_encoded_data) {
#if defined (_WIN32) && !defined (__CYGWIN__)
	if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) {
		fprintf(stdout, "%s", utf8_encoded_data); //just printout the raw utf8 bytes (not characters) under pre-NT windows
	} else {
		wchar_t* utf16_data = Convert_multibyteUTF8_to_wchar(utf8_encoded_data);
		fflush(stdout);
		
		APar_unicode_win32Printout(utf16_data, (char *) utf8_encoded_data);
		
		fflush(stdout);
		free(utf16_data);
		utf16_data = NULL;
	}
#else
	fprintf(stdout, "%s", utf8_encoded_data);
#endif
	return;
}

void APar_Mark_UserData_area(uint8_t track_num, short userdata_atom, bool quantum_listing) {
	if (quantum_listing && track_num > 0) {
		fprintf(stdout, "User data; level: track=%u; atom \"%s\" ", track_num, parsedAtoms[userdata_atom].AtomicName);
	} else if (quantum_listing && track_num == 0) {
		fprintf(stdout, "User data; level: movie; atom \"%s\" ", parsedAtoms[userdata_atom].AtomicName);
	} else {
		fprintf(stdout, "User data \"%s\" ", parsedAtoms[userdata_atom].AtomicName);
	}
	return;
}

//the difference between APar_PrintUnicodeAssest above and APar_SimplePrintUnicodeAssest below is:
//APar_PrintUnicodeAssest contains the entire contents of the atom, NULL bytes and all
//APar_SimplePrintUnicodeAssest contains a purely unicode string (either utf8 or utf16 with BOM)
//and slight output formatting differences

void APar_SimplePrintUnicodeAssest(char* unicode_string, int asset_length, bool print_encoding) { //3gp files
	if (strncmp(unicode_string, "\xFE\xFF", 2) == 0 ) { //utf16
		if (print_encoding) {
			fprintf(stdout, " (utf16): ");
		}
		unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, asset_length * 6, asset_length);

#if defined (_WIN32) && !defined (__CYGWIN__)
		if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) { //pre-NT or AP-utf8.exe (pish, thats my win98se, and without unicows support convert utf16toutf8 and output raw bytes)
			unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, asset_length * 6, asset_length-14);
			fprintf(stdout, "%s", utf8_data);
		
			free(utf8_data);
			utf8_data = NULL;
		
		} else {
			wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, asset_length / 2, true);
			//wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, (asset_length / 2) + 1, true);
			APar_unicode_win32Printout(utf16_data, (char*)utf8_data);
		
			free(utf16_data);
			utf16_data = NULL;
		}
#else
		fprintf(stdout, "%s", utf8_data);
#endif
		
		free(utf8_data);
		utf8_data = NULL;
	
	} else { //utf8
		if (print_encoding) {
			fprintf(stdout, " (utf8): ");
		}
		
		APar_fprintf_UTF8_data(unicode_string);
	
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          embedded file extraction                                 //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_Extract_uuid_binary_file
  uuid_atom - pointer to the struct holding the information describing the target atom
	originating_file - the full file path string to the parsed file
	output_path - a (possibly null) string where the embedded file will be extracted to

    If the output path is a null pointer, create a new path derived from originating file name & path - strip off the extension and use that as the base. Read into
		memory the contents of that particular uuid atom. Glob onto the base path the atom name and then the suffix that was embedded along with the file. Write out
		the file to the now fully formed uuid_outfile path.
----------------------*/
void APar_Extract_uuid_binary_file(AtomicInfo* uuid_atom, const char* originating_file, char* output_path) {
	uint32_t path_len = 0;
	uint64_t atom_offsets = 0;
	char* uuid_outfile = (char*)calloc(1, sizeof(char)*MAXPATHLEN+1); //malloc a new string because it may be a cli arg for a specific output path	
	if (output_path == NULL) {
		const char* orig_suffix = strrchr(originating_file, '.');
		if (orig_suffix == NULL) {
			fprintf(stdout, "AP warning: a file extension for the input file was not found.\n\tGlobbing onto original filename...\n");
			path_len = strlen(originating_file);
			memcpy(uuid_outfile, originating_file, path_len);
		} else {
			path_len = orig_suffix-originating_file;
			memcpy(uuid_outfile, originating_file, path_len);
		}
		
	} else {
		path_len = strlen(output_path);
		memcpy(uuid_outfile, output_path, path_len);
	}
	char* uuid_payload = (char*)calloc(1, sizeof(char) * (uuid_atom->AtomicLength - 36 +1) );
			
	APar_readX(uuid_payload, source_file, uuid_atom->AtomicStart + 36, uuid_atom->AtomicLength - 36);
	
	uint32_t descrip_len = UInt32FromBigEndian(uuid_payload);
	atom_offsets+=4+descrip_len;
	uint8_t suffix_len = (uint8_t)uuid_payload[atom_offsets];
	
	char* file_suffix = (char*)calloc(1, sizeof(char) * suffix_len+16 );
	memcpy(file_suffix, uuid_payload+atom_offsets+1, suffix_len);
	atom_offsets+=1+suffix_len;
	
	uint8_t mime_len = (uint8_t)uuid_payload[atom_offsets];
	uint64_t mimetype_string = atom_offsets+1;
	atom_offsets+=1+mime_len;
	uint64_t bin_len = UInt32FromBigEndian(uuid_payload+atom_offsets);
	atom_offsets+=4;

	sprintf(uuid_outfile+path_len, "-%s-uuid%s", uuid_atom->uuid_ap_atomname, file_suffix);
	
	FILE *outfile = APar_OpenFile(uuid_outfile, "wb");
	if (outfile != NULL) {
		fwrite(uuid_payload+atom_offsets, (size_t)bin_len, 1, outfile);
		fclose(outfile);
		fprintf(stdout, "Extracted uuid=%s attachment (mime-type=%s) to file: ", uuid_atom->uuid_ap_atomname, uuid_payload+mimetype_string);
		APar_fprintf_UTF8_data(uuid_outfile);
		fprintf(stdout, "\n");
	}
	
	free(uuid_payload); uuid_payload = NULL;
	free(uuid_outfile); uuid_outfile = NULL;
	free(file_suffix); file_suffix = NULL;
	return;
}

void APar_ExtractAAC_Artwork(short this_atom_num, char* pic_output_path, short artwork_count) {
	char *base_outpath=(char *)malloc(sizeof(char)*MAXPATHLEN+1);
	
	if (snprintf(base_outpath, MAXPATHLEN+1, "%s_artwork_%d", pic_output_path, artwork_count) > MAXPATHLEN) {
		free(base_outpath);
		return;
	}
	
	char* art_payload = (char*)malloc( sizeof(char) * (parsedAtoms[this_atom_num].AtomicLength-16) +1 );	
	memset(art_payload, 0, (parsedAtoms[this_atom_num].AtomicLength-16) +1 );
	
	APar_readX(art_payload, source_file, parsedAtoms[this_atom_num].AtomicStart+16, parsedAtoms[this_atom_num].AtomicLength-16);
	
	char* suffix = (char *)malloc(sizeof(char)*5);
	memset(suffix, 0, sizeof(char)*5);
	
	if (memcmp(art_payload, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0) {
				strcpy(suffix, ".png");
	}	else if (memcmp(art_payload, "\xFF\xD8\xFF\xE0", 4) == 0 || memcmp(art_payload, "\xFF\xD8\xFF\xE1", 4) == 0) {
				strcpy(suffix, ".jpg");
	}
	
	strcat(base_outpath, suffix);
	
	FILE *outfile = APar_OpenFile(base_outpath, "wb");
	if (outfile != NULL) {
		fwrite(art_payload, (size_t)(parsedAtoms[this_atom_num].AtomicLength-16), 1, outfile);
		fclose(outfile);
		fprintf(stdout, "Extracted artwork to file: ");
		APar_fprintf_UTF8_data(base_outpath);
		fprintf(stdout, "\n");
	}
	free(base_outpath);
	free(art_payload);
	free(suffix);
  return;
}

/*----------------------
APar_ImageExtractTest
	buffer - pointer to raw image data
	id3args - *currently unused* when testing raw image data from an image file, results like mimetype & imagetype will be placed here

    Loop through the ImageList array and see if the first few bytes in the image data in buffer match any of the known image_binaryheader types listed. If it does,
		and its png, do a further test to see if its type 0x01 which requires it to be 32x32
----------------------*/
ImageFileFormatDefinition* APar_ImageExtractTest(char* buffer, AdjunctArgs* id3args) {
	ImageFileFormatDefinition* thisImage = NULL;
	uint8_t total_image_tests = ImageListMembers();
	
	for (uint8_t itest = 0; itest < total_image_tests; itest++) {
		if (ImageList[itest].image_testbytes == 0) {
			if (id3args != NULL) {
				id3args->mimeArg = ImageList[itest].image_mimetype;
			}
			return &ImageList[itest];
		} else if (memcmp(buffer, ImageList[itest].image_binaryheader, ImageList[itest].image_testbytes) == 0) {
			if (id3args != NULL) {
				id3args->mimeArg = ImageList[itest].image_mimetype;
				if (id3args->pictype_uint8 == 0x01) {
					if (memcmp(buffer+16, "\x00\x00\x00\x20\x00\x00\x00\x20", 8) != 0 && itest != 2) {
						id3args->pictype_uint8 = 0x02;
					}
				}
			}
			thisImage = &ImageList[itest];
			break;
		}
	}
	return thisImage;
}

/*----------------------
APar_Extract_ID3v2_file
	id32_atom - pointer to the AtomicInfo ID32 atom that contains this while ID3 tag (containing all the frames like APIC)
	frame_str - either APIC or GEOB
	originfile - the originating mpeg-4 file that contains the ID32 atom
	destination_folder - *currently not used* TODO: extract to this folder
	id3args - *currently not used* TODO: extract by mimetype or imagetype or description

    Extracts (all) files of a particular frame type (APIC or GEOB - GEOB is currently not implemented) out to a file next to the originating mpeg-4 file. First, match
		frame_str to get the internal frameID number for APIC/GEOB frame. Locate the .ext of the origin file, duplicate the path including the basename (excluding the
		extension. Loop through the linked list of ID3v2Frame and search for the internal frameID number.
		When an image is found, test the data that the image contains and determine file extension from the ImageFileFormatDefinition structure (containing some popular
		image format/extension definitions). In combination with the file extension, use the image description and image type to create the name of the output file.
		The image (which if was compressed on disc was expanded when read in) and simply write out its data (stored in the 5th member of the frame's field strings.
----------------------*/
void APar_Extract_ID3v2_file(AtomicInfo* id32_atom, const char* frame_str, const char* originfile, const char* destination_folder, AdjunctArgs* id3args) {
	uint16_t iter = 0;
	ID3v2Frame* eval_frame = NULL;
	uint32_t basepath_len = 0;
	char* extract_filename = NULL;
	
	int frameID = MatchID3FrameIDstr(frame_str, id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion);
	int frameType = KnownFrames[frameID+1].ID3v2_FrameType;
	
	if (destination_folder == NULL) {
		basepath_len = (strrchr(originfile, '.') - originfile);
	}
	
	if (frameType == ID3_ATTACHED_PICTURE_FRAME || frameType == ID3_ATTACHED_OBJECT_FRAME) {
		if (id32_atom->ID32_TagInfo->ID3v2_FirstFrame == NULL) return;
		
		eval_frame = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
		extract_filename = (char*)malloc(sizeof(char*)*MAXPATHLEN+1);
		
		while (eval_frame != NULL) {
			if (frameType == eval_frame->ID3v2_FrameType) {
				memset(extract_filename, 0, sizeof(char*)*MAXPATHLEN+1);
				memcpy(extract_filename, originfile, basepath_len);
				iter++;
				
				if (eval_frame->ID3v2_FrameType == ID3_ATTACHED_PICTURE_FRAME) {
					ImageFileFormatDefinition* thisimage = APar_ImageExtractTest((eval_frame->ID3v2_Frame_Fields+4)->field_string, NULL);
					char* img_description = APar_ConvertField_to_UTF8(eval_frame, ID3_DESCRIPTION_FIELD);
					sprintf(extract_filename+basepath_len, "-img#%u-(desc=%s)-0x%02X%s",
				                                     iter, img_description, (uint8_t)((eval_frame->ID3v2_Frame_Fields+2)->field_string[0]), thisimage->image_fileextn);
				
					if (img_description != NULL) {
						free(img_description);
						img_description = NULL;
					}
				} else {
					char* obj_description = APar_ConvertField_to_UTF8(eval_frame, ID3_DESCRIPTION_FIELD);
					char* obj_filename = APar_ConvertField_to_UTF8(eval_frame, ID3_FILENAME_FIELD);
					sprintf(extract_filename+basepath_len, "-obj#%u-(desc=%s)-%s", iter, obj_description, obj_filename);
					
					if (obj_description != NULL) {
						free(obj_description);
						obj_description = NULL;
					}
					if (obj_filename != NULL) {
						free(obj_filename);
						obj_filename = NULL;
					}
				}
				
				FILE *extractfile = APar_OpenFile(extract_filename, "wb");
				if (extractfile != NULL) {
					fwrite((eval_frame->ID3v2_Frame_Fields+4)->field_string, (size_t)((eval_frame->ID3v2_Frame_Fields+4)->field_length), 1, extractfile);
					fclose(extractfile);
					fprintf(stdout, "Extracted %s to file: %s\n", (frameType == ID3_ATTACHED_PICTURE_FRAME ? "artwork" : "object"), extract_filename);
				}

			}
			eval_frame = eval_frame->ID3v2_NextFrame;
		}
	}
	if (extract_filename != NULL) {
		free(extract_filename);
		extract_filename = NULL;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                       iTunes-style metadata listings                              //
///////////////////////////////////////////////////////////////////////////////////////

void APar_ExtractDataAtom(int this_atom_number) {
	if ( source_file != NULL ) {
		AtomicInfo* thisAtom = &parsedAtoms[this_atom_number];
		char* parent_atom_name;
		
		AtomicInfo parent_atom_stats = parsedAtoms[this_atom_number-1];
		parent_atom_name = parent_atom_stats.AtomicName;

		uint32_t min_atom_datasize = 12;
		uint32_t atom_header_size = 16;
		
		if (thisAtom->AtomicClassification == EXTENDED_ATOM) {
			if (thisAtom->uuid_style == UUID_DEPRECATED_FORM) {
				min_atom_datasize +=4;
				atom_header_size +=4;
			} else {
				min_atom_datasize = 36;
				atom_header_size = 36;
			}
		}
 
		if (thisAtom->AtomicLength > min_atom_datasize ) {
			char* data_payload = (char*)malloc( sizeof(char) * (thisAtom->AtomicLength - atom_header_size +1) );
			memset(data_payload, 0, sizeof(char) * (thisAtom->AtomicLength - atom_header_size +1) );
			
			APar_readX(data_payload, source_file, thisAtom->AtomicStart + atom_header_size, thisAtom->AtomicLength - atom_header_size);
			
			if (thisAtom->AtomicVerFlags == AtomFlags_Data_Text) {
				if (thisAtom->AtomicLength < (atom_header_size + 4) ) {
					//tvnn was showing up with 4 chars instead of 3; easier to null it out for now
					data_payload[thisAtom->AtomicLength - atom_header_size] = '\00';
				}
				
				APar_fprintf_UTF8_data(data_payload);
				fprintf(stdout,"\n");
				
			} else {
				if ( (memcmp(parent_atom_name, "trkn", 4) == 0) || (memcmp(parent_atom_name, "disk", 4) == 0) ) {
					if (UInt16FromBigEndian(data_payload+4) != 0) {
						fprintf(stdout, "%u of %u\n", UInt16FromBigEndian(data_payload+2), UInt16FromBigEndian(data_payload+4) );
					} else {
						fprintf(stdout, "%u\n", UInt16FromBigEndian(data_payload+2) );
					}
					
				} else if (strncmp(parent_atom_name, "gnre", 4) == 0) {
					if ( thisAtom->AtomicLength - atom_header_size < 3 ) { //oh, a 1byte int for genre number
						char* genre_string = GenreIntToString( UInt16FromBigEndian(data_payload) );
						if (genre_string != NULL) {
							fprintf(stdout,"%s\n", genre_string);
						} else {
							fprintf(stdout," out of bound value - %u\n", UInt16FromBigEndian(data_payload) );
						}
					} else {
						fprintf(stdout," out of bound value - %u\n", UInt16FromBigEndian(data_payload) );
					}
					
				} else if ( (strncmp(parent_atom_name, "purl", 4) == 0) || (strncmp(parent_atom_name, "egid", 4) == 0) ) {
					fprintf(stdout,"%s\n", data_payload);
				
				} else {
						if (thisAtom->AtomicVerFlags == AtomFlags_Data_UInt && (thisAtom->AtomicLength <= 20 || thisAtom->AtomicLength == 24) ) {
						uint8_t bytes_rep = (uint8_t) (thisAtom->AtomicLength-atom_header_size);
						
						switch(bytes_rep) {
							case 1 : {
								if ( (memcmp(parent_atom_name, "cpil", 4) == 0) || (memcmp(parent_atom_name, "pcst", 4) == 0) || (memcmp(parent_atom_name, "pgap", 4) == 0) ) {
									if (data_payload[0] == 1) {
										fprintf(stdout, "true\n");
									} else {
										fprintf(stdout, "false\n");
									}
									
								} else if (strncmp(parent_atom_name, "stik", 4) == 0) {
									stiks* returned_stik = MatchStikNumber((uint8_t)data_payload[0]);
									if (returned_stik != NULL) {
										fprintf(stdout, "%s\n", returned_stik->stik_string);
									} else {
										fprintf(stdout, "Unknown value: %u\n", (uint8_t)data_payload[0]);
									}
									
								} else if (strncmp(parent_atom_name, "rtng", 4) == 0) { //okay, this is definitely an 8-bit number
									if (data_payload[0] == 2) {
										fprintf(stdout, "Clean Content\n");
									} else if (data_payload[0] != 0 ) {
										fprintf(stdout, "Explicit Content\n");
									} else {
										fprintf(stdout, "Inoffensive\n");
									}
									
								} else {
									fprintf(stdout, "%u\n", (uint8_t)data_payload[0] );
								}
								break;
							}
							case 2 : { //tmpo
								fprintf(stdout, "%u\n", UInt16FromBigEndian(data_payload) );
								break;
							}
							case 4 : { //tves, tvsn
								if (memcmp(parent_atom_name, "sfID", 4) == 0) {
									sfIDs* this_store = MatchStoreFrontNumber( UInt32FromBigEndian(data_payload) );
									if (this_store != NULL) {
										fprintf(stdout, "%s (%" PRIu32 ")\n", this_store->storefront_string, this_store->storefront_number );
									} else {
										fprintf(stdout, "Unknown (%" PRIu32 ")\n", UInt32FromBigEndian(data_payload) );
									}
								
								} else {
									fprintf(stdout, "%" PRIu32 "\n", UInt32FromBigEndian(data_payload) );
								}
								break;
							}
							case 8 : {
								fprintf(stdout, "%" PRIu64 "\n", UInt64FromBigEndian(data_payload) );
								break;
							}
					}
					
					} else if (thisAtom->AtomicClassification == EXTENDED_ATOM &&
											thisAtom->AtomicVerFlags == AtomFlags_Data_uuid_binary &&
											thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE) {
						uint64_t offset_into_uuiddata = 0;
						uint64_t descrip_len = UInt32FromBigEndian(data_payload);
						offset_into_uuiddata+=4;
						
						char* uuid_description = (char*)calloc(1, sizeof(char) * descrip_len+16 ); //char uuid_description[descrip_len+1];
						memcpy(uuid_description, data_payload+offset_into_uuiddata, descrip_len);
						offset_into_uuiddata+=descrip_len;
						
						uint8_t suffix_len = (uint8_t)data_payload[offset_into_uuiddata];
						offset_into_uuiddata+=1;
						
						char* file_suffix = (char*)calloc(1, sizeof(char) * suffix_len+16 ); //char file_suffix[suffix_len+1];
						memcpy(file_suffix, data_payload+offset_into_uuiddata, suffix_len);
						offset_into_uuiddata+=suffix_len;
						
						uint8_t mime_len = (uint8_t)data_payload[offset_into_uuiddata];
						offset_into_uuiddata+=1;
						
						char* uuid_mimetype = (char*)calloc(1, sizeof(char) * mime_len+16 ); //char uuid_mimetype[mime_len+1];
						memcpy(uuid_mimetype, data_payload+offset_into_uuiddata, mime_len);
						
						fprintf(stdout, "FILE%s; mime-type=%s; description=%s\n", file_suffix, uuid_mimetype, uuid_description);
						
						free(uuid_description);
						uuid_description = NULL;
						free(file_suffix);
						file_suffix = NULL;
						free(uuid_mimetype);
						uuid_description = NULL;
					
					} else {       //purl & egid would end up here too, but Apple switched it to a text string (0x00), so gets taken care above explicitly			  
						fprintf(stdout, "hex 0x");
						for( int hexx = 1; hexx <= (int)(thisAtom->AtomicLength - atom_header_size); ++hexx) {
							fprintf(stdout,"%02X", (uint8_t)data_payload[hexx-1]);
							if ((hexx % 4) == 0 && hexx >= 4) {
								fprintf(stdout," ");
							}
							if ((hexx % 16) == 0 && hexx > 16) {
								fprintf(stdout,"\n\t\t\t");
							}
							if (hexx == (int)(thisAtom->AtomicLength - atom_header_size) ) {
								fprintf(stdout,"\n");
							}
						}
					} //end if AtomFlags_Data_UInt
				}
					
				free(data_payload);
				data_payload = NULL;
			}
		}
	}
	return;
}

void APar_Print_iTunesData(const char *path, char* output_path, uint8_t supplemental_info, uint8_t target_information, AtomicInfo* ilstAtom) {
	printBOM();

	short artwork_count=0;
	
	if (ilstAtom == NULL) {
		ilstAtom = APar_FindAtom("moov.udta.meta.ilst", false, SIMPLE_ATOM, 0);
		if (ilstAtom == NULL) return;
	}

	for (int i=ilstAtom->AtomicNumber; i < atom_number; i++) { 
		AtomicInfo* thisAtom = &parsedAtoms[i];
		
		if ( strncmp(thisAtom->AtomicName, "data", 4) == 0) { //thisAtom->AtomicClassification == VERSIONED_ATOM) {
			
			AtomicInfo* parent = &parsedAtoms[ APar_FindParentAtom(i, thisAtom->AtomicLevel) ];
			
			if ( (thisAtom->AtomicVerFlags == AtomFlags_Data_Binary ||
            thisAtom->AtomicVerFlags == AtomFlags_Data_Text || 
            thisAtom->AtomicVerFlags == AtomFlags_Data_UInt) && target_information == PRINT_DATA ) {
				if (strncmp(parent->AtomicName, "----", 4) == 0) {
					if (memcmp(parsedAtoms[parent->AtomicNumber+2].AtomicName, "name", 4) == 0) {
						fprintf(stdout, "Atom \"%s\" [%s;%s] contains: ", parent->AtomicName, parsedAtoms[parent->AtomicNumber+1].ReverseDNSdomain, parsedAtoms[parent->AtomicNumber+2].ReverseDNSname);
						APar_ExtractDataAtom(i);
					}
				
				} else if (memcmp(parent->AtomicName, "covr", 4) == 0) { //libmp4v2 doesn't properly set artwork with the right flags (its all 0x00)
					artwork_count++;
					
				} else {
					//converts iso8859 © in '©ART' to a 2byte utf8 © glyph; replaces libiconv conversion
					memset(twenty_byte_buffer, 0, sizeof(char)*20);
					isolat1ToUTF8((unsigned char*)twenty_byte_buffer, 10, (unsigned char*)parent->AtomicName, 4);

					if (UnicodeOutputStatus == WIN32_UTF16) {
						fprintf(stdout, "Atom \"");
						APar_fprintf_UTF8_data(twenty_byte_buffer);
						fprintf(stdout, "\" contains: ");
					} else {
						fprintf(stdout, "Atom \"%s\" contains: ", twenty_byte_buffer);
					}
					
					APar_ExtractDataAtom(i);
				}
								
			} else if (memcmp(parent->AtomicName, "covr", 4) == 0) {
				artwork_count++;
				if (target_information == EXTRACT_ARTWORK) {
					APar_ExtractAAC_Artwork(thisAtom->AtomicNumber, output_path, artwork_count);
				}
			}
			if ( thisAtom->AtomicLength <= 12 ) {
				fprintf(stdout, "\n"); // (corrupted atom); libmp4v2 touching a file with copyright
			}
		}
	}
	
	if (artwork_count != 0 && target_information == PRINT_DATA) {
		if (artwork_count == 1) {
			fprintf(stdout, "Atom \"covr\" contains: %i piece of artwork\n", artwork_count);
		} else {
			fprintf(stdout, "Atom \"covr\" contains: %i pieces of artwork\n", artwork_count);
		}
	}
	
	if (supplemental_info) {
		fprintf(stdout, "---------------------------\n");
		dynUpd.updage_by_padding = false;
		//APar_DetermineDynamicUpdate(true); //gets the size of the padding
		APar_Optimize(true); //just to know if 'free' atoms can be considered padding, or (in the case of say a faac file) it's *just* 'free'
		
		if (supplemental_info && 0x02) { //PRINT_FREE_SPACE
			fprintf(stdout, "free atom space: %" PRIu32 "\n", APar_ProvideTallyForAtom("free") );
		}
		if (supplemental_info && 0x04) { //PRINT_PADDING_SPACE
			if (!moov_atom_was_mooved) {
				fprintf(stdout, "padding available: %" PRIu64 " bytes\n", dynUpd.padding_bytes);
			} else {
				fprintf(stdout, "padding available: 0 (reorg)\n");
			}
		}
		if (supplemental_info && 0x08 && dynUpd.moov_udta_atom != NULL) { //PRINT_USER_DATA_SPACE
			fprintf(stdout, "user data space: %" PRIu64 "\n", dynUpd.moov_udta_atom->AtomicLength);
		}
		if (supplemental_info && 0x10) { //PRINT_USER_DATA_SPACE
			fprintf(stdout, "media data space: %" PRIu32 "\n", APar_ProvideTallyForAtom("mdat") );
		}
	}
	
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          AP uuid metadata listings                                //
///////////////////////////////////////////////////////////////////////////////////////

void APar_Print_APuuidv5_contents(AtomicInfo* thisAtom) {
	memset(twenty_byte_buffer, 0, sizeof(char)*20);
	isolat1ToUTF8((unsigned char*)twenty_byte_buffer, 10, (unsigned char*)thisAtom->uuid_ap_atomname, 4);
	
	fprintf(stdout, "Atom uuid=");
	APar_print_uuid((ap_uuid_t*) thisAtom->AtomicName, false);
	fprintf(stdout, " (AP uuid for \"");
	APar_fprintf_UTF8_data(twenty_byte_buffer);
	fprintf(stdout, "\") contains: ");

	APar_ExtractDataAtom(thisAtom->AtomicNumber);
	return;
}

void APar_Print_APuuid_deprecated_contents(AtomicInfo* thisAtom) {
	memset(twenty_byte_buffer, 0, sizeof(char)*20);
	isolat1ToUTF8((unsigned char*)twenty_byte_buffer, 10, (unsigned char*)thisAtom->AtomicName, 4);
	
	if (UnicodeOutputStatus == WIN32_UTF16) {
		fprintf(stdout, "Atom uuid=\"");
		APar_fprintf_UTF8_data(twenty_byte_buffer);
		fprintf(stdout, "\" contains: ");
	} else {
		fprintf(stdout, "Atom uuid=\"%s\" contains: ", twenty_byte_buffer);
	}

	APar_ExtractDataAtom(thisAtom->AtomicNumber);
	return;
}

void APar_Print_APuuid_atoms(const char *path, char* output_path, uint8_t target_information) {
	AtomicInfo* thisAtom = NULL;

	printBOM();
	
	AtomicInfo* metaAtom = APar_FindAtom("moov.udta.meta", false, VERSIONED_ATOM, 0);
	
	if (metaAtom == NULL) return;

	for (int i=metaAtom->NextAtomNumber; i < atom_number; i++) {
		thisAtom = &parsedAtoms[i];
		if ( thisAtom->AtomicLevel <= metaAtom->AtomicLevel ) break; //we've gone too far
		if (thisAtom->AtomicClassification == EXTENDED_ATOM) {
			if ( thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE ) {
				if (target_information == PRINT_DATA) APar_Print_APuuidv5_contents(thisAtom);
				if (target_information == EXTRACT_ALL_UUID_BINARYS && thisAtom->AtomicVerFlags == AtomFlags_Data_uuid_binary) {
					APar_Extract_uuid_binary_file(thisAtom, path, output_path);
				}
			}
			if ( thisAtom->uuid_style == UUID_DEPRECATED_FORM && target_information == PRINT_DATA) APar_Print_APuuid_deprecated_contents(thisAtom);
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          3GP asset metadata listings                              //
///////////////////////////////////////////////////////////////////////////////////////

void APar_PrintUnicodeAssest(char* unicode_string, int asset_length) { //3gp files
	if (strncmp(unicode_string, "\xFE\xFF", 2) == 0 ) { //utf16
		fprintf(stdout, " (utf16)] : ");
		
		unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, (asset_length-13) * 6, asset_length-14);

#if defined (_WIN32) && !defined (__CYGWIN__)
		if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) { //pre-NT or AP-utf8.exe (pish, thats my win98se, and without unicows support convert utf16toutf8 and output raw bytes)
			unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, (asset_length -13) * 6, asset_length-14);
			fprintf(stdout, "%s", utf8_data);
		
			free(utf8_data);
			utf8_data = NULL;
		
		} else {
			wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, (asset_length - 16) / 2, true);
			APar_unicode_win32Printout(utf16_data, (char*)utf8_data);
		
			free(utf16_data);
			utf16_data = NULL;
		}
#else
		fprintf(stdout, "%s", utf8_data);
#endif
		
		free(utf8_data);
		utf8_data = NULL;
	
	} else { //utf8
		fprintf(stdout, " (utf8)] : ");
		
		APar_fprintf_UTF8_data(unicode_string);
	
	}
	return;
}

void APar_Print_single_userdata_atomcontents(uint8_t track_num, short userdata_atom, bool quantum_listing) {
	uint32_t box = UInt32FromBigEndian(parsedAtoms[userdata_atom].AtomicName);
	
	char bitpacked_lang[3];
	memset(bitpacked_lang, 0, 3);
	unsigned char unpacked_lang[3];
	
	uint32_t box_length = parsedAtoms[userdata_atom].AtomicLength;
	char* box_data = (char*)malloc(sizeof(char)*box_length);
	memset(box_data, 0, sizeof(char)*box_length);
	
	switch (box) {
		case 0x7469746C : //'titl'
		case 0x64736370 : //'dscp'
		case 0x63707274 : //'cprt'
		case 0x70657266 : //'perf'
		case 0x61757468 : //'auth'
		case 0x676E7265 : //'gnre'
		case 0x616C626D : //'albm'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + 12);
			APar_UnpackLanguage(unpacked_lang, packed_lang);
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 14, box_length-14); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang
			
			//get tracknumber *after* we read the whole tag; if we have a utf16 tag, it will have a BOM, indicating if we have to search for 2 NULLs or a utf8 single NULL, then the ****optional**** tracknumber
			uint16_t track_num = 1000; //tracknum is a uint8_t, so setting it > 256 means a number wasn't found
			if (box == 0x616C626D) { //'albm' has an *optional* uint8_t at the end for tracknumber; if the last byte in the tag is not 0, then it must be the optional tracknum (or a non-compliant, non-NULL-terminated string). This byte is the length - (14 bytes +1tracknum) or -15
				if (box_data[box_length - 15] != 0) {
					track_num = (uint16_t)box_data[box_length - 15];
					box_data[box_length - 15] = 0; //NULL out the last byte if found to be not 0 - it will impact unicode conversion if it remains
				}
			}
			
			fprintf(stdout, "[lang=%s", unpacked_lang);

			APar_PrintUnicodeAssest(box_data, box_length);
			
			if (box == 0x616C626D && track_num != 1000) {
				fprintf(stdout, "  |  Track: %u", track_num);
			}
			fprintf(stdout, "\n");
			break;
		}
		
		case 0x72746E67 : //'rtng'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 12, 4);
			
			fprintf(stdout, "[Rating Entity=%s", box_data);
			memset(box_data, 0, box_length);
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 16, 4);
			fprintf(stdout, " | Criteria=%s", box_data);
			
			uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + 20);
			APar_UnpackLanguage(unpacked_lang, packed_lang);
			fprintf(stdout, " lang=%s", unpacked_lang);
			
			memset(box_data, 0, box_length);
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 22, box_length-8);
			
			APar_PrintUnicodeAssest(box_data, box_length-8);
			fprintf(stdout, "\n");
			break;
		}
		
		case 0x636C7366 : //'clsf'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 12, box_length-12); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang
			
			fprintf(stdout, "[Classification Entity=%s", box_data);
			fprintf(stdout, " | Index=%u", UInt16FromBigEndian(box_data + 4) );
			
			uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + 18);
			APar_UnpackLanguage(unpacked_lang, packed_lang);
			fprintf(stdout, " lang=%s", unpacked_lang);
				
			APar_PrintUnicodeAssest(box_data +8, box_length-8);
			fprintf(stdout, "\n");
			break;
		}

		case 0x6B797764 : //'kywd'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			uint64_t box_offset = 12;
			
			uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset);
			box_offset+=2;
			
			APar_UnpackLanguage(unpacked_lang, packed_lang);
			
			uint8_t keyword_count = APar_read8(source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset);
			box_offset++;
			fprintf(stdout, "[Keyword count=%u", keyword_count);
			fprintf(stdout, " lang=%s]", unpacked_lang);
			
			char* keyword_data = (char*)malloc(sizeof(char)* box_length * 2);
			
			for(uint8_t x = 1; x <= keyword_count; x++) {
				memset(keyword_data, 0, box_length * 2);
				uint8_t keyword_length = APar_read8(source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset);
				box_offset++;
				
				APar_readX(keyword_data, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset, keyword_length);
				box_offset+=keyword_length;
				APar_SimplePrintUnicodeAssest(keyword_data, keyword_length, true);
			}
			free(keyword_data);
			keyword_data = NULL;
			
			fprintf(stdout, "\n");
			break;
		}
			
		case 0x6C6F6369 : //'loci' aka The Most Heinous Metadata Atom Every Invented - decimal meters? fictional location? Astromical Body? Say I shoot it on the International Space Station? That isn't a Astronimical Body. And 16.16 alt only goes up to 20.3 miles (because of negatives, its really 15.15) & the ISS is at 230 miles. Oh, pish.... what ever shall I do? I fear I am on the horns of a dilema.
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			uint64_t box_offset = 12;
			uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset);
			box_offset+=2;
			
			APar_UnpackLanguage(unpacked_lang, packed_lang);
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset, box_length);
			fprintf(stdout, "[lang=%s] ", unpacked_lang);
			
			//the length of the location string is unknown (max is box lenth), but the long/lat/alt/body/notes needs to be retrieved.
			//test if the location string is utf16; if so search for 0x0000 (or if utf8, find the first NULL).
			if ( strncmp(box_data, "\xFE\xFF", 2) == 0 ) {
				box_offset+= 2 * widechar_len(box_data, box_length) + 2; //*2 for utf16 (double-byte); +2 for the terminating NULL
				fprintf(stdout, "(utf16) ");
			} else {
				fprintf(stdout, "(utf8) ");
				box_offset+= strlen(box_data) + 1; //+1 for the terminating NULL
			}
			fprintf(stdout, "Location: ");
			APar_SimplePrintUnicodeAssest(box_data, box_length, false);
			
			uint8_t location_role = APar_read8(source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset);
			box_offset++;
			switch(location_role) {
				case 0 : {
					fprintf(stdout, " (Role: shooting location) ");
					break;
				}
				case 1 : {
					fprintf(stdout, " (Role: real location) ");
					break;
				}
				case 2 : {
					fprintf(stdout, " (Role: fictional location) ");
					break;
				}
				default : {
					fprintf(stdout, " (Role: [reserved]) ");
					break;
				}
			}
			
			char* float_buffer = (char*)malloc(sizeof(char)* 5);
			memset(float_buffer, 0, 5);
			
			fprintf(stdout, "[Long %lf", fixed_point_16x16bit_to_double( APar_read32(float_buffer, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset) ) );
			box_offset+=4;
			fprintf(stdout, " Lat %lf", fixed_point_16x16bit_to_double( APar_read32(float_buffer, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset) ) );
			box_offset+=4;
			fprintf(stdout, " Alt %lf ", fixed_point_16x16bit_to_double( APar_read32(float_buffer, source_file, parsedAtoms[userdata_atom].AtomicStart + box_offset) ) );
			box_offset+=4;
			free(float_buffer);
			float_buffer = NULL;
			
			if (box_offset < box_length) {
				fprintf(stdout, " Body: ");
				APar_SimplePrintUnicodeAssest(box_data+box_offset-14, box_length-box_offset, false);
				if ( strncmp(box_data+box_offset-14, "\xFE\xFF", 2) == 0 ) {
					box_offset+= 2 * widechar_len(box_data+box_offset-14, box_length-box_offset) + 2; //*2 for utf16 (double-byte); +2 for the terminating NULL
				} else {
					box_offset+= strlen(box_data+box_offset-14) + 1; //+1 for the terminating NULL
				}
			}
			fprintf(stdout, "]");
			
			if (box_offset < box_length) {
				fprintf(stdout, " Notes: ");
				APar_SimplePrintUnicodeAssest(box_data+box_offset-14, box_length-box_offset, false);
			}
			
			fprintf(stdout, "\n");
			break;
		}
			
		case 0x79727263 : //'yrrc'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			uint16_t recording_year = APar_read16(bitpacked_lang, source_file, parsedAtoms[userdata_atom].AtomicStart + 12);
			fprintf(stdout, ": %u\n", recording_year);
			break;
		}
		
		case 0x6E616D65  : //'name'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			APar_fprintf_UTF8_data(": ");
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom].AtomicStart + 8, box_length-8); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang
			APar_fprintf_UTF8_data(box_data);
			APar_fprintf_UTF8_data("\n");
			break;
			
		}
		case 0x686E7469  : //'hnti'
		{
			APar_Mark_UserData_area(track_num, userdata_atom, quantum_listing);
			
			APar_readX(box_data, source_file, parsedAtoms[userdata_atom+1].AtomicStart + 8, box_length-8); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang
			fprintf(stdout, "for %s:\n", parsedAtoms[userdata_atom+1].AtomicName);
			APar_fprintf_UTF8_data(box_data);
			break;
		}
		
		default : 
		{
			break;
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        id3 displaying functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

void APar_Print_ID3TextField(ID3v2Frame* textframe, ID3v2Fields* textfield, bool linefeed = false) {
	//this won't accommodate id3v2.4's multiple strings separated by NULLs
	if (textframe->ID3v2_Frame_Fields->field_string[0] == TE_LATIN1) { //all frames that have text encodings have the encoding as the first field
		if (textfield->field_length > 0) {
			char* conv_buffer = (char*)calloc(1, sizeof(char*)*(textfield->field_length *4) +2);
			isolat1ToUTF8((unsigned char*)conv_buffer, sizeof(char*)*(textfield->field_length *4) +2, (unsigned char*)textfield->field_string, textfield->field_length);
			fprintf(stdout, "%s", conv_buffer);
			free(conv_buffer);
			conv_buffer = NULL;
		}
		
	} else if (textframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF16LE_WITH_BOM) { //technically AP *writes* uff16LE here, but based on BOM, it could be utf16BE
		if (textfield->field_length > 2) {
			char* conv_buffer = (char*)calloc(1, sizeof(char*)*(textfield->field_length *2) +2);
			if (strncmp(textfield->field_string, "\xFF\xFE", 2) == 0) {
				UTF16LEToUTF8((unsigned char*)conv_buffer, sizeof(char*)*(textfield->field_length *4) +2, (unsigned char*)textfield->field_string+2, textfield->field_length);
				fprintf(stdout, "%s", conv_buffer);
			} else {
				UTF16BEToUTF8((unsigned char*)conv_buffer, sizeof(char*)*(textfield->field_length *4) +2, (unsigned char*)textfield->field_string+2, textfield->field_length);
				fprintf(stdout, "%s", conv_buffer);
			}			
			free(conv_buffer);
			conv_buffer = NULL;
		}

	} else if (textframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF16BE_NO_BOM) {
		if (textfield->field_length > 0) {
			char* conv_buffer = (char*)calloc(1, sizeof(char*)*(textfield->field_length *2) +2);
			UTF16BEToUTF8((unsigned char*)conv_buffer, sizeof(char*)*(textfield->field_length *4) +2, (unsigned char*)textfield->field_string, textfield->field_length);
			fprintf(stdout, "%s", conv_buffer);
			free(conv_buffer);
			conv_buffer = NULL;
		}
	} else if (textframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF8) {
		fprintf(stdout, "%s", textfield->field_string);
	} else {
		fprintf(stdout, "(unknown type: 0x%X", (uint8_t)textframe->ID3v2_Frame_Fields->field_string[0]);
	}
	if(linefeed) fprintf(stdout, "\n");
	return;
}

const char* APar_GetTextEncoding(ID3v2Frame* aframe, ID3v2Fields* textfield) {
	const char* text_encoding = NULL;
	if (aframe->ID3v2_Frame_Fields->field_string[0] == TE_LATIN1) text_encoding = "latin1";
	if (aframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF16BE_NO_BOM) {
		if (strncmp(textfield->field_string, "\xFF\xFE", 2) == 0) {
			text_encoding = "utf16le";
		} else if (strncmp(textfield->field_string, "\xFE\xFF", 2) == 0) {
			text_encoding = "utf16be";
		}			
	}
	if (aframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF16LE_WITH_BOM) text_encoding = "utf16le";	
	if (aframe->ID3v2_Frame_Fields->field_string[0] == TE_UTF8) text_encoding = "utf8";
	return text_encoding;
}

void APar_Print_ID3v2_tags(AtomicInfo* id32_atom) {
	//TODO properly printout latin1 for fields like owner
	//TODO for binary fields (like GRID group data) scan through to see if it needs to be printed in hex
	//fprintf(stdout, "Maj.Min.Rev version was 2.%u.%u\n", id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion, id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion);
	char* id32_level = (char*)calloc(1, sizeof(char*)*16);
	if (id32_atom->AtomicLevel == 2) {
		memcpy(id32_level, "file level", 10);
	} else if (id32_atom->AtomicLevel == 3) {
		memcpy(id32_level, "movie level", 11);
	} else if (id32_atom->AtomicLevel == 4) {
		sprintf(id32_level, "track #%u", 1); //unimplemented; need to pass a variable here
	}
	
	unsigned char unpacked_lang[3];
	APar_UnpackLanguage(unpacked_lang, id32_atom->AtomicLanguage);
	
	if (id32_atom->ID32_TagInfo->ID3v2_FirstFrame != NULL) {
		fprintf(stdout, "ID32 atom [lang=%s] at %s contains an ID3v2.%u.%u tag (%u tags, %u bytes):\n", unpacked_lang, id32_level, id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion, id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion, id32_atom->ID32_TagInfo->ID3v2_FrameCount, id32_atom->ID32_TagInfo->ID3v2Tag_Length);
	} else {
		fprintf(stdout, "ID32 atom [lang=%s] at %s contains an ID3v2.%u.%u tag. ", unpacked_lang, id32_level, id32_atom->ID32_TagInfo->ID3v2Tag_MajorVersion, id32_atom->ID32_TagInfo->ID3v2Tag_RevisionVersion);
		if (ID3v2_TestTagFlag(id32_atom->ID32_TagInfo->ID3v2Tag_Flags, ID32_TAGFLAG_UNSYNCRONIZATION)) {
			fprintf(stdout, "Unsynchronized flag set. Unsupported. No tags read. %" PRIu32 " bytes.\n", id32_atom->ID32_TagInfo->ID3v2Tag_Length);
		}
	}
	
	ID3v2Frame* target_frameinfo = id32_atom->ID32_TagInfo->ID3v2_FirstFrame;
	while (target_frameinfo != NULL) {
		if (ID3v2_TestFrameFlag(target_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_GROUPING) && target_frameinfo && target_frameinfo->ID3v2_FrameType != ID3_GROUP_ID_FRAME) {
			fprintf(stdout, " Tag: %s GID=0x%02X \"%s\" ", target_frameinfo->ID3v2_Frame_Namestr, target_frameinfo->ID3v2_Frame_GroupingSymbol,
			                                           KnownFrames[target_frameinfo->ID3v2_Frame_ID+1].ID3V2_FrameDescription );
		} else {
			fprintf(stdout, " Tag: %s \"%s\" ", target_frameinfo->ID3v2_Frame_Namestr, KnownFrames[target_frameinfo->ID3v2_Frame_ID+1].ID3V2_FrameDescription );
		}
		uint8_t frame_comp_idx = GetFrameCompositionDescription(target_frameinfo->ID3v2_FrameType);
		if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_UNKNOWN_FRAME) {
			fprintf(stdout, "(unknown frame) %" PRIu32 " bytes\n", target_frameinfo->ID3v2_Frame_Fields->field_length);
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_TEXT_FRAME) {
			ID3v2Fields* atextfield = target_frameinfo->ID3v2_Frame_Fields+1;
			
			if (target_frameinfo->textfield_tally > 1) {
				fprintf(stdout, "(%s) : { ", APar_GetTextEncoding(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+1) );
			} else {
				fprintf(stdout, "(%s) : ", APar_GetTextEncoding(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+1) );
			}
			
			while (true) {
				if (target_frameinfo->textfield_tally > 1) {
					fprintf(stdout, "\"");
				}
				
				if (target_frameinfo->ID3v2_Frame_ID == ID3v2_FRAME_CONTENTTYPE) {
					char* genre_string = NULL;
					int genre_idx = (int)strtol(atextfield->field_string, &genre_string, 10);
					if (genre_string != atextfield->field_string) {
						genre_string = ID3GenreIntToString(genre_idx);
						if (target_frameinfo->textfield_tally == 1) {
							fprintf(stdout, "%s\n", ID3GenreIntToString(genre_idx));
						} else {
							fprintf(stdout, "%s", ID3GenreIntToString(genre_idx));
						}
					} else {
						APar_Print_ID3TextField(target_frameinfo, atextfield, target_frameinfo->textfield_tally == 1 ? true : false);
					}
				
				} else if (target_frameinfo->ID3v2_Frame_ID == ID3v2_FRAME_COPYRIGHT) {
					APar_fprintf_UTF8_data("\xC2\xA9 ");
					APar_Print_ID3TextField(target_frameinfo, atextfield, target_frameinfo->textfield_tally == 1 ? true : false);
				
				} else if (target_frameinfo->ID3v2_Frame_ID == ID3v2_FRAME_PRODNOTICE) {
					APar_fprintf_UTF8_data("\xE2\x84\x97 ");
					APar_Print_ID3TextField(target_frameinfo, atextfield, target_frameinfo->textfield_tally == 1 ? true : false);
				
				} else {
					APar_Print_ID3TextField(target_frameinfo, atextfield, target_frameinfo->textfield_tally == 1 ? true : false);
				}
				
				if (target_frameinfo->textfield_tally > 1) {
					fprintf(stdout, "\"");
				} else {
					break;
				}
				
				atextfield = atextfield->next_field;
				if (atextfield == NULL) {
					fprintf(stdout, " }\n");
					break;
				} else {
					fprintf(stdout, ", ");
				}
			}
						
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_TEXT_FRAME_USERDEF) {
			fprintf(stdout, "(user-defined text frame) ");
			fprintf(stdout, "%u fields\n", target_frameinfo->ID3v2_FieldCount);
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_URL_FRAME) {
			fprintf(stdout, "(url frame) : %s\n", (target_frameinfo->ID3v2_Frame_Fields+1)->field_string);
			fprintf(stdout, "%u fields\n", target_frameinfo->ID3v2_FieldCount);
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_URL_FRAME_USERDEF) {
			fprintf(stdout, "(user-defined url frame) ");
			fprintf(stdout, "%u fields\n", target_frameinfo->ID3v2_FieldCount);
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_UNIQUE_FILE_ID_FRAME) {
			if (test_limited_ascii( (target_frameinfo->ID3v2_Frame_Fields+1)->field_string, (target_frameinfo->ID3v2_Frame_Fields+1)->field_length)) {
				fprintf(stdout, "(owner='%s') : %s\n", target_frameinfo->ID3v2_Frame_Fields->field_string, (target_frameinfo->ID3v2_Frame_Fields+1)->field_string);
			} else {
				fprintf(stdout, "(owner='%s') : 0x", target_frameinfo->ID3v2_Frame_Fields->field_string);
				for (uint32_t hexidx = 0; hexidx < (target_frameinfo->ID3v2_Frame_Fields+1)->field_length; hexidx++) {
					fprintf(stdout, "%02X", (uint8_t)(target_frameinfo->ID3v2_Frame_Fields+1)->field_string[hexidx]);
				}
				fprintf(stdout, "\n");
			}
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_CD_ID_FRAME) { //TODO: print hex representation
			uint8_t tracklistings = 0;
			if (target_frameinfo->ID3v2_Frame_Fields->field_length >= 16) {
				tracklistings = target_frameinfo->ID3v2_Frame_Fields->field_length / 8;
				fprintf(stdout, "(Music CD Identifier) : Entries for %u tracks + leadout track.\n   Hex: 0x", tracklistings-1);
			} else {
				fprintf(stdout, "(Music CD Identifier) : Unknown format (less then 16 bytes).\n   Hex: 0x");
			}
			for (uint16_t hexidx = 1; hexidx < target_frameinfo->ID3v2_Frame_Fields->field_length+1; hexidx++) {
				fprintf(stdout, "%02X", (uint8_t)target_frameinfo->ID3v2_Frame_Fields->field_string[hexidx-1]);
				if (hexidx % 4 == 0) fprintf(stdout, " ");
			}
			fprintf(stdout, "\n");
			
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_DESCRIBED_TEXT_FRAME) {
			fprintf(stdout, "(%s, lang=%s, desc[", APar_GetTextEncoding(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+2),
			                                  (target_frameinfo->ID3v2_Frame_Fields+1)->field_string );
			APar_Print_ID3TextField(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+2);
			fprintf(stdout, "]) : ");
			APar_Print_ID3TextField(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+3, true);
				
		} else if (FrameTypeConstructionList[frame_comp_idx].ID3_FrameType == ID3_ATTACHED_PICTURE_FRAME) {
			fprintf(stdout, "(type=0x%02X-'%s', mimetype=%s, %s, desc[", (uint8_t)(target_frameinfo->ID3v2_Frame_Fields+2)->field_string[0],
			                 ImageTypeList[ (uint8_t)(target_frameinfo->ID3v2_Frame_Fields+2)->field_string[0] ].imagetype_str, (target_frameinfo->ID3v2_Frame_Fields+1)->field_string,
											 APar_GetTextEncoding(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+1)  );
			APar_Print_ID3TextField(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+3);
			if (ID3v2_TestFrameFlag(target_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED)) {
				fprintf(stdout, "]) : %" PRIu32 " bytes (%" PRIu32 " compressed)\n",
				                     (target_frameinfo->ID3v2_Frame_Fields+4)->field_length, target_frameinfo->ID3v2_Frame_Length);
			} else {
				fprintf(stdout, "]) : %" PRIu32 " bytes\n", (target_frameinfo->ID3v2_Frame_Fields+4)->field_length);
			}

		} else if (target_frameinfo->ID3v2_FrameType == ID3_ATTACHED_OBJECT_FRAME) {
			fprintf(stdout, "(filename=");
			APar_Print_ID3TextField(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+2);
			fprintf(stdout, ", mimetype=%s, desc[", (target_frameinfo->ID3v2_Frame_Fields+1)->field_string);
			APar_Print_ID3TextField(target_frameinfo, target_frameinfo->ID3v2_Frame_Fields+3);
			if (ID3v2_TestFrameFlag(target_frameinfo->ID3v2_Frame_Flags, ID32_FRAMEFLAG_COMPRESSED)) {
				fprintf(stdout, "]) : %" PRIu32 " bytes (%" PRIu32 " compressed)\n",
				                     (target_frameinfo->ID3v2_Frame_Fields+4)->field_length, target_frameinfo->ID3v2_Frame_Length);
			} else {
				fprintf(stdout, "]) : %" PRIu32 " bytes\n", (target_frameinfo->ID3v2_Frame_Fields+4)->field_length);
			}
		
		} else if (target_frameinfo->ID3v2_FrameType == ID3_GROUP_ID_FRAME) {
			fprintf(stdout, "(owner='%s') : 0x%02X", target_frameinfo->ID3v2_Frame_Fields->field_string, (uint8_t)(target_frameinfo->ID3v2_Frame_Fields+1)->field_string[0]);
			if ((target_frameinfo->ID3v2_Frame_Fields+2)->field_length > 0) {
				fprintf(stdout, "; groupdata='%s'\n", (target_frameinfo->ID3v2_Frame_Fields+2)->field_string);
			} else {
				fprintf(stdout, "\n");
			}
			
		} else if (target_frameinfo->ID3v2_FrameType == ID3_PRIVATE_FRAME) {
			fprintf(stdout, "(owner='%s') : %s\n", target_frameinfo->ID3v2_Frame_Fields->field_string, (target_frameinfo->ID3v2_Frame_Fields+1)->field_string);
		
		} else if (target_frameinfo->ID3v2_FrameType == ID3_SIGNATURE_FRAME) {
			fprintf(stdout, "{GID=0x%02X) : %s\n", (uint8_t)target_frameinfo->ID3v2_Frame_Fields->field_string[0], (target_frameinfo->ID3v2_Frame_Fields+1)->field_string);
		
		} else if (target_frameinfo->ID3v2_FrameType == ID3_PLAYCOUNTER_FRAME) {
			if (target_frameinfo->ID3v2_Frame_Fields->field_length == 4) {
				fprintf(stdout, ": %" PRIu32 "\n", syncsafe32_to_UInt32(target_frameinfo->ID3v2_Frame_Fields->field_string) );
			} else if (target_frameinfo->ID3v2_Frame_Fields->field_length > 4) {
				fprintf(stdout, ": %" PRIu64 "\n", syncsafeXX_to_UInt64(target_frameinfo->ID3v2_Frame_Fields->field_string, target_frameinfo->ID3v2_Frame_Fields->field_length) );
			}
		
		} else if (target_frameinfo->ID3v2_FrameType == ID3_POPULAR_FRAME) {
			fprintf(stdout, "(owner='%s') : %u", target_frameinfo->ID3v2_Frame_Fields->field_string, (uint8_t)(target_frameinfo->ID3v2_Frame_Fields+1)->field_string[0]);
			if ((target_frameinfo->ID3v2_Frame_Fields+2)->field_length > 0) {
				if ((target_frameinfo->ID3v2_Frame_Fields+2)->field_length == 4) {
					fprintf(stdout, "; playcount=%" PRIu32 "\n", syncsafe32_to_UInt32((target_frameinfo->ID3v2_Frame_Fields+2)->field_string));
				} else if ((target_frameinfo->ID3v2_Frame_Fields+2)->field_length > 4) {
					fprintf(stdout, "; playcount=%" PRIu64 "\n", syncsafeXX_to_UInt64((target_frameinfo->ID3v2_Frame_Fields+2)->field_string, (target_frameinfo->ID3v2_Frame_Fields+2)->field_length));
				} else {
					fprintf(stdout, "\n"); //don't know what it was supposed to be, so skip it
				}
			} else {
				fprintf(stdout, "\n");
			}
		
		} else {
			fprintf(stdout, " [idx=%u;%d]\n", frame_comp_idx, FrameTypeConstructionList[frame_comp_idx].ID3_FrameType);
		}
		target_frameinfo = target_frameinfo->ID3v2_NextFrame;
	}
	free(id32_level);
	id32_level = NULL;
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        metadata scheme searches                                   //
///////////////////////////////////////////////////////////////////////////////////////

void APar_Print_metachild_atomcontents(uint8_t track_num, short metachild_atom, bool quantum_listing) {
	if (memcmp(parsedAtoms[metachild_atom].AtomicName, "ID32", 4) == 0) {
		APar_ID32_ScanID3Tag(source_file, &parsedAtoms[metachild_atom]);
		APar_Print_ID3v2_tags(&parsedAtoms[metachild_atom]);
	}
	return;
}

void APar_PrintMetaChildren(AtomicInfo* metaAtom, AtomicInfo* hdlrAtom, bool quantum_listing) {
	if (metaAtom != NULL && hdlrAtom != NULL) {
		if (hdlrAtom->ancillary_data == 0x49443332) {
			for (int i=metaAtom->NextAtomNumber; i < atom_number; i++) {
				if ( parsedAtoms[i].AtomicLevel <= metaAtom->AtomicLevel ) break; //we've gone too far
				if ( parsedAtoms[i].AtomicLevel == metaAtom->AtomicLevel + 1 ) APar_Print_metachild_atomcontents(0, i, quantum_listing);
			}		
		}
	}
	return;
}

void APar_PrintID32Metadata(bool quantum_listing) {
	uint8_t total_tracks = 0;
	uint8_t a_track = 0;
	AtomicInfo* metaAtom = NULL;
	AtomicInfo* metahandlerAtom = NULL;
	char trackmeta_atom_path[50];
	
	printBOM();
	
	//file level
	metaAtom = APar_FindAtom("meta", false, VERSIONED_ATOM, 0);
	metahandlerAtom = APar_FindAtom("meta.hdlr", false, VERSIONED_ATOM, 0);
	APar_PrintMetaChildren(metaAtom, metahandlerAtom, quantum_listing);

	//movie level
	metaAtom = APar_FindAtom("moov.meta", false, VERSIONED_ATOM, 0);
	metahandlerAtom = APar_FindAtom("moov.meta.hdlr", false, VERSIONED_ATOM, 0);
	APar_PrintMetaChildren(metaAtom, metahandlerAtom, quantum_listing);
	
	//track level
	APar_FindAtomInTrack(total_tracks, a_track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.
	for (uint8_t i = 1; i <= total_tracks; i++) {
		memset(&trackmeta_atom_path, 0, 50);
		sprintf(trackmeta_atom_path, "moov.trak[%u].meta", i);
		
		metaAtom = APar_FindAtom(trackmeta_atom_path, false, VERSIONED_ATOM, 0);
		sprintf(trackmeta_atom_path, "moov.trak[%u].meta.hdlr", i);
		metahandlerAtom = APar_FindAtom(trackmeta_atom_path, false, VERSIONED_ATOM, 0);
		APar_PrintMetaChildren(metaAtom, metahandlerAtom, quantum_listing);
	}
	return;
}

/*----------------------
APar_Print_ISO_UserData_per_track
	quantum_listing - controls whether to simply print each asset, or preface each asset with "movie level"

    This will only show what is under moov.trak.udta atoms (not moov.udta). Get the total number of tracks; construct the moov.trak[index].udta path to find,
		then if the atom after udta is of a greater level, read in from the file & print out what it contains.
----------------------*/
void APar_PrintUserDataAssests(bool quantum_listing) {
	printBOM();
	
	AtomicInfo* udtaAtom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);
	
	if (udtaAtom != NULL) {
		for (int i=udtaAtom->NextAtomNumber; i < atom_number; i++) {
			if ( parsedAtoms[i].AtomicLevel <= udtaAtom->AtomicLevel ) break; //we've gone too far
			if ( parsedAtoms[i].AtomicLevel == udtaAtom->AtomicLevel + 1 ) APar_Print_single_userdata_atomcontents(0, i, quantum_listing);
		}
	}
	APar_PrintID32Metadata(quantum_listing);
	APar_Print_APuuid_atoms(NULL, NULL, PRINT_DATA);
	return;
}

/*----------------------
APar_Print_ISO_UserData_per_track

    This will only show what is under moov.trak.udta atoms (not moov.udta). Get the total number of tracks; construct the moov.trak[index].udta path to find,
		then if the atom after udta is of a greater level, read in from the file & print out what it contains.
----------------------*/
void APar_Print_ISO_UserData_per_track() {
	uint8_t total_tracks = 0;
	uint8_t a_track = 0;//unused
	short a_trak_atom = 0;
	char iso_atom_path[400];
	AtomicInfo* trak_udtaAtom = NULL;

	APar_FindAtomInTrack(total_tracks, a_track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.	
	
	for (uint8_t i = 1; i <= total_tracks; i++) {
		memset(&iso_atom_path, 0, 400);
		sprintf(iso_atom_path, "moov.trak[%u].udta", i);
		
		trak_udtaAtom = APar_FindAtom(iso_atom_path, false, SIMPLE_ATOM, 0);
		
		if (trak_udtaAtom != NULL && parsedAtoms[trak_udtaAtom->NextAtomNumber].AtomicLevel == trak_udtaAtom->AtomicLevel+1) {
			a_trak_atom = trak_udtaAtom->NextAtomNumber;
			while (parsedAtoms[a_trak_atom].AtomicLevel > trak_udtaAtom->AtomicLevel) { //only work on moov.trak[i].udta's child atoms
				
				if (parsedAtoms[a_trak_atom].AtomicLevel == trak_udtaAtom->AtomicLevel+1) APar_Print_single_userdata_atomcontents(i, a_trak_atom, true);
				
				a_trak_atom = parsedAtoms[a_trak_atom].NextAtomNumber;
			}
		}
	}
	APar_PrintUserDataAssests(true);
	return;	
}

///////////////////////////////////////////////////////////////////////////////////////
//                                 Atom Tree                                         //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_PrintAtomicTree

    Following the linked list (by NextAtomNumber), list each atom as they exist in the hieararchy, reflecting positions of moving, eliminating & additions.
		This listing can occur during the course of tagging as well to assist in diagnosing problems.
----------------------*/
void APar_PrintAtomicTree() {
	bool unknown_atom = false;
	char* tree_padding = (char*)malloc(sizeof(char)*126); //for a 25-deep atom tree (4 spaces per atom)+single space+term.
	uint32_t freeSpace = 0;
	short thisAtomNumber = 0;

	printBOM();
		
	//loop through each atom in the struct array (which holds the offset info/data)
 	while (true) {
		AtomicInfo* thisAtom = &parsedAtoms[thisAtomNumber];
		memset(tree_padding, 0, sizeof(char)*126);
		memset(twenty_byte_buffer, 0, sizeof(char)*20);
		
		if (thisAtom->uuid_ap_atomname != NULL) {
			isolat1ToUTF8((unsigned char*)twenty_byte_buffer, 10, (unsigned char*)thisAtom->uuid_ap_atomname, 4); //converts iso8859 © in '©ART' to a 2byte utf8 © glyph
		} else {
			isolat1ToUTF8((unsigned char*)twenty_byte_buffer, 10, (unsigned char*)thisAtom->AtomicName, 4); //converts iso8859 © in '©ART' to a 2byte utf8 © glyph
		}
		
		
		strcpy(tree_padding, "");
		if ( thisAtom->AtomicLevel != 1 ) {
			for (uint8_t pad=1; pad < thisAtom->AtomicLevel; pad++) {
				strcat(tree_padding, "    "); // if the atom depth is over 1, then add spaces before text starts to form the tree
			}
			strcat(tree_padding, " "); // add a single space
		}
		
		if (thisAtom->AtomicLength == 0) {
			fprintf(stdout,
				"%sAtom %s @ %" PRIu64 " of size: %" PRIu64 " (%" PRIu64 "*), ends @ %" PRIu64 "\n",
				tree_padding, twenty_byte_buffer, thisAtom->AtomicStart,
				( (uint64_t)file_size - thisAtom->AtomicStart),
				thisAtom->AtomicLength, (uint64_t)file_size );
			fprintf(stdout,
				"\t\t\t (*)denotes length of atom goes to End-of-File\n");
		
		} else if (thisAtom->AtomicLength == 1) {
			fprintf(stdout, "%sAtom %s @ %" PRIu64 " of size: %" PRIu64 " (^), ends @ %" PRIu64 "\n", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLengthExtended, (thisAtom->AtomicStart + thisAtom->AtomicLengthExtended) );
			fprintf(stdout, "\t\t\t (^)denotes a 64-bit atom length\n");
			
		//uuid atoms of any sort
		} else if (thisAtom->AtomicClassification == EXTENDED_ATOM && thisAtom->uuid_style == UUID_DEPRECATED_FORM) {

			if (UnicodeOutputStatus == WIN32_UTF16) {
				fprintf(stdout, "%sAtom uuid=", tree_padding);
				APar_fprintf_UTF8_data(twenty_byte_buffer);
				fprintf(stdout, " @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			} else {
				fprintf(stdout, "%sAtom uuid=%s @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			}
			
		} else if (thisAtom->AtomicClassification == EXTENDED_ATOM && thisAtom->uuid_style != UUID_DEPRECATED_FORM) {
			if (thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE) {
				fprintf(stdout, "%sAtom uuid=", tree_padding);
				APar_print_uuid( (ap_uuid_t*)thisAtom->AtomicName, false);
				fprintf(stdout, "(APuuid=%s) @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			} else {
				fprintf(stdout, "%sAtom uuid=", tree_padding);
				APar_print_uuid( (ap_uuid_t*)thisAtom->AtomicName, false);
				fprintf(stdout, " @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			}

		//3gp assets (most of them anyway)
		} else if (thisAtom->AtomicClassification == PACKED_LANG_ATOM) {
			unsigned char unpacked_lang[3];
			APar_UnpackLanguage(unpacked_lang, thisAtom->AtomicLanguage);

			if (UnicodeOutputStatus == WIN32_UTF16) {
				fprintf(stdout, "%sAtom ", tree_padding);
				APar_fprintf_UTF8_data(twenty_byte_buffer);
				fprintf(stdout, " [%s] @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", unpacked_lang, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			} else {
				fprintf(stdout, "%sAtom %s [%s] @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64 "\n", tree_padding, twenty_byte_buffer, unpacked_lang, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			}

		//all other atoms (the bulk of them will fall here)
		} else {
		
			if (UnicodeOutputStatus == WIN32_UTF16) {
				fprintf(stdout, "%sAtom ", tree_padding);
				APar_fprintf_UTF8_data(twenty_byte_buffer);
				fprintf(stdout, " @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			} else {
				fprintf(stdout, "%sAtom %s @ %" PRIu64 " of size: %" PRIu64 ", ends @ %" PRIu64, tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart + thisAtom->AtomicLength) );
			}

			if (thisAtom->AtomicContainerState == UNKNOWN_ATOM_TYPE) {
				for (uint8_t i = 0; i < (5-thisAtom->AtomicLevel); i ++) {
					fprintf(stdout, "\t");
				}
				fprintf(stdout, "\t\t\t ~\n");
				unknown_atom = true;
			} else {
				fprintf(stdout, "\n");
			}
		}
		
		//simple tally & percentage of free space info
		if (memcmp(thisAtom->AtomicName, "free", 4) == 0) {
			freeSpace = freeSpace+thisAtom->AtomicLength;
		}
		//this is where the *raw* audio/video file is, the rest is container-related fluff.
		if ( (memcmp(thisAtom->AtomicName, "mdat", 4) == 0) && (thisAtom->AtomicLength > 100) ) {
			mdatData+= thisAtom->AtomicLength;
		} else if ( memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && thisAtom->AtomicLength == 0 ) { //mdat.length = 0 = ends at EOF
			mdatData = file_size - thisAtom->AtomicStart;
		} else if (memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && thisAtom->AtomicLengthExtended != 0 ) {
			mdatData+= thisAtom->AtomicLengthExtended; //this is still adding a (limited) uint64_t into a uint32_t
		}
		
		if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
			break;
		} else {
			thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
		}
	}
	
	if (unknown_atom) {
		fprintf(stdout, "\n ~ denotes an unknown atom\n");
	}
	
	fprintf(stdout, "------------------------------------------------------\n");
	fprintf(stdout, "Total size: %" PRIu64 " bytes; ", (uint64_t)file_size);
	fprintf(stdout, "%i atoms total.\n", atom_number-1);
	fprintf(stdout, "Media data: %" PRIu64 " bytes; %" PRIu64 " bytes all other atoms (%2.3lf%% atom overhead).\n", 
		mdatData, file_size - mdatData,
		(double)(file_size - mdatData)/(double)file_size * 100.0 );

	fprintf(stdout, "Total free atom space: %" PRIu32 " bytes; %2.3lf%% waste.",
		freeSpace, (double)freeSpace/(double)file_size * 100.0 );

	if (freeSpace) {
		dynUpd.updage_by_padding = false;
		//APar_DetermineDynamicUpdate(true); //gets the size of the padding
		APar_Optimize(true); //just to know if 'free' atoms can be considered padding, or (in the case of say a faac file) it's *just* 'free'
		if (!moov_atom_was_mooved) {
			fprintf(stdout, " Padding available: %" PRIu64 " bytes.", dynUpd.padding_bytes);
		}
	}
	if (gapless_void_padding > 0) {
		fprintf(stdout, "\nGapless playback null space at end of file: %" PRIu64 " bytes.", gapless_void_padding);
	}
	fprintf(stdout, "\n------------------------------------------------------\n");
	ShowVersionInfo();
	fprintf(stdout, "------------------------------------------------------\n");
	
	free(tree_padding);
	tree_padding = NULL;
		
	return;
}

/*----------------------
APar_SimpleAtomPrintout

		print a simple flat list of atoms as they were created
----------------------*/
void APar_SimpleAtomPrintout() { //loop through each atom in the struct array (which holds the offset info/data)
	printBOM();

 	for (int i=0; i < atom_number; i++) { 
		AtomicInfo* thisAtom = &parsedAtoms[i]; 
		
		fprintf(stdout, "%i  -  Atom \"%s\" (level %u) has next atom at #%i\n", i, thisAtom->AtomicName, thisAtom->AtomicLevel, thisAtom->NextAtomNumber);
	}
	fprintf(stdout, "Total of %i atoms.\n", atom_number-1);
}
