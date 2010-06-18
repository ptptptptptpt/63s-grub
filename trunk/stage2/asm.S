/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:
	
   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

#define ASM_FILE

#include "shared.h"

#ifdef STAGE1_5
# define	ABS(x)	((x) - EXT_C(main) + 0x2200)
#else
# define	ABS(x)	((x) - EXT_C(main) + 0x8200)
#endif
#define 	io_map	0xFC00
#define		bios_key_map	0xFC80
#define		ascii_key_map	0xFD00
	
	.file	"asm.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

	/* 
	 * In stage2, do not link start.S with the rest of the source
	 * files directly, so define the start symbols here just to
	 * force ld quiet. These are not referred anyway.
	 */
	.globl	start, _start
start:
_start:
	
ENTRY(main)
	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200 in stage2 and
	 *  at 0x0:0x2200 in stage1.5.
	 */
	ljmp $0, $ABS(codestart)

	/*
	 *  Compatibility version number
	 *
	 *  These MUST be at byte offset 6 and 7 of the executable
	 *  DO NOT MOVE !!!
	 */
	. = EXT_C(main) + 0x6
	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR

	/*
	 *  This is a special data area 8 bytes from the beginning.
	 */

	. = EXT_C(main) + 0x8

VARIABLE(install_partition)
	.long	0xFFFFFF
VARIABLE(force_lba)
	.byte	0
VARIABLE(version_string)
	.string VERSION

	/*
	 *  Leave some breathing room for the config file name.
	 */

//	. = EXT_C(main) + 0x70

/* the real mode code continues... */
codestart:
	cli		/* we're not safe here! */

	/* set up %ds, %ss, and %es */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

#ifndef SUPPORT_DISKLESS
	/*
	 * Save the sector number of the second sector (i.e. this sector)
	 * in INSTALL_SECOND_SECTOR. See also "stage2/start.S".
	 */
	ADDR32	movl	%ebp, EXT_C(install_second_sector)
#endif
	
	/* set up the real mode/BIOS stack */
	movl	$STACKOFF, %ebp
	movl	%ebp, %esp

	sti		/* we're safe again */

#ifndef SUPPORT_DISKLESS
	/* save boot drive reference */
	ADDR32	movb	%dl, EXT_C(boot_drive)

	/* reset disk system (%ah = 0) */
	int	$0x13
#endif

	/* transition to protected mode */
	DATA32	call EXT_C(real_to_prot)

	/* The ".code32" directive takes GAS out of 16-bit mode. */
	.code32

	/* clean out the bss */

	/* set %edi to the bss starting address */
#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	movl	$__bss_start, %edi
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	movl	$_edata, %edi
#elif defined(HAVE_EDATA_SYMBOL)
	movl	$edata, %edi
#endif

	/* set %ecx to the bss end */	
#if defined(HAVE_END_SYMBOL)
	movl	$end, %ecx
#elif defined(HAVE_USCORE_END_SYMBOL)
	movl	$_end, %ecx
#endif

	/* compute the bss length */
	subl	%edi, %ecx
	
	/* zero %al */
	xorb	%al, %al

	/* set the direction */
	cld
	
	/* clean out */
	rep
	stosb
	
	/*
	 *  Call the start of main body of C code, which does some
	 *  of it's own initialization before transferring to "cmain".
	 */
	call EXT_C(init_bios_info)


/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */

ENTRY(stop)
	call	EXT_C(prot_to_real)

	/*
	 * This next part is sort of evil.  It takes advantage of the
	 * byte ordering on the x86 to work in either 16-bit or 32-bit
	 * mode, so think about it before changing it.
	 */

ENTRY(hard_stop)
	hlt
	jmp EXT_C(hard_stop)

#ifndef STAGE1_5





	
/*
 * chain_stage1(segment, offset, part_table_addr)
 *
 *  This starts another stage1 loader, at segment:offset.
 */

ENTRY(chain_stage1)
	/* no need to save anything, just use %esp */

	/* store %ESI, presuming %ES is 0 */
	movl	0xc(%esp), %esi

	/* store new offset */
	movl	0x8(%esp), %eax
	movl	%eax, offset

	/* store new segment */
	movw	0x4(%esp), %ax
	movw	%ax, segment

	/* set up to pass boot drive */
	movb	EXT_C(boot_drive), %dl

	call	EXT_C(prot_to_real)
	.code16

