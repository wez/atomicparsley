//==================================================================//
/*
    AtomicParsley - AP_arrays.h

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

char* GenreIntToString(int genre);
uint8_t StringGenreToInt(const char* genre_string);
void ListGenresValues();

stiks* MatchStikString(const char* stik_string);
stiks* MatchStikNumber(uint8_t in_stik_num);
void ListStikValues();

sfIDs* MatchStoreFrontNumber(uint32_t storefrontnum);

bool MatchLanguageCode(const char* in_code);
void ListLanguageCodes();

void ListMediaRatings();
char* Expand_cli_mediastring(char* cli_rating);

char* ID3GenreIntToString(int genre);
uint8_t ID3StringGenreToInt(const char* genre_string);
