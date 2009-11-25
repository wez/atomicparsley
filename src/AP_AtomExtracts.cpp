//==================================================================//
/*
    AtomicParsley - AP_AtomExtracts.cpp

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "AtomicParsley.h"
#include "AP_ID3v2_tags.h"
#include "AP_MetadataListings.h"
#include "AP_AtomExtracts.h"

MovieInfo movie_info = {0};
iods_OD iods_info = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

///////////////////////////////////////////////////////////////////////////////////////
//                             File reading routines                                 //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_skip_filler
  isofile - the file to be scanned
  start_position - the offset from the start of file where to commence possible skipping

    I can't remember where exactly I stumbled over what to skip, but this touches on it: http://www.geocities.com/xhelmboyx/quicktime/formats/mp4-layout.txt
		(not that everything there is the gospel truth).
----------------------*/
uint8_t APar_skip_filler(FILE* isofile, uint32_t start_position) {
	uint8_t skip_bytes = 0;
	
	while (true) {
		uint8_t eval_byte = APar_read8(isofile, start_position + skip_bytes);
		
		if (eval_byte == 0x80 || eval_byte == 0x81 || eval_byte == 0xFE) { //seems sometimes QT writes 0x81
			skip_bytes++;
		} else {
			break;
		}
	}
	return skip_bytes;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                string routines                                    //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
uint32tochar4
  lnum - the number to convert to a string
  data - the string to hold the conversion

    Returns a pointer to the originating string (used to print out a string; different from the other function which converts returning void)
----------------------*/
char* uint32tochar4(uint32_t lnum, char* data) {
	data[0] = (lnum >> 24) & 0xff;
	data[1] = (lnum >> 16) & 0xff;
	data[2] = (lnum >>  8) & 0xff;
	data[3] = (lnum >>  0) & 0xff;
	return data;
}

/*----------------------
purge_extraneous_characters
  data - the string which may contain low or high ascii

    Just change most non-textual characters (like a pesky new line char) to something less objectionable - for stdout formatting only
----------------------*/
uint16_t purge_extraneous_characters(char* data) {
	uint16_t purgings = 0;
	uint16_t str_len = strlen(data);
	for(uint16_t str_offset = 0; str_offset < str_len; str_len++) {
		if (data[str_offset] < 32 || data[str_offset] == 127) {
			data[str_offset] = 19;
			purgings++;
			break;
		}
	}
	return purgings;
}

void mem_append(char* add_string, char* dest_string) {
	uint8_t str_len = strlen(dest_string);
	if (str_len > 0) {
		memcpy(dest_string+str_len, ", ", 2);
		memcpy(dest_string+str_len+2, add_string, strlen(add_string) );
	} else {
		memcpy(dest_string, add_string, strlen(add_string) );
	}
	return;
}

/*----------------------
secsTOtime
  seconds - duration in seconds as a floating point number

    Convert decimal seconds to hh:mm:ss.milliseconds. Take the whole seconds and manually separate out the hours, minutes and remaining seconds. For the milliseconds,
		sprintf into a separate string because there doesn't seem to be a way to print without the leading zero; so copy form that string the digits we want then.
----------------------*/
char* secsTOtime(double seconds) {
	ap_time time_duration = {0};
	uint32_t whole_secs = (uint32_t)(seconds / 1);
	
	time_duration.rem_millisecs = seconds - (double)whole_secs;
	time_duration.hours = whole_secs / 3600;
	whole_secs -= time_duration.hours * 3600;
	time_duration.minutes = whole_secs / 60;
	whole_secs -= time_duration.minutes * 60;
	time_duration.seconds = whole_secs;
	
	static char hhmmss_time[20];
	memset(hhmmss_time, 0, 20);
	char milli[5];
	memset(milli, 0, 5);
	
	uint8_t time_offset = 0;
	if (time_duration.hours > 0) {
		if (time_duration.hours < 10) {
			sprintf(hhmmss_time, "0%u:", time_duration.hours);
		} else {
			sprintf(hhmmss_time, "%u:", time_duration.hours);
		}
		time_offset+=3;
	}
	if (time_duration.minutes > 0) {
		if (time_duration.minutes < 10) {
			sprintf(hhmmss_time+time_offset, "0%u:", time_duration.minutes);
		} else {
			sprintf(hhmmss_time+time_offset, "%u:", time_duration.minutes);
		}
		time_offset+=3;
	} else {
		memcpy(hhmmss_time+time_offset, "0:", 2);
		time_offset+=2;
	}
	if (time_duration.seconds > 0) {
		if (time_duration.seconds < 10) {
			sprintf(hhmmss_time+time_offset, "0%u", time_duration.seconds);
		} else {
			sprintf(hhmmss_time+time_offset, "%u", time_duration.seconds);
		}
		time_offset+=2;
	} else {
		memcpy(hhmmss_time+time_offset, "0.", 2);
		time_offset+=1;
	}
	
	sprintf(milli, "%.2lf", time_duration.rem_millisecs); //sprintf the double float into a new string because I don't know if there is a way to print without a leading zero
	memcpy(hhmmss_time+time_offset, milli+1, 3);
	
	return *&hhmmss_time;
}

///////////////////////////////////////////////////////////////////////////////////////
//                              Print Profile Info                                   //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
APar_ShowMPEG4VisualProfileInfo
	track_info - a pointer to the struct holding all the information gathered as a single 'trak' atom was traversed

    If a movie-level iods (containing profiles on a movie-level basis), prefer that mechanism for choosing which profile, otherwise fall back to 'esds' profiles.
		Much of this was garnered from ISO 14496-2:2001 - up to 'Simple Studio'.
----------------------*/
void APar_ShowMPEG4VisualProfileInfo(TrackInfo* track_info) {
	fprintf(stdout, "  MPEG-4 Visual ");
	uint8_t mp4v_profile = 0;
	if (movie_info.contains_iods) {
		mp4v_profile = iods_info.video_profile_level;
	} else {
		mp4v_profile = track_info->m4v_profile;
	}
	
	//unparalleled joy - Annex G table g1 - a binary listing (this from 14496-2:2001)
	if (mp4v_profile == 0x01) {
		fprintf(stdout, "Simple Profile, Level 1"); //00000001
	} else if (mp4v_profile == 0x02) {
		fprintf(stdout, "Simple Profile, Level 2"); //00000010
	} else if (mp4v_profile == 0x03) {
		fprintf(stdout, "Simple Profile, Level 3");     //most files will land here  //00000011
		
	} else if (mp4v_profile == 0x08) {            //Compressor can create these in 3gp files
		fprintf(stdout, "Simple Profile, Level 0"); //ISO 14496-2:2004(e) //00001000
	
	//Reserved 00000100 - 00000111
	} else if (mp4v_profile == 0x10) {
		fprintf(stdout, "Simple Scalable Profile, Level 0"); //00010000
	} else if (mp4v_profile == 0x11) {
		fprintf(stdout, "Simple Scalable Profile, Level 1"); //00010001
	} else if (mp4v_profile == 0x12) {
		fprintf(stdout, "Simple Scalable Profile, Level 2"); //00010010
		
	//Reserved 00010011 - 00100000 
	} else if (mp4v_profile == 0x21) {
		fprintf(stdout, "Core Profile, Level 1"); //00100001
	} else if (mp4v_profile == 0x22) {
		fprintf(stdout, "Core Profile, Level 2"); //00100010
	
	//Reserved 00100011 - 00110001 
	} else if (mp4v_profile == 0x32) {
		fprintf(stdout, "Main Profile, Level 2"); //00110010
	} else if (mp4v_profile == 0x33) {
		fprintf(stdout, "Main Profile, Level 3"); //00110011
	} else if (mp4v_profile == 0x34) {
		fprintf(stdout, "Main Profile, Level 4"); //00110100
		
	//Reserved 00110101 - 01000001  
	} else if (mp4v_profile == 0x42) {
		fprintf(stdout, "N-bit Profile, Level 2"); //01000010
		
	//Reserved 01000011 - 01010000  
	} else if (mp4v_profile == 0x51) {
		fprintf(stdout, "Scalable Texture Profile, Level 1"); //01010001
	
	//Reserved 01010010 - 01100000 
	} else if (mp4v_profile == 0x61) {
		fprintf(stdout, "Simple Face Animation, Level 1"); //01100001
	} else if (mp4v_profile == 0x62) {
		fprintf(stdout, "Simple Face Animation, Level 2"); //01100010
		
	} else if (mp4v_profile == 0x63) {
		fprintf(stdout, "Simple FBA Profile, Level 1"); //01100011
	} else if (mp4v_profile == 0x64) {
		fprintf(stdout, "Simple FBA Profile, Level 2"); //01100100
		
	//Reserved 01100101 - 01110000 
	} else if (mp4v_profile == 0x71) {
		fprintf(stdout, "Basic Animated Texture Profile, Level 1"); //01110001
	} else if (mp4v_profile == 0x72) {
		fprintf(stdout, "Basic Animated Texture Profile, Level 2"); //01110010
		
	//Reserved 01110011 - 10000000
	} else if (mp4v_profile == 0x81) {
		fprintf(stdout, "Hybrid Profile, Level 1"); //10000001
	} else if (mp4v_profile == 0x82) {
		fprintf(stdout, "Hybrid Profile, Level 2"); //10000010
		
	//Reserved 10000011 - 10010000 
	} else if (mp4v_profile == 0x91) {
		fprintf(stdout, "Advanced Real Time Simple Profile, Level 1"); //10010001
	} else if (mp4v_profile == 0x92) {
		fprintf(stdout, "Advanced Real Time Simple Profile, Level 2"); //10010010
	} else if (mp4v_profile == 0x93) {
		fprintf(stdout, "Advanced Real Time Simple Profile, Level 3"); //10010011
	} else if (mp4v_profile == 0x94) {
		fprintf(stdout, "Advanced Real Time Simple Profile, Level 4"); //10010100
	
	//Reserved 10010101 - 10100000
	} else if (mp4v_profile == 0xA1) {
		fprintf(stdout, "Core Scalable Profile, Level 1"); //10100001
	} else if (mp4v_profile == 0xA2) {
		fprintf(stdout, "Core Scalable Profile, Level 2"); //10100010
	} else if (mp4v_profile == 0xA3) {
		fprintf(stdout, "Core Scalable Profile, Level 3"); //10100011
		
	//Reserved 10100100 - 10110000 
	} else if (mp4v_profile == 0xB1) {
		fprintf(stdout, "Advanced Coding Efficiency Profile, Level 1"); //10110001
	} else if (mp4v_profile == 0xB2) {
		fprintf(stdout, "Advanced Coding Efficiency Profile, Level 2"); //10110010
	} else if (mp4v_profile == 0xB3) {
		fprintf(stdout, "Advanced Coding Efficiency Profile, Level 3"); //10110011
	} else if (mp4v_profile == 0xB4) {
		fprintf(stdout, "Advanced Coding Efficiency Profile, Level 4"); //10110100
		
	//Reserved 10110101 Ð 11000000 
	} else if (mp4v_profile == 0xC1) {
		fprintf(stdout, "Advanced Core Profile, Level 1"); //11000001
	} else if (mp4v_profile == 0xC2) {
		fprintf(stdout, "Advanced Core Profile, Level 2"); //11000010
		
	//Reserved 11000011 Ð 11010000
	} else if (mp4v_profile == 0xD1) {
		fprintf(stdout, "Advanced Scalable Texture, Level 1"); //11010001
	} else if (mp4v_profile == 0xD2) {
		fprintf(stdout, "Advanced Scalable Texture, Level 2"); //11010010
	} else if (mp4v_profile == 0xD2) {
		fprintf(stdout, "Advanced Scalable Texture, Level 3"); //11010011
	
	//from a draft document - 1999 (earlier than the 2000 above!!)
	} else if (mp4v_profile == 0xE1) {
		fprintf(stdout, "Simple Studio Profile, Level 1"); //11100001
	} else if (mp4v_profile == 0xE2) {
		fprintf(stdout, "Simple Studio Profile, Level 2"); //11100010
	} else if (mp4v_profile == 0xE3) {
		fprintf(stdout, "Simple Studio Profile, Level 3"); //11100011
	} else if (mp4v_profile == 0xE4) {
		fprintf(stdout, "Simple Studio Profile, Level 4"); //11100100
		
	} else if (mp4v_profile == 0xE5) {
		fprintf(stdout, "Core Studio Profile, Level 1"); //11100101
	} else if (mp4v_profile == 0xE6) {
		fprintf(stdout, "Core Studio Profile, Level 2"); //11100110
	} else if (mp4v_profile == 0xE7) {
		fprintf(stdout, "Core Studio Profile, Level 3"); //11100111
	} else if (mp4v_profile == 0xE8) {
		fprintf(stdout, "Core Studio Profile, Level 4"); //11101000
		
	//Reserved 11101001 - 11101111
	//ISO 14496-2:2004(e)
	} else if (mp4v_profile == 0xF0) {
		fprintf(stdout, "Advanced Simple Profile, Level 0"); //11110000
	} else if (mp4v_profile == 0xF1) {
		fprintf(stdout, "Advanced Simple Profile, Level 1"); //11110001
	} else if (mp4v_profile == 0xF2) {
		fprintf(stdout, "Advanced Simple Profile, Level 2"); //11110010  ////3gp files that QT says is H.263 have esds to 0xF2 & their ObjectType set to 0x20 (mpeg-4 visual)
		                                                                 ////...and its been figured out - FILE EXTENSION of all things determines mpeg-4 ASP or H.263
	} else if (mp4v_profile == 0xF3) {
		fprintf(stdout, "Advanced Simple Profile, Level 3"); //11110011
	} else if (mp4v_profile == 0xF4) {
		fprintf(stdout, "Advanced Simple Profile, Level 4"); //11110100
	} else if (mp4v_profile == 0xF5) {
		fprintf(stdout, "Advanced Simple Profile, Level 5"); //11110101
		
	//Reserved 11110110
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Advanced Simple Profile, Level 3b"); //11110111
	
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 0"); //11111000
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 1"); //11111001
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 2"); //11111010
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 3"); //11111011
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 4"); //11111100
	} else if (mp4v_profile == 0xF7) {
		fprintf(stdout, "Fine Granularity Scalable Profile/Level 5"); //11111101

	//Reserved 11111110
	//Reserved for Escape 11111111

	} else {
		fprintf(stdout, "Unknown profile: 0x%X", mp4v_profile);
	}
	return;
}

/*----------------------
APar_ShowMPEG4AACProfileInfo
	track_info - a pointer to the struct holding all the information gathered as a single 'trak' atom was traversed

----------------------*/
void APar_ShowMPEG4AACProfileInfo(TrackInfo* track_info) {
	if (track_info->descriptor_object_typeID == 1) {
		fprintf(stdout, "  MPEG-4 AAC Main Profile");
	} else if (track_info->descriptor_object_typeID == 2) {
		fprintf(stdout, "  MPEG-4 AAC Low Complexity/LC Profile");     //most files will land here
	} else if (track_info->descriptor_object_typeID == 3) {
		fprintf(stdout, "  MPEG-4 AAC Scaleable Sample Rate/SSR Profile");
	} else if (track_info->descriptor_object_typeID == 4) {
		fprintf(stdout, "  MPEG-4 AAC Long Term Prediction Profile");
	} else if (track_info->descriptor_object_typeID == 5) {
		fprintf(stdout, "  MPEG-4 AAC High Efficiency/HE Profile");
	} else if (track_info->descriptor_object_typeID == 6) {
		fprintf(stdout, "  MPEG-4 AAC Scalable Profile");
	} else if (track_info->descriptor_object_typeID == 7) {
		fprintf(stdout, "  MPEG-4 AAC Transform domain Weighted INterleave Vector Quantization/TwinVQ Profile");
	} else if (track_info->descriptor_object_typeID == 8) {
		fprintf(stdout, "  MPEG-4 AAC Code Excited Linear Predictive/CELP Profile");
	} else if (track_info->descriptor_object_typeID == 9) {
		fprintf(stdout, "  MPEG-4 AAC HVXC Profile");

	} else if (track_info->descriptor_object_typeID == 12) {
		fprintf(stdout, "  MPEG-4 AAC TTSI Profile");
	} else if (track_info->descriptor_object_typeID == 13) {
		fprintf(stdout, "  MPEG-4 AAC Main Synthesis Profile");
	} else if (track_info->descriptor_object_typeID == 14) {
		fprintf(stdout, "  MPEG-4 AAC Wavetable Synthesis Profile");
	} else if (track_info->descriptor_object_typeID == 15) {
		fprintf(stdout, "  MPEG-4 AAC General MIDI Profile");
	} else if (track_info->descriptor_object_typeID == 16) {
		fprintf(stdout, "  MPEG-4 AAC Algorithmic Synthesis & Audio FX Profile");
	} else if (track_info->descriptor_object_typeID == 17) {
		fprintf(stdout, "  MPEG-4 AAC AAC Low Complexity/LC (+error recovery) Profile");
	
	} else if (track_info->descriptor_object_typeID == 19) {
		fprintf(stdout, "  MPEG-4 AAC Long Term Prediction (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 20) {
		fprintf(stdout, "  MPEG-4 AAC Scalable (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 21) {
		fprintf(stdout, "  MPEG-4 AAC Transform domain Weighted INterleave Vector Quantization/TwinVQ (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 22) {
		fprintf(stdout, "  MPEG-4 AAC Bit Sliced Arithmetic Coding/BSAC (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 23) {
		fprintf(stdout, "  MPEG-4 AAC Low Delay/LD (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 24) {
		fprintf(stdout, "  MPEG-4 AAC Code Excited Linear Predictive/CELP (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 25) {
		fprintf(stdout, "  MPEG-4 AAC HXVC (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 26) {
		fprintf(stdout, "  MPEG-4 AAC Harmonic and Individual Lines plus Noise/HILN (+error recovery) Profile");
	} else if (track_info->descriptor_object_typeID == 27) {
		fprintf(stdout, "  MPEG-4 AAC Parametric (+error recovery) Profile");
		
	} else if (track_info->descriptor_object_typeID == 31) {
		fprintf(stdout, "  MPEG-4 ALS Audio Lossless Coding"); //I think that mp4alsRM18 writes the channels wrong after objectedID: 0xF880 has 0 channels; 0xF890 is 2ch
	} else {
		fprintf(stdout, "  MPEG-4 Unknown profile: 0x%X", track_info->descriptor_object_typeID);
	}
	return;
}

/*----------------------
APar_ShowObjectProfileInfo
  track_type - broadly used to determine what types of information (like channels or avc1 profiles) to display
	track_info - a pointer to the struct holding all the information gathered as a single 'trak' atom was traversed

    Based on the ObjectTypeIndication in 'esds', show the type of track. For mpeg-4 audio & mpeg-4 visual are handled in a subroutine because there are so many
		enumerations. avc1 contains 'avcC' which supports a different mechanism.
----------------------*/
void APar_ShowObjectProfileInfo(uint8_t track_type, TrackInfo* track_info) {
	if (track_info->contains_esds) {
		switch (track_info->ObjectTypeIndication) {
			//0x00 es Lambada/Verboten/Forbidden
			case 0x01:
			case 0x02: {
				fprintf(stdout, "  MPEG-4 Systems (BIFS/ObjDesc)");
				break;
			}
			case 0x03: {
				fprintf(stdout, "  Interaction Stream");
				break;
			}
			case 0x04: {
				fprintf(stdout, "  MPEG-4 Systems Extended BIFS");
				break;
			}
			case 0x05: {
				fprintf(stdout, "  MPEG-4 Systems AFX");
				break;
			}
			case 0x06: {
				fprintf(stdout, "  Font Data Stream");
				break;
			}
			case 0x08: {
				fprintf(stdout, "  Synthesized Texture Stream");
				break;
			}
			case 0x07: {
				fprintf(stdout, "  Streaming Text Stream");
				break;
			}
			//0x09-0x1F reserved
			case 0x20: {
				APar_ShowMPEG4VisualProfileInfo(track_info);
				break;
			}
			
			case 0x40: { //vererable mpeg-4 aac
				APar_ShowMPEG4AACProfileInfo(track_info);
				break;
			}
			
			//0x41-0x5F reserved
			case 0x60: {
				fprintf(stdout, "  MPEG-2 Visual Simple Profile"); //'Visual ISO/IEC 13818-2 Simple Profile'
				break;
			}
			case 0x61: {
				fprintf(stdout, "  MPEG-2 Visual Main Profile"); //'Visual ISO/IEC 13818-2 Main Profile'
				break;
			}
			case 0x62: {
				fprintf(stdout, "  MPEG-2 Visual SNR Profile"); //'Visual ISO/IEC 13818-2 SNR Profile'
				break;
			}
			case 0x63: {
				fprintf(stdout, "  MPEG-2 Visual Spatial Profile"); //'Visual ISO/IEC 13818-2 Spatial Profile'
				break;
			}
			case 0x64: {
				fprintf(stdout, "  MPEG-2 Visual High Profile"); //'Visual ISO/IEC 13818-2 High Profile'
				break;
			}
			case 0x65: {
				fprintf(stdout, "  MPEG-2 Visual 4:2:2 Profile"); //'Visual ISO/IEC 13818-2 422 Profile'
				break;
			}
			case 0x66: {
				fprintf(stdout, "  MPEG-2 AAC Main Profile"); //'Audio ISO/IEC 13818-7 Main Profile'
				break;
			}
			case 0x67: {
				fprintf(stdout, "  MPEG-2 AAC Low Complexity Profile"); //Audio ISO/IEC 13818-7 LowComplexity Profile
				break;
			}
			case 0x68: {
				fprintf(stdout, "  MPEG-2 AAC Scaleable Sample Rate Profile"); //'Audio ISO/IEC 13818-7 Scaleable Sampling Rate Profile'
				break;
			}
			case 0x69: {
				fprintf(stdout, "  MPEG-2 Audio"); //'Audio ISO/IEC 13818-3'
				break;
			}
			case 0x6A: {
				fprintf(stdout, "  MPEG-1 Visual"); //'Visual ISO/IEC 11172-2'
				break;
			}
			case 0x6B: {
				fprintf(stdout, "  MPEG-1 Audio"); //'Audio ISO/IEC 11172-3'
				break;
			}
			case 0x6C: {
				fprintf(stdout, "  JPEG"); //'Visual ISO/IEC 10918-1'
				break;
			}
			case 0x6D: {
				fprintf(stdout, "  PNG"); //http://www.mp4ra.org/object.html
				break;
			}
			case 0x6E: {
				fprintf(stdout, "  JPEG2000"); //'Visual ISO/IEC 15444-1'
				break;
			}
			case 0xA0: {
				fprintf(stdout, "  3GPP2 EVRC Voice"); //http://www.mp4ra.org/object.html
				break;
			}
			case 0xA1: {
				fprintf(stdout, "  3GPP2 SMV Voice"); //http://www.mp4ra.org/object.html
				break;
			}
			case 0xA2: {
				fprintf(stdout, "  3GPP2 Compact Multimedia Format"); //http://www.mp4ra.org/object.html
				break;
			}
			
			//0xC0-0xE0 user private
			case 0xE1: {
				fprintf(stdout, "  3GPP2 QCELP (14K Voice)"); //http://www.mp4ra.org/object.html
				break;
			}
			//0xE2-0xFE user private
			//0xFF no object type specified
			
			default: {
				//so many profiles, so little desire to list them all (in 14496-2 which I don't have)
				if(movie_info.contains_iods && iods_info.audio_profile == 0xFE) {
					fprintf(stdout, "  Private user object: 0x%X", track_info->ObjectTypeIndication);
				} else {
					fprintf(stdout, "  Object Type Indicator: 0x%X  Description Ojbect Type ID: 0x%X\n", track_info->ObjectTypeIndication, track_info->descriptor_object_typeID);
				}
				break;
			}
		}

	} else if (track_type == AVC1_TRACK) {
		//profiles & levels are in the 14496-10 pdf (which I don't have access to), so...
		//http://lists.mpegif.org/pipermail/mp4-tech/2006-January/006255.html
		//http://iphome.hhi.de/suehring/tml/doc/lenc/html/configfile_8c-source.html
		//66=baseline, 77=main, 88=extended; 100=High, 110=High 10, 122=High 4:2:2, 144=High 4:4:4
		
		switch(track_info->profile) {
			case 66: {
				fprintf(stdout, "  AVC Baseline Profile");
				break;
			}
			case 77: {
				fprintf(stdout, "  AVC Main Profile");
				break;
			}
			case 88: {
				fprintf(stdout, "  AVC Extended Profile");
				break;
			}
			case 100: {
				fprintf(stdout, "  AVC High Profile");
				break;
			}
			case 110: {
				fprintf(stdout, "  AVC High 10 Profile");
				break;
			}
			case 122: {
				fprintf(stdout, "  AVC High 4:2:2 Profile");
				break;
			}
			case 144: {
				fprintf(stdout, "  AVC High 4:4:4 Profile");
				break;
			}
			default: {
				fprintf(stdout, "  Unknown Profile: %u", track_info->profile);
				break;
			}
		} //end profile switch
		
		//Don't have access to levels either, but working off of:
		//http://iphome.hhi.de/suehring/tml/doc/lenc/html/configfile_8c-source.html
		
		//and the 15 levels it says here: http://www.chiariglione.org/mpeg/technologies/mp04-avc/index.htm (1b in http://en.wikipedia.org/wiki/H.264 seems nonsensical)
		//working backwards, we get... a simple 2 digit number (with '20' just drop the 0; with 21, put in a decimal)
		if (track_info->level > 0) {
			switch (track_info->level) {
				case 10:
				case 20:
				case 30:
				case 40:
				case 50: {
					fprintf(stdout, ",  Level %u", track_info->level / 10);
					break;
				}
				case 11:
				case 12:
				case 13:
				case 21:
				case 22:
				case 31:
				case 32:
				case 41:
				case 42:
				case 51: {
					fprintf(stdout, ",  Level %u.%u", track_info->level / 10, track_info->level % 10);
					break;
				}
				default: {
					fprintf(stdout, ", Unknown level %u.%u", track_info->level / 10, track_info->level % 10);
				}
			
			} //end switch
		} //end level if
	} else if (track_type == S_AMR_TRACK) {
		char* amr_modes = (char*)calloc(1, sizeof(char)*500);
		if (track_info->track_codec == 0x73616D72 || track_info->track_codec == 0x73617762) {
			if (track_info->amr_modes & 0x0001) mem_append("0", amr_modes);
			if (track_info->amr_modes & 0x0002) mem_append("1", amr_modes);
			if (track_info->amr_modes & 0x0004) mem_append("2", amr_modes);
			if (track_info->amr_modes & 0x0008) mem_append("3", amr_modes);
			if (track_info->amr_modes & 0x0010) mem_append("4", amr_modes);
			if (track_info->amr_modes & 0x0020) mem_append("5", amr_modes);
			if (track_info->amr_modes & 0x0040) mem_append("6", amr_modes);
			if (track_info->amr_modes & 0x0080) mem_append("7", amr_modes);
			if (track_info->amr_modes & 0x0100) mem_append("8", amr_modes);
			if (strlen(amr_modes) == 0) memcpy(amr_modes, "none", 4);
		} else if (track_info->track_codec == 0x73766D72) {
			if (track_info->amr_modes & 0x0001) mem_append("VMR-WB Mode 0, ", amr_modes);
			if (track_info->amr_modes & 0x0002) mem_append("VMR-WB Mode 1, ", amr_modes);
			if (track_info->amr_modes & 0x0004) mem_append("VMR-WB Mode 2, ", amr_modes);
			if (track_info->amr_modes & 0x0008) mem_append("VMR-WB Mode 3 (AMR-WB interoperable mode), ", amr_modes);
			if (track_info->amr_modes & 0x0010) mem_append("VMR-WB Mode 4, ", amr_modes);
			if (track_info->amr_modes & 0x0020) mem_append("VMR-WB Mode 2 with maximum half-rate, ", amr_modes);
			if (track_info->amr_modes & 0x0040) mem_append("VMR-WB Mode 4 with maximum half-rate, ", amr_modes);
			uint16_t amr_modes_len = strlen(amr_modes);
			if (amr_modes_len > 0) memset(amr_modes+(amr_modes_len-1), 0, 2);
		}

		if (track_info->track_codec == 0x73616D72) { //samr
			fprintf(stdout, "  AMR Narrow-Band. Modes: %s. Encoder vendor code: %s\n", amr_modes, track_info->encoder_name);
		} else if (track_info->track_codec == 0x73617762) { //sawb
			fprintf(stdout, "  AMR Wide-Band. Modes: %s. Encoder vendor code: %s\n", amr_modes, track_info->encoder_name);
		} else if (track_info->track_codec == 0x73617770) { //sawp
			fprintf(stdout, "  AMR Wide-Band WB+. Encoder vendor code: %s\n", track_info->encoder_name);
		} else if (track_info->track_codec == 0x73766D72) { //svmr
			fprintf(stdout, "  AMR VBR Wide-Band. Encoder vendor code: %s\n", track_info->encoder_name);
		}
		free(amr_modes); amr_modes=NULL;
		
	} else if (track_type == EVRC_TRACK) {
		fprintf(stdout, "  EVRC (Enhanced Variable Rate Coder). Encoder vendor code: %s\n", track_info->encoder_name);
		
	} else if (track_type == QCELP_TRACK) {
		fprintf(stdout, "  QCELP (Qualcomm Code Excited Linear Prediction). Encoder vendor code: %s\n", track_info->encoder_name);
	
	} else if (track_type == S263_TRACK) {
		if (track_info->profile == 0) {
			fprintf(stdout, "  H.263 Baseline Profile, Level %u. Encoder vendor code: %s", track_info->level, track_info->encoder_name);
		} else {
			fprintf(stdout, "  H.263 Profile: %u, Level %u. Encoder vendor code: %s", track_info->profile, track_info->level, track_info->encoder_name);
		}
	}
	if (track_type == AUDIO_TRACK) {
		if (track_info->section5_length == 0) {
			fprintf(stdout, "    channels: (%u)\n", track_info->channels );
		} else {
			fprintf(stdout, "    channels: [%u]\n", track_info->channels );
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                           Movie & Track Level Info                                //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
calcuate_sample_size
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	stsz_atom - the atom number of the stsz atom

    This will get aggregate a number of the size of all chunks in the track. The stsz atom holds a table of these sizes along with a count of how many there are.
		Loop through the count, summing in the sizes. This is called called for all tracks, but used only when a hardcoded bitrate (in esds) isn't found (like avc1)
		and is displayed with the asterisk* at track-level.
----------------------*/
uint64_t calcuate_sample_size(char* uint32_buffer, FILE* isofile, short stsz_atom) {
	uint32_t sample_size = 0;
	uint32_t sample_count = 0;
	uint64_t total_size = 0;
	
	sample_size = APar_read32(uint32_buffer, isofile, parsedAtoms[stsz_atom].AtomicStart + 12);
	sample_count = APar_read32(uint32_buffer, isofile, parsedAtoms[stsz_atom].AtomicStart + 16);
	
	if (sample_size == 0) {
		for (uint32_t atom_offset = 20; atom_offset < parsedAtoms[stsz_atom].AtomicLength; atom_offset +=4) {
			total_size += (uint64_t)APar_read32(uint32_buffer, isofile, parsedAtoms[stsz_atom].AtomicStart + atom_offset);
		}
	} else {
		total_size = (uint64_t)sample_size * (uint64_t)sample_count;
	}
	return total_size;
}

/*----------------------
APar_TrackLevelInfo
	track - pointer to a struct providing the track we are looking for
	track_search_atom_name - the name of the atom to be found in this track

    Looping through the atoms one by one, note a 'trak' atom. If we are looking for the total amount of tracks (by setting the track_num to 0), simply return the
		count of tracks back in the same struct to that later functions can loop through each track individually, looking for a specific atom.
		If track's track_num is a non-zero number, then find that atom that *matches* the atom name. Set track's track_atom to that atom for later use
----------------------*/
void APar_TrackLevelInfo(Trackage* track, char* track_search_atom_name) {
	uint8_t track_tally = 0;
	short iter = 0;
	
	while (parsedAtoms[iter].NextAtomNumber != 0) {
	
		if ( strncmp(parsedAtoms[iter].AtomicName, "trak", 4) == 0) {
			track_tally += 1;
			if (track->track_num == 0) {
				track->total_tracks += 1;
				
			} else if (track->track_num == track_tally) {
				
				short next_atom = parsedAtoms[iter].NextAtomNumber;
				while (parsedAtoms[next_atom].AtomicLevel > parsedAtoms[iter].AtomicLevel) {
					
					if (strncmp(parsedAtoms[next_atom].AtomicName, track_search_atom_name, 4) == 0) {
					
						track->track_atom = parsedAtoms[next_atom].AtomicNumber;
						return;
					} else {
						next_atom = parsedAtoms[next_atom].NextAtomNumber;
					}
					if (parsedAtoms[next_atom].AtomicLevel == parsedAtoms[iter].AtomicLevel) {
						track->track_atom = 0;
					}
				}
			}
		}
		iter=parsedAtoms[iter].NextAtomNumber;
	}
	return;
}

/*----------------------
APar_ExtractChannelInfo
	isofile - the file to be scanned
	pos - the position within the file that carries the channel info (in esds)

    The channel info in esds is bitpacked, so read it in isolation and shift the bits around to get at it
----------------------*/
uint8_t APar_ExtractChannelInfo(FILE* isofile, uint32_t pos) {
	uint8_t packed_channels = APar_read8(isofile, pos);
	uint8_t unpacked_channels = (packed_channels << 1); //just shift the first bit off the table
	unpacked_channels = (unpacked_channels >> 4); //and slide it on over back on the uint8_t
	return unpacked_channels;
}

/*----------------------
APar_Extract_iods_Info
	isofile - the file to be scanned
	iods_atom - a pointer to the struct that will store the profile levels found in iods

    'iods' info mostly comes from: http://www.geocities.com/xhelmboyx/quicktime/formats/mp4-layout.txt Just as 'esds' has 'filler' bytes to skip over, so does this.
		Mercifully, the profiles come one right after another. The only problem is that in many files, the iods profiles don't match the esds profiles. This is resolved
		by ignoring it for audio (mostly, unless is 0xFE user defined). For MPEG-4 Visual, it is preferred over 'esds' (occurs in APar_ShowMPEG4VisualProfileInfo); for
		all other video types it is ignored.
----------------------*/
void APar_Extract_iods_Info(FILE* isofile, AtomicInfo* iods_atom) {
	uint32_t iods_offset = iods_atom->AtomicStart+8;
	if (iods_atom->AtomicVerFlags == 0 && APar_read8(isofile, iods_offset+4) == 0x10) {
		iods_offset+=5;
		iods_offset += APar_skip_filler(isofile, iods_offset);
		uint8_t iods_objdescrip_len = APar_read8(isofile, iods_offset);
		iods_offset++;
		if (iods_objdescrip_len >= 7) { 
			iods_info.od_profile_level = APar_read8(isofile, iods_offset+2);
			iods_info.scene_profile_level = APar_read8(isofile, iods_offset+3);
			iods_info.audio_profile = APar_read8(isofile, iods_offset+4);
			iods_info.video_profile_level = APar_read8(isofile, iods_offset+5);
			iods_info.graphics_profile_level = APar_read8(isofile, iods_offset+6);
		}
	}
	return;
}

/*----------------------
APar_Extract_AMR_Info
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	track_level_atom - the number of the 'esds' atom in the linked list of parsed atoms
	track_info - a pointer to the struct carrying track-level info to be filled with information

    The only interesting info here is the encoding tool & the amr modes used. ffmpeg's amr output seems to lack some compliance - no damr atom for sawb
----------------------*/
void APar_Extract_AMR_Info(char* uint32_buffer, FILE* isofile, short track_level_atom, TrackInfo* track_info) {
	uint32_t amr_specific_offet = 8;
	APar_readX(track_info->encoder_name, isofile, parsedAtoms[track_level_atom].AtomicStart + amr_specific_offet, 4);
	if (track_info->track_codec == 0x73616D72 || track_info->track_codec == 0x73617762 || track_info->track_codec == 0x73766D72) { //samr,sawb & svmr contain modes only
		track_info->amr_modes = APar_read16(uint32_buffer, isofile, parsedAtoms[track_level_atom].AtomicStart + amr_specific_offet + 4+1);
	}
	return;
}

/*----------------------
APar_Extract_d263_Info
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	track_level_atom - the number of the 'esds' atom in the linked list of parsed atoms
	track_info - a pointer to the struct carrying track-level info to be filled with information

    'd263' only holds 4 things; the 3 of interest are gathered here. Its possible that a 'bitr' atom follows 'd263', which would hold bitrates, but isn't parsed here
----------------------*/
void APar_Extract_d263_Info(char* uint32_buffer, FILE* isofile, short track_level_atom, TrackInfo* track_info) {
	uint32_t offset_into_d263 = 8;
	APar_readX(track_info->encoder_name, isofile, parsedAtoms[track_level_atom].AtomicStart + offset_into_d263, 4);
	track_info->level = APar_read8(isofile, parsedAtoms[track_level_atom].AtomicStart + offset_into_d263 + 4+1);
	track_info->profile = APar_read8(isofile, parsedAtoms[track_level_atom].AtomicStart + offset_into_d263 + 4+2);
	//possible 'bitr' bitrate box afterwards
	return;
}

/*----------------------
APar_Extract_devc_Info
	isofile - the file to be scanned
	track_level_atom - the number of the 'esds' atom in the linked list of parsed atoms
	track_info - a pointer to the struct carrying track-level info to be filled with information

    'devc' only holds 3 things: encoder vendor, decoder version & frames per sample; only encoder vendor is gathered
----------------------*/
void APar_Extract_devc_Info(FILE* isofile, short track_level_atom, TrackInfo* track_info) {
	uint32_t offset_into_devc = 8;
	APar_readX(track_info->encoder_name, isofile, parsedAtoms[track_level_atom].AtomicStart + offset_into_devc, 4);
	return;
}

/*----------------------
APar_Extract_esds_Info
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	track_level_atom - the number of the 'esds' atom in the linked list of parsed atoms
	track_info - a pointer to the struct carrying track-level info to be filled with information

    'esds' contains a wealth of information. Memory fails where I figured out how to parse this atom, but this seems like a decent outline in retrospect:
		http://www.geocities.com/xhelmboyx/quicktime/formats/mp4-layout.txt - but its misleading in lots of places too. For those tracks that support 'esds'
		(notably not avc1 or alac), this atom comes in at most 4 sections (section 3 to section 6). Each section is numbered (3 is 0x03) followed by an optional
		amount of filler (see APar_skip_filler), then the length of the section to the end of the atom or the end of another section.
----------------------*/
void APar_Extract_esds_Info(char* uint32_buffer, FILE* isofile, short track_level_atom, TrackInfo* track_info) {
	uint32_t offset_into_stsd = 0;
	
	while (offset_into_stsd < parsedAtoms[track_level_atom].AtomicLength) {
		offset_into_stsd ++;
		if ( APar_read32(uint32_buffer, isofile, parsedAtoms[track_level_atom].AtomicStart + offset_into_stsd) == 0x65736473 ) {
			track_info->contains_esds = true;
		
			uint32_t esds_start = parsedAtoms[track_level_atom].AtomicStart + offset_into_stsd - 4;
			uint32_t esds_length = APar_read32(uint32_buffer, isofile, esds_start);
			uint32_t offset_into_esds = 12; //4bytes length + 4 bytes name + 4bytes null
						
			if ( APar_read8(isofile, esds_start + offset_into_esds) == 0x03 ) {
				offset_into_esds++;
				offset_into_esds += APar_skip_filler(isofile, esds_start + offset_into_esds);
			}

			uint8_t section3_length = APar_read8(isofile, esds_start + offset_into_esds);
			if ( section3_length <= esds_length && section3_length != 0) {
				track_info->section3_length = section3_length;
			} else {
				break;
			}
			
			//for whatever reason, when mp4box muxes in ogg into an mp4 container, section 3 gets a 0x9D byte (which doesn't fall inline with what AP considers 'filler')
			//then again, I haven't *completely* read the ISO specifications, so I could just be missing it the the ->voluminous<- 14496-X specifications.
			uint8_t test_byte = APar_read8(isofile, esds_start + offset_into_esds+1);
			if (test_byte != 0) {
				offset_into_esds++;
			}
			
			offset_into_esds+= 4; //1 bytes section 0x03 length + 2 bytes + 1 byte
			
			if ( APar_read8(isofile, esds_start + offset_into_esds) == 0x04 ) {
				offset_into_esds++;
				offset_into_esds += APar_skip_filler(isofile, esds_start + offset_into_esds);
			}
			
			uint8_t section4_length = APar_read8(isofile, esds_start + offset_into_esds);
			if ( section4_length <= section3_length && section4_length != 0) {
				track_info->section4_length = section4_length;
				
				if (section4_length == 0x9D) offset_into_esds++; //upper limit? when gpac puts an ogg in, section 3 is 9D - so is sec4 (section 4 real length with ogg = 0x0E86)
				
				offset_into_esds++;
				track_info->ObjectTypeIndication = APar_read8(isofile, esds_start + offset_into_esds);
				
				//this is just so that ogg in mp4 won't have some bizarre high bitrate of like 2.8megabits/sec
				uint8_t a_v_flag = APar_read8(isofile, esds_start + offset_into_esds + 1); //mp4box with ogg will set this to DD, mp4a has it as 0x40, mp4v has 0x20
				
				if (track_info->ObjectTypeIndication < 0xC0 && a_v_flag < 0xA0) {//0xC0 marks user streams; but things below that might still be wrong (like 0x6D - png)
					offset_into_esds+= 5;
					track_info->max_bitrate = APar_read32(uint32_buffer, isofile, esds_start + offset_into_esds);
					offset_into_esds+= 4;
					track_info->avg_bitrate = APar_read32(uint32_buffer, isofile, esds_start + offset_into_esds);
					offset_into_esds+= 4;
				}
			} else {
				break;
			}
			
			if ( APar_read8(isofile, esds_start + offset_into_esds) == 0x05 ) {
				offset_into_esds++;
				offset_into_esds += APar_skip_filler(isofile, esds_start + offset_into_esds);
			
				uint8_t section5_length = APar_read8(isofile, esds_start + offset_into_esds);
				if ( (section5_length <= section4_length || section4_length == 1) && section5_length != 0) {
					track_info->section5_length = section5_length;
					offset_into_esds+=1;
					
					if (track_info->type_of_track & AUDIO_TRACK) {
						uint8_t packed_objID = APar_read8(isofile, esds_start + offset_into_esds); //its packed with channel, but channel is fetched separately
						track_info->descriptor_object_typeID = packed_objID >> 3;
						offset_into_esds+=1;

						track_info->channels = (uint16_t)APar_ExtractChannelInfo(isofile, esds_start + offset_into_esds);
						
					} else if (track_info->type_of_track & VIDEO_TRACK) {
						//technically, visual_object_sequence_start_code should be tested aginst 0x000001B0
						if (APar_read16(uint32_buffer, isofile, esds_start + offset_into_esds+2) == 0x01B0) {
							track_info->m4v_profile = APar_read8(isofile, esds_start + offset_into_esds+2+2);
						}
					}
				}
				break; //uh, I've extracted the pertinent info
			}
		
		}
		if (offset_into_stsd > parsedAtoms[track_level_atom].AtomicLength) {
			break;
		}
	}
	if ( (track_info->section5_length == 0 && track_info->type_of_track & AUDIO_TRACK) || track_info->channels == 0) {
		track_info->channels = APar_read16(uint32_buffer, isofile, parsedAtoms[track_level_atom].AtomicStart + 40);
	}
	return;
}

/*----------------------
APar_ExtractTrackDetails
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	track - the struct proving tracking of this 'trak' atom so we can jump around in this track
	track_info - a pointer to the struct carrying track-level info to be filled with information

    This function jumps all around in a single 'trak' atom gathering information from different child constituent atoms except 'esds' which is handled
		on its own.
----------------------*/
void APar_ExtractTrackDetails(char* uint32_buffer, FILE* isofile, Trackage* track, TrackInfo* track_info) {
	uint32_t _offset = 0;

	APar_TrackLevelInfo(track, "tkhd");
	if ( APar_read8(isofile, parsedAtoms[track->track_atom].AtomicStart + 8) == 0) {
		if (APar_read8(isofile, parsedAtoms[track->track_atom].AtomicStart + 11) & 1) {
			track_info->track_enabled = true;
		}
		track_info->creation_time = (uint64_t)APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 12);
		track_info->modified_time = (uint64_t)APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 16);
		track_info->duration = (uint64_t)APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 28);
	} else {
		track_info->creation_time = APar_read64(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 12);
		track_info->modified_time = APar_read64(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 20);
		track_info->duration = APar_read64(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 36);
	}
	
	//language code
	APar_TrackLevelInfo(track, "mdhd");
	memset(uint32_buffer, 0, 5);
	uint16_t packed_language = APar_read16(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 28);
	memset(track_info->unpacked_lang, 0, 4);
	APar_UnpackLanguage(track_info->unpacked_lang, packed_language); //http://www.w3.org/WAI/ER/IG/ert/iso639.htm

	//track handler type
	APar_TrackLevelInfo(track, "hdlr");
	memset(uint32_buffer, 0, 5);
	track_info->track_type = APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 16);
	if (track_info->track_type == 0x736F756E ) { //soun
		track_info->type_of_track = AUDIO_TRACK;
	} else if (track_info->track_type == 0x76696465 ) { //vide
		track_info->type_of_track = VIDEO_TRACK;
	}
	if ( parsedAtoms[track->track_atom].AtomicLength > 34) {
		memset(track_info->track_hdlr_name, 0, 100);
		APar_readX(track_info->track_hdlr_name, isofile, parsedAtoms[track->track_atom].AtomicStart + 32, parsedAtoms[track->track_atom].AtomicLength - 32);
	}
	
	//codec section
	APar_TrackLevelInfo(track, "stsd");
	memset(uint32_buffer, 0, 5);
	track_info->track_codec = APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 20);

	if (track_info->type_of_track & VIDEO_TRACK ) { //vide
		track_info->video_width = APar_read16(uint32_buffer, isofile, parsedAtoms[track->track_atom+1].AtomicStart + 32);
		track_info->video_height = APar_read16(uint32_buffer, isofile, parsedAtoms[track->track_atom+1].AtomicStart + 34);
		track_info->macroblocks = (track_info->video_width / 16) * (track_info->video_height / 16);
		
		//avc profile & level
		if ( track_info->track_codec == 0x61766331  || track_info->track_codec == 0x64726D69) { //avc1 or drmi
			track_info->contains_esds = false;
			APar_TrackLevelInfo(track, "avcC");
			//get avc1 profile/level; atom 'avcC' is :
			//byte 1	configurationVersion    byte 2	AVCProfileIndication    byte 3  profile_compatibility    byte 4	AVCLevelIndication
			track_info->avc_version = APar_read8(isofile, parsedAtoms[track->track_atom].AtomicStart + 8);
			if (track_info->avc_version == 1) {
				track_info->profile = APar_read8(isofile, parsedAtoms[track->track_atom].AtomicStart + 9);
				//uint8_t profile_compatibility = APar_read8(isofile, parsedAtoms[track.track_atom].AtomicStart + 10); /* is this reserved ?? */
				track_info->level = APar_read8(isofile, parsedAtoms[track->track_atom].AtomicStart + 11);
			}
			
			//avc1 doesn't have a hardcoded bitrate, so calculate it (off of stsz table summing) later
		} else 	if (track_info->track_codec == 0x73323633) { //s263
			APar_TrackLevelInfo(track, "d263");
			if ( memcmp(parsedAtoms[track->track_atom].AtomicName, "d263", 4) == 0) {
				APar_Extract_d263_Info(uint32_buffer, isofile, track->track_atom, track_info);
			}
			
		} else { //mp4v
			APar_TrackLevelInfo(track, "esds");
			if ( memcmp(parsedAtoms[track->track_atom].AtomicName, "esds", 4) == 0) {
				APar_Extract_esds_Info(uint32_buffer, isofile, track->track_atom-1, track_info); //right, backtrack to the atom before 'esds' so we can offset_into_stsd++
			} else if (track_info->track_codec == 0x73323633) { //s263
				track_info->type_of_track = VIDEO_TRACK;
			} else if (track_info->track_codec == 0x73616D72 || track_info->track_codec == 0x73617762
			           || track_info->track_codec == 0x73617770 || track_info->track_codec == 0x73766D72) { //samr, sawb, sawp & svmr
				track_info->type_of_track = AUDIO_TRACK;
			} else {
				track_info->type_of_track = OTHER_TRACK; //a 'jpeg' track will fall here
			}
		}
		
	} else if ( track_info->type_of_track & AUDIO_TRACK) {
		if (track_info->track_codec == 0x73616D72 || track_info->track_codec == 0x73617762
			  || track_info->track_codec == 0x73617770 || track_info->track_codec == 0x73766D72) { //samr,sawb, svmr (sawp doesn't contain modes)
			APar_Extract_AMR_Info(uint32_buffer, isofile, track->track_atom+2, track_info);
		
		} else if (track_info->track_codec == 0x73657663) { //sevc
			APar_TrackLevelInfo(track, "devc");
			if ( memcmp(parsedAtoms[track->track_atom].AtomicName, "devc", 4) == 0) {
				APar_Extract_devc_Info(isofile, track->track_atom, track_info);
			}
			
		} else if (track_info->track_codec == 0x73716370) { //sqcp 
			APar_TrackLevelInfo(track, "dqcp");
			if ( memcmp(parsedAtoms[track->track_atom].AtomicName, "dqcp", 4) == 0) {
				APar_Extract_devc_Info(isofile, track->track_atom, track_info); //its the same thing
			}
			
		} else if (track_info->track_codec == 0x73736D76) { //ssmv 
			APar_TrackLevelInfo(track, "dsmv");
			if ( memcmp(parsedAtoms[track->track_atom].AtomicName, "dsmv", 4) == 0) {
				APar_Extract_devc_Info(isofile, track->track_atom, track_info); //its the same thing
			}
		
		} else {
			APar_Extract_esds_Info(uint32_buffer, isofile, track->track_atom, track_info);
		}
	}
	
	//in case bitrate isn't found, manually determine it off of stsz summing
	if ( (track_info->type_of_track & AUDIO_TRACK || track_info->type_of_track & VIDEO_TRACK ) && track_info->avg_bitrate == 0)  {
		if (track_info->track_codec == 0x616C6163 ) { //alac
			track_info->channels = APar_read16(uint32_buffer, isofile, parsedAtoms[track->track_atom+1].AtomicStart + 24);
		}
	}
	
	APar_TrackLevelInfo(track, "stsz");
	if (memcmp(parsedAtoms[track->track_atom].AtomicName, "stsz", 4) == 0) {
		track_info->sample_aggregate = calcuate_sample_size(uint32_buffer, isofile, track->track_atom);
	}

	//get what exactly 'drmX' stands in for
	if (track_info->track_codec >= 0x64726D00 && track_info->track_codec <= 0x64726DFF) {
		track_info->type_of_track += DRM_PROTECTED_TRACK;
		APar_TrackLevelInfo(track, "frma");
		memset(uint32_buffer, 0, 5);
		track_info->protected_codec = APar_read32(uint32_buffer, isofile, parsedAtoms[track->track_atom].AtomicStart + 8);
	}
		
	//Encoder string; occasionally, it appears under stsd for a video track; it is typcally preceded by ' ²' (1st char is unprintable) or 0x01B2
	if (track_info->contains_esds) {
		APar_TrackLevelInfo(track, "esds");
		
		//technically, user_data_start_code should be tested aginst 0x000001B2; TODO: it should only be read up to section 3's length too
		_offset = APar_FindValueInAtom(uint32_buffer, isofile, track->track_atom, 24, 0x01B2);
		
		if (_offset > 0 && _offset < parsedAtoms[track->track_atom].AtomicLength) {
			_offset +=2;
			memset(track_info->encoder_name, 0, parsedAtoms[track->track_atom].AtomicLength - _offset);
			APar_readX(track_info->encoder_name, isofile, parsedAtoms[track->track_atom].AtomicStart + _offset, parsedAtoms[track->track_atom].AtomicLength - _offset);
		}
	}
	return;
}

/*----------------------
APar_ExtractMovieDetails
	uint32_buffer - a buffer to read bytes in from the file
	isofile - the file to be scanned
	mvhd_atom - pointer to the 'mvhd' atom and where in the file it can be found

    Get information out of 'mvhd' - most important of which are timescale & duration which get used to calcuate bitrate if needed and determine duration
		of a track in seconds. A rough approximation of the overall bitrate is done off this too using the sum of the mdat lengths.
----------------------*/
void APar_ExtractMovieDetails(char* uint32_buffer, FILE* isofile, AtomicInfo* mvhd_atom) {	
	if (mvhd_atom->AtomicVerFlags & 0x01000000) {
		movie_info.creation_time = APar_read64(uint32_buffer, isofile, mvhd_atom->AtomicStart + 12);
		movie_info.modified_time = APar_read64(uint32_buffer, isofile, mvhd_atom->AtomicStart + 20);
		movie_info.timescale = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 28);
		movie_info.duration = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 32);
		movie_info.timescale = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 36);
		movie_info.duration = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 40);
		movie_info.playback_rate = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 44);
		movie_info.volume = APar_read16(uint32_buffer, isofile, mvhd_atom->AtomicStart + 48);
	} else {
		movie_info.creation_time = (uint64_t)APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 12);
		movie_info.modified_time = (uint64_t)APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 16);
		movie_info.timescale = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 20);
		movie_info.duration = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 24);
		movie_info.playback_rate = APar_read32(uint32_buffer, isofile, mvhd_atom->AtomicStart + 28);
		movie_info.volume = APar_read16(uint32_buffer, isofile, mvhd_atom->AtomicStart + 32);
	}
	
	movie_info.seconds = (float)movie_info.duration / (float)movie_info.timescale;
