/*	idea.c - C source code for IDEA block cipher.
	IDEA (International Data Encryption Algorithm), formerly known as
	IPES (Improved Proposed Encryption Standard).
	Algorithm developed by Xuejia Lai and James L. Massey, of ETH Zurich.
	This implementation modified and derived from original C code
	developed by Xuejia Lai.  Last modified 8 July 92.

	Zero-based indexing added, names changed from IPES to IDEA.
	CFB functions added.  Random number routines added.

	The IDEA(tm) block cipher is covered by a patent held by ETH and a
	Swiss company called Ascom-Tech AG.  The Swiss patent number is
	PCT/CH91/00117.  International patents are pending. IDEA(tm) is a
	trademark of Ascom-Tech AG.  There is no license fee required for
	noncommercial use.  Commercial users may obtain licensing details
	from Dieter Profos, Ascom Tech AG, Solothurn Lab, Postfach 151, 4502
	Solothurn, Switzerland, Tel +41 65 242885, Fax +41 65 235761.

	The IDEA block cipher uses a 64-bit block size, and a 128-bit key 
	size.  It breaks the 64-bit cipher block into four 16-bit words
	because all of the primitive inner operations are done with 16-bit 
	arithmetic.  It likewise breaks the 128-bit cipher key into eight
	16-bit words.

	For further information on the IDEA cipher, see these papers:
	1)	Xuejia Lai, "Detailed Description and a Software Implementation of
    	the IPES Cipher", Institute for Signal and Information
    	Processing, ETH-Zentrum, Zurich, Switzerland, 1991
	2)	Xuejia Lai, James L. Massey, Sean Murphy, "Markov Ciphers and
    	Differential Cryptanalysis", Advances in Cryptology- EUROCRYPT'91

	This code assumes that each pair of 8-bit bytes comprising a 16-bit
	word in the key and in the cipher block are externally represented
	with the Most Significant Byte (MSB) first, regardless of the
	internal native byte order of the target CPU */

#include <stdio.h>
#include <stdlib.h>
#include "md5.h"

/* Defines to convert from HPACK -> PGP standard defines/typedefs */

#define word32		LONG
#define word16		WORD
#define boolean		BOOLEAN
#define byte		BYTE
#define byteptr		BYTE *

#define IDEABLOCKSIZE	8

/* CPU endianness */

extern BOOLEAN bigEndian;

/* Not everything has a min() macro */

#ifndef min
  #define min(a,b)	( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )	/* Lithp! */
#endif /* !min */

/* #define lower16(x) ((word16)((x) & mask16)) */ /* unoptimized version */
#define lower16(x)   ((word16)(x))	/* optimized version */

#define maxim        0x10001L
#define fuyi         0x10000L	/* Why did Lai pick a name like this? */
#define mask16       ((word16) 0xffff)
#define ROUNDS       8		/* Don't change this value, should be 8 */

static void en_key_idea(word16 userkey[8], word16 Z[6][ROUNDS+1]);
static void cipher_idea(word16 inblock[4], word16 outblock[4],
		 word16 Z[6][ROUNDS+1]);

/*	Multiplication, modulo (2**16)+1 */
static word16 mul(register word16 a, register word16 b)
{
	register word32 q;
	register long int p;
	if (a==0)
		p = maxim-b;
	else 
		if (b==0)
			p = maxim-a;
		else
		{	q = (word32)a * (word32)b; 
			p = (q & mask16) - (q>>16);
			if (p<=0) 
				p = p+maxim;
		}
	return (lower16(p));
}        /* mul */

/*	Compute IDEA encryption subkeys Z */
static void en_key_idea(word16 userkey[8], word16 Z[6][ROUNDS+1])
{
	volatile word16 S[54];	/* SAS/C needs 'volatile', don't ask me why */
	int i,j,r;
	/*   shifts   */
	for (i = 0; i<8; i++)
		S[i] = userkey[i];
	for (i = 8; i<54; i++)
	{	if ((i+2)%8 == 0)  /* for S[14],S[22],...  */
			S[i] = lower16((S[i-7] << 9) ^ (S[i-14] >> 7));
		else
			if ((i+1)%8 == 0)  /* for S[15],S[23],...   */
			S[i] = lower16((S[i-15] << 9) ^ (S[i-14] >> 7));
		else
			S[i] = lower16((S[i-7] << 9) ^ (S[i-6] >> 7));
	}
	/* get subkeys */
	for (r=0; r<ROUNDS+1; r++)
		for (j=0; j<6; j++)
			Z[j][r] = S[6*r + j];

	/* clear sensitive key data from memory... */
	for (i=0; i<54; i++) S[i] = 0;
}        /* en_key_idea */

