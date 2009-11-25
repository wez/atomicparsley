//==================================================================//
/*
    AtomicParsley - AP_iconv.cpp

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

#include <string.h>
#include <stdlib.h>

#if defined (WIN32)
#include <windows.h>
#include "AP_iconv.h"
#endif

#include "AP_commons.h"

//==================================================================//
// utf conversion functions from libxml2
/*

 Copyright (C) 1998-2003 Daniel Veillard.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is fur-
nished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
DANIEL VEILLARD BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Daniel Veillard shall not
be used in advertising or otherwise to promote the sale, use or other deal-
ings in this Software without prior written authorization from him.

*/

// Original code for IsoLatin1 and UTF-16 by "Martin J. Duerst" <duerst@w3.org>

static int xmlLittleEndian = 1;

void xmlInitEndianDetection() {
	unsigned short int tst = 0x1234;
	unsigned char *ptr = (unsigned char *) &tst;

	if (*ptr == 0x12) xmlLittleEndian = 0;
    else if (*ptr == 0x34) xmlLittleEndian = 1;
		
	return;
}

/**
 * isolat1ToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of ISO Latin 1 chars
 * @inlen:  the length of @in
 *
 * Take a block of ISO Latin 1 chars in and try to convert it to an UTF-8
 * block of chars out.
 * Returns the number of bytes written if success, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     if the return value is positive, else unpredictable.
 * The value of @outlen after return is the number of octets consumed.
 */
int isolat1ToUTF8(unsigned char* out, int outlen, const unsigned char* in, int inlen) {
    unsigned char* outstart = out;
    const unsigned char* base = in;
    unsigned char* outend;
    const unsigned char* inend;
    const unsigned char* instop;

    if ((out == NULL) || (in == NULL) || (outlen == 0) || (inlen == 0))
	return(-1);

    outend = out + outlen;
    inend = in + (inlen);
    instop = inend;
    
    while (in < inend && out < outend - 1) {
    	if (*in >= 0x80) {
	    *out++ = (((*in) >>  6) & 0x1F) | 0xC0;
        *out++ = ((*in) & 0x3F) | 0x80;
	    ++in;
	}
	if (instop - in > outend - out) instop = in + (outend - out); 
	while (in < instop && *in < 0x80) {
	    *out++ = *in++;
	}
    }	
    if (in < inend && out < outend && *in < 0x80) {
        *out++ = *in++;
    }
    outlen = out - outstart;
    inlen = in - base;
    return(outlen);
}

/**
 * UTF8Toisolat1:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an ISO Latin 1
 * block of chars out.
 *
 * Returns the number of bytes written if success, -2 if the transcoding fails,
           or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     if the return value is positive, else unpredictable.
 * The value of @outlen after return is the number of octets consumed.
 */
int UTF8Toisolat1(unsigned char* out, int outlen, const unsigned char* in, int inlen) {
    const unsigned char* processed = in;
    const unsigned char* outend;
    const unsigned char* outstart = out;
    const unsigned char* instart = in;
    const unsigned char* inend;
    unsigned int c, d;
    int trailing;

    if ((out == NULL) || (outlen == 0) || (inlen == 0)) return(-1);
    if (in == NULL) {
        /*
	 * initialization nothing to do
	 */
	outlen = 0;
	inlen = 0;
	return(0);
    }
    inend = in + (inlen);
    outend = out + (outlen);
    while (in < inend) {
	d = *in++;
	if      (d < 0x80)  { c= d; trailing= 0; }
	else if (d < 0xC0) {
	    /* trailing byte in leading position */
	    outlen = out - outstart;
	    inlen = processed - instart;
	    return(-2);
        } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
        else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
        else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
	else {
	    /* no chance for this in IsoLat1 */
	    outlen = out - outstart;
	    inlen = processed - instart;
	    return(-2);
	}

	if (inend - in < trailing) {
	    break;
	} 

	for ( ; trailing; trailing--) {
	    if (in >= inend)
		break;
	    if (((d= *in++) & 0xC0) != 0x80) {
		outlen = out - outstart;
		inlen = processed - instart;
		return(-2);
	    }
	    c <<= 6;
	    c |= d & 0x3F;
	}

	/* assertion: c is a single UTF-4 value */
	if (c <= 0xFF) {
	    if (out >= outend)
		break;
	    *out++ = c;
	} else {
	    /* no chance for this in IsoLat1 */
	    outlen = out - outstart;
	    inlen = processed - instart;
	    return(-2);
	}
	processed = in;
    }
    outlen = out - outstart;
    inlen = processed - instart;
    return(outlen);
}