#ifdef ABSOLUTE_WITHOUT_ASTERISK
	DATA32	ADDR32	ljmp	(offset)
#else
	DATA32	ADDR32	ljmp	*(offset)
#endif
	.code32
#endif /* STAGE1_5 */



	
/*
 *  These next two routines, "real_to_prot" and "prot_to_real" are structured
 *  in a very specific way.  Be very careful when changing them.
 *
 *  NOTE:  Use of either one messes up %eax and %ebp.
 */

ENTRY(real_to_prot)
	.code16
	cli

	/* load the GDT register */
	DATA32	ADDR32	lgdt	gdtdesc

	/* turn on protected mode */
	movl	%cr0, %eax
	orl	$CR0_PE_ON, %eax
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	DATA32	ljmp	$PROT_MODE_CSEG, $protcseg

	/*
	 *  The ".code32" directive only works in GAS, the GNU assembler!
	 *  This gets out of "16-bit" mode.
	 */
	.code32

protcseg:
	/* reload other segment registers */
	movw	$PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* put the return address in a known safe location */
	movl	(%esp), %eax
	movl	%eax, STACKOFF

	/* get protected mode stack */
	movl	protstack, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* get return address onto the right stack */
	movl	STACKOFF, %eax
	movl	%eax, (%esp)

	/* zero %eax */
	xorl	%eax, %eax

	/* return on the old (or initialized) stack! */
	ret


ENTRY(prot_to_real)
	/* just in case, set GDT */
	lgdt	gdtdesc

	/* save the protected mode stack */
	movl	%esp, %eax
	movl	%eax, protstack

	/* get the return address */
	movl	(%esp), %eax
	movl	%eax, STACKOFF

	/* set up new stack */
	movl	$STACKOFF, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	ljmp	$PSEUDO_RM_CSEG, $tmpcseg	/* jump to a 16 bit segment */

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andl 	$CR0_PE_OFF, %eax
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	DATA32	ljmp	$0, $realcseg

realcseg:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* zero %eax */
	xorl	%eax, %eax

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* restore interrupts */
	sti

	/* return on new stack! */
	DATA32	ret

	.code32


/*
 *   int biosdisk_int13_extensions (int ax, int drive, void *dap)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ax=AX) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

ENTRY(biosdisk_int13_extensions)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%esi
	pushl	%ebx

	/* compute the address of disk_address_packet */
	movl	0x10(%ebp), %eax
	movw	%ax, %si
	xorw	%ax, %ax
	shrl	$4, %eax
	movw	%ax, %cx	/* save the segment to cx */

	/* drive */
	movb	0xc(%ebp), %dl
	/* ax */
	movw	0x8(%ebp), %bx
	/* enter real mode */
	call	EXT_C(prot_to_real)
	
	.code16
	movw	%bx, %ax
	movw	%cx, %ds
	int	$0x13		/* do the operation */
	movb	%ah, %dl	/* save return value */
	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dl, %al	/* return value in %eax */

	popl	%ebx
	popl	%esi
	popl	%ebp

	ret
	
/*
 *   int biosdisk_standard (int ah, int drive, int coff, int hoff, int soff,
 *                          int nsec, int segment)
 *
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */

ENTRY(biosdisk_standard)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%edi
	pushl	%esi

	/* set up CHS information */
	movl	0x10(%ebp), %eax
	movb	%al, %ch
	movb	0x18(%ebp), %al
	shlb	$2, %al
	shrw	$2, %ax
	movb	%al, %cl
	movb	0x14(%ebp), %dh
	/* drive */
	movb	0xc(%ebp), %dl
	/* segment */
	movw	0x20(%ebp), %bx
	/* save nsec and ah to %di */
	movb	0x8(%ebp), %ah
	movb	0x1c(%ebp), %al
	movw	%ax, %di
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	movw	%bx, %es
	xorw	%bx, %bx
	movw	$3, %si		/* attempt at least three times */

1:	
	movw	%di, %ax
	int	$0x13		/* do the operation */
	jnc	2f		/* check if successful */

	movb	%ah, %bl	/* save return value */
	/* if fail, reset the disk system */
	xorw	%ax, %ax
	int	$0x13
	
	decw	%si
	cmpw	$0, %si
	je	2f
	xorb	%bl, %bl
	jmp	1b		/* retry */
