//==================================================================//
/*
    AtomicParsley - AP_AtomExtracts.h

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

#include "AP_commons.h" /* win32 requires the uintX_t's defined */

typedef struct { //if any of these are unused, they are set to 0xFF
	uint8_t od_profile_level;
	uint8_t scene_profile_level;
	uint8_t audio_profile;
	uint8_t video_profile_level;
	uint8_t graphics_profile_level;
} iods_OD;

typedef struct {
	uint64_t creation_time;
	uint64_t modified_time;
	uint64_t duration;
	bool track_enabled;
	unsigned char unpacked_lang[4];
	char track_hdlr_name[100];
	char encoder_name[100];
	
	uint32_t track_type;
	uint32_t track_codec;
	uint32_t protected_codec;
	
	bool contains_esds;
	
	uint32_t section3_length;
	uint32_t section4_length;
	uint8_t ObjectTypeIndication;
	uint32_t max_bitrate;
	uint32_t avg_bitrate;
	uint32_t section5_length;
	uint8_t descriptor_object_typeID;
	uint16_t channels;
	uint32_t section6_length; //unused

	//specifics
	uint8_t m4v_profile;
	uint8_t avc_version;
	uint8_t profile;
	uint8_t level;
	uint16_t video_height;
	uint16_t video_width;
	uint32_t macroblocks;
	uint64_t sample_aggregate;
	uint16_t amr_modes;
	
	uint8_t type_of_track;
	
} TrackInfo;

typedef struct {
	uint64_t creation_time;
	uint64_t modified_time;
	uint32_t timescale;
	uint32_t duration;
	uint32_t playback_rate; //fixed point 16.16
	uint16_t volume; //fixed 8.8 point
	
	double seconds;
	double simple_bitrate_calc;
	
	bool contains_iods;
} MovieInfo;

typedef struct {
	uint8_t total_tracks;
	uint8_t track_num;
	short track_atom;
} Trackage;

typedef struct {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	double rem_millisecs;
} ap_time;

enum {
	UNKNOWN_TRACK = 0,
	VIDEO_TRACK = 2,
	AUDIO_TRACK = 4,
	DRM_PROTECTED_TRACK = 8,
	OTHER_TRACK = 16
};

enum {
	MP4V_TRACK = 65,
	AVC1_TRACK = 66,
	S_AMR_TRACK = 67,
	S263_TRACK = 68,
	EVRC_TRACK = 69,
	QCELP_TRACK = 70,
	SMV_TRACK = 71
};

enum {
	SHOW_TRACK_INFO = 2,
	SHOW_DATE_INFO = 4
};

void APar_ExtractDetails(FILE* isofile, uint8_t optional_output);
void APar_ExtractBrands(char* filepath);
