@ This ARM Assembler code should implement a matching function, for use in the MasterMind program, as
@ described in the CW2 specification. It should produce as output 2 numbers, the first for the
@ exact matches (peg of right colour and in right position) and approximate matches (peg of right
@ color but not in right position). Make sure to count each peg just once!
	
@ Example (first sequence is secret, second sequence is guess):
@ 1 2 1
@ 3 1 3 ==> 0 1
@ You can return the result as a pointer to two numbers, or two values
@ encoded within one number
@
@ -----------------------------------------------------------------------------

.text
@ this is the matching fct that should be called from the C part of the CW	
.global         matches
@ use the name `main` here, for standalone testing of the assembler code
@ when integrating this code into `master-mind.c`, choose a different name
@ otw there will be a clash with the main function in the C code
.global         _main
_main: 
	LDR  R2, =secret	@ pointer to secret sequence
	LDR  R3, =guess		@ pointer to guess sequence
	BL   matches		@ call the matching function
	B    exit		@ exit program

exit:	MOV 	 R7, #1		@ load system call code
	SWI 	 0		@ return this value

@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C	
matches:			@ Input: R0, R1 ... ptr to int arrays to match ; Output: R0 ... exact matches (10s) and approx matches (1s) of base COLORS
	PUSH    {R4-R8, LR}	@ Save registers
	MOV     R4, R0		@ R4 = secret sequence pointer
	MOV     R5, R1		@ R5 = guess sequence pointer
	MOV     R6, #0		@ R6 = exact matches
	MOV     R7, #0		@ R7 = approximate matches

	@ Allocate space on stack for used flags (3 bytes each for used1 and used2)
	SUB     SP, SP, #6	@ 6 bytes total
	MOV     R0, #0
	STRB    R0, [SP]	@ used1[0] = 0
	STRB    R0, [SP, #1]	@ used1[1] = 0
	STRB    R0, [SP, #2]	@ used1[2] = 0
	STRB    R0, [SP, #3]	@ used2[0] = 0
	STRB    R0, [SP, #4]	@ used2[1] = 0
	STRB    R0, [SP, #5]	@ used2[2] = 0

	@ Count exact matches first
	MOV     R8, #0		@ index
exact_loop:
	CMP     R8, #LEN
	BGE     exact_done
	LDR     R0, [R4, R8, LSL #2] @ secret[i]
	LDR     R1, [R5, R8, LSL #2] @ guess[i]
	CMP     R0, R1
	BNE     exact_next
	ADD     R6, R6, #1	@ Increment exact matches
	MOV     R0, #1
	STRB    R0, [SP, R8]	@ Mark used1[i] = 1
	ADD     R1, SP, #3	@ Get address of used2
	STRB    R0, [R1, R8]	@ Mark used2[i] = 1
exact_next:
	ADD     R8, R8, #1
	B       exact_loop
exact_done:

	@ Count approximate matches
	MOV     R8, #0		@ guess index
approx_outer:
	CMP     R8, #LEN
	BGE     approx_done
	ADD     R0, SP, #3	@ Get address of used2
	LDRB    R0, [R0, R8]	@ Check used2[i]
	CMP     R0, #1
	BEQ     approx_next_outer @ Skip if already used in exact match
	LDR     R0, [R5, R8, LSL #2] @ guess[i]
	MOV     R1, #0		@ secret index
approx_inner:
	CMP     R1, #LEN
	BGE     approx_next_outer
	LDRB    R2, [SP, R1]	@ Check used1[j]
	CMP     R2, #1
	BEQ     approx_next_inner @ Skip if already used
	LDR     R2, [R4, R1, LSL #2] @ secret[j]
	CMP     R0, R2
	BNE     approx_next_inner
	ADD     R7, R7, #1	@ Increment approximate matches
	MOV     R2, #1
	STRB    R2, [SP, R1]	@ Mark used1[j] = 1
	B       approx_next_outer @ Move to next guess after finding a match
approx_next_inner:
	ADD     R1, R1, #1
	B       approx_inner
approx_next_outer:
	ADD     R8, R8, #1
	B       approx_outer
approx_done:

	@ Encode result: exact * 10 + approx
	MOV     R1, #10
	MUL     R0, R6, R1
	ADD     R0, R0, R7

	ADD     SP, SP, #6	@ Deallocate stack space
	POP     {R4-R8, PC}	@ Restore registers and return

@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
	PUSH    {R4-R6, LR}
	MOV     R4, R0		@ Save sequence pointer
	LDR     R5, =f4str	@ Load format string
	LDR     R1, [R4]	@ seq[0]
	LDR     R2, [R4, #4]	@ seq[1]
	LDR     R3, [R4, #8]	@ seq[2]
	MOV     R0, R5		@ Format string to R0
	BL      printf
	POP     {R4-R6, PC}
	
	
@ =============================================================================

.data

@ constants about the basic setup of the game: length of sequence and number of colors	
.equ LEN, 3
.equ COL, 3
.equ NAN1, 8
.equ NAN2, 9

@ a format string for printf that can be used in showseq
f4str: .asciz "Seq:    %d %d %d\n"

@ a memory location, initialised as 0, you may need this in the matching fct
n: .word 0x00
	
@ INPUT DATA for the matching function
.align 4
secret: .word 1 
	.word 2 
	.word 1 

.align 4
guess:	.word 3 
	.word 1 
	.word 3 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 0 1
.align 4
expect: .byte 0
	.byte 1

.align 4
secret1: .word 1 
	 .word 2 
	 .word 3 

.align 4
guess1:	.word 1 
	.word 1 
	.word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 1
.align 4
expect1: .byte 1
	 .byte 1

.align 4
secret2: .word 2 
	 .word 3
	 .word 2 

.align 4
guess2:	.word 3 
	.word 3 
	.word 1 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
	 .byte 0