/*	IDEA encryption/decryption algorithm */
static void cipher_idea(word16 inblock[4], word16 outblock[4],
		 register word16 Z[6][ROUNDS+1])
{	/* Note that inblock and outblock can be the same buffer */
	int r;
	register word16 x1, x2, x3, x4, kk, t1, t2, a;
	x1=inblock[0];
	x2=inblock[1];
	x3=inblock[2];
	x4=inblock[3];
	for (r=0; r<ROUNDS; r++)
	{	x1 = mul(x1, Z[0][r]);
		x4 = mul(x4, Z[3][r]);
		x2 = lower16(x2 + Z[1][r]);
		x3 = lower16(x3 + Z[2][r]);
		kk = mul(Z[4][r], (word16)(x1^x3));
		t1 = mul(Z[5][r], lower16(kk + (x2^x4)) );
		t2 = lower16(kk + t1);
		x1 = x1^t1;
		x4 = x4^t2;
		a  = x2^t2;
		x2 = x3^t1;
		x3 = a;
	}
	outblock[0] = mul(x1, Z[0][ROUNDS]);
	outblock[3] = mul(x4, Z[3][ROUNDS]);
	outblock[1] = lower16(x3 + Z[1][ROUNDS]);
	outblock[2] = lower16(x2 + Z[2][ROUNDS]);
}        /* cipher_idea */

/*************************************************************************/


#define byteglue(lo,hi) ((((word16) hi) << 8) + (word16) lo)

/*
**	xorbuf - change buffer via xor with random mask block
**	Used for Cipher Feedback (CFB) or Cipher Block Chaining
**	(CBC) modes of encryption.
**	Can be applied for any block encryption algorithm,
**	with any block size, such as the DES or the IDEA cipher.
*/
static void xorbuf(register byteptr buf, register byteptr mask, register int count)
/*	count must be > 0 */
{	if (count)
		do
			*buf++ ^= *mask++;
		while (--count);
}	/* xorbuf */


/*
**	cfbshift - shift bytes into IV for CFB input
**	Used only for Cipher Feedback (CFB) mode of encryption.
**	Can be applied for any block encryption algorithm with any
**	block size, such as the DES or the IDEA cipher.
*/
static void cfbshift(register byteptr iv, register byteptr buf, 
		register int count, int blocksize)
/* 	iv is the initialization vector.
	buf is the buffer pointer.
	count is the number of bytes to shift in...must be > 0.
	blocksize is 8 bytes for DES or IDEA ciphers.
*/
{	int retained;
	if (count)
	{	retained = blocksize-count;	/* number bytes in iv to retain */
		/* left-shift retained bytes of IV over by count bytes to make room */
		while (retained--)
		{	*iv = *(iv+count);
			iv++;
		}
		/* now copy count bytes from buf to shifted tail of IV */
		do	*iv++ = *buf++;
		while (--count);
	}
}	/* cfbshift */

/* Key schedules for IDEA encryption and decryption */
static word16 Z[6][ROUNDS+1];
static word16 *iv_idea;		/* pointer to IV for CFB or CBC */
static boolean cfb_dc_idea; /* TRUE iff CFB decrypting */

/* initkey_idea initializes IDEA for ECB mode operations */
void initkey_idea(byte key[16])
{
	volatile word16 userkey[8];	/* IDEA key is 16 bytes long */
	int i;
	/* Assume each pair of bytes comprising a word is ordered MSB-first. */
	for (i=0; i<8; i++)
	{	userkey[i] = byteglue(*(key+1),*key);
		key++; key++;
	}
	en_key_idea((word16 *)userkey,Z);
	for (i=0; i<8; i++)	/* Erase dangerous traces */
		userkey[i] = 0;
}	/* initkey_idea */