2:	
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%bl, %al	/* return value in %eax */
	
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp

	ret


/*
 *   int check_int13_extensions (int drive)
 *
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */

ENTRY(check_int13_extensions)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13		/* do the operation */
	
	/* check the result */
	jc	1f
	cmpw	$0xaa55, %bx
	jne	1f

	movb	%ah, %bl	/* save the major version into %bl */

	/* check if AH=0x42 is supported if FORCE_LBA is zero */
	movb	EXT_C(force_lba), %al
	testb	%al, %al
	jnz	2f
	andw	$1, %cx
	jnz	2f
	
1:
	xorb	%bl, %bl
2:
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%bl, %al	/* return value in %eax */

	popl	%ebx
	popl	%ebp

	ret


/*
 *   int get_diskinfo_standard (int drive, unsigned long *cylinders, 
 *                              unsigned long *heads, unsigned long *sectors)
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_standard)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%edi

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	movb	$0x8, %ah
	int	$0x13		/* do the operation */
	/* check if successful */
	testb	%ah, %ah
	jnz	1f
	/* bogus BIOSes may not return an error number */
	testb	$0x3f, %cl	/* 0 sectors means no disk */
	jnz	1f		/* if non-zero, then succeed */
	/* XXX 0x60 is one of the unused error numbers */
	movb	$0x60, %ah
1:
	movb	%ah, %bl	/* save return value in %bl */
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	/* restore %ebp */
	leal	0x8(%esp), %ebp
	
	/* heads */
	movb	%dh, %al
	incl	%eax		/* the number of heads is counted from zero */
	movl	0x10(%ebp), %edi
	movl	%eax, (%edi)

	/* sectors */
	xorl	%eax, %eax
	movb	%cl, %al
	andb	$0x3f, %al
	movl	0x14(%ebp), %edi
	movl	%eax, (%edi)

	/* cylinders */
	shrb	$6, %cl
	movb	%cl, %ah
	movb	%ch, %al
	incl	%eax		/* the number of cylinders is 
				   counted from zero */
	movl	0xc(%ebp), %edi
	movl	%eax, (%edi)

	xorl	%eax, %eax
	movb	%bl, %al	/* return value in %eax */

	popl	%edi
	popl	%ebx
	popl	%ebp

	ret


#if 0		
/*
 *   int get_diskinfo_floppy (int drive, unsigned long *cylinders, 
 *                            unsigned long *heads, unsigned long *sectors)
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_floppy)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi

	/* drive */
	movb	0x8(%ebp), %dl
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16
	/* init probe value */
	movl	$probe_values-1, %esi
1:
	xorw	%ax, %ax
	int	$0x13		/* reset floppy controller */

	incw	%si
	movb	(%si), %cl
	cmpb	$0, %cl		/* probe failed if zero */
	je	2f

	/* perform read */
	movw	$SCRATCHSEG, %ax
	movw	%ax, %es
	xorw	%bx, %bx
	movw	$0x0201, %ax
	movb	$0, %ch
	movb	$0, %dh
	int	$0x13

	/* FIXME: Read from floppy may fail even if the geometry is correct.
	   So should retry at least three times.  */
	jc	1b		/* next value */
	
	/* succeed */
	jmp	2f
	
probe_values:
	.byte	36, 18, 15, 9, 0
	
2:
	/* back to protected mode */
	DATA32	call	EXT_C(real_to_prot)
	.code32

	/* restore %ebp */
	leal	0x8(%esp), %ebp
	
	/* cylinders */
	movl	0xc(%ebp), %eax
	movl	$80, %ebx
	movl	%ebx, (%eax)
	/* heads */
	movl	0x10(%ebp), %eax
	movl	$2, %ebx
	movl	%ebx, (%eax)
	/* sectors */
	movl	0x14(%ebp), %eax
	movzbl	%cl, %ebx
	movl	%ebx, (%eax)

	/* return value in %eax */
	xorl	%eax, %eax
	cmpb	$0, %cl
	jne	3f
	incl	%eax		/* %eax = 1 (non-zero) */
3:
	popl	%esi
	popl	%ebx
	popl	%ebp

	ret
#endif
	

