;****************************************************************************
;*																			*
;*							HPACK Multi-System Archiver						*
;*							===========================						*
;*																			*
;*								ARM RSA Routines							*
;*							  RSA.S  Updated 11/07/93						*
;*																			*
;* This program is protected by copyright and as such any use or copying of	*
;*  this code for your own purposes directly or indirectly is highly uncool	*
;* 					and if you do so there will be....trubble.				*
;* 				And remember: We know where your kids go to school.			*
;*																			*
;*				Copyright 1993  Jason Williams.  All rights reserved		*
;*																			*
;****************************************************************************

; These replace the C variants of these functions to provide a speed increase
; of about 10 times - mainly due to the problems the poor old compiler was
; having with processing the data as shorts, and the inaccessibility of the
; carry flag from C.
;
; This code MUST have 16 bytes of free space that it may corrupt on the end
; of the data (though it won't always corrupt this much, 16 is the maximum).
; There is enough slack in the MP registers to handle this.
;
; Also, it currently can't handle unitPrecision values larger than 128 for
; add and sub (rotateleft is fine) because it uses a totally unrolled loop.
; Rolling the loop requires a test of the counter reaching zero, which will
; corrupt the carry flag, and thus means a few cycles more work on each
; iteration to preserve the carry.
;
; Here are some times calculated from 100,000 iterations of mp_add. The time
; is in seconds, and is the time for one call to the function to process a
; 32-element array (i.e. 64 bytes of data)
;
; cache off (i.e roughly ARM2 speed)
;
;	original code from hell:	0.0019150 s
;	ARM assembler:				0.0001820 s
;
; cache on
;
;	original code from hell:	0.00007310 s
;	ARM assembler:				0.00000680 s

a1 RN 0
a2 RN 1
a3 RN 2
a4 RN 3
v1 RN 4
v2 RN 5
v3 RN 6
v4 RN 7
v5 RN 8
v6 RN 9
sl RN 10
fp RN 11
ip RN 12
sp RN 13
lr RN 14
pc RN 15

; Global register definitions, as used by all 3 functions...

count RN 14			; We'll use r14 as a temp variable, as our functions are
					; all leaf functions, and we stack r14 for return anyway...

r1a RN 2			; Add and Sub both work in 4-register bites
r1b RN 3			; Source/result words (1)
r1c RN 4			; (replace 8 iterations of "reg1" in C source)
r1d RN 5

; The following register definitions are only used for add, sub

r2a RN 6			; Source words (2)
r2b RN 7			; (replace 8 iterations of "reg2" in C source)
r2c RN 8
r2d RN 9

; The following register definitions are only used for rotateleft

carry1 RN 1			; (Initial carryin is passed in as argument2)
carry2 RN 6			; (We don't use the r2# series of registers in rotateleft)

		AREA |C$$code|, CODE, READONLY

		IMPORT	|__main|
|x$codeseg|
		B		|__main|

L000030				; extern int unitPrecision
		IMPORT	globalPrecision
		DCD		globalPrecision

		EXPORT	mp_add
mp_add						; extern void mp_add( MP_REG *a1, MP_REG *a2 );

		; The code produced by cc was very slow (40 ARM2 cycles per item
		; of data to process), mainly because of the overhead involved with
		; processing shorts on the ARM.
		;
		; The following code is a bit more efficient (25 ARM2 cycles per
		; 8 items -- approx 3 cycles per item -- a speedup of over 10 times)
		;
		; I wasn't actually intending to unroll the loop, but checking a loop
		; counter will corrupt the carry flag, which adds lots more overhead
		; as we'd then have to copy the carry into a register all the time.
		;
		; The code works by calculating how many chunks of 8 data-items to
		; process, and then branches into an unrolled loop at the appropriate
		; point. Note that there are two important side effects of this:
		;   1) The code may overrun the data area by up to 4 words (16 bytes)
		;      so 16 bytes of extra room must be available at the end of the
		;      argument-1 array.
		;   2) The unrolled loop can process up to 16*8 = 128 data items -
		;      any more than this and results will be, er... "unpredictable"
		;      To increase this, change the #128 (4 instructions below), and
		;      add the appropriate number of extra iterations of the loop below

		STMFD	sp!, {r1a-r2d, lr}			; Preserve registers we will use

		LDR		count, [pc, #L000030-.-8]	; Find &unitPrecision
		LDR		count, [count]				; count = *(&unitPrecision)

		RSB		count, count, #128			; We can't cope with more than
											; 128 entries!

		MOV		count, count, LSR #3		; / 8 (doing 8 entries per itn)

		RSB		count, count, count, LSL #3	; * 7  ((count * 8) - count)
											; (Each iteration is 7 instrns)

		SUB		count, count, #1			; Subtract 1 instruction to
											; compensate for pipelining
											; (trust me on this one!)

		ADDS	pc, pc, count, ASL #2		; * 4 to get instruction offset
											; ...use (S) to clear the carry


		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 1
		LDMIA	a2!, {r2a, r2b, r2c, r2d}	; load 8 (short) entries in one go

		; Now for each pair, we add the relevant registers together, keeping
		; track of the carry, and including any previous carry, and leaving
		; the carry set up for the next iteration...

		ADCS	r1a, r1a, r2a				; Add regs[0] and regs[1]
		ADCS	r1b, r1b, r2b				;     regs[2] and regs[3]
		ADCS	r1c, r1c, r2c				;     regs[4] and regs[5]
		ADCS	r1d, r1d, r2d				;     regs[6] and regs[7]

		STMIA	a1!, {r1a, r1b, r1c, r1d}	; Store the results in *reg1

		; Unroll this loop, because any loop-end checks will corrupt the
		; carry bit! Argh! We thus jump into the loop at an appropriate
		; position so that no more than 7 entries too many are processed...
		; i.e. we can overwrite up to 14 bytes beyond where we should stop.
		; Peter says that's OK, so blame him if it breaks!

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 2
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 3
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 4
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 5
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 6
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 7
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 8
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 9
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 10
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 11
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 12
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 13
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 14
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 15
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 16
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		ADCS	r1a, r1a, r2a
		ADCS	r1b, r1b, r2b
		ADCS	r1c, r1c, r2c
		ADCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMFD	sp!, {r1a-r2d, pc}^



		EXPORT	mp_sub
mp_sub
		; This is very similar to mp_add, except SBC is used rather than ADC,
		; and the initial setup code has to set the carry flag rather than
		; clear it.
		; There is one other thing that is different - mp_sub also returns
		; the state of the borrow at completion (see the end).

		STMFD	sp!, {r1a-r2d, lr}
		MOV		count, #1<<31				; Shift a 1 into the carry
		MOVS	count, count, LSL #1		; flag (must start with carry = 1)

		LDR		count, [pc, #L000030-.-8]	; Find &unitPrecision
		LDR		count, [count]				; count = *(&unitPrecision)

		RSB		count, count, #128			; We can't cope with more than
											; 128 entries!

		MOV		count, count, LSR #3		; / 8 (doing 8 entries per itn)

		RSB		count, count, count, LSL #3	; * 7  ((count * 8) - count)

		SUB		count, count, #1			; Subtract 1 instruction to
											; compensate for pipelining
											; (trust me on this one!)

		ADD		pc, pc, count, ASL #2		; * 4 to get instruction offset
											; DON'T use (S) this time!

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 1
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a				; Same as for mp_add, but have
		SBCS	r1b, r1b, r2b				; replaced ADC with SBC
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 2
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 3
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 4
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 5
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 6
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 7
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 8
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 9
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 10
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 11
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 12
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 13
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 14
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 15
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		LDMIA	a1,  {r1a, r1b, r1c, r1d}	; iteration 16
		LDMIA	a2!, {r2a, r2b, r2c, r2d}
		SBCS	r1a, r1a, r2a
		SBCS	r1b, r1b, r2b
		SBCS	r1c, r1c, r2c
		SBCS	r1d, r1d, r2d
		STMIA	a1!, {r1a, r1b, r1c, r1d}

		MOVCS	a1, #0						; Finally, return the borrow state
		MOVCC	a1, #1
		LDMFD	sp!, {r1a-r2d, pc}^



		EXPORT	mp_rotateleft
mp_rotateleft
		; void mp_rotateleft(MP_REG *a1, int carry1)   - a2 = carryIn (carry1)
		; This is a bit different from the above two. We only operate on
		; one array, shifting its entire contents left by one bit. This
		; requires shifting the top bit of one word into the bottom of the
		; next. We do a loop this time (as we are not worried about corrupting
		; the carry flag as I do carries manually), but it still does 8 data
		; items per iteration, so the loop overhead is pretty small

		STMFD	sp!, {r1a-r1d, carry2, lr}

		LDR		count, [pc, #L000030-.-8]	; Find &unitPrecision
		LDR		count, [count]				; count = *(&unitPrecision)

mp_rotateleft_loop							; Actually use loop for this one!
		LDMIA	a1,  {r1a, r1b, r1c, r1d}

		; Initially, carry1 = carryin, carry2 = carryout
		; This is toggled as each word is processed - so we must process
		; in pairs of words. We actually do 4 words (8 data items) in a go
		; because we can, and we are guaranteed 16 bytes of overrun for all of
		; these functions, so we might as well!

		; Carries - 1 = in, 2 = out
		MOV		carry2, r1a, LSR #31		; carryOut = (*reg & 1<<31) ? 1:0;
		ORR		r1a, carry1, r1a, LSL #1	; *reg = (*reg << 1) | carryIn;

		; Carries - 2 = in, 1 = out
		MOV		carry1, r1b, LSR #31
		ORR		r1b, carry2, r1b, LSL #1

		; Carries - 1 = in, 2 = out
		MOV		carry2, r1c, LSR #31
		ORR		r1c, carry1, r1c, LSL #1

		; Carries - 2 = in, 1 = out
		MOV		carry1, r1d, LSR #31
		ORR		r1d, carry2, r1d, LSL #1

		STMIA	a1!, {r1a, r1b, r1c, r1d}

		SUBS	count, count, #8			; Decrement counter (8 items per
		BPL		mp_rotateleft_loop			; iteration), and loop until done

		LDMFD	sp!, {r1a-r1d, carry2, pc}^

		AREA |C$$data|

|x$dataseg|

		END
