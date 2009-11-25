//==================================================================//
/*
    AtomicParsley - AP_NSFile_utils.h

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

uint32_t APar_4CC_CreatorCode(const char* filepath, uint32_t new_type_code);
void APar_SupplySelectiveTypeCreatorCodes(const char *inputPath, const char *outputPath, uint8_t forced_type_code);