/* Source files are splitted, as they have different copyrights.  */
#ifndef STAGE1_5
# include "setjmp.S"
#endif /* ! STAGE1_5 */
		
	


#ifndef STAGE1_5


		
/*
 * gateA20(int linear)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 */

ENTRY(gateA20)
	/* first, try a BIOS call */
	pushl	%ebp
	movl	8(%esp), %edx
	
	call	EXT_C(prot_to_real)
	
	.code16
	movw	$0x2400, %ax
	testw	%dx, %dx
	jz	1f
	incw	%ax
1:	stc
	int	$0x15
	jnc	2f

	/* set non-zero if failed */
	movb	$1, %ah

	/* save the status */
2:	movb	%ah, %dl

	DATA32	call	EXT_C(real_to_prot)
	.code32

	popl	%ebp
	testb	%dl, %dl
	jnz	3f
	ret

3:	/*
	 * try to switch gateA20 using PORT92, the "Fast A20 and Init"
	 * register
	*/
	mov $0x92, %dx
	inb %dx, %al
	/* skip the port92 code if it's unimplemented (read returns 0xff) */
	cmpb $0xff, %al
	jz 6f
	
	/* set or clear bit1, the ALT_A20_GATE bit */
	movb 4(%esp), %ah
	testb %ah, %ah
	jz 4f
	orb $2, %al
	jmp 5f
4:	and $0xfd, %al
	
	/* clear the INIT_NOW bit don't accidently reset the machine */
5:	and $0xfe, %al
	outb %al, %dx
	
6:	/* use keyboard controller */
	pushl	%eax

	call    gloop1

	movb	$KC_CMD_WOUT, %al
	outb	$K_CMD

gloopint1:
	inb	$K_STATUS
	cmpb	$0xff, %al
	jz	gloopint1_done
	andb	$K_IBUF_FUL, %al
	jnz	gloopint1

gloopint1_done:	
	movb	$KB_OUTPUT_MASK, %al
	cmpb	$0, 0x8(%esp)
	jz	gdoit

	orb	$KB_A20_ENABLE, %al
gdoit:
	outb	$K_RDWR

	call	gloop1

	/* output a dummy command (USB keyboard hack) */
	movb	$0xff, %al
	outb	$K_CMD
	call	gloop1
	
	popl	%eax
	ret

gloop1:
	inb	$K_STATUS
	cmpb	$0xff, %al
	jz	gloop2ret
	andb	$K_IBUF_FUL, %al
	jnz	gloop1

gloop2:
	inb	$K_STATUS
	andb	$K_OBUF_FUL, %al
	jz	gloop2ret
	inb	$K_RDWR
	jmp	gloop2

gloop2ret:
	ret


ENTRY(patch_code)	/* labels start with "pc_" */
	.code16

	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	ADDR32	movl	$0, 0
pc_stop:
	hlt
	DATA32	jmp	pc_stop
ENTRY(patch_code_end)

	.code32


#endif /* ! STAGE1_5 */
	
/*
 * void console_putchar (int c)
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUPUT BIOS call doesn't
 * support setting a background attribute.
 */
ENTRY(console_putchar)
	movl	0x4(%esp), %edx
	pusha
#ifdef STAGE1_5
	movb	$0x07, %bl
#else
	movl	EXT_C(console_current_color), %ebx
#endif
	
	call	EXT_C(prot_to_real)
	.code16
	movb	%dl, %al
	xorb	%bh, %bh

#ifndef STAGE1_5
	/* use teletype output if control character */
	cmpb	$0x7, %al
	je	1f
	cmpb	$0x8, %al
	je	1f
	cmpb	$0xa, %al
	je	1f
	cmpb	$0xd, %al
	je	1f

	/* save the character and the attribute on the stack */
	pushw	%ax
	pushw	%bx
	
	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

	/* check the column with the width */
	cmpb	$79, %dl
	jl	2f
	
	/* print CR and LF, if next write will exceed the width */	
	movw	$0x0e0d, %ax
	int	$0x10
	movb	$0x0a, %al
	int	$0x10
	
	/* get the current position */
	movb	$0x3, %ah
	int	$0x10

2:	
	/* restore the character and the attribute */
	popw	%bx
	popw	%ax
	
	/* write the character with the attribute */
	movb	$0x9, %ah
	movw	$1, %cx
	int	$0x10

	/* move the cursor forward */
	incb	%dl
	movb	$0x2, %ah
	int	$0x10

	jmp	3f