/**
 * UTF16BEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16 passed as a byte array
 * @inlenb:  the length of @in in UTF-16 chars
 *
 * Take a block of UTF-16 ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assumes the endian property
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of bytes written, or -1 if lack of space, or -2
 *     if the transcoding fails (if *in is not a valid utf16 string)
 * The value of *inlen after return is the number of octets consumed
 *     if the return value is positive, else unpredictable.
 */
int UTF16BEToUTF8(unsigned char* out, int outlen, const unsigned char* inb, int inlenb)
{
    unsigned char* outstart = out;
    const unsigned char* processed = inb;
    unsigned char* outend = out + outlen;
    unsigned short* in = (unsigned short*) inb;
    unsigned short* inend;
    unsigned int c, d, inlen;
    unsigned char *tmp;
    int bits;

    if ((inlenb % 2) == 1)
        (inlenb)--;
    inlen = inlenb / 2;
    inend= in + inlen;
    while (in < inend) {
	if (xmlLittleEndian) {
	    tmp = (unsigned char *) in;
	    c = *tmp++;
	    c = c << 8;
	    c = c | (unsigned int) *tmp;
	    in++;
	} else {
	    c= *in++;
			
			if (c == 0xFEFF) {
				c= *in++; //skip BOM
			}
	} 
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
	    if (in >= inend) {           /* (in > inend) shouldn't happens */
		outlen = out - outstart;
		inlenb = processed - inb;
	        return(-2);
	    }
	    if (xmlLittleEndian) {
		tmp = (unsigned char *) in;
		d = *tmp++;
		d = d << 8;
		d = d | (unsigned int) *tmp;
		in++;
	    } else {
		d= *in++;
	    }
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else {
		outlen = out - outstart;
		inlenb = processed - inb;
	        return(-2);
	    }
        }

	/* assertion: c is a single UTF-4 value */
        if (out >= outend) 
	    break;
        if      (c <    0x80) {  *out++=  c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }
 
        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend) 
	        break;
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
	processed = (const unsigned char*) in;
    }
    outlen = out - outstart;
    inlenb = processed - inb;
    return(outlen);
}
	 
/**
 * UTF8ToUTF16BE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16BE
 * block of chars out.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding failed. 
 */
int UTF8ToUTF16BE(unsigned char* outb, int outlen, const unsigned char* in, int inlen)
{
    unsigned short* out = (unsigned short*) outb;
    const unsigned char* processed = in;
    const unsigned char *const instart = in;
    unsigned short* outstart= out;
    unsigned short* outend;
    const unsigned char* inend= in+inlen;
    unsigned int c, d;
    int trailing;
    unsigned char *tmp;
    unsigned short tmp1, tmp2;

    /* UTF-16BE has no BOM */
    if ((outb == NULL) || (outlen == 0) || (inlen == 0)) return(-1);
    if (in == NULL) {
	outlen = 0;
	inlen = 0;
	return(0);
    }
    outend = out + (outlen / 2);
    while (in < inend) {
      d= *in++;
      if      (d < 0x80)  { c= d; trailing= 0; }
      else if (d < 0xC0)  {
          /* trailing byte in leading position */
	  outlen = out - outstart;
	  inlen = processed - instart;
	  return(-2);
      } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
      else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
      else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
      else {
          /* no chance for this in UTF-16 */
	  outlen = out - outstart;
	  inlen = processed - instart;
	  return(-2);
      }

      if (inend - in < trailing) {
          break;
      } 

      for ( ; trailing; trailing--) {
          if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))  break;
          c <<= 6;
          c |= d & 0x3F;
      }

      /* assertion: c is a single UTF-4 value */
        if (c < 0x10000) {
            if (out >= outend)  break;
	    if (xmlLittleEndian) {
		tmp = (unsigned char *) out;
		*tmp = c >> 8;
		*(tmp + 1) = c;
		out++;
	    } else {
		*out++ = c;
	    }
        }
        else if (c < 0x110000) {
            if (out+1 >= outend)  break;
            c -= 0x10000;
	    if (xmlLittleEndian) {
		tmp1 = 0xD800 | (c >> 10);
		tmp = (unsigned char *) out;
		*tmp = tmp1 >> 8;
		*(tmp + 1) = (unsigned char) tmp1;
		out++;

		tmp2 = 0xDC00 | (c & 0x03FF);
		tmp = (unsigned char *) out;
		*tmp = tmp2 >> 8;
		*(tmp + 1) = (unsigned char) tmp2;
		out++;
	    } else {
		*out++ = 0xD800 | (c >> 10);
		*out++ = 0xDC00 | (c & 0x03FF);
	    }
        }
        else
	    break;
	processed = in;
    }
    outlen = (out - outstart) * 2;
    inlen = processed - instart;
    return(outlen);
}