/*	Run a 64-bit block thru IDEA in ECB (Electronic Code Book) mode,
	using the currently selected key schedule.
*/
void idea_ecb(word16 *inbuf, word16 *outbuf)
{
	union {
		word16 w[4];
		byte b[8];
	} inbuf2, outbuf2;
	register byte *p;

	/* Assume each pair of bytes comprising a word is ordered MSB-first. */

	if( !bigEndian )
		{
		/* If this is a least-significant-byte-first CPU */
		/* Invert the byte order for each 16-bit word for internal use. */
		p = (byte *) inbuf;
		inbuf2.b[1] = *p++;
		inbuf2.b[0] = *p++;
		inbuf2.b[3] = *p++;
		inbuf2.b[2] = *p++;
		inbuf2.b[5] = *p++;
		inbuf2.b[4] = *p++;
		inbuf2.b[7] = *p++;
		inbuf2.b[6] = *p;
		cipher_idea(inbuf2.w,outbuf2.w,Z);
		/* Now invert the byte order for each 16-bit word for external use. */
		p = (byte *) outbuf;
		*p++ = outbuf2.b[1];
		*p++ = outbuf2.b[0];
		*p++ = outbuf2.b[3];
		*p++ = outbuf2.b[2];
		*p++ = outbuf2.b[5];
		*p++ = outbuf2.b[4];
		*p++ = outbuf2.b[7];
		*p = outbuf2.b[6];
		}
	else
		/* Byte order for internal and external representations is the same. */
		cipher_idea(inbuf,outbuf,Z);
}	/* idea_ecb */


/*
**	initcfb - Initializes the IDEA key schedule tables via key,
**	and initializes the Cipher Feedback mode IV.
**	References context variables cfb_dc_idea and iv_idea.
*/
void initcfb_idea(word16 iv0[4], byte key[16], boolean decryp)
/* 	iv0 is copied to global iv_idea, buffer will be destroyed by ideacfb.
	key is pointer to key buffer.
	decryp is TRUE if decrypting, FALSE if encrypting.
*/
{	iv_idea = iv0;
	cfb_dc_idea = decryp;
	initkey_idea(key);
} /* initcfb_idea */


/*
**	ideacfb - encipher a buffer with IDEA enciphering algorithm,
**		using Cipher Feedback (CFB) mode.
**
**	Assumes initcfb_idea has already been called.
**	References context variables cfb_dc_idea and iv_idea.
*/
void ideacfb(byteptr buf, int count)
/*	buf is input, output buffer, may be more than 1 block.
	count is byte count of buffer.  May be > IDEABLOCKSIZE.
*/
{	int chunksize;	/* smaller of count, IDEABLOCKSIZE */
	word16 temp[IDEABLOCKSIZE/2];

	while ((chunksize = min(count,IDEABLOCKSIZE)) > 0)
	{
		idea_ecb(iv_idea,temp);  /* encrypt iv_idea, making temp. */

		if (cfb_dc_idea)	/* buf is ciphertext */
			/* shift in ciphertext to IV... */
			cfbshift((byte *)iv_idea,buf,chunksize,IDEABLOCKSIZE);

		/* convert buf via xor */
		xorbuf(buf,(byte *)temp,chunksize); /* buf now has enciphered output */

		if (!cfb_dc_idea)	/* buf was plaintext, is now ciphertext */
			/* shift in ciphertext to IV... */
			cfbshift((byte *)iv_idea,buf,chunksize,IDEABLOCKSIZE);

		count -= chunksize;
		buf += chunksize;
	}
} /* ideacfb */

/*
	close_idea function erases all the key schedule information when
	we are all done with a set of operations for a particular IDEA key
	context.  This is to prevent any sensitive data from being left
	around in memory.
*/
void close_idea(void)	/* erase current key schedule tables */
{	short r,j;
	for (r=0; r<ROUNDS+1; r++)
		for (j=0; j<6; j++)
			Z[j][r] = 0;
}	/* close_idea() */