#if defined (_MSC_VER)
	__int64 media_bits = (__int64)mdatData * 8;
#else
	uint64_t media_bits = (uint64_t)mdatData * 8;
#endif
	movie_info.simple_bitrate_calc = ( (double)media_bits / movie_info.seconds) / 1000.0;

	return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                         Get at some track-level info                              //
///////////////////////////////////////////////////////////////////////////////////////

void APar_Print_TrackDetails(TrackInfo* track_info) {
	if (track_info->max_bitrate > 0 && track_info->avg_bitrate > 0) {
			fprintf(stdout, "     %.2f kbp/s", (float)track_info->avg_bitrate/1000.0);
	} else { //some ffmpeg encodings have avg_bitrate set to 0, but an inexact max_bitrate - actually, their esds seems a mess to me
#if defined (_MSC_VER)
		fprintf(stdout, "     %.2lf* kbp/s", ( (double)((__int64)track_info->sample_aggregate) /
																						( (double)((__int64)track_info->duration) / (double)((__int64)movie_info.timescale)) ) / 1000.0 * 8);
		fprintf(stdout, "  %.3f sec", (float)((uint32_t)track_info->duration) / (float)((uint32_t)movie_info.timescale));
#else
		fprintf(stdout, "     %.2lf* kbp/s", ( (double)track_info->sample_aggregate /
																						( (double)track_info->duration / (double)movie_info.timescale) ) / 1000.0 * 8);
		fprintf(stdout, "  %.3f sec", (float)track_info->duration / (float)movie_info.timescale);
#endif
	}
						
	if (track_info->track_codec == 0x6D703476 ) { //mp4v profile
		APar_ShowObjectProfileInfo(MP4V_TRACK, track_info);											
	} else if (track_info->track_codec == 0x6D703461 || track_info->protected_codec == 0x6D703461 ) { //mp4a profile
		APar_ShowObjectProfileInfo(AUDIO_TRACK, track_info);
	} else if (track_info->track_codec == 0x616C6163) { //alac - can't figure out a hardcoded bitrate either
		fprintf(stdout, "  Apple Lossless    channels: [%u]\n", track_info->channels);
	} else if (track_info->track_codec == 0x61766331  || track_info->protected_codec == 0x61766331) {
		if (track_info->avc_version == 1) { //avc profile & level
			APar_ShowObjectProfileInfo(AVC1_TRACK, track_info);
		}
	} else if (track_info->track_codec == 0x73323633) { //s263 in 3gp
		APar_ShowObjectProfileInfo(S263_TRACK, track_info);
	} else if (track_info->track_codec == 0x73616D72 || track_info->track_codec == 0x73617762
	            || track_info->track_codec == 0x73617770 || track_info->track_codec == 0x73766D72) { //samr,sawb,sawp & svmr in 3gp
		track_info->type_of_track = S_AMR_TRACK;
		APar_ShowObjectProfileInfo(track_info->type_of_track, track_info);
	} else if (track_info->track_codec == 0x73657663) { //evrc in 3gp
		track_info->type_of_track = EVRC_TRACK;
		APar_ShowObjectProfileInfo(track_info->type_of_track, track_info);
	} else if (track_info->track_codec == 0x73716370) { //qcelp in 3gp
		track_info->type_of_track = QCELP_TRACK;
		APar_ShowObjectProfileInfo(track_info->type_of_track, track_info);
	} else if (track_info->track_codec == 0x73736D76) { //smv in 3gp
		track_info->type_of_track = SMV_TRACK;
		APar_ShowObjectProfileInfo(track_info->type_of_track, track_info);
	}  else { //unknown everything, 0 hardcoded bitrate
		APar_ShowObjectProfileInfo(track_info->type_of_track, track_info);
		fprintf(stdout, "\n");
	}
					
	if (track_info->type_of_track & VIDEO_TRACK && 
			( ( track_info->max_bitrate > 0 && track_info->ObjectTypeIndication == 0x20) || track_info->avc_version == 1 || track_info->protected_codec != 0) ) {
		fprintf(stdout, "  %ux%u  (%u macroblocks)\n", track_info->video_width, track_info->video_height, track_info->macroblocks);
	} else if (track_info->type_of_track & VIDEO_TRACK) {
		fprintf(stdout, "\n");
	}
	return;
}

