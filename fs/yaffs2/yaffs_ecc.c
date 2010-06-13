/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This code implements the ECC algorithm used in SmartMedia.
 *
 * The ECC comprises 22 bits of parity information and is stuffed into 3 bytes. 
 * The two unused bit are set to 1.
 * The ECC can correct single bit errors in a 256-byte page of data. Thus, two such ECC 
 * blocks are used on a 512-byte NAND page.
 *
 */

/* Table generated by gen-ecc.c
 * Using a table means we do not have to calculate p1..p4 and p1'..p4'
 * for each byte of data. These are instead provided in a table in bits7..2.
 * Bit 0 of each entry indicates whether the entry has an odd or even parity, and therefore
 * this bytes influence on the line parity.
 */

const char *yaffs_ecc_c_version =
    "$Id: yaffs_ecc.c,v 1.2 2008-07-17 23:07:00 lhhuang Exp $";

#include "yportenv.h"

#include "yaffs_ecc.h"

static const unsigned char column_parity_table[] = {
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
};

/* Count the bits in an unsigned char or a U32 */

static int yaffs_CountBits(unsigned char x)
{
	int r = 0;
	while (x) {
		if (x & 1)
			r++;
		x >>= 1;
	}
	return r;
}

/* Calculate the ECC for a 256-byte block of data */
void yaffs_ECCCalculate(const unsigned char *data, unsigned char *ecc)
{
	unsigned int i;

	unsigned char col_parity = 0;
	unsigned char line_parity = 0;
	unsigned char line_parity_prime = 0;
	unsigned char t;
	unsigned char b;

	for (i = 0; i < 256; i++) {
		b = column_parity_table[*data++];
		col_parity ^= b;

		if (b & 0x01)	// odd number of bits in the byte
		{
			line_parity ^= i;
			line_parity_prime ^= ~i;
		}

	}

	ecc[2] = (~col_parity) | 0x03;

	t = 0;
	if (line_parity & 0x80)
		t |= 0x80;
	if (line_parity_prime & 0x80)
		t |= 0x40;
	if (line_parity & 0x40)
		t |= 0x20;
	if (line_parity_prime & 0x40)
		t |= 0x10;
	if (line_parity & 0x20)
		t |= 0x08;
	if (line_parity_prime & 0x20)
		t |= 0x04;
	if (line_parity & 0x10)
		t |= 0x02;
	if (line_parity_prime & 0x10)
		t |= 0x01;
	ecc[1] = ~t;

	t = 0;
	if (line_parity & 0x08)
		t |= 0x80;
	if (line_parity_prime & 0x08)
		t |= 0x40;
	if (line_parity & 0x04)
		t |= 0x20;
	if (line_parity_prime & 0x04)
		t |= 0x10;
	if (line_parity & 0x02)
		t |= 0x08;
	if (line_parity_prime & 0x02)
		t |= 0x04;
	if (line_parity & 0x01)
		t |= 0x02;
	if (line_parity_prime & 0x01)
		t |= 0x01;
	ecc[0] = ~t;

#ifdef CONFIG_YAFFS_ECC_WRONG_ORDER
	// Swap the bytes into the wrong order
	t = ecc[0];
	ecc[0] = ecc[1];
	ecc[1] = t;
#endif
}


/* Correct the ECC on a 256 byte block of data */

