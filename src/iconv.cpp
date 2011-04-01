//==================================================================//
/*
    AtomicParsley - iconv.cpp

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
    with contributions from others; see the CREDITS file
*/
//==================================================================//
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

#include "AtomicParsley.h"

const unsigned short cp437upperbytes[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

const unsigned short cp850upperbytes[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0,
	0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
	0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE,
	0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
	0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
	0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
	0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
	0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0
};

const unsigned short cp852upperbytes[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x016F, 0x0107, 0x00E7,
	0x0142, 0x00EB, 0x0150, 0x0151, 0x00EE, 0x0179, 0x00C4, 0x0106,
	0x00C9, 0x0139, 0x013A, 0x00F4, 0x00F6, 0x013D, 0x013E, 0x015A,
	0x015B, 0x00D6, 0x00DC, 0x0164, 0x0165, 0x0141, 0x00D7, 0x010D,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x0104, 0x0105, 0x017D, 0x017E,
	0x0118, 0x0119, 0x00AC, 0x017A, 0x010C, 0x015F, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x011A,
	0x015E, 0x2563, 0x2551, 0x2557, 0x255D, 0x017B, 0x017C, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x0102, 0x0103,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
	0x0111, 0x0110, 0x010E, 0x00CB, 0x010F, 0x0147, 0x00CD, 0x00CE,
	0x011B, 0x2518, 0x250C, 0x2588, 0x2584, 0x0162, 0x016E, 0x2580,
	0x00D3, 0x00DF, 0x00D4, 0x0143, 0x0144, 0x0148, 0x0160, 0x0161,
	0x0154, 0x00DA, 0x0155, 0x0170, 0x00FD, 0x00DD, 0x0163, 0x00B4,
	0x00AD, 0x02DD, 0x02DB, 0x02C7, 0x02D8, 0x00A7, 0x00F7, 0x00B8,
	0x00B0, 0x00A8, 0x02D9, 0x0171, 0x0158, 0x0159, 0x25A0, 0x00A0
};

const unsigned short cp855upperbytes[128] = {
	0x0452, 0x0402, 0x0453, 0x0403, 0x0451, 0x0401, 0x0454, 0x0404,
	0x0455, 0x0405, 0x0456, 0x0406, 0x0457, 0x0407, 0x0458, 0x0408,
	0x0459, 0x0409, 0x045A, 0x040A, 0x045B, 0x040B, 0x045C, 0x040C,
	0x045E, 0x040E, 0x045F, 0x040F, 0x044E, 0x042E, 0x044A, 0x042A,
	0x0430, 0x0410, 0x0431, 0x0411, 0x0446, 0x0426, 0x0434, 0x0414,
	0x0435, 0x0415, 0x0444, 0x0424, 0x0433, 0x0413, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x0445, 0x0425, 0x0438,
	0x0418, 0x2563, 0x2551, 0x2557, 0x255D, 0x0439, 0x0419, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x043A, 0x041A,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
	0x043B, 0x041B, 0x043C, 0x041C, 0x043D, 0x041D, 0x043E, 0x041E,
	0x043F, 0x2518, 0x250C, 0x2588, 0x2584, 0x041F, 0x044F, 0x2580,
	0x042F, 0x0440, 0x0420, 0x0441, 0x0421, 0x0442, 0x0422, 0x0443,
	0x0423, 0x0436, 0x0416, 0x0432, 0x0412, 0x044C, 0x042C, 0x2116,
	0x00AD, 0x044B, 0x042B, 0x0437, 0x0417, 0x0448, 0x0428, 0x044D,
	0x042D, 0x0449, 0x0429, 0x0447, 0x0427, 0x00A7, 0x25A0, 0x00A0
};

const unsigned short cp858upperbytes[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0,
	0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
	0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x20AC, 0x00CD, 0x00CE,
	0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
	0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
	0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
	0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
	0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0
};

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

static int xmlLittleEndian = 
#ifdef WORDS_BIGENDIAN
	0
#else
	1
#endif
	;

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

#if defined (_WIN32) && !defined (__CYGWIN__)
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