void APar_ExtractDetails(FILE* isofile, uint8_t optional_output) {
	char* uint32_buffer=(char*)malloc( sizeof(char)*5 );
	Trackage track = {0};
	
	AtomicInfo* mvhdAtom = APar_FindAtom("moov.mvhd", false, VERSIONED_ATOM, 0);
	if (mvhdAtom != NULL) {
		APar_ExtractMovieDetails(uint32_buffer, isofile, mvhdAtom);
		fprintf(stdout, "Movie duration: %.3lf seconds (%s) - %.2lf* kbp/sec bitrate (*=approximate)\n", movie_info.seconds, secsTOtime(movie_info.seconds), movie_info.simple_bitrate_calc);
		if (optional_output & SHOW_DATE_INFO) {
			fprintf(stdout, "  Presentation Creation Date (UTC):     %s\n", APar_extract_UTC(movie_info.creation_time) );
			fprintf(stdout, "  Presentation Modification Date (UTC): %s\n", APar_extract_UTC(movie_info.modified_time) );
		}
	}
		
	AtomicInfo* iodsAtom = APar_FindAtom("moov.iods", false, VERSIONED_ATOM, 0);
	if (iodsAtom != NULL) {
		movie_info.contains_iods = true;
		APar_Extract_iods_Info(isofile, iodsAtom);
	}
	
	if (optional_output & SHOW_TRACK_INFO) {
		APar_TrackLevelInfo(&track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.

		fprintf(stdout, "Low-level details. Total tracks: %u \n", track.total_tracks);
		fprintf(stdout, "Trk  Type  Handler                    Kind  Lang  Bytes\n");
		
		if (track.total_tracks > 0) {
			while (track.total_tracks > track.track_num) {
				track.track_num+= 1;
				TrackInfo track_info = {0};
				
				//tracknum, handler type, handler name
				APar_ExtractTrackDetails(uint32_buffer, isofile, &track, &track_info);
				uint16_t more_whitespace = purge_extraneous_characters(track_info.track_hdlr_name);
				
				if (strlen(track_info.track_hdlr_name) == 0) {
					memcpy(track_info.track_hdlr_name, "[none listed]", 13);
				}
				fprintf(stdout, "%u    %s  %s", track.track_num,  uint32tochar4(track_info.track_type, uint32_buffer), track_info.track_hdlr_name);
				
				uint16_t handler_len = strlen(track_info.track_hdlr_name);
				if (handler_len < 25 + more_whitespace) {
					for (uint16_t i=handler_len; i < 25 + more_whitespace; i++) {
						fprintf(stdout, " ");
					}
				}
				
				//codec, language
				fprintf(stdout, "  %s  %s   %llu", uint32tochar4(track_info.track_codec, uint32_buffer), track_info.unpacked_lang, track_info.sample_aggregate);
				
				if (track_info.encoder_name[0] != 0 && track_info.contains_esds) {
					purge_extraneous_characters(track_info.encoder_name);
					fprintf(stdout, "   Encoder: %s", track_info.encoder_name);
				}
				if (track_info.type_of_track & DRM_PROTECTED_TRACK) {
					fprintf(stdout, " (protected %s)", uint32tochar4(track_info.protected_codec, uint32_buffer) );
				}
				
				fprintf(stdout, "\n");			
				/*---------------------------------*/
				
				if (track_info.type_of_track & VIDEO_TRACK || track_info.type_of_track & AUDIO_TRACK) {
					APar_Print_TrackDetails(&track_info);
				}
				
				if (optional_output & SHOW_DATE_INFO) {
					fprintf(stdout, "       Creation Date (UTC):     %s\n", APar_extract_UTC(track_info.creation_time) );
					fprintf(stdout, "       Modification Date (UTC): %s\n", APar_extract_UTC(track_info.modified_time) );
				}
					
			}
		}
	}
	return;
}

//provided as a convenience function so that 3rd party utilities can know beforehand
void APar_ExtractBrands(char* filepath) {
	FILE* a_file = APar_OpenISOBaseMediaFile(filepath, true);
	char* buffer = (char *)calloc(1, sizeof(char)*16);
	uint32_t atom_length = 0;
	uint8_t file_type_offset = 0;
	uint32_t compatible_brand = 0;
	bool cb_V2ISOBMFF = false;
	
	fseek(a_file, 4, SEEK_SET); //this fseek will to.... the first 30 or so bytes; fseeko isn't required
	fread(buffer, 1, 4, a_file);
	if (memcmp(buffer, "ftyp", 4) == 0) {
		atom_length = APar_read32(buffer, a_file, 0);
	} else {
		APar_readX(buffer, a_file, 0, 12);
		if (memcmp(buffer, "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A", 12) == 0 ) {
			APar_readX(buffer, a_file, 12, 12);
			if (memcmp(buffer+4, "ftypmjp2", 8) == 0 || memcmp(buffer+4, "ftypmj2s", 8) == 0) {
				atom_length = UInt32FromBigEndian(buffer);
				file_type_offset = 12;
			}
		}
	}
	
	if (atom_length > 0) {
		memset(buffer, 0, 16);
		APar_readX(buffer, a_file, 8+file_type_offset, 4);
		printBOM();
		fprintf(stdout, " Major Brand: %s", buffer);
		APar_IdentifyBrand(buffer);
		
		if (memcmp(buffer, "isom", 4) == 0) {
			APar_ScanAtoms(filepath); //scan_file = true;
		}
		
		uint32_t minor_version = APar_read32(buffer, a_file, 12+file_type_offset);
		fprintf(stdout, "  -  version %u\n", minor_version);
		
		fprintf(stdout, " Compatible Brands:");
		for (uint32_t i = 16+file_type_offset; i < atom_length; i+=4) {
			APar_readX(buffer, a_file, i, 4);
			compatible_brand = UInt32FromBigEndian(buffer);
			if (compatible_brand != 0) {
				fprintf(stdout, " %s", buffer);
				if (compatible_brand == 0x6D703432 || compatible_brand == 0x69736F32) {
					cb_V2ISOBMFF = true;
				}
			}
		}
		fprintf(stdout, "\n");
	}
	
	APar_OpenISOBaseMediaFile(filepath, false);
	
	fprintf(stdout, " Tagging schemes available:\n");
	switch(metadata_style) {
		case ITUNES_STYLE: {
			fprintf(stdout, "   iTunes-style metadata allowed.\n");
			break;
		}
		case THIRD_GEN_PARTNER:
		case THIRD_GEN_PARTNER_VER1_REL6:
		case THIRD_GEN_PARTNER_VER1_REL7:
		case THIRD_GEN_PARTNER_VER2: {
			fprintf(stdout, "   3GP-style asset metadata allowed.\n");
			break;
		}
		case THIRD_GEN_PARTNER_VER2_REL_A: {
			fprintf(stdout, "   3GP-style asset metadata allowed [& unimplemented GAD (Geographical Area Description) asset].\n");
			break;
		}
	}
	if (cb_V2ISOBMFF || metadata_style == THIRD_GEN_PARTNER_VER1_REL7) {
		fprintf(stdout, "   ID3 tags on ID32 atoms @ file/movie/track level allowed.\n");
	}
	fprintf(stdout, "   ISO-copyright notices @ movie and/or track level allowed.\n   uuid private user extension tags allowed.\n");
	
	free(buffer); buffer=NULL;
	return;
}
