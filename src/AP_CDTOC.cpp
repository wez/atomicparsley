//==================================================================//
/*
    AtomicParsley - AP_CDTOC.cpp

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

//gathering of a CD's Table of Contents is going to be hardware specific
//currently only Mac OS X is implemented - using IOKit framework.
//another avenue (applicable to other *nix platforms): ioctl

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#if defined (HAVE_LINUX_CDROM_H)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <fcntl.h>

/* From <linux/cdrom.h>: */
//#define	CDROM_LEADOUT		0xAA
#elif defined (DARWIN_PLATFORM)

#include <stdio.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/IOBSD.h>

const uint8_t MACOSX_LEADOUT_TRACK = 0xA2;

#elif defined (WIN32)
#include <string.h>
#include <windows.h>

#endif

#include "AP_CDTOC.h"

const uint8_t CDOBJECT_DATACD = 0;
const uint8_t CDOBJECT_AUDIOCD = 1;

struct CD_TDesc {
	uint8_t  	session;
	uint8_t   controladdress;
	uint8_t   unused1; //refered to as 'tno' which equates to "track number" - but... its 'point' that actually is the tracknumber in mode1 TOC. set to zero for all mode1 TOC
	uint8_t   tracknumber; //refered to as 'point', but this is actually the tracknumber in mode1 audio toc entries.
	uint8_t   rel_minutes;
	uint8_t   rel_seconds;
	uint8_t   rel_frames;
	uint8_t   zero_space;
	uint8_t   abs_minutes;
	uint8_t   abs_seconds;
	uint8_t   abs_frames;
	void*     next_description;
};
typedef struct CD_TDesc CD_TDesc;

struct CD_TOC_ {
	uint16_t   toc_length;
	uint8_t    first_session;
	uint8_t    last_session;
	CD_TDesc*  track_description; //entry to the first track in the linked list
};
typedef struct CD_TOC_ CD_TOC_;

CD_TOC_* cdTOC = NULL;

#if defined (DARWIN_PLATFORM)
	uint8_t LEADOUT_TRACK_NUMBER = MACOSX_LEADOUT_TRACK;
#elif defined (HAVE_LINUX_CDROM_H)
	uint8_t LEADOUT_TRACK_NUMBER = CDROM_LEADOUT;
#elif defined (WIN32)
	uint8_t LEADOUT_TRACK_NUMBER = 0xAA; //NOTE: for WinXP IOCTL_CDROM_READ_TOC_EX code, its 0xA2
#endif


/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
MCDI describes the CD TOC - actually talks about "a binary dump of the TOC". So, a TOC is made up of:
a 4 byte TOC header (2 bytes length of the entire TOC, 1 byte start session, 1 byte end session)
an array of track entries, and depending on the mode, of varying lengths. For audio CDs, TOC track entries are mode1 (or for CD-R/RW mode5, but lets stick to mode1)
a mode1 track entry is 11 bytes:
1byte session, 1 byte (packed control/address), 1byte NULL (unused TNO), 1 byte for tracknumber (expressed as the word POINT in mmc nomenclature), 3bytes relative start frametime, 1 byte NULL, 3 bytes duration timeframe

while "binary dump of the TOC" is there, its also modified so that the timeframe listing in mm:ss::frames (3bytes) is converted to a 4byte LBA timecode.
Combining the first 4 bytes of the "binary dump of the TOC" with the modifications of the 3byte(frame)->4byte(block), we arrive at MCDI as:
struct mcdi_track_entry {
  uint8_t   cd_toc_session;
  uint8_t   cd_toc_controladdress; //bitpacked uint4_t of control & address
  uint8_t   cd_toc_TNO = 0; //hardcoded to 0 for mode1 audio tracks in the TOC
  uint8_t   cd_toc_tracknumber; //this is the 1-99 tracknumber (listed in mmc-2 as POINT)
  uint32_t  cd_frame_address; //converted from the 3byte mm:ss:frame absolute duration
};
struct toc_header {
	uint16_t  toc_length;
	uin8_t    first_track;
	uint8_t   last_track;
};
struct mcdi_frame {
	struct toc_header;
  struct mcdi_track_entry[total_audio_tracks];
  struct mcdi_track_entry lead_out;
  };

