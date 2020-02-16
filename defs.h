#ifndef _DEFS_DEFINED

/* Make sure none of the following have been defined elsewhere */

#ifdef BYTE
  #undef BYTE
#endif /* BYTE */
#ifdef WORD
  #undef WORD
#endif /* WORD */
#ifdef LONG
  #undef LONG
#endif /* LONG */
#ifdef BOOLEAN
  #undef BOOLEAN
#endif /* BOOLEAN */

/* Various types */

typedef unsigned char		BYTE;
typedef unsigned short int	WORD;	/* 16-bit unsigned on most systems */
#ifndef _OS2EMX_H
typedef unsigned long int	LONG;	/* 32-bit unsigned on most systems */
#endif /* _OS2EMX.H */

typedef int		BOOLEAN;

/* Size of various data types */

#define BITS_PER_BYTE	8
#define BITS_PER_WORD	16
#define BITS_PER_LONG	32

/* Boolean constants */

#define FALSE	0
#define TRUE	1

/* Exit status of functions. */

#define OK		0
#define ERROR	-1

/* 'inline' patch for compilers which can't handle this */

#if !( defined( __CPLUSPLUS__ ) || defined( __cplusplus ) ) || defined( __TSC__ )
  #define inline
#endif /* !( __CPLUSPLUS__ || __cplusplus ) */

/* Flag the fact we've been #included */

#define _DEFS_DEFINED

#endif /* _DEFS_DEFINED */
