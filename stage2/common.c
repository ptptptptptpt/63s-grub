/* common.c - miscellaneous shared variables and routines */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
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

#include <shared.h>


/*
 *  Shared BIOS/boot data.
 */

//struct multiboot_info mbi;
unsigned long saved_drive;
unsigned long saved_partition;
unsigned long cdrom_drive;
#ifndef STAGE1_5
unsigned long saved_mem_upper;

/* This saves the maximum size of extended memory (in KB).  */
unsigned long extended_memory;
#endif

/*
 *  Error code stuff.
 */

grub_error_t errnum = ERR_NONE;

#ifndef STAGE1_5

char *err_list[] =
{
  [ERR_NONE] = 0,
  [ERR_BAD_ARGUMENT] = "Invalid argument",
  [ERR_BAD_FILENAME] =
  "Filename must be either an absolute pathname or blocklist",
  [ERR_BAD_FILETYPE] = "Bad file or directory type",
  [ERR_BAD_GZIP_DATA] = " ",
  [ERR_BAD_GZIP_HEADER] = " ",
  [ERR_BAD_PART_TABLE] = "Partition table invalid or corrupt",
  [ERR_BAD_VERSION] = " ",
  [ERR_BELOW_1MB] = " ",
  [ERR_BOOT_COMMAND] = "Kernel must be loaded before booting",
  [ERR_BOOT_FAILURE] = "Unknown boot failure",
  [ERR_BOOT_FEATURES] = " ",
  [ERR_DEV_FORMAT] = "Unrecognized device string",
  [ERR_DEV_NEED_INIT] = "Device not initialized yet",
  [ERR_DEV_VALUES] = "Invalid device requested",
  [ERR_EXEC_FORMAT] = "Invalid or unsupported executable format",
  [ERR_FILELENGTH] =
  "Filesystem compatibility error, cannot read whole file",
  [ERR_FILE_NOT_FOUND] = "File not found",
  [ERR_FSYS_CORRUPT] = "Inconsistent filesystem structure",
  [ERR_FSYS_MOUNT] = "Cannot mount selected partition",
  [ERR_GEOM] = "Selected cylinder exceeds",
  [ERR_NEED_LX_KERNEL] = " ",
  [ERR_NEED_MB_KERNEL] = " ",
  [ERR_NO_DISK] = "Selected disk does not exist",
  [ERR_NO_DISK_SPACE] = " ",
  [ERR_NO_PART] = "No such partition",
  [ERR_NUMBER_OVERFLOW] = "Overflow while parsing number",
  [ERR_NUMBER_PARSING] = "Error while parsing number",
  [ERR_OUTSIDE_PART] = "Attempt to access block outside partition",
  [ERR_PRIVILEGED] = " ",
  [ERR_READ] = "Disk read error",
  [ERR_SYMLINK_LOOP] = "Too many symbolic links",
  [ERR_UNALIGNED] = " ",
  [ERR_UNRECOGNIZED] = "Unrecognized command",
  [ERR_WONT_FIT] = "Selected item cannot fit into memory",
  [ERR_WRITE] = "Disk write error",
};


/* static for BIOS memory map fakery */
//static struct AddrRangeDesc fakemap[3] =


/* A big problem is that the memory areas aren't guaranteed to be:
   (1) contiguous, (2) sorted in ascending order, or (3) non-overlapping.
   Thus this kludge.  */
//static unsigned long
//mmap_avail_at (unsigned long bottom)





#endif /* ! STAGE1_5 */

/* This queries for BIOS information.  */
void
init_bios_info (void)
{
#ifndef STAGE1_5
//  unsigned long cont, memtmp, addr;
//  int drive;
#endif

  /*
   *  Get information from BIOS on installed RAM.
   */

//  mbi.mem_lower = get_memsize (0);
//  mbi.mem_upper = get_memsize (1);

#ifndef STAGE1_5
  /*
   *  We need to call this somewhere before trying to put data
   *  above 1 MB, since without calling it, address line 20 will be wired
   *  to 0.  Not too desirable.
   */

  gateA20 (1);

  /* Store the size of extended memory in EXTENDED_MEMORY, in order to
     tell it to non-Multiboot OSes.  */
//  extended_memory = mbi.mem_upper;
  
  /*
   *  The "mbi.mem_upper" variable only recognizes upper memory in the
   *  first memory region.  If there are multiple memory regions,
   *  the rest are reported to a Multiboot-compliant OS, but otherwise
   *  unused by GRUB.
   */











  /* Get the drive info.  */
  /* FIXME: This should be postponed until a Multiboot kernel actually
     requires it, because this could slow down the start-up
     unreasonably.  */








  /* Get the ROM configuration table by INT 15, AH=C0h.  */
//  mbi.config_table = get_rom_config_table ();

  /* Set the boot loader name.  */
//  mbi.boot_loader_name = (unsigned long) "GNU GRUB " VERSION;

  /*
   *  Initialize other Multiboot Info flags.
   */

//  mbi.flags = (MB_INFO_MEMORY | MB_INFO_CMDLINE | MB_INFO_BOOTDEV
//	       | MB_INFO_DRIVE_INFO | MB_INFO_CONFIG_TABLE
//	       | MB_INFO_BOOT_LOADER_NAME);
//  


#endif /* STAGE1_5 */

  /* Set boot drive and partition.  */
  saved_drive = boot_drive;
  saved_partition = install_partition;

  /* Set cdrom drive.  */
  {
    struct geometry geom;
    
    /* Get the geometry.  */
    if (get_diskinfo (boot_drive, &geom)
	|| ! (geom.flags & BIOSDISK_FLAG_CDROM))
      cdrom_drive = GRUB_INVALID_DRIVE;
    else
      cdrom_drive = boot_drive;
  }
  
  /* Start main routine here.  */
  cmain ();
}
