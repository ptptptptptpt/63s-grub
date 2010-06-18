/* builtins.c - the GRUB builtin commands */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

/* Include stdio.h before shared.h, because we can't define
   WITHOUT_LIBC_STUBS here.  */
   
//#ifdef GRUB_UTIL
//# include <stdio.h>
//#endif

#include <shared.h>
#include <filesys.h>
#include <term.h>






/* The type of kernel loaded.  */
kernel_t kernel_type;
/* The boot device.  */
static int bootdev;
/* True when the debug mode is turned on, and false
   when it is turned off.  */
int debug = 0;
/* The default entry.  */
int default_entry = 0;
/* The fallback entry.  */
int fallback_entryno;
int fallback_entries[MAX_FALLBACK_ENTRIES];
/* The number of current entry.  */
int current_entryno;
/* The address for Multiboot command-line buffer.  */
static char *mb_cmdline;


/* Whether to quiet boot messages or not. */
int quiet_boot = 0;
/* The BIOS drive map.  */
//static unsigned short bios_drive_map[DRIVE_MAP_SIZE + 1];



static long chainloader_edx = 0;

extern unsigned short chain_load_segment;//0x0000;
extern unsigned short chain_load_offset;//0x7c00;
extern unsigned int chain_load_length;//0x200;
extern unsigned short chain_boot_CS;//0x0000;
extern unsigned short chain_boot_IP;//0x7c00;
extern unsigned long chain_ebx;
extern unsigned long chain_ebx_set;
extern unsigned long chain_edx;
extern unsigned long chain_edx_set;
extern unsigned long chain_enable_gateA20;
extern char HMA_start[];


/* Initialize the data for builtins.  */
void
init_builtins (void)
{
  kernel_type = KERNEL_TYPE_NONE;
  /* BSD and chainloading evil hacks!  */
  bootdev = set_bootdev (0);
  mb_cmdline = (char *) MB_CMDLINE_BUF;
}



/* boot */
static int
boot_func (char *arg, int flags)
{
  /* Clear the int15 handler if we can boot the kernel successfully.
     This assumes that the boot code never fails only if KERNEL_TYPE is
     not KERNEL_TYPE_NONE. Is this assumption is bad?  */
  if (kernel_type != KERNEL_TYPE_NONE)
    unset_int15_handler ();


  grub_printf("Starting up ...\n");
  switch (kernel_type)
  {
    case KERNEL_TYPE_CHAINLOADER:
        /* Chainloader */
        
        gateA20 (0);
        boot_drive = saved_drive;
        chain_stage1 (0, BOOTSEC_LOCATION, boot_part_addr);
        break;

    case KERNEL_TYPE_NTLDR:
      /* ntldr */
      gateA20 (0);
      boot_drive = saved_drive;
      
      chain_ebx = boot_drive;
      chain_ebx_set = 1;
      chain_edx = boot_drive;
      chain_edx_set = 1;
      //chain_enable_gateA20 = 1;
      chain_load_segment = 0x2000;
      chain_load_offset = 0;
      chain_boot_CS = 0x2000;
      chain_boot_IP = 0;
      /* move the code to a safe place at 0x2B0000 */
      grub_memmove((char *)HMA_ADDR, HMA_start, 0x200/*0xfff0*/);
      /* Jump to high memory area. This will move boot code at
      * 0x200000 to the destination load-segment:load-offset;
      * setup edx and ebx registers; switch to real mode;
      * and jump to boot-cs:boot-ip.
      */
      ((void (*)(void))HMA_ADDR)();  /* no return */
      break;

    case KERNEL_TYPE_GRUB:
      /* stage2 / core.img */
      gateA20 (0);
      boot_drive = saved_drive;
      
      chain_ebx = boot_drive;
      chain_ebx_set = 1;
      chain_edx = boot_drive;
      chain_edx_set = 1;
      //chain_enable_gateA20 = 1;
      chain_load_segment = 0;
      chain_load_offset = 0x8000;
      chain_boot_CS = 0;
      chain_boot_IP = 0x8200;
      /* move the code to a safe place at 0x2B0000 */
      grub_memmove((char *)HMA_ADDR, HMA_start, 0x200/*0xfff0*/);
      /* Jump to high memory area. This will move boot code at
      * 0x200000 to the destination load-segment:load-offset;
      * setup edx and ebx registers; switch to real mode;
      * and jump to boot-cs:boot-ip.
      */
      ((void (*)(void))HMA_ADDR)();  /* no return */
      break;

    default:
      errnum = ERR_BOOT_COMMAND;
      return 1;
  }


  return 0;
}

static struct builtin builtin_boot =
{
  "boot",
  boot_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};