The problem with including the TOC header is that it can't be used directly because on the CD toc entries are 3byte msf address, but here they are 4byte LBA. In any event
this header should not have ever been included because the length can be deduced from the frame length & tracks by dividing by 8. So, the header length that MCDI refers to:
is it for MSF or LBA addressing? Well, since the rest of MCDI is LBA-based, lets say LBA - which means it needs to be calculated. As it just so happens, then its the
length of the frame. All that needs to be added are the first & last tracks.

Unfortunately, this frame can't be used as a CD Identifier *AS IS* across platforms. Because the leadout track is platform specific (a Linux leadout is 0xAA, MacOSX leadout
is 0xA2), consideration of the leadout track would have to be given by anything else using this frame.
ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

///////////////////////////////////////////////////////////////////////////////////////
//                      Generating MCDI data from a CD TOC                           //
///////////////////////////////////////////////////////////////////////////////////////

uint8_t DataControlField(uint8_t controlfield) {
#if defined (__ppc__) || defined (__ppc64__)
	if (controlfield & 0x04) { // data uninterrupted or increment OR reserved; this field is already bitpacked as controlfield
		return 1;
	}
#else
	if (controlfield & 0x40) { // data uninterrupted or increment OR reserved; bitpacked already
		return 1;
	}
#endif
	return 0;
}

uint8_t DetermineCDType(CD_TOC_* cdTOCdata) {
	CD_TDesc* track_TOC_desc = cdTOCdata->track_description;
	while (track_TOC_desc != NULL) {
		if (track_TOC_desc->tracknumber >= 1 && track_TOC_desc->tracknumber <= 99 && !DataControlField(track_TOC_desc->controladdress)) {
			return CDOBJECT_AUDIOCD;
		}
		track_TOC_desc = (CD_TDesc*)track_TOC_desc->next_description;
	}
	return CDOBJECT_DATACD;
}

CD_TDesc* LeadOutTrack(CD_TOC_* cdTOCdata) {
	CD_TDesc* track_TOC_desc = cdTOCdata->track_description;
	while (track_TOC_desc != NULL) {
		if (track_TOC_desc->tracknumber == LEADOUT_TRACK_NUMBER) {
			return track_TOC_desc;
		}
		track_TOC_desc = (CD_TDesc*)track_TOC_desc->next_description;
	}
	return NULL;
}

uint8_t FillSingleMCDIentry(CD_TDesc* atrack, char* mcdi_data_entry) {
	mcdi_data_entry[0] = atrack->session;
	mcdi_data_entry[1] = atrack->controladdress;
	mcdi_data_entry[2] = 0;
	mcdi_data_entry[3] = atrack->tracknumber;
	//LBA=(M*60+S)*75+F - 150 (table 374)
	uint32_t frameduration = ((((atrack->abs_minutes*60) + atrack->abs_seconds) * 75) + atrack->abs_frames)-150; 
	UInt32_TO_String4(frameduration, mcdi_data_entry + 4);
	return 8;
}

