/* -*-Asm-*- */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004   Free Software Foundation, Inc.
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

#include <stage1.h>
	
/*
 *  defines for the code go here
 */

	/* Absolute addresses
	   This makes the assembler generate the address without support
	   from the linker. (ELF can't relocate 16-bit addresses!) */
#define ABS(x) (x-_start+0x7c00)

	/* Print message string */
#define MSG(x)	movw $ABS(x), %si; call message

	/* XXX:	binutils-2.9.1.0.x doesn't produce a short opcode for this. */
#define	MOV_MEM_TO_AL(x)	.byte 0xa0;  .word x
	
	.file	"stage1.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

.globl _start; _start:
	/*
	 * _start is loaded at 0x7c00 and is jumped to with CS:IP 0:0x7c00
	 */

	/*
	 * Beginning of the sector is compatible with the FAT/HPFS BIOS
	 * parameter block.
	 */

	jmp	after_BPB
	nop	/* do I care about this ??? */

	/*
	 * This space is for the BIOS parameter block!!!!  Don't change
	 * the first jump, nor start the code anywhere but right after
	 * this area.
	 */

	. = _start + 4

	/* scratch space */
mode:
	.byte	0
disk_address_packet:	
sectors:
	.long	0
heads:
	.long	0
cylinders:
	.word	0
sector_start:
	.byte	0
head_start:
	.byte	0
cylinder_start:
	.word	0
	/* more space... */

	. = _start + STAGE1_BPBEND

	/*
	 * End of BIOS parameter block.
	 */

stage1_version:	
	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR
boot_drive:	
	.byte	GRUB_INVALID_DRIVE	/* the disk to load stage2 from */
force_lba:
	.byte	0
stage2_address:
	.word	0x8000
stage2_sector:
	.long	1
stage2_segment:
	.word	0x800



after_BPB:


1:
	cli
	xorw	%bx, %bx
	movw	%bx, %ss
	movw	$0x580, %sp		/* temp safe stack space */
	call	1f
1:
	popw	%bx			/* Instruction Pointer of 1b */
	subw	$(1b - _start), %bx	/* CS:BX=_start */

	shrw	$4, %bx
	movw	%cs, %ax
	addw	%ax, %bx		/* BX:0000=_start */


	/* BX:0000=_start */

	movw	%bx, %ds

	/* Let CS:0000=_start */
	pushw	%bx

	#;pushw	$(1f - _start)
	.byte	0x6A, (1f - _start)

	lret
	. = . - (. - _start) / 0x80
1:


	
	/* CS:0000=DS:0000=_start */

	/* we are loaded by BIOS or another boot loader */



	pushw	$0x7e0 /*Not 0x7e00! 就是这个错误，昨天搞了一天，午饭都没吃，到今日凌晨1点15分，才算搞定。他奶奶的。*/
	popw	%ss
	movw	$0xf000, %sp
	sti
	
	
	pushw	%ss		/* SS=CS */
	popw	%es		/* ES=CS */


	movw	$0x003f, %cx /*读取63个扇区*/
1:
	movw	$0x0201, %ax	/* read 1 sector */
	movw	%cx, %bx
	decw	%bx
	shlw	$9, %bx
	movw	$0x0080, %dx
	pushw	%cx
	pushw	%bx
	call	int13
	popw	%bx
	popw	%cx
	jc	read_error
	//MSG(test_string)
	loop	1b
	negb	%ah		/* set CF=1 if non-zero */
1:
	pushw	%cs		/* DS=0 */
	popw	%ds		/* DS=CS */
	pushfw			/* CF=1 on error */
	


	movb	$0x80, %dl	/* this makes sure %dl is our "boot" drive */

	ljmp	$0, $0x8200


/********************************************************************************************************************************/
/********************************************************************************************************************************/
/********************************************************************************************************************************/




3:	jmp	3b
	
	
	
	
int13:
	pushw	%ds
	pushw	%es
//	pushw	%bx
	pushw	%dx
	pushw	%si
	pushw	%di
	pushw	%bp
	stc
	int	$0x13
	popw	%bp
	popw	%di
	popw	%si
	popw	%dx
//	popw	%bx
	popw	%es
	popw	%ds
	ret
	




/*
 * Read error on the disk.
 */
read_error:
	MSG(read_error_string)

/* go here when you need to stop the machine hard after an error condition */
stop:	jmp	stop



test_string:	.string "Read 1 sector.  "
notification_string:	.string "63S-GRUB "
geometry_error_string:	.string "Geom"
hd_probe_error_string:	.string "Hard Disk"
read_error_string:	.string "Read"
general_error_string:	.string " Error"
	




/*
 * message: write the string pointed to by %si
 *
 *   WARNING: trashes %si, %ax, and %bx
 */

	/*
	 * Use BIOS "int 10H Function 0Eh" to write character in teletype mode
	 *	%ah = 0xe	%al = character
	 *	%bh = page	%bl = foreground color (graphics modes)
	 */
1:
	movw	$0x0001, %bx
	movb	$0xe, %ah
	int	$0x10		/* display a byte */
message:
	lodsb
	cmpb	$0, %al
	jne	1b	/* if not end of string, jmp to display */
	ret

	

	/*
	 *  Windows NT breaks compatibility by embedding a magic
	 *  number here.
	 */

	. = _start + STAGE1_WINDOWS_NT_MAGIC
nt_magic:	
	.long 0
	.word 0

	/*
	 *  This is where an MBR would go if on a hard disk.  The code
	 *  here isn't even referenced unless we're on a floppy.  Kinda
	 *  sneaky, huh?
	 */

part_start:	
	. = _start + STAGE1_PARTSTART

		

	. = _start + STAGE1_PARTEND

/* the last 2 bytes in the sector 0 contain the signature */
	.word	STAGE1_SIGNATURE
