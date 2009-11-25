//==================================================================//
/*
    AtomicParsley - AP_NSImage.h

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
                                                                   */
//==================================================================//

struct PicPrefs  {
	int max_dimension;
	int dpi;
	int max_Kbytes;
	bool squareUp;
	bool allJPEG;
	bool allPNG;
	bool addBOTHpix;
	bool removeTempPix;
	bool force_dimensions;
	int force_height;
	int force_width;
};

#if defined (DARWIN_PLATFORM)
bool ResizeGivenImage(const char* filePath, PicPrefs myPicPrefs, char* resized_path);
#endif
