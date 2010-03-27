/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2004,2005  Free Software Foundation, Inc.
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
#include <term.h>

#include "version.h"


char* p_cfg_start = 0x8000;
char* p_cfg_default_entryno = 0x8000 + 8;
char* p_cfg_entries_start = 0x8000 + 16;

char* p_cfg_top_end_color = 0x8000 + 3;
char* p_cfg_menu_color = 0x8000 + 11;

char* heap; //作为缓存

int grub_timeout = -1;
int grub_current_entryno = 1;
int num_entries = 0;

int top_end_color = 0x20;     //顶部和底部的颜色
int menu_color = 0x17;     //菜单颜色


grub_jmp_buf restart_env;



static void
print_top_and_end (void)
{
    int i;

    //打印顶栏
    console_current_color = top_end_color ;
    console_gotoxy (3,1);
    for (i=0;i<74;i++) grub_printf (" ");
    console_gotoxy (34,1);
    grub_printf ("63S-GRUB " S63_GRUB_VERSION );
    
    //打印底栏
    console_gotoxy (3,23);
    for (i=0;i<74;i++) grub_printf (" ");
    console_gotoxy (6,23);
    grub_printf ("Press E to show details of the entry.  Press C to enter command-line.");
    
}



static void
print_entries (void)
{
    int entryno;

    //逐条打印启动项
    console_current_color = menu_color;
    for (entryno=1; entryno<=num_entries; entryno++)
    {
        console_gotoxy (32, 6 + (entryno-1)*2 );
        if (entryno == grub_current_entryno)
        {
            console_current_color = ( ( (menu_color & 0x7) << 4) | ( (menu_color >> 4) & 0xf) );
            grub_printf ( " [%d] %s ", entryno, p_cfg_entries_start + (entryno-1)*80 );
            console_current_color = menu_color;
        }
        else
          grub_printf (" [%d] %s ", entryno, p_cfg_entries_start + (entryno-1)*80 );
    }

}




static void
print_countdown (void)
{
    int i;
    console_gotoxy (3,3);
    grub_printf (" Autobooting in %d seconds...", grub_timeout);

}



static int
boot_entry (char *entry)
{
    console_cls ();
    console_current_color = 0x7;
    grub_printf ("Booting %s ...\n\n");
    if ( run_script (entry+16, heap) ) //如果执行命令出错，或命令都已成功执行 但未执行 boot
    {
        grub_printf ("\nPress any key...");
        (void) console_getkey ();
        return 1;
    }
}




static void
run_menu (void)
{
    char c;
    int time1, time2 = -1;
    char* tmp;
    
restart1:
    console_cls ();
    print_top_and_end ();

restart2:
    print_entries ();
    while (1)
    {
        if (console_checkkey () >= 0)
        {
            if (grub_timeout > 0)
            {
                grub_timeout = -1;
                goto restart1;
            }
            
            c = ASCII_CHAR (console_getkey ());
            
            if ( (c >= 0x31) && (c <= num_entries + 0x30) )
            {
                grub_current_entryno = c - 0x30;
                goto restart2;
            }
            
            else if (c == 'c')
            {
                enter_cmdline (heap);
                //if press 'ESC'
                goto restart1;
            }
            
            else if (c == 'e')
            {
                console_cls ();
                console_gotoxy (0, 0 );
                tmp = p_cfg_entries_start + (grub_current_entryno-1)*80;
                //grub_printf ("[%d] %s\n\n", grub_current_entryno, tmp);
                tmp += 16;
                while(*(tmp))
                {
                    grub_printf ("%s\n", tmp);
                    while(*(tmp++)) ;
                }
                
                grub_printf ("\n\nPress any key...");
                (void) console_getkey ();
                
                goto restart1; 
                
            }
            
            else if (c == 16 )
            {
                /* Up */
                if (grub_current_entryno != 1)
                {
                    grub_current_entryno--;
                }
                else
                {
                    grub_current_entryno = num_entries;
                }
                goto restart2; 
            }
            
            else if (c == 14 )
            {
                /* Down */
                if (grub_current_entryno != num_entries)
                {
                    grub_current_entryno++;
                }
                else
                {
                    grub_current_entryno = 1;
                }
                goto restart2; 
            }
            
            else if ( (c == '\n' ) || (c == '\r') ) //启动
            {
                boot_entry (p_cfg_entries_start + (grub_current_entryno-1)*80 );//若返回，说明执行命令出错，或命令都已成功执行，但未执行 boot。此时返回启动菜单。
                goto restart1;
            }
            else 
            ;
        }
        
          if (grub_timeout == 0) 
          {
                //启动默认启动项
                boot_entry (p_cfg_entries_start + (grub_current_entryno-1)*80 ); //若返回，说明执行命令出错，或命令都已成功执行，但未执行 boot。此时返回启动菜单。
                goto restart1;
          }
          
          if (grub_timeout > 0)
          {
              time1 = getrtsecs();
              if ( time1 != time2  &&  time1 != 0xFF )
              {
                  time2 = time1;
                  grub_timeout--;
                  print_countdown ();
              }
          }

    }

}



/* This is the starting function in C.  */
void
cmain (void)
{
    char *kill_buf = (char *) KILL_BUF;
   
    auto void reset (void);
    void reset (void)
    {
        //count_lines = -1;
        num_entries = 0;
        heap = (char *) mbi.drives_addr + mbi.drives_length;
        //current_term = term_table;
        console_setcursor(0);
    }
  
  /* Initialize the environment for restarting Stage 2.  */
  grub_setjmp (restart_env);
  
  /* Initialize the kill buffer.  */
  *kill_buf = 0;
  
  /* Never return.  */
  for (;;)
    {
        char* tmp;
        tmp = p_cfg_entries_start;
        
        reset ();
        console_cls ();

        if (*p_cfg_start)
          safe_parse_maxint (&p_cfg_start, &grub_timeout);
        if (grub_timeout > 0) grub_timeout++; //若无此步，倒计时会少显示一秒。

        if (*p_cfg_default_entryno)
          safe_parse_maxint (&p_cfg_default_entryno, &grub_current_entryno); //读取默认启动序号

        if (*p_cfg_top_end_color)
          safe_parse_maxint (&p_cfg_top_end_color, &top_end_color); //读取颜色
        
        if (*p_cfg_menu_color)
          safe_parse_maxint (&p_cfg_menu_color, &menu_color);


        //数一数有几个启动项
        while( num_entries <= 6 && *(tmp) )
        {
            num_entries++;
            tmp += 80;
        }
        
        if (! num_entries) enter_cmdline (heap);
        else run_menu ();
    }
}
