//==================================================================//
/*
    AtomicParsley - nsfile.mm

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
#import <Cocoa/Cocoa.h>

/*----------------------
APar_TestTracksForKind

    By testing which tracks are contained within the file, for Mac OS X we can avoid having to change file extension by instead using Finder.app metadata to signal
		the same info as file extension. For each trak atom, find the 'stsd' atom - its ancillary_data will contain the track type that is contained - the info is filled
		in as the file was initially parsed in APar_ScanAtoms. Then using Mac OS X Cocoa calls (in AP_NSFile_utils), set the Finder TYPE/CREATOR codes to signal to the
		OS/Finder/iTunes that this file is .m4a or .m4v without having to change its extension based on what the tracks actually contain.
		
		There are 2 issues with this - iTunes requires the Quicktime player type/creator codes for video that has multi-channel audio, and for chapterized video files.
		TODO: address these issues.
----------------------*/
void APar_TestTracksForKind() {
	uint8_t total_tracks = 0;
	uint8_t track_num = 0;
	AtomicInfo* codec_atom = NULL; //short codec_atom = 0;

	//With track_num set to 0, it will return the total trak atom into total_tracks here.
	APar_FindAtomInTrack(total_tracks, track_num, NULL);
	
	if (total_tracks > 0) {
		while (total_tracks > track_num) {
			track_num+= 1;
			
			codec_atom = APar_FindAtomInTrack(total_tracks, track_num, "stsd");
			if (codec_atom == NULL) return;
			
			//now test this trak's stsd codec against these 4cc codes:			
			switch(codec_atom->ancillary_data) {
				//video types
				case 0x61766331 : // "avc1"
					track_codecs.has_avc1 = true;
					break;
				case 0x6D703476 : // "mp4v"
					track_codecs.has_mp4v = true;
					break;
				case 0x64726D69 : // "drmi"
					track_codecs.has_drmi = true;
					break;
					
				//audio types
				case 0x616C6163 : // "alac"
					track_codecs.has_alac = true;
					break;
				case 0x6D703461 : // "mp4a"
					track_codecs.has_mp4a = true;
					break;
				case 0x64726D73 : // "drms"
					track_codecs.has_drms = true;
					break;
				
				//chapterized types (audio podcasts or movies)
				case 0x74657874 : // "text"
					track_codecs.has_timed_text = true;
					break;
				case 0x6A706567 : // "jpeg"
					track_codecs.has_timed_jpeg = true;
					break;
				
				//either podcast type (audio-only) or timed text subtitles
				case 0x74783367 : // "tx3g"
					track_codecs.has_timed_tx3g = true;
					break;
				
				//other
				case 0x6D703473 : // "mp4s"
					track_codecs.has_mp4s = true;
					break;
				case 0x72747020  : // "rtp "
					track_codecs.has_rtp_hint = true;
					break;
			}
		}
	}	
	return;
}


//TODO: there is a problem with this code seen in: "5.1channel audio-orig.mp4"
//it makes no difference what the file contains, iTunes won't see any (ANY) metadata if its hook/'M4A '.
//in fact, iTunes won't play the file at all
//changing the exact file (with all kinds of metadata) to TVOD/mpg4 - iTunes can play it fine, but doesn't fetch any metadata
//
//it might be beneficial to eval for channels and if its audio only & multichannel to NOT change the TYPE/creator codes

uint32_t APar_4CC_CreatorCode(const char* filepath, uint32_t new_type_code) {
	uint32_t return_value = 0;
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	
	NSString *inFile = [NSString stringWithUTF8String: filepath];
	
	if (new_type_code) {
		NSNumber* creator_code = [NSNumber numberWithUnsignedLong:'hook'];
		NSNumber* type_code = [NSNumber numberWithUnsignedLong:new_type_code];
		NSDictionary* output_attributes = [NSDictionary dictionaryWithObjectsAndKeys:creator_code, NSFileHFSCreatorCode, 
																																							type_code, NSFileHFSTypeCode, nil];
		
		if (![[NSFileManager defaultManager] changeFileAttributes:output_attributes atPath:inFile]) {
			NSLog(@" AtomicParsley error: setting type and creator code on %@", inFile);
		}																																					
				
	} else {
		NSDictionary* file_attributes = [[NSFileManager defaultManager] fileAttributesAtPath:inFile traverseLink:YES];
		return_value = [[file_attributes objectForKey:NSFileHFSTypeCode] unsignedLongValue ];
		
		//NSLog(@"code: %@\n", [file_attributes objectForKey:NSFileHFSTypeCode] );
	}
		
	[pool release];
	
	return return_value;
}

//there is a scenario that is as of now unsupported (or botched, depending if you use the feature), although it would be easy to implement. To make a file bookmarkable, the TYPE code is set to 'M4B ' - which can be *also* done by changing the extension to ".m4b". However, due to the way that the file is tested here, a ".mp4" with 'M4B ' type code will get changed into a normal audio file (not-bookmarkable).

void APar_SupplySelectiveTypeCreatorCodes(const char *inputPath, const char *outputPath, uint8_t forced_type_code) {
	if (forced_type_code != NO_TYPE_FORCING) {
		if (forced_type_code == FORCE_M4B_TYPE) {
			APar_4CC_CreatorCode(outputPath, 'M4B ');
		}
		return;
	}
	
	char* input_suffix = strrchr(inputPath, '.');
	//user-defined output paths may have the original file as ".m4a" & show up fine when output to ".m4a"
	//output to ".mp4" and it becomes a generic (sans TYPE/CREATOR) file that defaults to Quicktime Player
	char* output_suffix = strrchr(outputPath, '.');
	
	char* typecode = (char*)malloc( sizeof(char)* 4 );
	memset(typecode, 0, sizeof(char)*4);
	
	uint32_t type_code = APar_4CC_CreatorCode(inputPath, 0);
	
	UInt32_TO_String4(type_code, typecode);
	
	//fprintf(stdout, "%s - %s\n", typecode, input_suffix);
	APar_TestTracksForKind();
	
	if (strncasecmp(input_suffix, ".mp4", 4) == 0 || strncasecmp(output_suffix, ".mp4", 4) == 0) { //only work on the generic .mp4 extension
		if (track_codecs.has_avc1 || track_codecs.has_mp4v || track_codecs.has_drmi) {
			type_code = APar_4CC_CreatorCode(outputPath, 'M4V ');
			
		//for a podcast an audio track with either a text, jpeg or url track is required, otherwise it will fall through to generic m4a;
		//files that are already .m4b or 'M4B ' don't even enter into this situation, so they are safe
		//if the file had video with subtitles (tx3g), then it would get taken care of above in the video section - unsupported by QT currently
		} else if (track_codecs.has_mp4a && (track_codecs.has_timed_text || track_codecs.has_timed_jpeg || track_codecs.has_timed_tx3g) ) {
			type_code = APar_4CC_CreatorCode(outputPath, 'M4B ');
			
		//default to audio; technically so would a drms iTMS drm audio file with ".mp4". But that would also mean it was renamed. They should be 'M4P '
		} else {
			type_code = APar_4CC_CreatorCode(outputPath, 'M4A ');
		}
	} else if (track_codecs.has_avc1 || track_codecs.has_mp4v || track_codecs.has_drmi) {
			type_code = APar_4CC_CreatorCode(outputPath, 'M4V ');
	}
	
	return;
}