int yaffs_ECCCorrect(unsigned char *data, unsigned char *read_ecc,
		     const unsigned char *test_ecc)
{
	unsigned char d0, d1, d2;	/* deltas */

	d0 = read_ecc[0] ^ test_ecc[0];
	d1 = read_ecc[1] ^ test_ecc[1];
	d2 = read_ecc[2] ^ test_ecc[2];

	if ((d0 | d1 | d2) == 0)
		return 0; /* no error */

	if (((d0 ^ (d0 >> 1)) & 0x55) == 0x55 &&
	    ((d1 ^ (d1 >> 1)) & 0x55) == 0x55 &&
	    ((d2 ^ (d2 >> 1)) & 0x54) == 0x54) {
		/* Single bit (recoverable) error in data */

		unsigned byte;
		unsigned bit;

#ifdef CONFIG_YAFFS_ECC_WRONG_ORDER
		// swap the bytes to correct for the wrong order
		unsigned char t;

		t = d0;
		d0 = d1;
		d1 = t;
#endif

		bit = byte = 0;

		if (d1 & 0x80)
			byte |= 0x80;
		if (d1 & 0x20)
			byte |= 0x40;
		if (d1 & 0x08)
			byte |= 0x20;
		if (d1 & 0x02)
			byte |= 0x10;
		if (d0 & 0x80)
			byte |= 0x08;
		if (d0 & 0x20)
			byte |= 0x04;
		if (d0 & 0x08)
			byte |= 0x02;
		if (d0 & 0x02)
			byte |= 0x01;

		if (d2 & 0x80)
			bit |= 0x04;
		if (d2 & 0x20)
			bit |= 0x02;
		if (d2 & 0x08)
			bit |= 0x01;

		data[byte] ^= (1 << bit);

		return 1; /* Corrected the error */
	}

	if ((yaffs_CountBits(d0) + 
	     yaffs_CountBits(d1) + 
	     yaffs_CountBits(d2)) ==  1) {
		/* Reccoverable error in ecc */

		read_ecc[0] = test_ecc[0];
		read_ecc[1] = test_ecc[1];
		read_ecc[2] = test_ecc[2];

		return 1; /* Corrected the error */
	}
	
	/* Unrecoverable error */

	return -1;

}

#if defined(CONFIG_YAFFS_ECC_RS)
#ifdef __KERNEL__ 
#include <linux/rslib.h>
struct rs_control *rs_decoder;
#else
#include "ssfdc_rs_ecc.h"
void *rs_init_user;
#endif

/* Transfer 16 bytes to 26 5-bit symbols */
void Data2Sym(const unsigned char *in, unsigned char *out)
{
	int i, j, shift;

        for (i = 0; i < 26; i++){
		j = (5 * i) >> 3;
		shift = (5 * i) & 0x7;
		if (shift > 3)
			out[i] = ((in[j] >> shift) | (in[j+1] << (8 - shift))) & 0x1f;
		else
			out[i] = (in[j] >> shift) & 0x1f;
	}
	out[25] &= 0x7; /* the last symbol has only 3 bits */ 
}

/* Transfer 26 5-bit symbols to 16 bytes*/
void Sym2Data(unsigned char *in, unsigned char *out)
{
	int i, j,n;
	unsigned long long ullin = 0,ullout = 0;
	
	n = 0;
	for(j = 0;j < 26 / 8;j++)
	{
		ullin = *((unsigned long long *)in + j);
	    ullout = 0;
	    for (i = 0; i < 8; i++){
			ullout |= (((ullin >> (i * 8)) & 0x1f) << (i * 5));
		}
		memcpy((out + n),(unsigned char *)&ullout,5);
		n += 5;
	}
	out[15] = ((in[24] & 0x1f)| ((in[25] & 0x3) << 5));
}


/*
 * It does reed solomon ECC calcs on 16 bytes of oob data
 */
void yaffs_ECCCalculateOther(const unsigned char *data, unsigned nBytes,
			     yaffs_ECCOther * eccOther)
{
	unsigned short *par = (unsigned short *)&eccOther->lineParity;
	unsigned char data5[26];


	Data2Sym(data, data5);
	memset(par, 0, 8);

#ifdef __KERNEL__ 
        /* Encode 26 symbols in data5. Store parities of 4 symbols in 
         * buffer par whose size is 8 bytes(2 bytes per symbol) */
	encode_rs8 (rs_decoder, data5, 26, par, 0);
#else
        /* init reed solomon ECC for nand oob area
	 * Symbolsize is 5 (bits)
	 * Primitive polynomial is x^5+x^2+1
	 * first consecutive root is 0
	 * primitive element to generate roots = 1
	 * generator polynomial degree (number of roots) = 4
         * pad = (1<<Symbolsize-1) - nroot - 26 = 1
	 */
	rs_init_user = (void *) init_rs_int (5, 0x25, 1, 1, 4, 1);
	encode_rs(rs_init_user, data5, par);
	free_rs_int(rs_init_user);
#endif
}