/* chainloader */
static int
chainloader_func (char *arg, int flags)
{
  int force = 0;
  char *file = arg;

  /* If the option `--force' is specified?  */
  if (substring ("--force", arg) <= 0)
    {
      force = 1;
      file = skip_to (0, arg);
    }

  /* Open the file.  */
  if (! grub_open (file))
    {
      kernel_type = KERNEL_TYPE_NONE;
      return 1;
    }

  /* Read the first block.  */
  if (grub_read ((char *) BOOTSEC_LOCATION, SECTOR_SIZE) != SECTOR_SIZE)
    {
      grub_close ();
      kernel_type = KERNEL_TYPE_NONE;

      /* This below happens, if a file whose size is less than 512 bytes
     is loaded.  */
      if (errnum == ERR_NONE)
    errnum = ERR_EXEC_FORMAT;
      
      return 1;
    }

  /* If not loading it forcibly, check for the signature.  */
  if (! force
      && (*((unsigned short *) (BOOTSEC_LOCATION + BOOTSEC_SIG_OFFSET))
      != BOOTSEC_SIGNATURE))
    {
      grub_close ();
      errnum = ERR_EXEC_FORMAT;
      kernel_type = KERNEL_TYPE_NONE;
      return 1;
    }

  grub_close ();
  kernel_type = KERNEL_TYPE_CHAINLOADER;

  /* XXX: Windows evil hack. For now, only the first five letters are
     checked.  */
  if (IS_PC_SLICE_TYPE_FAT (current_slice)
      && ! grub_memcmp ((char *) BOOTSEC_LOCATION + BOOTSEC_BPB_SYSTEM_ID,
            "MSWIN", 5))
    *((unsigned long *) (BOOTSEC_LOCATION + BOOTSEC_BPB_HIDDEN_SECTORS))
      = part_start;

  errnum = ERR_NONE;
  
  return 0;
}

static struct builtin builtin_chainloader =
{
  "chainloader",
  chainloader_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};





static int
read_file (char *arg)
{
    int len;
    if (! grub_open (arg))
    {
        kernel_type = KERNEL_TYPE_NONE;
        return 1;
    }
    /* read the new loader to physical address 2MB, overwriting
     * the backup area of DOS memory.
    */
    len = grub_read ((char *) 0x200000, -1);
    if (! len)
    {
        grub_close ();
        kernel_type = KERNEL_TYPE_NONE;
        if (errnum == ERR_NONE)    errnum = ERR_READ;
        return 0;
     }
    else
    {
        grub_printf("File lenth : %x \n", len);
    }
    grub_close ();
    
    grub_printf("%s loaded.\n", arg );
    chain_load_length = len;
    
    return 1; 
}





static int
loadgrub_func (char *arg, int flags)
{
    
    kernel_type = KERNEL_TYPE_GRUB;
    
    read_file (arg);
    
    //grub_printf("Will boot %s from drive=0x%x, partition=0x%x(hidden sectors=0x%x)\n", arg, current_drive, ((current_partition >> 16) & 0xff), (long)part_start);

    return 1; 
    
}


static struct builtin builtin_loadgrub =
{
  "loadgrub",
  loadgrub_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};





static int
ntldr_func (char *arg, int flags)
{
    kernel_type = KERNEL_TYPE_NTLDR;
    
    read_file (arg);
    
    //读取BPB
    chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
    if ((chainloader_edx & 0xFF00) == 0xFF00)
        grub_sprintf ((char *)SCRATCHADDR, "(%d)+1", (chainloader_edx & 0xff));
    else
        grub_sprintf ((char *)SCRATCHADDR, "(%d,%d)+1", (chainloader_edx & 0xff), ((chainloader_edx >> 8) & 0xff));

    if (! grub_open ((char *)SCRATCHADDR))
    {
        grub_printf("Open partition error.\n");
        return 0; 
    }
    /* Read the boot sector of the partition onto 0000:7C00    */
    if (grub_read ((char *) SCRATCHADDR, SECTOR_SIZE ) != SECTOR_SIZE)
    {
        grub_close ();
        kernel_type = KERNEL_TYPE_NONE;
        grub_printf("Read boot sector error.\n");
        return 0; 
    }
    grub_close ();
    
    /* modify the hidden sectors */
    /* FIXME: Does the boot drive number also need modifying? */
    
    if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)))
       *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)) = (unsigned long)part_start;

    //grub_printf("Will boot %s from drive=0x%x, partition=0x%x(hidden sectors=0x%x)\n", arg, current_drive, ((current_partition >> 16) & 0xff), (long)part_start);
    
    grub_memmove ((char *)0x7C00, (char *)SCRATCHADDR, 512);
    
    return 1; 
}



static struct builtin builtin_ntldr =
{
  "ntldr",
  ntldr_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};






/* Print the root device information.  */
static void
print_root_device (void)
{
  if (saved_drive == NETWORK_DRIVE)
    {
      /* Network drive.  */
      grub_printf (" (nd):");
    }
  else if (saved_drive & 0x80)
    {
      /* Hard disk drive.  */
      grub_printf (" (hd%d", saved_drive - 0x80);
      
      if ((saved_partition & 0xFF0000) != 0xFF0000)
    grub_printf (",%d", saved_partition >> 16);

      if ((saved_partition & 0x00FF00) != 0x00FF00)
    grub_printf (",%c", ((saved_partition >> 8) & 0xFF) + 'a');

      grub_printf ("):");
    }
  else
    {
      /* Floppy disk drive.  */
      grub_printf (" (fd%d):", saved_drive);
    }

  /* Print the filesystem information.  */
  current_partition = saved_partition;
  current_drive = saved_drive;
  if (!quiet_boot)
    print_fsys_type ();
}