uint16_t FormMCDIdata(char* mcdi_data) {
	uint16_t mcdi_len = 0;
	uint8_t first_track = 0;
	uint8_t last_track = 0;
	
	CD_TDesc* track_TOC_desc = cdTOC->track_description;
	
	if (cdTOC->track_description != NULL) {
		mcdi_len +=4;
	
		while (track_TOC_desc != NULL) {
			if (track_TOC_desc->tracknumber >= 1 && track_TOC_desc->tracknumber <= 99 && !DataControlField(track_TOC_desc->controladdress)) {
				mcdi_len += FillSingleMCDIentry(track_TOC_desc, mcdi_data+mcdi_len);
				if (first_track == 0) {
					first_track = track_TOC_desc->tracknumber;
				}
				last_track = track_TOC_desc->tracknumber;
			}
			track_TOC_desc = (CD_TDesc*)track_TOC_desc->next_description;
		}
		if (mcdi_len > 0) {
			CD_TDesc* leadout = LeadOutTrack(cdTOC);
			if (leadout != NULL) {
				mcdi_len += FillSingleMCDIentry(leadout, mcdi_data+mcdi_len);
			}
		}
		//backtrack & fill in the header
		UInt16_TO_String2(mcdi_len, mcdi_data);
		mcdi_data[2] = first_track;
		mcdi_data[3] = last_track;
	}
	return mcdi_len;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                Platform Specifics                                 //
///////////////////////////////////////////////////////////////////////////////////////

#if defined (HAVE_LINUX_CDROM_H)
void Linux_ReadCDTOC(int cd_fd) {
	cdrom_tochdr toc_header;
	cdrom_tocentry toc_entry;
	CD_TDesc* a_TOC_desc = NULL;
	CD_TDesc* prev_desc = NULL;
	
	if (ioctl(cd_fd, CDROMREADTOCHDR, &toc_header) == -1) {
		fprintf(stderr, "AtomicParsley error: there was an error reading the CD Table of Contents header.\n");
		return;
	}
	cdTOC = (CD_TOC_*)calloc(1, sizeof(CD_TOC_));
	//cdTOC->toc_length = 0; //not used anyway
	//cdTOC->first_session = 0; //not used anyway
	//cdTOC->last_session = 0; //not used anyway
	cdTOC->track_description = NULL;

	for (uint8_t i = toc_header.cdth_trk0; i <= toc_header.cdth_trk1+1; i++) {
		memset(&toc_entry, 0, sizeof(toc_entry));
		if (i == toc_header.cdth_trk1+1) {
			toc_entry.cdte_track = CDROM_LEADOUT;
		} else {
			toc_entry.cdte_track = i;
		}
		toc_entry.cdte_format = CDROM_MSF; //although it could just be easier to use CDROM_LBA
		
		if (ioctl(cd_fd, CDROMREADTOCENTRY, &toc_entry) == -1) {
      fprintf(stderr, "AtomicParsley error: there was an error reading a CD Table of Contents entry (linux cdrom).\n");
			return;
    }
		
		if (cdTOC->track_description == NULL) {
			cdTOC->track_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = cdTOC->track_description;
			prev_desc = a_TOC_desc;
		} else {
			prev_desc->next_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = (CD_TDesc*)prev_desc->next_description;
			prev_desc = a_TOC_desc;
		}

		a_TOC_desc->session = 1; //and for vanilla audio CDs it is session 1, but for multi-session...
#if defined (__ppc__) || defined (__ppc64__)
		a_TOC_desc->controladdress = (toc_entry.cdte_ctrl << 4) | toc_entry.cdte_adr;
#else
		a_TOC_desc->controladdress = (toc_entry.cdte_adr << 4) | toc_entry.cdte_ctrl;
#endif
		a_TOC_desc->unused1 = 0;
		a_TOC_desc->tracknumber = toc_entry.cdte_track;
		a_TOC_desc->rel_minutes = 0; //is there anyway to even find this out on linux without playing the track? //cdmsf_min0
		a_TOC_desc->rel_seconds = 0;
		a_TOC_desc->rel_frames = 0;
		a_TOC_desc->zero_space = 0;
		a_TOC_desc->abs_minutes = toc_entry.cdte_addr.msf.minute;
		a_TOC_desc->abs_seconds = toc_entry.cdte_addr.msf.second;
		a_TOC_desc->abs_frames = toc_entry.cdte_addr.msf.frame;
	}
	return;
}

uint16_t Linux_ioctlProbeTargetDrive(const char* id3args_drive, char* mcdi_data) {
	uint16_t mcdi_data_len = 0;
	int cd_fd = 0;
	
	cd_fd = open(id3args_drive, O_RDONLY | O_NONBLOCK);
	
	if (cd_fd != -1) {
		int cd_mode = ioctl(cd_fd, CDROM_DISC_STATUS);
		if (cd_mode != CDS_AUDIO || cd_mode != CDS_MIXED) {
			Linux_ReadCDTOC(cd_fd);
			mcdi_data_len = FormMCDIdata(mcdi_data);			
		} else {
			//scan for available devices
		}
	} else {
		//scan for available devices
	}
	
	return mcdi_data_len;
}
#endif

#if defined (DARWIN_PLATFORM)
uint16_t Extract_cdTOCrawdata(CFDataRef cdTOCdata, char* cdTOCrawdata) {
	CFRange	cdrange;
	CFIndex cdTOClen = CFDataGetLength (cdTOCdata);
	cdTOCrawdata = (char*)calloc(1, sizeof(char)*cdTOClen+1);	
	cdrange = CFRangeMake(0, cdTOClen+1);
	CFDataGetBytes(cdTOCdata, cdrange, (unsigned char*)cdTOCrawdata);
	
	cdTOC = (CD_TOC_*)calloc(1, sizeof(CD_TOC_));
	cdTOC->toc_length = UInt16FromBigEndian(cdTOCrawdata);
	cdTOC->first_session = cdTOCrawdata[2];
	cdTOC->first_session = cdTOCrawdata[3];
	cdTOC->track_description = NULL;
	
	CD_TDesc* a_TOC_desc = NULL;
	CD_TDesc* prev_desc = NULL;
	
	uint16_t toc_offset = 0;
	for (toc_offset = 4; toc_offset <= cdTOClen; toc_offset +=11) {
		if (cdTOC->track_description == NULL) {
			cdTOC->track_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = cdTOC->track_description;
			prev_desc = a_TOC_desc;
		} else {
			prev_desc->next_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = (CD_TDesc*)prev_desc->next_description;
			prev_desc = a_TOC_desc;
		}
		a_TOC_desc->session = cdTOCrawdata[toc_offset];
		a_TOC_desc->controladdress = cdTOCrawdata[toc_offset+1];
		a_TOC_desc->unused1 = cdTOCrawdata[toc_offset+2];
		a_TOC_desc->tracknumber = cdTOCrawdata[toc_offset+3];
		a_TOC_desc->rel_minutes = cdTOCrawdata[toc_offset+4];
		a_TOC_desc->rel_seconds = cdTOCrawdata[toc_offset+5];
		a_TOC_desc->rel_frames = cdTOCrawdata[toc_offset+6];
		a_TOC_desc->zero_space = 0;
		a_TOC_desc->abs_minutes = cdTOCrawdata[toc_offset+8];
		a_TOC_desc->abs_seconds = cdTOCrawdata[toc_offset+9];
		a_TOC_desc->abs_frames = cdTOCrawdata[toc_offset+10];
	}
	
	return (uint16_t)cdTOClen;
}

void OSX_ReadCDTOC(io_object_t cdobject) {
	CFMutableDictionaryRef cd_props = 0;
	CFDataRef cdTOCdata = NULL;
	char* cdTOCrawdata = NULL;
	
	if (IORegistryEntryCreateCFProperties(cdobject, &cd_props, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess) return;
												
	cdTOCdata = (CFDataRef)CFDictionaryGetValue(cd_props, CFSTR (kIOCDMediaTOCKey));
	if (cdTOCdata != NULL) {
		uint16_t cdTOClen = Extract_cdTOCrawdata(cdTOCdata, cdTOCrawdata);
	}
	CFRelease(cd_props);
	cd_props = NULL;
	return;
}

void OSX_ScanForCDDrive() {
	io_iterator_t	drive_iter	= MACH_PORT_NULL;
	io_object_t		driveobject	= MACH_PORT_NULL;
	CFTypeRef drive_path = NULL;
	char drive_path_str[20];
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(kIOCDMediaClass), &drive_iter) != kIOReturnSuccess) { //create the iterator
		fprintf(stdout, "No device capable of reading cd media present\n");
	}
	
	driveobject = IOIteratorNext(drive_iter);
	while ( driveobject != MACH_PORT_NULL ) {
		drive_path = IORegistryEntryCreateCFProperty(driveobject, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0);
		if (drive_path != NULL) {
			CFStringGetCString((CFStringRef)drive_path, (char*)&drive_path_str, 20, kCFStringEncodingASCII);
			fprintf(stdout, "Device '%s' contains cd media\n", drive_path_str);
			OSX_ReadCDTOC(driveobject);
			if (cdTOC != NULL) {
				uint8_t cdType = DetermineCDType(cdTOC);
				if (cdType == CDOBJECT_AUDIOCD) {
					fprintf(stdout, "Good news, device '%s' is an Audio CD and can be used for 'MCDI' setting\n", drive_path_str);
				} else {
					fprintf(stdout, "Tragically, it was a data CD.\n");
				}
				free(cdTOC); //the other malloced members should be freed also
			}
		}
		IOObjectRelease(driveobject);
		driveobject = IOIteratorNext(drive_iter);
	}
	
	if (drive_path_str[0] == (uint8_t)0x00) {
		fprintf(stdout, "No CD media was found in any device\n");
	}
	IOObjectRelease(drive_iter);
	drive_iter = MACH_PORT_NULL;
	exit(0);
}

