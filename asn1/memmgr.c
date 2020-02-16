/****************************************************************************
*																			*
*							HPACK Multi-System Archiver						*
*							===========================						*
*																			*
*							   Dynamic Memory Manager 						*
*							 MEMMGR.C  Updated 17/03/92						*
*																			*
* This program is protected by copyright and as such any use or copying of	*
*  this code for your own purposes directly or indirectly is highly uncool	*
*  					and if you do so there will be....trubble.				*
* 				And remember: We know where your kids go to school.			*
*																			*
*			  Copyright 1992  Peter C Gutmann.  All rights reserved			*
*																			*
****************************************************************************/

/* "Where weirdly angled ramparts loom,
	Gaunt sentinels whose shadows gloom
	Upon an undead hell-beasts tomb -
	  And gods and mortals fear to tread;
	Where gateways to forbidden spheres
	And times are closed, but monstrous fears
	Await the passing of strange years -
	  When that will wake which is not dead...

				- "Legends of the Olden Runes", transl. by Thelred Gustau.

	(Remind you of anything?) */

#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "hpacklib.h"
#if defined( __ATARI__ )
  #include "asn1\memmgr.h"
#elif defined( __MAC__ )
  #include "memmgr.h"
#else
  #include "asn1/memmgr.h"
#endif /* System-specific include directives */

/* The data structure used to contain information on the heap.  Note that
   this structure is automagically aligned to (at least) a 32-bit boundary */

typedef struct memHdr {
					  struct memHdr *next;		/* Next header in list */
					  size_t size;				/* Size of header */
					  } MEM_HDR;

#define MEM_HDR_SIZE	sizeof( MEM_HDR )

#define NO_HEADERS	100		/* The number of available headers */

/* Accounting variables used for the heap.  There are two approaches for
   handling the header pointers: Store them in the heap, or store them
   in the header array.  If we store them in the header array, we can't
   use handles easily since the pointer will be part of a larger structure,
   thus we store them in the heap.  This has the disadvantage that if we
   overwrite the bounds of a memory block, we trash the entire memory
   manager, but has the advantage that we can add checking for this by adding
   a picket to the header and check this has a reasonable value */

static void *heap;			/* The heap */
static size_t heapSize;		/* It's size */
static MEM_HDR headerPtr;	/* The free list header */
static int nextFree = 0;	/* Next free entry in array of headers */
static BYTE *headers[ NO_HEADERS ];
static int firstUsed = 0;	/* First used entry in array of headers */

/* Initialize and shut down the memory manager */

BOOLEAN memInit( size_t size )
	{
	/* Allocate space for the memory */
	if( ( heap = hmalloc( size ) ) == NULL )
		return( FALSE );
	heapSize = size;

	/* Initialize the free list header */
	headerPtr.size = 0;
	headerPtr.next = ( MEM_HDR * ) heap;

	/* Allocate a header in the heap to cover the entire heap */
	headerPtr.next->size = size - MEM_HDR_SIZE;
	headerPtr.next->next = NULL;

	/* Clear the array of headers */
	memset( headers, 0, NO_HEADERS * sizeof( BYTE * ) );

	return( TRUE );
	}

void memEnd( void )
	{
	/* Free the heap */
	if( heap != NULL )
		hfree( heap );
	}

/* Move all pointers in a given range around in memory */

static void bigBlt( BYTE *low, BYTE *high, size_t size )
	{
	int index;

	/* Perform a sanity check */
	if( !size || low >= high )
		return;

	/* Scan through the array of headers looking for ones to update */
	while( headers[ firstUsed ] == NULL )
		{
		if( firstUsed == NO_HEADERS - 1 )
			return;		/* Nothing to update, exit */
		firstUsed++;
		}

	/* Shuffle all the header pointers around */
	for( index = firstUsed; index < NO_HEADERS; index++ )
		if( headers[ index ] >= low && headers[ index ] < high )
			{
			headers[ index ] -= size;
			if( index == firstUsed )
				firstUsed++;
			}
	}

/* Compact the heap */

static BOOLEAN compactHeap( void )
	{
	MEM_HDR *currHdr, *nextHdr;
	BYTE *src, *dest;
	size_t size;

	/* Perform a sanity check for the free list containing zero or one blocks */
	currHdr = headerPtr.next;
	if( currHdr == NULL || currHdr->next == NULL )
		return( FALSE );

	dest = ( BYTE * ) currHdr;
	do
		{
		/* Move the block around in memory */
		nextHdr = currHdr->next;
		src = ( BYTE * ) currHdr + currHdr->size + MEM_HDR_SIZE;
		size = ( size_t ) ( ( ( nextHdr == NULL ) ?
							( BYTE * ) heap + heapSize : ( BYTE * ) nextHdr ) - src );
		memmove( dest, src, size );

		/* Update the master array */
		bigBlt( src, src + size, ( size_t ) ( src - dest ) );

		/* Update destination for next step */
		dest += size;
		currHdr = nextHdr;
		}
	while( nextHdr != NULL );

	/* Reset firstUsed for the next call to compactHeap() */
	firstUsed = 0;

	/* Update the free list */
	headerPtr.next = currHdr = ( MEM_HDR * ) dest;
	currHdr->next = NULL;
	currHdr->size = heapSize - MEM_HDR_SIZE -
					( size_t ) ( dest - ( BYTE * ) heap );

	return( TRUE );
	}

