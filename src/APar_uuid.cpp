//==================================================================//
/*
    AtomicParsley - APar_uuid.cpp

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

//==================================================================//
/*
    Much of AP_Create_UUID_ver5_sha1_name was derived from
		         http://www.ietf.org/rfc/rfc4122.txt
	  which I don't believe conflicts with or restricts the GPL.
		And this page:
			  http://home.famkruithof.net/guid-uuid-namebased.html
		tells me I'm not on crack when I try to calculate the uuids myself.

			Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
			Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
			Digital Equipment Corporation, Maynard, Mass.
			Copyright (c) 1998 Microsoft.
			To anyone who acknowledges that this file is provided "AS IS"
			without any express or implied warranty: permission to use, copy,
			modify, and distribute this file for any purpose is hereby
			granted without fee, provided that the above copyright notices and
			this notice appears in all source code copies, and that none of
			the names of Open Software Foundation, Inc., Hewlett-Packard
			Company, Microsoft, or Digital Equipment Corporation be used in
			advertising or publicity pertaining to distribution of the software
			without specific, written prior permission. Neither Open Software
			Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital
			Equipment Corporation makes any representations about the
			suitability of this software for any purpose.
                                                                   */
//==================================================================//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AP_commons.h"
#include "APar_sha1.h"
#include "APar_uuid.h"

/*----------------------
print_hash
  hash - the string array of the sha1 hash

		prints out the hex representation of the 16 byte hash - this relates to sha1, but its here to keep the sha1 files as close to original as possible
----------------------*/
void print_hash(char hash[]) {
	for (int i=0; i < 20; i++) {
      fprintf(stdout,"%02x", (uint8_t)hash[i]);
	}
	fprintf(stdout, "\n");
	return;
}

/*----------------------
Swap_Char
  in_str - the string to have the swap operation performed on
	str_len - the amount of bytes to swap in the string

		Make a pointer to the start & end of the string, as well as a temporary string to hold the swapped byte. As the start increments up, the end decrements down. Copy
		the byte at each advancing start position. Copy the byte of the diminishing end string into the start byte, then advance the start byte. Finaly, set each byte of
		the decrementing end pointer to the temp string byte.
----------------------*/
void Swap_Char(char *in_str, uint8_t str_len) {
	char *start_str, *end_str, temp_str;

	start_str= in_str;
	end_str=start_str+ str_len;
	while ( start_str<--end_str ) {
		temp_str=*start_str;
		*start_str++=*end_str;
		*end_str=temp_str;
	}
	return;
}

/*----------------------
APar_endian_uuid_bin_str_conversion
  raw_uuid - a binary string representation of a uuid

		As a string representation of a uuid, there is a 32-bit & 2 16-bit numbers in the uuid. These members need to be swapped on big endian systems.
----------------------*/
void APar_endian_uuid_bin_str_conversion(char* raw_uuid) {
#if defined (__ppc__) || defined (__ppc64__)
	return; //we are *naturally* network byte ordered - simplicity
#else
	Swap_Char(raw_uuid, 4);
	Swap_Char(raw_uuid+4, 2);
	Swap_Char(raw_uuid+4+2, 2);
	return;
#endif
}

/*----------------------
APar_print_uuid
  uuid - a uuid structure containing the uuid

		Print out a full string representation of a uuid
----------------------*/
void APar_print_uuid(ap_uuid_t* uuid, bool new_line) {
	fprintf(stdout, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
									uuid->time_low,
									uuid->time_mid,
									uuid->time_hi_and_version,
									uuid->clock_seq_hi_and_reserved,
									uuid->clock_seq_low,
									uuid->node[0], uuid->node[1], uuid->node[2], uuid->node[3], uuid->node[4], uuid->node[5]);
	if (new_line) fprintf(stdout, "\n");
	return;
}