static int
real_root_func (char *arg, int attempt_mount)
{
  int hdbias = 0;
  char *biasptr;
  char *next;

  /* If ARG is empty, just print the current root device.  */
    if (! *arg)
    {
        print_root_device ();
        return 0;
    }

    
  /* Call set_device to get the drive and the partition in ARG.  */
  next = set_device (arg);
  if (! next)
    return 1;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (attempt_mount)
    {
      if (! open_device () && errnum != ERR_FSYS_MOUNT)
    return 1;
    }
  else
    {
      /* This is necessary, because the location of a partition table
     must be set appropriately.  */
      if (open_partition ())
    {
      set_bootdev (0);
      if (errnum)
        return 1;
    }
    }
  
  /* Clear ERRNUM.  */
  errnum = 0;
  saved_partition = current_partition;
  saved_drive = current_drive;

  if (attempt_mount)
    {
      /* BSD and chainloading evil hacks !!  */
      biasptr = skip_to (0, next);
      safe_parse_maxint (&biasptr, &hdbias);
      errnum = 0;
      bootdev = set_bootdev (hdbias);
      if (errnum)
    return 1;
      
      /* Print the type of the filesystem.  */
      if (!quiet_boot)
        print_fsys_type ();
    }
  
  return 0;
}

static int
root_func (char *arg, int flags)
{
  return real_root_func (arg, 1);
}

static struct builtin builtin_root =
{
  "root",
  root_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};


/* rootnoverify */
static int
rootnoverify_func (char *arg, int flags)
{
  return real_root_func (arg, 0);
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  " ",
  " "
};




/* find */
/* Search for the filename ARG in all of partitions.  */
static int
find_func (char *arg, int flags)
{
  char *filename;
  unsigned long drive;
  unsigned long tmp_drive = saved_drive;
  unsigned long tmp_partition = saved_partition;
  int got_file = 0;

  int set_root = 0;
  char root_found[16];


  for (;;)
  {
    if (grub_memcmp (arg, "--set-root", 10) == 0)
    {
      set_root = 1;
    }
    else
      break;
    arg = skip_to (0, arg);
  }

  filename = arg;


  /* Hard disks.  */
  for (drive = 0x80; drive < 0x88; drive++)
    {
      unsigned long part = 0xFFFFFF;
      unsigned long start, len, offset, ext_offset, gpt_offset;
      int type, entry, gpt_count, gpt_size;
      char buf[SECTOR_SIZE];

      current_drive = drive;
      while (next_partition (drive, 0xFFFFFF, &part, &type,
                 &start, &len, &offset, &entry,
                 &ext_offset, &gpt_offset,
                 &gpt_count, &gpt_size, buf))
        {
            if (type != PC_SLICE_TYPE_NONE
                && ! IS_PC_SLICE_TYPE_BSD (type)
                && ! IS_PC_SLICE_TYPE_EXTENDED (type))
            {
                current_partition = part;
                if (open_device ())
                {
                    saved_drive = current_drive;
                    saved_partition = current_partition;
                    if (grub_open (filename))
                    {
                        int bsd_part = (part >> 8) & 0xFF;
                        int pc_slice = part >> 16;
                        
                        grub_close ();
                        
                        if (bsd_part == 0xFF)
                          grub_printf (" (hd%d,%d)\n",
                                drive - 0x80, pc_slice);
                        else
                          grub_printf (" (hd%d,%d,%c)\n",
                                drive - 0x80, pc_slice, bsd_part + 'a');

                        got_file = 1;
                        
                        if (set_root)
                        {
                            if (bsd_part == 0xFF)
                              grub_sprintf (root_found, "(hd%d,%d)\0",
                                     drive - 0x80, pc_slice);
                            else
                              grub_sprintf (root_found, "(hd%d,%d,%c)\0",
                                     drive - 0x80, pc_slice, bsd_part + 'a');
                             
                             break;
                        }
                    }
                }
            }

            /* We want to ignore any error here.  */
            errnum = ERR_NONE;
        }

      if (set_root && got_file)
        break;
      
      /* next_partition always sets ERRNUM in the last call, so clear
     it.  */
      errnum = ERR_NONE;
    }

  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (got_file)
    {
      errnum = ERR_NONE;
      if (set_root)
        return real_root_func (root_found, 0);
      else
        return 0;
    }

  errnum = ERR_FILE_NOT_FOUND;
  return 1;
}

static struct builtin builtin_find =
{
  "find",
  find_func,
  BUILTIN_CMDLINE | BUILTIN_HELP_LIST,
  "find FILENAME",
  "Search for the filename FILENAME in all of partitions and print the list of"
  " the devices which contain the file."
};







/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
  &builtin_boot,
  &builtin_chainloader,
  
  &builtin_find,
  &builtin_loadgrub,
  &builtin_ntldr,
  
  &builtin_root,
  &builtin_rootnoverify,

  
  0
};
