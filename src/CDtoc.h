//==================================================================//
/*
    AtomicParsley - CDtoc.h

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

#include "ap_types.h"

#if defined (_WIN32)
//these #defines & structs are copied from the MS W2k DDK headers so the entire DDK isn't required to be installed to compile AP_CDTOC for MCDI support
#ifndef DEVICE_TYPE
#define DEVICE_TYPE ULONG
#endif

#define FILE_DEVICE_CD_ROM           0x00000002

#define IOCTL_CDROM_BASE             FILE_DEVICE_CD_ROM

#define METHOD_BUFFERED              0

#define FILE_READ_ACCESS             ( 0x0001 )    // file & pipe

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_CDROM_READ_TOC         CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)

#define MAXIMUM_NUMBER_TRACKS 100

//
// CD ROM Table OF Contents (TOC)
// Format 0 - Get table of contents
//

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {

    //
    // Header
    //

    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;

    //
    // Track data
    //

    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;
#endif

uint16_t GenerateMCDIfromCD(const char* drive, char* dest_buffer);