/*----------------------
APar_sprintf_uuid
  uuid - a uuid structure containing the uuid
	destination - the end result uuid will be placed here

		Put a binary representation of a uuid to a human-readable ordered uuid string
----------------------*/
void APar_sprintf_uuid(ap_uuid_t* uuid, char* destination) {
	sprintf(destination, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
									uuid->time_low,
									uuid->time_mid,
									uuid->time_hi_and_version,
									uuid->clock_seq_hi_and_reserved,
									uuid->clock_seq_low,
									uuid->node[0], uuid->node[1], uuid->node[2], uuid->node[3], uuid->node[4], uuid->node[5]);
	return;
}

/*----------------------
APar_uuid_scanf
  in_formed_uuid - pointer to a string (or a place in a string) where to place a binary (hex representation) string uuid of 16 bytes
	raw_uuid - the string that contains a string representation of a uuid (from cli input for example). This string isn't 16 bytes - its 36

		Skip past a hyphen, make any upper case characters lower (ahh, that hex 'Q') to do a manual scanf on the string. Add its hex representation as a number
		for 1/2 of the bits (a single byte is 2 hex characters), shift it over to the upper bits, and repeat adding the lower bits. Repeat until done.
----------------------*/
uint8_t APar_uuid_scanf(char* in_formed_uuid, char* raw_uuid) {
	char *uuid_str, *end_uuid_str, *uuid_byte;
	uint8_t uuid_pos, uuid_len;
	uint8_t keeprap = 0;
	uuid_len = strlen(raw_uuid); //it will be like "55534d54-21d2-4fce-bb88-695cfac9c740"
	uuid_str = raw_uuid;
	uuid_pos = 0;
	end_uuid_str = uuid_str+uuid_len;
	
	while (uuid_str < end_uuid_str) {
		uuid_byte = &in_formed_uuid[uuid_pos];
		if (uuid_str[0] == '-') uuid_str++;
		if (uuid_str[0] >= 'A' && uuid_str[0] <= 90) uuid_str[0] +=32;
		if (uuid_str[1] >= 'A' && uuid_str[1] <= 90) uuid_str[0] +=32;
		
		for (int i = 0; i <= 1; i++) {
			switch(uuid_str[i]) {
				case '0' :  {
					keeprap = 0;
					break;
				}
				case '1' : {
					keeprap = 1;
					break;
				}
				case '2' : {
					keeprap = 2;
					break;
				}
				case '3' : {
					keeprap = 3;
					break;
				}
				case '4' : {
					keeprap = 4;
					break;
				}
				case '5' : {
					keeprap = 5;
					break;
				}
				case '6' : {
					keeprap = 6;
					break;
				}
				case '7' : {
					keeprap = 7;
					break;
				}
				case '8' : {
					keeprap = 8;
					break;
				}
				case '9' : {
					keeprap = 9;
					break;
				}
				case 'a' :  {
					keeprap = 10;
					break;
				}
				case 'b' :  {
					keeprap = 11;
					break;
				}
				case 'c' :  {
					keeprap = 12;
					break;
				}
				case 'd' :  {
					keeprap = 13;
					break;
				}
				case 'e' :  {
					keeprap = 14;
					break;
				}
				case 'f' :  {
					keeprap = 15;
					break;
				}
			}
			
			if (i == 0) {
				*uuid_byte = keeprap << 4;
			} else {
				*uuid_byte |= keeprap; //(keeprap & 0xF0);
			}
		}
		uuid_str+=2;
		uuid_pos++;
	}
	APar_endian_uuid_bin_str_conversion(in_formed_uuid);
	return uuid_pos;
}

/*----------------------
APar_extract_uuid_version
  uuid - a uuid structure containing the uuid
	binary_uuid_str - a binary string rep of a uuid (without dashes, just the hex)

		Test the 6th byte in a str and push the bits to get the version or take the 3rd member of a uuid (uint16_t) and shift bits by 12
----------------------*/
uint8_t APar_extract_uuid_version(ap_uuid_t* uuid, char* binary_uuid_str) {
	uint8_t uuid_ver = 0;
	if (binary_uuid_str != NULL) {
		uuid_ver = (binary_uuid_str[6] >> 4);
	} else if (uuid != NULL) {
		uuid_ver = (uuid->time_hi_and_version >> 12);
	}
	return uuid_ver;
}