/**
 * UTF16LEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16LE passwd as a byte array
 * @inlenb:  the length of @in in UTF-16LE chars
 *
 * Take a block of UTF-16LE ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assumes the endian property
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of bytes written, or -1 if lack of space, or -2
 *     if the transcoding fails (if *in is not a valid utf16 string)
 *     The value of *inlen after return is the number of octets consumed
 *     if the return value is positive, else unpredictable.
 */
int UTF16LEToUTF8(unsigned char* out, int outlen, const unsigned char* inb, int inlenb)
{
    unsigned char* outstart = out;
    const unsigned char* processed = inb;
    unsigned char* outend = out + outlen;
    unsigned short* in = (unsigned short*) inb;
    unsigned short* inend;
    unsigned int c, d, inlen;
    unsigned char *tmp;
    int bits;

    if ((inlenb % 2) == 1)
        (inlenb)--;
    inlen = inlenb / 2;
    inend = in + inlen;
    while ((in < inend) && (out - outstart + 5 < outlen)) {
        if (xmlLittleEndian) {
	    c= *in++;
	} else {
	    tmp = (unsigned char *) in;
	    c = *tmp++;
	    c = c | (((unsigned int)*tmp) << 8);
	    in++;
	}
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
	    if (in >= inend) {           /* (in > inend) shouldn't happens */
		break;
	    }
	    if (xmlLittleEndian) {
		d = *in++;
	    } else {
		tmp = (unsigned char *) in;
		d = *tmp++;
		d = d | (((unsigned int)*tmp) << 8);
		in++;
	    }
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else {
		outlen = out - outstart;
		inlenb = processed - inb;
	        return(-2);
	    }
        }

	/* assertion: c is a single UTF-4 value */
        if (out >= outend)
	    break;
        if      (c <    0x80) {  *out++=  c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }
 
        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend)
	        break;
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
	processed = (const unsigned char*) in;
    }
    outlen = out - outstart;
    inlenb = processed - inb;
    return(outlen);
}

/**
 * UTF8ToUTF16LE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16LE
 * block of chars out.
 *
 * Returns the number of bytes written, or -1 if lack of space, or -2
 *     if the transcoding failed. 
 */