uint16_t OSX_ProbeTargetDrive(const char* id3args_drive, char* mcdi_data) {
	uint16_t mcdi_data_len = 0;
	io_object_t	cdobject = MACH_PORT_NULL;
	
	if (memcmp(id3args_drive, "disk", 4) != 0) {
		OSX_ScanForCDDrive();
		exit(0);
	}
	cdobject = IOServiceGetMatchingService(kIOMasterPortDefault, IOBSDNameMatching (kIOMasterPortDefault, 0, id3args_drive) );

	if (cdobject == MACH_PORT_NULL) {
		fprintf(stdout, "No device found at %s; searching for possible drives...\n", id3args_drive);
		OSX_ScanForCDDrive();
		
	} else if (IOObjectConformsTo(cdobject, kIOCDMediaClass) == false) {
		fprintf (stdout, "No cd present in drive at %s\n", id3args_drive );
		IOObjectRelease(cdobject);
		cdobject = MACH_PORT_NULL;
		OSX_ScanForCDDrive();
	} else {
		//we now have a cd object
		OSX_ReadCDTOC(cdobject);
		if (cdTOC != NULL) {
			uint8_t cdType = DetermineCDType(cdTOC);
			if (cdType == CDOBJECT_AUDIOCD) {
				mcdi_data_len = FormMCDIdata(mcdi_data);
			}
		}
	}

	IOObjectRelease(cdobject);
	cdobject = MACH_PORT_NULL;
	return mcdi_data_len;
}

