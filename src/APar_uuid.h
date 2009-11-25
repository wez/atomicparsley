//==================================================================//
/*
    AtomicParsley - APar_uuid.h

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

typedef struct {
    uint32_t       time_low;
    uint16_t       time_mid;
    uint16_t       time_hi_and_version;
    uint8_t        clock_seq_hi_and_reserved;
    uint8_t        clock_seq_low;
    unsigned char  node[6];
} ap_uuid_t;

void APar_print_uuid(ap_uuid_t* uuid, bool new_line = true);
void APar_sprintf_uuid(ap_uuid_t* uuid, char* destination);
uint8_t APar_uuid_scanf(char* in_formed_uuid, char* raw_uuid);

void APar_endian_uuid_bin_str_conversion(char* raw_uuid);

uint8_t APar_extract_uuid_version(ap_uuid_t* uuid, char* binary_uuid_str);
void APar_generate_uuid_from_atomname(char* atom_name, char* uuid_binary_str);
void APar_generate_random_uuid(char* uuid_binary_str);