#endif /* ! STAGE1_5 */
	
1:	movb	$0xe, %ah
	int	$0x10
	
3:	DATA32	call	EXT_C(real_to_prot)
	.code32
	
	popa
	ret


#ifndef STAGE1_5

/* this table is used in translate_keycode below */
translation_table:
	.word	KEY_LEFT, 2
	.word	KEY_RIGHT, 6
	.word	KEY_UP, 16
	.word	KEY_DOWN, 14
	.word	KEY_HOME, 1
	.word	KEY_END, 5
	.word	KEY_DC, 4
	.word	KEY_BACKSPACE, 8
	.word	KEY_PPAGE, 7
	.word	KEY_NPAGE, 3
	.word	0
	
/*
 * translate_keycode translates the key code %dx to an ascii code.
 */
	.code16

translate_keycode:
	pushw	%bx
	pushw	%si
	
	movw	$ABS(translation_table), %si
	
1:	lodsw
	/* check if this is the end */
	testw	%ax, %ax
	jz	2f
	/* load the ascii code into %ax */
	movw	%ax, %bx
	lodsw
	/* check if this matches the key code */
	cmpw	%bx, %dx
	jne	1b
	/* translate %dx, if successful */
	movw	%ax, %dx

2:	popw	%si
	popw	%bx
	ret

	.code32
	

/*
 * remap_ascii_char remaps the ascii code %dl to another if the code is
 * contained in ASCII_KEY_MAP.
 */
	.code16
	
remap_ascii_char:
	pushw	%si
	
	movw	$ascii_key_map, %si
1:
	lodsw
	/* check if this is the end */
	testw	%ax, %ax
	jz	2f
	/* check if this matches the ascii code */
	cmpb	%al, %dl
	jne	1b
	/* if so, perform the mapping */
	movb	%ah, %dl
2:
	/* restore %si */
	popw	%si

	ret

	.code32

/*
 * int console_getkey (void)
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

ENTRY(console_getkey)
	push	%ebp

wait_for_key:
	call	EXT_C(console_checkkey)
	incl	%eax
	jz	wait_for_key
	
	call	EXT_C(prot_to_real)
	.code16

	//hlt
	
	int	$0x16

	movw	%ax, %dx		/* real_to_prot uses %eax */
	call	translate_keycode
	call	remap_ascii_char
	
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movw	%dx, %ax

	pop	%ebp
	ret


/*
 * int console_checkkey (void)
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 */
ENTRY(console_checkkey)
	push	%ebp
	xorl	%edx, %edx
	
	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movb	$0x1, %ah
	int	$0x16

	DATA32	jz	notpending
	
	movw	%ax, %dx
	call	translate_keycode
	call	remap_ascii_char
	DATA32	jmp	pending

notpending:
	hlt
	movl	$0xFFFFFFFF, %edx

pending:
	DATA32	call	EXT_C(real_to_prot)
	.code32

	mov	%edx, %eax

	pop	%ebp
	ret







	
/*
 * int console_getxy (void)
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(console_getxy)
	push	%ebp
	push	%ebx                    /* save EBX */

	call	EXT_C(prot_to_real)
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dl, %ah
	movb	%dh, %al

	pop	%ebx
	pop	%ebp
	ret


/*
 * void console_gotoxy(int x, int y)
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(console_gotoxy)
	push	%ebp
	push	%ebx                    /* save EBX */

	movb	0xc(%esp), %dl           /* %dl = x */
	movb	0x10(%esp), %dh          /* %dh = y */

	call	EXT_C(prot_to_real)
	.code16

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x2, %ah
	int	$0x10			/* set cursor position */

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%ebx
	pop	%ebp
	ret

	
/*
 * void console_cls (void)
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */


ENTRY(console_cls)
	push	%ebp
	push	%ebx                    /* save EBX */

	call	EXT_C(prot_to_real)
	.code16

	/* move the cursor to the beginning */
	movb	$0x02, %ah
	xorb	%bh, %bh
	xorw	%dx, %dx
	int	$0x10

	/* write spaces to the entire screen */
	movw	$0x0920, %ax
	movw	$0x07, %bx
	movw	$(80 * 25), %cx
        int	$0x10

	/* move back the cursor */
	movb	$0x02, %ah
	int	$0x10

	DATA32	call	EXT_C(real_to_prot)
	.code32

	pop	%ebx
	pop	%ebp
	ret

	