#endif

#if defined (WIN32)
void Windows_ioctlReadCDTOC(HANDLE cdrom_device) {
	DWORD bytes_returned;
	CDROM_TOC win_cdrom_toc;
	
	//WARNING: "This IOCTL is obsolete beginning with the Microsoft Windows Vista. Do not use this IOCTL to develop drivers in Microsoft Windows Vista."
	if (DeviceIoControl(cdrom_device, IOCTL_CDROM_READ_TOC, NULL, 0, &win_cdrom_toc, sizeof(CDROM_TOC), &bytes_returned, NULL) == 0) {
		fprintf(stderr, "AtomicParsley error: there was an error reading the CD Table of Contents header (win32).\n");
		return;
	}
	
	
	cdTOC = (CD_TOC_*)calloc(1, sizeof(CD_TOC_));
	cdTOC->toc_length = ((win_cdrom_toc.Length[0] & 0xFF) << 8) | (win_cdrom_toc.Length[1] & 0xFF);
	//cdTOC->first_session = 0; //seems windows doesn't store session info with IOCTL_CDROM_READ_TOC, all tracks from all sessions are available
	//cdTOC->last_session = 0; //not used anyway; IOCTL_CDROM_READ_TOC_EX could be used to get session information, but its only available on XP
	//...............and interestingly enough in XP, IOCTL_CDROM_TOC_EX returns the leadout track as 0xA2, which makes a Win2k MCDI & a WinXP MCDI different (by 1 byte)
	cdTOC->track_description = NULL;

	CD_TDesc* a_TOC_desc = NULL;
	CD_TDesc* prev_desc = NULL;

	for (uint8_t i = win_cdrom_toc.FirstTrack; i <= win_cdrom_toc.LastTrack+1; i++) {
		TRACK_DATA* thisTrackData = &(win_cdrom_toc.TrackData[i-1]);
		
		if (cdTOC->track_description == NULL) {
			cdTOC->track_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = cdTOC->track_description;
			prev_desc = a_TOC_desc;
		} else {
			prev_desc->next_description = (CD_TDesc*)calloc(1, (sizeof(CD_TDesc)));
			a_TOC_desc = (CD_TDesc*)prev_desc->next_description;
			prev_desc = a_TOC_desc;
		}
		
		a_TOC_desc->session = 1; //and for vanilla audio CDs it is session 1, but for multi-session...
#if defined (__ppc__) || defined (__ppc64__)
		a_TOC_desc->controladdress = (thisTrackData->Control << 4) | thisTrackData->Adr;
#else
		a_TOC_desc->controladdress = (thisTrackData->Adr << 4) | thisTrackData->Control;
#endif
		a_TOC_desc->unused1 = 0;
		a_TOC_desc->tracknumber = thisTrackData->TrackNumber;
		a_TOC_desc->rel_minutes = 0; //didn't look too much into finding this since it is unused
		a_TOC_desc->rel_seconds = 0;
		a_TOC_desc->rel_frames = 0;
		a_TOC_desc->zero_space = 0;
		a_TOC_desc->abs_minutes = thisTrackData->Address[1];
		a_TOC_desc->abs_seconds = thisTrackData->Address[2];
		a_TOC_desc->abs_frames = thisTrackData->Address[3];
	}
	
	return;
}

