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

#define MAX_MUN_ENTRIES 20


char* heap; //作为缓存

int num_entries = 0;

int grub_timeout = 10;
int grub_current_entryno = 1;

int top_end_color = 0x00;     //顶部和底部的颜色
int menu_color = 0x17;     //菜单颜色


struct an_entry {
  char * tittle;
  char * contents;
};

struct an_entry all_entries[ MAX_MUN_ENTRIES ];



grub_jmp_buf restart_env;



static int
get_all_entries( void )   //将所有启动项起始地址存入 all_entries[] ，同时数出 num_entries 。
{
    char* ptr = (char*)0x8000;
    int end = 0x8000 + 496;
    
    while ( !(*ptr) && ( (int)ptr < end ) )  ptr++; //找到第一个非空字符
    if ( !(*ptr) ) return 0; //从头到 end 未找到非空字符，无菜单
    
    struct an_entry *tmp = all_entries;
    num_entries = 0;
    while( (int)ptr < end  &&  num_entries < MAX_MUN_ENTRIES )
    {
        tmp -> tittle = ptr;
        while (*ptr) ptr++;
        ptr++;
        tmp -> contents = ptr;
        
        while (1)
        {
            while (*ptr) ptr++;
            ptr++;
            if ( !(*ptr) ) break;
        }

        num_entries++;
        tmp++;
        
        while ( !(*ptr) && ( (int)ptr < end ) )  ptr++;

    }
    return 1; 

}





static void
print_top_and_end (void)
{
    int i;

    //打印顶栏
    console_current_color = top_end_color ;
    console_gotoxy (2,0);
    for (i=0;i<76;i++) grub_printf (" ");
    console_gotoxy (32,0);
    grub_printf ("63S-GRUB " VERSION );
    
    //打印底栏
    console_gotoxy (2,24);
    for (i=0;i<76;i++) grub_printf (" ");
    console_gotoxy (6,24);
    grub_printf ("Press E to show details of the entry.  Press C to enter command-line.");
    
}




static void
print_an_entry ( char * p, int entry_no)
{
    int i=0;
    grub_printf ( "  " );
    if ( entry_no < 10 ) grub_putchar ('0');
    grub_printf ( "%d  ", entry_no);
    for (i=0;i<27;i++)
    {
        if (*p)
        {
            grub_putchar (*p);
            p++;
        }
        else
        {
            grub_putchar (' ');
        }
    }

}



static void
print_entries (void)
{
    int entryno;

    //逐条打印启动项
    console_current_color = menu_color;
    for (entryno=1; entryno<=num_entries; entryno++)
    {
        console_gotoxy (24, 2 + entryno );
        if (entryno == grub_current_entryno)
        {
            console_current_color = ( ( (menu_color & 0x7) << 4) | ( (menu_color >> 4) & 0xf) );
            print_an_entry ( (all_entries + entryno -1) -> tittle , entryno );
            console_current_color = menu_color;
        }
        else
            print_an_entry ( (all_entries + entryno -1) -> tittle , entryno );
    }

}




static void
countdown ( int show )
{
    int i;
    console_gotoxy (60, 2 + grub_current_entryno );
    console_current_color = ( menu_color & 0xf );
    if (show)
      grub_printf ("%d  ", grub_timeout);
    else
      for (i=0; i<20; i++) grub_putchar(' ');

}



static int
boot_entry (struct an_entry * entry )
{
    console_cls ();
    console_current_color = 0x7;
    grub_printf ("Booting %s ...\n\n", entry -> tittle );

    if ( run_script (entry -> contents, heap) ) //如果执行命令出错，或命令都已成功执行 但未执行 boot
    {
        grub_printf ("\nPress any key...");
        (void) console_getkey ();
        return 1;
    }
    return 0;
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
                countdown (0);
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
                tmp = (all_entries + grub_current_entryno -1) -> contents;
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
                boot_entry ( all_entries + grub_current_entryno -1 );//若返回，说明执行命令出错，或命令都已成功执行，但未执行 boot。此时返回启动菜单。
                goto restart1;
            }
            else 
            ;
        }
        
          if (grub_timeout == 0) 
          {
                //启动默认启动项
                grub_timeout = -1; //若无此句，启动失败后，将陷入死循环。
                boot_entry ( all_entries + grub_current_entryno -1 ); //若返回，说明执行命令出错，或命令都已成功执行，但未执行 boot。此时返回启动菜单。
                goto restart1;
          }
          
          if (grub_timeout > 0)
          {
              time1 = getrtsecs();
              if ( time1 != time2  &&  time1 != 0xFF )
              {
                  time2 = time1;
                  grub_timeout--;
                  countdown (1);
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
        heap = (char *) (MENU_BUF);
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
        reset ();
        console_cls ();

        grub_timeout = *(unsigned char*)(0x8000+496); //倒计时
        if (grub_timeout > 0) grub_timeout++; //若无此步，倒计时会少显示一秒。
        grub_current_entryno = *(unsigned char*)(0x8000+497); //默认启动项
        top_end_color = *(unsigned char*)(0x8000+498); //读取颜色
        menu_color = *(unsigned char*)(0x8000+499);

        get_all_entries(); //将所有启动项起始地址存入 all_entries[] ，同时数出 num_entries 。

        if (! num_entries) enter_cmdline (heap);
        else run_menu ();
    }
}

