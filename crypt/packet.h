/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*						Encryption Packet Interface Header						*
*							PACKET.H  Updated 12/07/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*		Copyright 1991 - 1992  Peter C.Gutmann.  All rights reserved		*
*																			*
****************************************************************************/

/* Note that the following packet definitions aren't compatible with PGP
   packets since HPACK uses its own packet format.  Seperate packet
   definitions for PGP are given below */

/* Cipher Type Byte (CTB) definitions */

#define CTB_BITS			0xA0	/* Magic bits to signify CTB */
#define CTB_BIT_MASK		0xF0	/* Mask value for magic bits */
#define CTB_MORE			0x08	/* Value for 'more' bitflag */
#define CTB_MORE_MASK		0x08	/* Mask value for more bitflag */
#define CTB_TYPE_MASK		0x07	/* Mask value for type field */

/* Defines to extract information from the CTB */

#define isCTB(ctb)			( ( ( ctb ) & CTB_BIT_MASK ) == CTB_BITS )
#define hasMore(ctb)		( ( ctb ) & CTB_MORE_MASK )
#define ctbType(ctb)		( ( ctb ) & CTB_TYPE_MASK )

/* Magic values for the CTB as used by HPACK */

#define CTBTYPE_PKE			0	/* Packet encrypted with RSA public key */
#define CTBTYPE_SKE			1	/* Packet signed with RSA secret key */
#define CTBTYPE_MD			2	/* Message digest packet */
#define CTBTYPE_MDINFO		3	/* Message digest information packet */
#define CTBTYPE_CKE			4	/* Conventional-key-encryption packet */
#define CTBTYPE_CKEINFO		5	/* Conventional-key-encryption info packet */

/* A set of mysterious defines to build up a CTB */

#define	makeCtb(type)		( CTB_BITS | type )

#define CTB_PKE 			makeCtb( CTBTYPE_PKE )
#define CTB_SKE 			makeCtb( CTBTYPE_SKE )
#define CTB_MD 				makeCtb( CTBTYPE_MD )
#define CTB_MDINFO			makeCtb( CTBTYPE_MDINFO )
#define CTB_CKE				makeCtb( CTBTYPE_CKE )
#define CTB_CKEINFO			makeCtb( CTBTYPE_CKEINFO )

/****************************************************************************
*																			*
*							PGP Packet Information							*
*																			*
****************************************************************************/

/* PGP CTB length values */

#define ctbLength(ctb)		( ( ctb ) & 3 )

enum { CTB_LEN_BYTE, CTB_LEN_WORD, CTB_LEN_LONG, CTB_LEN_NONE };

/* PGP CTB types */

#define CTBTYPE_CERT_SECKEY	0x95	/* Secret key certificate */
#define CTBTYPE_CERT_PUBKEY	0x98	/* Public key certificate */
#define CTBTYPE_USERID		0xB4	/* UserID */

#define CTB_CERT_SECKEY 	( CTBTYPE_CERT_SECKEY | CTB_LEN_WORD )
#define CTB_CERT_PUBKEY 	( CTBTYPE_CERT_PUBKEY | CTB_LEN_WORD )
#define CTB_USERID			( CTBTYPE_USERID | CTB_LEN_BYTE )

/* PGP version byte */

#define PGP_VERSION			2		/* PGP version 2.x */

/****************************************************************************
*																			*
*								PKCS Information							*
*																			*
****************************************************************************/

/* Block types as per PKCS #1, Section 8 */

#define BT_PRIVATE	1		/* Block type for private-key operation */
#define BT_PUBLIC	2		/* Block type for public-key operation */

/****************************************************************************
*																			*
*					ID Bytes and Packet Layout Information					*
*																			*
****************************************************************************/

/* Public-key encryption algorithm selector bytes */

#define PKE_ALGORITHM_RSA		1	/* RSA */

/* Conventional encryption algorithm selector bytes */

#define CKE_ALGORITHM_NONE		0	/* Plaintext */
#define CKE_ALGORITHM_IDEA		1	/* IDEA, CFB mode */
#define CKE_ALGORITHM_MDC		64	/* MDC-128, CFB mode */
#define CKE_ALGORITHM_MDC_R		65	/* MDC-128, CFB mode, rekeyed */

/* Signature algorithm selector bytes */

#define SIG_ALGORITHM_RSA		1	/* RSA */

/* Message digest algorithm selector bytes */

#define MD_ALGORITHM_MD5		1	/* MD5 message digest */

/* The length of the data in a message digest packet (the algo.ID byte, the
   MD, and the timestamp), the length of the entire packet (the CTB, the
   length byte, and the MD info length), and the offsets of various fields
   within the packet */

#define MD_INFO_LENGTH			( 1 + 16 + 4 )
#define MD_PACKET_LENGTH		( 1 + 1 + MD_INFO_LENGTH )
#define MD_TIME_OFS				( MD_INFO_LENGTH - sizeof( LONG ) )
#define MD_DIGEST_OFS			sizeof( BYTE )

/* The length of the data in a PKE packet (the CTB, the length byte, the
   algo.ID byte, and the DEK info) */

#define PKE_PACKET_LENGTH		( 1 + 1 + 1 + BLOCKSIZE )

/* No.of bytes in key ID modulus fragment, and the number of bytes the user
   sees */

#define KEYFRAG_SIZE 			8		/* 64-bit keyID */
#define KEYDISPLAY_SIZE			3		/* 24 bits shown to user */