uint16_t Windows_ioctlProbeTargetDrive(const char* id3args_drive, char* mcdi_data) {
	uint16_t mcdi_data_len = 0;
	char cd_device_path[16];
				
	memset(cd_device_path, 0, 16);
	sprintf(cd_device_path, "\\\\.\\%s:", id3args_drive);
	
	HANDLE cdrom_device = CreateFile(cd_device_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (cdrom_device != INVALID_HANDLE_VALUE) {
		Windows_ioctlReadCDTOC(cdrom_device);
		if (cdTOC != NULL) {
			uint8_t cdType = DetermineCDType(cdTOC);
			if (cdType == CDOBJECT_AUDIOCD) {
				mcdi_data_len = FormMCDIdata(mcdi_data);
			}
		}
		CloseHandle(cdrom_device);
	}
	
	return mcdi_data_len;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////
//                                 CD TOC Entry Area                                 //
///////////////////////////////////////////////////////////////////////////////////////

uint16_t GenerateMCDIfromCD(char* drive, char* dest_buffer) {
	uint16_t mcdi_bytes = 0;
#if defined (DARWIN_PLATFORM)
	mcdi_bytes = OSX_ProbeTargetDrive(drive, dest_buffer);
#elif defined (HAVE_LINUX_CDROM_H)
	mcdi_bytes = Linux_ioctlProbeTargetDrive(drive, dest_buffer);
#elif defined (WIN32)
	mcdi_bytes = Windows_ioctlProbeTargetDrive(drive, dest_buffer);
#endif
	return mcdi_bytes;
}