/*
 * int console_setcursor (int on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */

console_cursor_state:
	.byte	1
console_cursor_shape:
	.word	0
	
ENTRY(console_setcursor)
	push	%ebp
	push	%ebx

	/* check if the standard cursor shape has already been saved */
	movw	console_cursor_shape, %ax
	testw	%ax, %ax
	jne	1f

	call	EXT_C(prot_to_real)
	.code16

	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movw	%cx, console_cursor_shape
1:
	/* set %cx to the designated cursor shape */
	movw	$0x2000, %cx
	movl	0xc(%esp), %ebx
	testl	%ebx, %ebx
	jz	2f
	movw	console_cursor_shape, %cx
2:	
	call	EXT_C(prot_to_real)
	.code16

	movb    $0x1, %ah
	int     $0x10 

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movzbl	console_cursor_state, %eax
	movb	%bl, console_cursor_state
	
	pop	%ebx
	pop	%ebp
	ret




/*
 * getrtsecs()
 *	if a seconds value can be read, read it and return it (BCD),
 *      otherwise return 0xFF
 * BIOS call "INT 1AH Function 02H" to check whether a character is pending
 *	Call with	%ah = 0x2
 *	Return:
 *		If RT Clock can give correct values
 *			%ch = hour (BCD)
 *			%cl = minutes (BCD)
 *                      %dh = seconds (BCD)
 *                      %dl = daylight savings time (00h std, 01h daylight)
 *			Carry flag = clear
 *		else
 *			Carry flag = set
 *                         (this indicates that the clock is updating, or
 *                          that it isn't running)
 */
ENTRY(getrtsecs)
	push	%ebp

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	movb	$0x2, %ah
	int	$0x1a

	DATA32	jnc	gottime
	movb	$0xff, %dh

gottime:
	DATA32	call	EXT_C(real_to_prot)
	.code32

	movb	%dh, %al

	pop	%ebp
	ret

	
/*
 * currticks()
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
ENTRY(currticks)
	pushl	%ebp

	call	EXT_C(prot_to_real)	/* enter real mode */
	.code16

	/* %ax is already zero */
        int	$0x1a

	DATA32	call	EXT_C(real_to_prot)
	.code32

	movl	%ecx, %eax
	shll	$16, %eax
	movw	%dx, %ax

	popl	%ebp
	ret

#endif /* STAGE1_5 */

/*
 *  This is the area for all of the special variables.
 */

	.p2align	2	/* force 4-byte alignment */

protstack:
	.long	PROTSTACKINIT

VARIABLE(boot_drive)
#ifdef SUPPORT_DISKLESS
	.long	NETWORK_DRIVE
#else
	.long	0
#endif

VARIABLE(install_second_sector)
	.long	0
	
	/* an address can only be long-jumped to if it is in memory, this
	   is used by multiple routines */
offset:
	.long	0x8000
segment:
	.word	0

VARIABLE(apm_bios_info)
	.word	0	/* version */
	.word	0	/* cseg */
	.long	0	/* offset */
	.word	0	/* cseg_16 */
	.word	0	/* dseg_16 */
	.word	0	/* cseg_len */
	.word	0	/* cseg_16_len */
	.word	0	/* dseg_16_len */
	
/*
 * This is the Global Descriptor Table
 *
 *  An entry, a "Segment Descriptor", looks like this:
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|0|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |
 * |                             |                            |
 * ------------------------------------------------------------
 *
 *  Note the ordering of the data items is reversed from the above
 *  description.
 */

	.p2align	2	/* force 4-byte alignment */
gdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	/* code segment */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	/* data segment */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* 16 bit real mode CS */
	.word	0xFFFF, 0
	.byte	0, 0x9E, 0, 0

	/* 16 bit real mode DS */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0


/* this is the GDT descriptor */
gdtdesc:
	.word	0x27			/* limit */
	.long	gdt			/* addr */
	
	
	
	
	
	
/* this code will be moved to and get executed at HMA_ADDR=0x2B0000 */

/* our gdt starts at HMA_ADDR=0x2B0000 */