#ifdef __KERNEL__ 
/*
 * It does reed solomon ECC correction on 16 bytes of oob data
 */
int yaffs_ECCCorrectOther(unsigned char *data, unsigned nBytes,
			  yaffs_ECCOther * read_ecc,
			  const yaffs_ECCOther * test_ecc)
{
	unsigned short *par = (unsigned short *)&read_ecc->lineParity;
	unsigned char data5[26];
	int numerr;

	Data2Sym(data, data5);

	/* Decode 26 symbols in data5. */
	numerr = decode_rs8 (rs_decoder, data5, par, 26, NULL, 0, NULL, 0, NULL);

	if (numerr == 0)
		return 0;
	else if (numerr > 0 && numerr < 3)
	{
		Sym2Data(data5,data); 
		return 1;
	}
	else
		return -1;
}
#else
int yaffs_ECCCorrectOther(unsigned char *data, unsigned nBytes,
			  yaffs_ECCOther * read_ecc,
			  const yaffs_ECCOther * test_ecc)
{
	return 0;
}
#endif

#else

static int yaffs_CountBits32(unsigned x)
{
	int r = 0;
	while (x) {
		if (x & 1)
			r++;
		x >>= 1;
	}
	return r;
}

/*
 * ECCxxxOther does ECC calcs on arbitrary n bytes of data
 */
void yaffs_ECCCalculateOther(const unsigned char *data, unsigned nBytes,
			     yaffs_ECCOther * eccOther)
{
	unsigned int i;

	unsigned char col_parity = 0;
	unsigned line_parity = 0;
	unsigned line_parity_prime = 0;
	unsigned char b;

	for (i = 0; i < nBytes; i++) {
		b = column_parity_table[*data++];
		col_parity ^= b;

		if (b & 0x01)	 {
			/* odd number of bits in the byte */
			line_parity ^= i;
			line_parity_prime ^= ~i;
		}

	}

	eccOther->colParity = (col_parity >> 2) & 0x3f;
	eccOther->lineParity = line_parity;
	eccOther->lineParityPrime = line_parity_prime;
}

int yaffs_ECCCorrectOther(unsigned char *data, unsigned nBytes,
			  yaffs_ECCOther * read_ecc,
			  const yaffs_ECCOther * test_ecc)
{
	unsigned char cDelta;	/* column parity delta */
	unsigned lDelta;	/* line parity delta */
	unsigned lDeltaPrime;	/* line parity delta */
	unsigned bit;

	cDelta = read_ecc->colParity ^ test_ecc->colParity;
	lDelta = read_ecc->lineParity ^ test_ecc->lineParity;
	lDeltaPrime = read_ecc->lineParityPrime ^ test_ecc->lineParityPrime;

	if ((cDelta | lDelta | lDeltaPrime) == 0)
		return 0; /* no error */

	if (lDelta == ~lDeltaPrime && 
	    (((cDelta ^ (cDelta >> 1)) & 0x15) == 0x15))
	{
		/* Single bit (recoverable) error in data */

		bit = 0;

		if (cDelta & 0x20)
			bit |= 0x04;
		if (cDelta & 0x08)
			bit |= 0x02;
		if (cDelta & 0x02)
			bit |= 0x01;

		if(lDelta >= nBytes)
			return -1;
			
		data[lDelta] ^= (1 << bit);

		return 1; /* corrected */
	}

	if ((yaffs_CountBits32(lDelta) + yaffs_CountBits32(lDeltaPrime) +
	     yaffs_CountBits(cDelta)) == 1) {
		/* Reccoverable error in ecc */

		*read_ecc = *test_ecc;
		return 1; /* corrected */
	}

	/* Unrecoverable error */

	return -1;

}
#endif /* YAFFS_ECC_RS */