int UTF8ToUTF16LE(unsigned char* outb, int outlen, const unsigned char* in, int inlen)
{
    unsigned short* out = (unsigned short*) outb;
    const unsigned char* processed = in;
    const unsigned char *const instart = in;
    unsigned short* outstart= out;
    unsigned short* outend;
    const unsigned char* inend= in+inlen;
    unsigned int c, d;
    int trailing;
    unsigned char *tmp;
    unsigned short tmp1, tmp2;

    /* UTF16LE encoding has no BOM */
    if ((out == NULL) || (outlen == 0) || (inlen == 0)) return(-1);
    if (in == NULL) {
		outlen = 0;
		inlen = 0;
		return(0);
    }
    outend = out + (outlen / 2);
    while (in < inend) {
      d= *in++;
      if      (d < 0x80)  { c= d; trailing= 0; }
      else if (d < 0xC0) {
          /* trailing byte in leading position */
	  outlen = (out - outstart) * 2;
	  inlen = processed - instart;
	  return(-2);
      } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
      else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
      else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
      else {
	/* no chance for this in UTF-16 */
	outlen = (out - outstart) * 2;
	inlen = processed - instart;
	return(-2);
      }

      if (inend - in < trailing) {
          break;
      } 

      for ( ; trailing; trailing--) {
          if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))
	      break;
          c <<= 6;
          c |= d & 0x3F;
      }

      /* assertion: c is a single UTF-4 value */
        if (c < 0x10000) {
            if (out >= outend)
	        break;
	    if (xmlLittleEndian) {
		*out++ = c;
	    } else {
		tmp = (unsigned char *) out;
		*tmp = c ;
		*(tmp + 1) = c >> 8 ;
		out++;
	    }
        }
        else if (c < 0x110000) {
            if (out+1 >= outend)
	        break;
            c -= 0x10000;
	    if (xmlLittleEndian) {
		*out++ = 0xD800 | (c >> 10);
		*out++ = 0xDC00 | (c & 0x03FF);
	    } else {
		tmp1 = 0xD800 | (c >> 10);
		tmp = (unsigned char *) out;
		*tmp = (unsigned char) tmp1;
		*(tmp + 1) = tmp1 >> 8;
		out++;

		tmp2 = 0xDC00 | (c & 0x03FF);
		tmp = (unsigned char *) out;
		*tmp  = (unsigned char) tmp2;
		*(tmp + 1) = tmp2 >> 8;
		out++;
	    }
        }
        else
	    break;
	processed = in;
    }
    outlen = (out - outstart) * 2;
    inlen = processed - instart;
    return(outlen);
}

int isUTF8(const char* in_string) {
	//fprintf(stdout, "utf8 test-> %s\n", in_string);
	int str_bytes = 0;
	if (in_string != NULL ) {
		str_bytes = strlen(in_string);
	} else {
		return -1;
	}
	
	bool is_validUTF8 = true;
	bool is_high_ascii= false;

	int index = 0;
	while (index < str_bytes && is_validUTF8) {
		char achar = in_string[index];
		int supplemental_bytes = 0;
		
		if ( (unsigned char)achar > 0x80) {
			is_high_ascii = true;
		}
		
		if ((achar & 0x80) == 0) {  // 0xxxxxxx 
			++index;
			
		} else if ((achar & 0xF8) == 0xF0) {  // 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
			++index;
			supplemental_bytes = 3;
			is_high_ascii = true;
		
		} else if ((achar & 0xE0) == 0xE0) {  // 1110zzzz 10yyyyyy 10xxxxxx
			++index;
			supplemental_bytes = 2;
			is_high_ascii = true;
		
		} else if ((achar & 0xE0) == 0xC0) {  // 110yyyyy 10xxxxxx
			++index;
			supplemental_bytes = 1;
			is_high_ascii = true;
			
		} else {
			is_validUTF8 = false;
		}
			
		while (is_validUTF8 && supplemental_bytes--) {
			if (index >= str_bytes) {
				is_validUTF8 = false;
			} else if ((in_string[index++] & 0xC0) != 0x80) {  // 10uuzzzz
				is_validUTF8 = false;
			}
		}
	}
	
	if (is_high_ascii) {
		return 8;
	} else if (is_validUTF8) {
		return 1;
	} else {
		return 0;
	}
}

/*----------------------
utf8_length
  in_string - pointer to location of a utf8 string
	char_limit - either 0 (count all characters) or non-zero (limit utf8 to that character count)

    Because of the lovely way utf8 is aligned, test only the first byte in each. If char_limit is 0, return the number of CHARACTERS in the string, if the
		char_limit is not zero (the char_limit will equal utf_string_leghth because of the break), so change gears, save space and just return the byte_count.
----------------------*/
#include <stdio.h>
unsigned int utf8_length(const char *in_string, unsigned int char_limit) {
	const char *utf8_str = in_string;
	unsigned int utf8_string_length = 0;
	unsigned int in_str_len = strlen(in_string);
	unsigned int byte_count = 0;
	unsigned int bytes_in_char = 0;

	if (in_string == NULL) return 0;

	while ( byte_count < in_str_len) {
		bytes_in_char = 0;
		if ((*utf8_str & 0x80) == 0x00)
			bytes_in_char = 1;
		else if ((*utf8_str & 0xE0) == 0xC0)
			bytes_in_char = 2;
		else if ((*utf8_str & 0xF0) == 0xE0)
			bytes_in_char = 3;
		else if ((*utf8_str & 0xF8) == 0xF0)
			bytes_in_char = 4;
		
		if (bytes_in_char > 0) {
			utf8_string_length++;
			utf8_str += bytes_in_char;
			byte_count += bytes_in_char;
		} else {
			break;
		} 
				
		if (char_limit != 0 && char_limit == utf8_string_length) {
			utf8_string_length = byte_count;
			break;
		}
	}
	return utf8_string_length;
}