/*----------------------
AP_Create_UUID_ver5_sha1_name
  uuid - pointer to the final version 5 sha1 hash bashed uuid
	desired_namespace - the input uuid used as a basis for the hash; a ver5 uuid is a namespace/name uuid. This is the namespace portion
	name - this is the name portion used to make a v5 sha1 hash
	namelen - length of name (currently a strlen() value)

		This will create a version 5 sha1 based uuid of a name in a namespace. The desired_namespace has its endian members swapped to newtwork byte ordering. The sha1
		hash algorithm is fed the reordered netord_namespace uuid to create a hash; the name is then added to the hash to create a hash of the name in the namespace.
		The final hash is then copied into the out_uuid, and the endian members of the ap_uuid_t are swapped to endian ordering.
----------------------*/
void AP_Create_UUID_ver5_sha1_name(ap_uuid_t* out_uuid, ap_uuid_t desired_namespace, char *name, int namelen) {
	sha1_ctx sha_state;
	char hash[20];
	ap_uuid_t networkorderd_namespace;

	//swap the endian members of uuid to network byte order for hash uniformity across platforms (the NULL or AP.sf.net uuid)
	networkorderd_namespace = desired_namespace;
	networkorderd_namespace.time_low = SWAP32(networkorderd_namespace.time_low);
	networkorderd_namespace.time_mid = SWAP16(networkorderd_namespace.time_mid);
	networkorderd_namespace.time_hi_and_version = SWAP16(networkorderd_namespace.time_hi_and_version);
	
	//make a hash of the input desired_namespace (as netord_ns); add the name (the AP.sf.net namespace as string or the atom name)
	
	sha1_init_ctx(&sha_state);
	sha1_process_bytes( (char*)&networkorderd_namespace, sizeof networkorderd_namespace, &sha_state);
	sha1_process_bytes(name, namelen, &sha_state);
	sha1_finish_ctx(&sha_state, hash);

	// quasi uuid sha1hash is network byte ordered. swap the endian members of the uuid (uint32_t & uint16_t) to local byte order
	// this creates additional requirements later that have to be APar_endian_uuid_bin_str_conversion() swapped, but leave this for now for coherence
	memcpy(out_uuid, hash, sizeof *out_uuid);
	out_uuid->time_low = SWAP32(out_uuid->time_low);
	out_uuid->time_mid = SWAP16(out_uuid->time_mid);
	out_uuid->time_hi_and_version = SWAP16(out_uuid->time_hi_and_version);

	out_uuid->time_hi_and_version &= 0x0FFF;     //mask hash octes 6&7
	out_uuid->time_hi_and_version |= (5 << 12);  //set bits 12-15 to the version (5 for sha1 namespace/name hash uuids)
	out_uuid->clock_seq_hi_and_reserved &= 0x3F; //mask hash octet 8
	out_uuid->clock_seq_hi_and_reserved |= 0x80; //Set the two most significant bits (bits 6 and 7) of the clock_seq_hi_and_reserved to zero and one, respectively.
	return;
}