/* Get the next free entry in the header array */

static int getNextFree( void )
	{
	int i, result;

	/* Search the header block for a free header.  nextFree speeds the search
	   by pointing just past the location where a previous header was found */
	for( i = 0; i < NO_HEADERS; i++ )
		{
		result = nextFree++;
		nextFree %= NO_HEADERS;
		if( headers[ result ] == NULL )
			return( result );
		}

	/* No free header found */
	return( ERROR );
	}

/* Get a block of memory */

void **memAlloc( size_t size )
	{
	MEM_HDR *currHdr, *prevHdr, *nextHdr;
	int fit, headerIndex;

	/* Perform sanity check and align size to 32-bit boundary */
	if( !size || headerPtr.next == NULL )
		return( NULL );
	size = ( size + 3 ) & ( ~3 );

	/* Look for block of required size using first-fit algorithm */
	prevHdr = &headerPtr;
	currHdr = headerPtr.next;
	while( currHdr != NULL )
		{
		/* Check for a fit */
		if( ( fit = currHdr->size - size ) >= 0 )
			{
			/* We've found a free block big enough, now see if there's any
			   slop.  If it's less than the size of an allocatable block, we
			   add it to the block being allocated */
			fit = ( fit < MEM_HDR_SIZE ) ? 1 : 0;
			break;
			}

		/* Move to next header */
		prevHdr = currHdr;
		currHdr = currHdr->next;
		}

	/* If there was no match, try and compact the heap */
	if( fit < 0 )
		if( !compactHeap() )
			/* Couldn't compact, out of memory */
			return( NULL );
		else
			/* Try again */
			return( memAlloc( size ) );
	else
		{
		/* We've found a block, allocate a new header entry */
		if( ( headerIndex = getNextFree() ) == ERROR )
			/* Out of headers, can't allocate more */
			return( NULL );

		/* If we've got a perfect match, just unlink the current free block */
		if( fit == 1 )
			prevHdr->next = currHdr->next;
		else
			{
			/* Try to merge the block with neighbouring blocks.  First we
			   update the predecessor's next pointer */
			prevHdr->next = nextHdr =
							( MEM_HDR * ) ( ( BYTE * ) currHdr + size + MEM_HDR_SIZE );

			/* Set up the new fields in the leftover part */
			nextHdr->next = currHdr->next;
			nextHdr->size = currHdr->size - size - MEM_HDR_SIZE;

			/* Set length of new block */
			currHdr->size = size;
			}

		/* Skip the header */
		headers[ headerIndex ] = ( BYTE * ) ( currHdr + 1 );
		return( ( void ** ) &headers[ headerIndex ] );
		}
	}

/* Free a block of memory */

void memFree( void **block )
	{
	MEM_HDR *prevBlock, *nextBlock, *temp;
	BYTE **blockPtr;
	size_t size;

	/* Perform sanity check */
	blockPtr = ( BYTE ** ) block;
	if( blockPtr == NULL || blockPtr < headers || blockPtr >= headers + NO_HEADERS )
		return;

	/* Search the free list for the blocks to the left and right of the one
	   to be freed */
	prevBlock = &headerPtr;
	nextBlock = headerPtr.next;
	*blockPtr = ( BYTE * ) ( ( MEM_HDR * ) *blockPtr - 1 );
	size = ( temp = ( MEM_HDR * ) *blockPtr )->size;
	while( nextBlock != NULL && ( BYTE * ) nextBlock < *blockPtr )
		 {
		 prevBlock = nextBlock;
		 nextBlock = nextBlock->next;
		 }

	/* Check if we're merging the block with the left neighbour */
	if( ( prevBlock != &headerPtr ) &&
		( ( BYTE * ) prevBlock + prevBlock->size + MEM_HDR_SIZE == *blockPtr ) )
		{
		temp = prevBlock;
		prevBlock->size += size + MEM_HDR_SIZE;
		}
	else
		{
		/* Add the block to the free list.  The correct size is already
		   stored in the header */
		temp->next = nextBlock;
		prevBlock->next = temp;
		}

	/* Check if we're merging the block with the right neighbour */
	if( temp + temp->size + MEM_HDR_SIZE == nextBlock )
		{
		temp->size += nextBlock->size + MEM_HDR_SIZE;
		temp->next = nextBlock->next;
		}

	/* Clear the corresponding pointer in the header array */
	*block = NULL;
	}