#if defined (WIN32)
unsigned char APar_Return_rawutf8_CP(unsigned short cp_bound_glyph) {
	unsigned short total_known_points = 0;
	unsigned int win32cp = GetConsoleCP();
	
	if (win32cp == 437 || win32cp == 850 || win32cp == 852 || win32cp == 855 || win32cp == 858) {
		total_known_points = 128;
	} else {
		if (cp_bound_glyph >= 0x0080) {
			exit(win32cp);
		}
	}
	if (cp_bound_glyph < 0x0080) {
		return cp_bound_glyph << 0;
	} else if (total_known_points) {
		if (win32cp == 437) {
			for (uint16_t i = 0; i < total_known_points; i++) {
				if (cp_bound_glyph == cp437upperbytes[i]) {
					return i+128;
				}
			}
		} else if (win32cp == 850) {
			for (uint16_t i = 0; i < total_known_points; i++) {
				if (cp_bound_glyph == cp850upperbytes[i]) {
					return i+128;
				}
			}
		} else if (win32cp == 852) {
			for (uint16_t i = 0; i < total_known_points; i++) {
				if (cp_bound_glyph == cp852upperbytes[i]) {
					return i+128;
				}
			}
		} else if (win32cp == 855) {
			for (uint16_t i = 0; i < total_known_points; i++) {
				if (cp_bound_glyph == cp855upperbytes[i]) {
					return i+128;
				}
			}
		} else if (win32cp == 858) {
			for (uint16_t i = 0; i < total_known_points; i++) {
				if (cp_bound_glyph == cp858upperbytes[i]) {
					return i+128;
				}
			}
		} else {
			fprintf(stderr, "AtomicParsley error: this windows codepage(%u) is unsupported.\nProvide the output of the 'CPTester' utility run from the bat script\n", win32cp);
			exit(win32cp);
		}
	}
	return 0;
}

int strip_bogusUTF16toRawUTF8 (unsigned char* out, int inlen, wchar_t* in, int outlen) {

    unsigned char* outstart = out;
    unsigned char* outend;
    const wchar_t* inend;
    const wchar_t* instop;

    if ((out == NULL) || (in == NULL) || (outlen == 0) || (inlen == 0))
	return(-1);

    outend = out + outlen;
    inend = in + (inlen);
    instop = inend;
    
    while (in < inend && out < outend - 1) {
    	*out++ = APar_Return_rawutf8_CP(*in); //*in << 0;
	    ++in;
	}
    outlen = out - outstart;
    return(outlen);
}
#endif

/*----------------------
test_conforming_alpha_string
  in_string - pointer to location of a utf8 string

    limit string to A-Z or a-z
----------------------*/
int test_conforming_alpha_string(char* in_string) {
	int valid_bytes = 0;
	int string_len = 0;
	char* test_str = in_string;
	if (in_string != NULL ) {
		string_len = strlen(in_string);
	} else {
		return -1;
	}
	
	while (valid_bytes < string_len) {
		if ( (*test_str >= 65  && *test_str <= 90) || (*test_str >= 97 && *test_str <= 122) || *test_str == 95 || (*test_str >= 48  && *test_str <= 57) ) {
			valid_bytes++;
		} else {
			break;
		}
		test_str++;
	}
	return valid_bytes;
}

bool test_limited_ascii(char* in_string, unsigned int str_len) {
	char* test_str = in_string;
	while (test_str < in_string+str_len) {
		if ( *test_str < 32 || *test_str > 126) {
			return false;
		}
		test_str++;
	}
	return true;
}
