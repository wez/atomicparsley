//==================================================================//
/*
    AtomicParsley - APar_zlib.h

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

#if defined (WIN32)
bool APar_win32_zlib_LoadLibrary();
void APar_win32_zlib_FreeLibrary();
#endif

void APar_zlib_inflate(char* in_buffer, uint32_t in_buf_len, char* out_buffer, uint32_t out_buf_len);
uint32_t APar_zlib_deflate(char* in_buffer, uint32_t in_buf_len, char* out_buffer, uint32_t out_buf_len);