/*----------------------
AP_Create_UUID_ver3_random
  out_uuid - pointer to the final version 3 randomly generated uuid

		Use a high quality random number generator (still requiring a seed) to generate a random uuid.
		In 2.5 million creations, all have been unique (at least on Mac OS X with sranddev providing the initial seed).
----------------------*/
void AP_Create_UUID_ver3_random(ap_uuid_t* out_uuid) {
	uint32_t rand1 = 0;

	out_uuid->time_low = xor4096i();
	rand1 = xor4096i();
	out_uuid->time_mid = (rand1 >> 16) & 0xFFFF;
	out_uuid->node[0] = (rand1 >> 8) & 0xFF;
	out_uuid->node[1] = (rand1 >> 0) & 0xFF;
	rand1 = xor4096i();
	out_uuid->node[2] = (rand1 >> 24) & 0xFF;
	out_uuid->node[3] = (rand1 >> 16) & 0xFF;
	out_uuid->node[4] = (rand1 >> 8) & 0xFF;
	out_uuid->node[5] = (rand1 >> 0) & 0xFF;
	rand1 = xor4096i();
	out_uuid->time_hi_and_version = (rand1 >> 16) & 0xFFFF;
	out_uuid->clock_seq_low = (rand1 >> 8) & 0xFF;
	out_uuid->clock_seq_hi_and_reserved = (rand1 >> 0) & 0x3F;
	out_uuid->clock_seq_hi_and_reserved |= 0x40; //bits 6 & 7 must be 0 & 1 respectively

	out_uuid->time_hi_and_version &= 0x0FFF;
	out_uuid->time_hi_and_version |= (3 << 12);  //set bits 12-15 to the version (3 for peusdo-random/random)
	return;
}

/*----------------------
APar_generate_uuid_from_atomname
  atom_name - the 4 character atom name to create a uuid for
	uuid_binary_str - the destination for the created uuid (as a hex string)

		This will create a 16-byte universal unique identifier for any atom name in the AtomicParsley namespace.
		1. Make a namespace for all uuids to derive from (here its AP.sf.net)
		2. Make a sha1 hash of the namespace string; use that hash as the basis for a v5 uuid into a NULL/blank uuid to create an AP namespace uuid
		3. Make a sha2 hash of the atom_name string; use that hash as the basis for a v5 uuid into an AtomicParsley namespace uuid to create the final uuid.
		4. copy the uuid structure into a binary string representation
----------------------*/
void APar_generate_uuid_from_atomname(char* atom_name, char* uuid_binary_str) {
	
	ap_uuid_t blank_namespace = { 0x00000000, 0x0000, 0x0000,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	ap_uuid_t APar_namespace_uuid;
	ap_uuid_t AP_atom_uuid;

	AP_Create_UUID_ver5_sha1_name(&APar_namespace_uuid, blank_namespace, "AtomicParsley.sf.net", 20);	
	AP_Create_UUID_ver5_sha1_name(&AP_atom_uuid, APar_namespace_uuid, atom_name, 4);
	
	memset(uuid_binary_str, 0, 20);
	memcpy(uuid_binary_str, &AP_atom_uuid, sizeof(AP_atom_uuid) );
		
	return;
}

void APar_generate_random_uuid(char* uuid_binary_str) {
	ap_uuid_t rand_uuid = { 0x00000000, 0x0000, 0x0000,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	AP_Create_UUID_ver3_random(&rand_uuid);
	memcpy(uuid_binary_str, &rand_uuid, sizeof(rand_uuid) );
	return;
}

void APar_generate_test_uuid() {
	ap_uuid_t blank_ns = { 0x00000000, 0x0000, 0x0000,
													0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	ap_uuid_t APar_ns_uuid;
	ap_uuid_t APar_test_uuid;
	
	AP_Create_UUID_ver5_sha1_name(&APar_ns_uuid, blank_ns, "AtomicParsley.sf.net", 20);
	APar_print_uuid(&APar_ns_uuid);   //should be aa80eaf3-1f72-5575-9faa-de9388dc2a90
	fprintf(stdout, "uuid for 'cprt' in AP namespace: ");
	AP_Create_UUID_ver5_sha1_name(&APar_test_uuid, APar_ns_uuid, "cprt", 4);
	APar_print_uuid(&APar_test_uuid);          //'cprt' should be 4bd39a57-e2c8-5655-a4fb-7a19620ef151
	//uuid_t domain_ns_uuid = { 0x6ba7b810, 0x9dad, 0x11d1,
	//														0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }; // 6ba7b810-9dad-11d1-80b4-00c04fd430c8 a blank representation of a blank domain
	return;
}