ENTRY(HMA_start)

	/* the first entry of GDT, i.e., the default null entry,
	 * can be any value. it never get used. So we use these
	 * 8 bytes for our jmp and GDT descriptor.
	 */

	. = EXT_C(HMA_start) + 0	/* GDT entry: default null */

	jmp	1f		/* two-byte short jmp */

	. = EXT_C(HMA_start) + 2

	/* 6-byte GDT descriptor */
gdtdescHMA:
	.word	0x27		/* limit */
	.long	HMA_ADDR	/* linear base address */

	. = EXT_C(HMA_start) + 8	/* GDT entry: unused */

	/* code segment, although it is no use here for now */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	. = EXT_C(HMA_start) + 0x10	/* GDT entry: unused */

	/* data segment, although it is no use here for now */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	. = EXT_C(HMA_start) + 0x18	/* GDT entry: 16-bit code */

	/* 16-bit code segment base=0x2B0000 */
	.word	0xFFFF, 0x0000
	.byte	0x2B, 0x9E, 0, 0

	. = EXT_C(HMA_start) + 0x20	/* GDT entry: 16-bit data */

	/* real mode data segment base=0x200 */
	.word	0xFFFF, 0x0200
	.byte	0x00, 0x92, 0, 0

1:
	/* set up to pass boot drive */
	movb	EXT_C(boot_drive), %dl

	/* check if the --ebx option is given. */
	movl	(EXT_C(chain_ebx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movl	(EXT_C(chain_ebx) - EXT_C(HMA_start) + HMA_ADDR), %ebx
1:

	/* check if the --edx option is given. */
	movl	(EXT_C(chain_edx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movl	(EXT_C(chain_edx) - EXT_C(HMA_start) + HMA_ADDR), %edx
1:

	/* move new loader from extended memory to conventional memory.
	 * this will overwrite our GRUB code, data and stack, so we should not
	 * use instuctions like push/pop/call/ret, and we should not use
	 * functions like gateA20().
	 */

	/* the new loader is currently at 0x200000 */
	movl	$0x00200000, %esi
	xorl	%eax, %eax
	xorl	%edi, %edi
	movw	(EXT_C(chain_load_segment) - EXT_C(HMA_start) + HMA_ADDR), %di
	shll	$4, %edi
	movw	(EXT_C(chain_load_offset) - EXT_C(HMA_start) + HMA_ADDR), %ax
	addl	%eax, %edi
	//movl	$0x00007c00, %edi
	movl	(EXT_C(chain_load_length) - EXT_C(HMA_start) + HMA_ADDR), %ecx
	cld
	repz movsb

	/* switch to real mode */

	/* set new GDT */
	lgdt	(gdtdescHMA - EXT_C(HMA_start) + HMA_ADDR)

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	movl	$0x200, %esp	/* points to end of interrupt vector table */
				/* SS base=0x200, so SS:SP=physical 0x400 */

	/* jump to a 16 bit segment, this might be an extra step:
	 * set up CS limit, also clear high word of EIP
	 */
	ljmp	$PSEUDO_RM_CSEG, $(1f - EXT_C(HMA_start))
1:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	//andl	$CR0_PE_OFF, %eax
	andl 	$0x0000FFFE, %eax
	movl	%eax, %cr0

	/* setup DS, ES, SS, FS and GS before loading CS */
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$0x400, %esp
	movw	%ax, %fs
	movw	%ax, %gs

	/* flush prefetch queue, reload %cs */

	.byte	0xEA		/* ljmp 0000:7c00 */
VARIABLE(chain_boot_IP)
	.word	0x7c00		/* offset */
VARIABLE(chain_boot_CS)
	.word	0x0000		/* segment */

VARIABLE(chain_load_offset)
	.word	0x7c00
VARIABLE(chain_load_segment)
	.word	0x0000
VARIABLE(chain_load_length)
	.long	0x200
VARIABLE(chain_ebx)
	.long	0
VARIABLE(chain_ebx_set)
	.long	0
VARIABLE(chain_edx)
	.long	0
VARIABLE(chain_edx_set)
	.long	0
VARIABLE(chain_enable_gateA20)
	.long	0

	/* max length of code is 1 sector */
	. = . - (. - EXT_C(HMA_start))/0x201

	/* ensure this resides in the first 64KB */
	. = . - (ABS(.) / 0x10001)

	.code32


