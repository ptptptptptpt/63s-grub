/* char_io.c - basic console input and output */
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
#include <term.h>
#include <stdarg.h>


#ifndef STAGE1_5


static int max_lines = 24;
#endif

void
print_error (void)
{
  if (errnum > ERR_NONE && errnum < MAX_ERR_NUM)
#ifndef STAGE1_5
    /* printf("\7\n %s\n", err_list[errnum]); */
    printf ("\nError %u: %s\n", errnum, err_list[errnum]);
#else /* STAGE1_5 */
    printf ("Error %u\n", errnum);
#endif /* STAGE1_5 */
}

char *
convert_to_ascii (char *buf, int c, unsigned long num)
{
  unsigned long mult = 10;
  char *ptr = buf;

#ifndef STAGE1_5
  if (c == 'x' || c == 'X')
    mult = 16;

  if ((num & 0x80000000uL) && c == 'd')
    {
      num = (~num) + 1;
      *(ptr++) = '-';
      buf++;
    }
#endif

  do
    {
      int dig = num % mult;
      *(ptr++) = ((dig > 9) ? dig + 'a' - 10 : '0' + dig);
    }
  while (num /= mult);

  /* reorder to correct direction!! */
  {
    char *ptr1 = ptr - 1;
    char *ptr2 = buf;
    while (ptr1 > ptr2)
      {
	int tmp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = tmp;
	ptr1--;
	ptr2++;
      }
  }

  return ptr;
}

void
grub_putstr (const char *str)
{
  while (*str)
    grub_putchar (*str++);
}

void
grub_printf (const char *format,...)
{
  va_list dataptr;
  char c, str[16];

  va_start(dataptr, format);

  while ((c = *(format++)) != 0)
    {
      if (c != '%')
	grub_putchar (c);
      else
	switch (c = *(format++))
	  {
#ifndef STAGE1_5
	  case 'd':
	  case 'x':
	  case 'X':
#endif
	  case 'u':
	    *convert_to_ascii (str, c, va_arg(dataptr, unsigned long)) = 0;
	    grub_putstr (str);
	    break;

#ifndef STAGE1_5
	  case 'c':
	    grub_putchar (va_arg(dataptr, int) & 0xff);
	    break;

	  case 's':
	    grub_putstr (va_arg(dataptr, char *));
	    break;
#endif
	  }
    }
  va_end(dataptr);
}

#ifndef STAGE1_5
int
grub_sprintf (char *buffer, const char *format, ...)
{
  /* XXX hohmuth
     ugly hack -- should unify with printf() */
  va_list dataptr;
  char c, *ptr, str[16];
  char *bp = buffer;

  va_start(dataptr, format);

  while ((c = *format++) != 0)
    {
      if (c != '%')
	*bp++ = c; /* putchar(c); */
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii (str, c, va_arg(dataptr, unsigned long)) = 0;

	    ptr = str;

	    while (*ptr)
	      *bp++ = *(ptr++); /* putchar(*(ptr++)); */
	    break;

	  case 'c': *bp++ = (va_arg(dataptr, int) & 0xff);
	    /* putchar((*(dataptr++))&0xff); */
	    break;

	  case 's':
	    ptr = va_arg(dataptr, char *);

	    while ((c = *ptr++) != 0)
	      *bp++ = c; /* putchar(c); */
	    break;
	  }
    }

  va_end(dataptr);
  *bp = 0;
  return bp - buffer;
}



/* The number of the history entries.  */
static int num_history = 0;

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
static char *
get_history (int no)
{
  if (no < 0 || no >= num_history)
    return 0;

  return (char *) HISTORY_BUF + MAX_CMDLINE * no;
}

/* Add CMDLINE to the history buffer.  */
static void
add_history (const char *cmdline, int no)
{
  grub_memmove ((char *) HISTORY_BUF + MAX_CMDLINE * (no + 1),
		(char *) HISTORY_BUF + MAX_CMDLINE * no,
		MAX_CMDLINE * (num_history - no));
  grub_strcpy ((char *) HISTORY_BUF + MAX_CMDLINE * no, cmdline);
  if (num_history < HISTORY_SIZE)
    num_history++;
}

static int
real_get_cmdline (char *prompt, char *cmdline, int maxlen,
		  int echo_char, int readline)
{
  /* This is a rather complicated function. So explain the concept.
     
     A command-line consists of ``section''s. A section is a part of the
     line which may be displayed on the screen, but a section is never
     displayed with another section simultaneously.

     Each section is basically 77 or less characters, but the exception
     is the first section, which is 78 or less characters, because the
     starting point is special. See below.

     The first section contains a prompt and a command-line (or the
     first part of a command-line when it is too long to be fit in the
     screen). So, in the first section, the number of command-line
     characters displayed is 78 minus the length of the prompt (or
     less). If the command-line has more characters, `>' is put at the
     position 78 (zero-origin), to inform the user of the hidden
     characters.

     Other sections always have `<' at the first position, since there
     is absolutely a section before each section. If there is a section
     after another section, this section consists of 77 characters and
     `>' at the last position. The last section has 77 or less
     characters and doesn't have `>'.

     Each section other than the last shares some characters with the
     previous section. This region is called ``margin''. If the cursor
     is put at the magin which is shared by the first section and the
     second, the first section is displayed. Otherwise, a displayed
     section is switched to another section, only if the cursor is put
     outside that section.  */

  /* XXX: These should be defined in shared.h, but I leave these here,
     until this code is freezed.  */
#define CMDLINE_MARGIN	10

  /* command-line limits */
  int cmdline_width = 78, col_start = 0;
  int xpos, lpos, c, section;
  /* The length of PROMPT.  */
  int plen;
  /* The length of the command-line.  */
  int llen;
  /* The index for the history.  */
  int history = -1;
  /* The working buffer for the command-line.  */
  char *buf = (char *) CMDLINE_BUF;
  /* The kill buffer.  */
  char *kill_buf = (char *) KILL_BUF;
  
  /* Nested function definitions for code simplicity.  */

  /* The forward declarations of nested functions are prefixed
     with `auto'.  */
  auto void cl_refresh (int full, int len);
  auto void cl_backward (int count);
  auto void cl_forward (int count);
  auto void cl_insert (const char *str);
  auto void cl_delete (int count);
  auto void cl_init (void);
  
  /* Move the cursor backward.  */
  void cl_backward (int count)
    {
      lpos -= count;
      
      /* If the cursor is in the first section, display the first section
	 instead of the second.  */
      if (section == 1 && plen + lpos < cmdline_width)
	cl_refresh (1, 0);
      else if (xpos - count < 1)
	cl_refresh (1, 0);
      else
	{
	  xpos -= count;
      console_gotoxy (xpos + col_start, console_getxy () & 0xFF);
	}
    }

  /* Move the cursor forward.  */
  void cl_forward (int count)
    {
      lpos += count;

      /* If the cursor goes outside, scroll the screen to the right.  */
      if (xpos + count >= cmdline_width)
	cl_refresh (1, 0);
      else
	{
	  xpos += count;

	    console_gotoxy (xpos + col_start, console_getxy () & 0xFF);
	}
    }

  /* Refresh the screen. If FULL is true, redraw the full line, otherwise,
     only LEN characters from LPOS.  */
  void cl_refresh (int full, int len)
    {
      int i;
      int start;
      int pos = xpos;
      
      if (full)
	{
	  /* Recompute the section number.  */
	  if (lpos + plen < cmdline_width)
	    section = 0;
	  else
	    section = ((lpos + plen - cmdline_width)
		       / (cmdline_width - 1 - CMDLINE_MARGIN) + 1);

	  /* From the start to the end.  */
	  len = cmdline_width;
	  pos = 0;
	  grub_putchar ('\r');

	  /* If SECTION is the first section, print the prompt, otherwise,
	     print `<'.  */
	  if (section == 0)
	    {
	      grub_printf ("%s", prompt);
	      len -= plen;
	      pos += plen;
	    }
	  else
	    {
	      grub_putchar ('<');
	      len--;
	      pos++;
	    }
	}

      /* Compute the index to start writing BUF and the resulting position
	 on the screen.  */
      if (section == 0)
	{
	  int offset = 0;
	  
	  if (! full)
	    offset = xpos - plen;
	  
	  start = 0;
	  xpos = lpos + plen;
	  start += offset;
	}
      else
	{
	  int offset = 0;
	  
	  if (! full)
	    offset = xpos - 1;
	  
	  start = ((section - 1) * (cmdline_width - 1 - CMDLINE_MARGIN)
		   + cmdline_width - plen - CMDLINE_MARGIN);
	  xpos = lpos + 1 - start;
	  start += offset;
	}

      /* Print BUF. If ECHO_CHAR is not zero, put it instead.  */
      for (i = start; i < start + len && i < llen; i++)
	{
	  if (! echo_char)
	    grub_putchar (buf[i]);
	  else
	    grub_putchar (echo_char);

	  pos++;
	}
      
      /* Fill up the rest of the line with spaces.  */
      for (; i < start + len; i++)
	{
	  grub_putchar (' ');
	  pos++;
	}
      
      /* If the cursor is at the last position, put `>' or a space,
	 depending on if there are more characters in BUF.  */
      if (pos == cmdline_width)
	{
	  if (start + len < llen)
	    grub_putchar ('>');
	  else
	    grub_putchar (' ');
	  
	  pos++;
	}
      
      /* Back to XPOS.  */
	console_gotoxy (xpos + col_start, console_getxy () & 0xFF);
    }

  /* Initialize the command-line.  */
  void cl_init (void)
    {
      /* Distinguish us from other lines and error messages!  */
      grub_putchar ('\n');

      /* Print full line and set position here.  */
      cl_refresh (1, 0);
    }

  /* Insert STR to BUF.  */
  void cl_insert (const char *str)
    {
      int l = grub_strlen (str);

      if (llen + l < maxlen)
	{
	  if (lpos == llen)
	    grub_memmove (buf + lpos, str, l + 1);
	  else
	    {
	      grub_memmove (buf + lpos + l, buf + lpos, llen - lpos + 1);
	      grub_memmove (buf + lpos, str, l);
	    }
	  
	  llen += l;
	  lpos += l;
	  if (xpos + l >= cmdline_width)
	    cl_refresh (1, 0);
	  else if (xpos + l + llen - lpos > cmdline_width)
	    cl_refresh (0, cmdline_width - xpos);
	  else
	    cl_refresh (0, l + llen - lpos);
	}
    }

  /* Delete COUNT characters in BUF.  */
  void cl_delete (int count)
    {
      grub_memmove (buf + lpos, buf + lpos + count, llen - count + 1);
      llen -= count;
      
      if (xpos + llen + count - lpos > cmdline_width)
	cl_refresh (0, cmdline_width - xpos);
      else
	cl_refresh (0, llen + count - lpos);
    }

  max_lines = 24;


  plen = grub_strlen (prompt);
  llen = grub_strlen (cmdline);

  if (maxlen > MAX_CMDLINE)
    {
      maxlen = MAX_CMDLINE;
      if (llen >= MAX_CMDLINE)
	{
	  llen = MAX_CMDLINE - 1;
	  cmdline[MAX_CMDLINE] = 0;
	}
    }
  lpos = llen;
  grub_strcpy (buf, cmdline);

  cl_init ();

  while ((c = ASCII_CHAR (console_getkey ())) != '\n' && c != '\r')
    {
      /* If READLINE is non-zero, handle readline-like key bindings.  */
      if (readline)
	{
	  switch (c)
	    {
	    case 9:		/* TAB lists completions */
	      {
		int i;
		/* POS points to the first space after a command.  */
		int pos = 0;
		int ret;
		char *completion_buffer = (char *) COMPLETION_BUF;
		int equal_pos = -1;
		int is_filename;

		/* Find the first word.  */
		while (buf[pos] == ' ')
		  pos++;
		while (buf[pos] && buf[pos] != '=' && buf[pos] != ' ')
		  pos++;

		is_filename = (lpos > pos);

		/* Find the position of the equal character after a
		   command, and replace it with a space.  */
		for (i = pos; buf[i] && buf[i] != ' '; i++)
		  if (buf[i] == '=')
		    {
		      equal_pos = i;
		      buf[i] = ' ';
		      break;
		    }

		/* Find the position of the first character in this
		   word.  */
		for (i = lpos; i > 0 && buf[i - 1] != ' '; i--)
		  ;

		/* Invalidate the cache, because the user may exchange
		   removable disks.  */
		buf_drive = -1;

		/* Copy this word to COMPLETION_BUFFER and do the
		   completion.  */
		grub_memmove (completion_buffer, buf + i, lpos - i);
		completion_buffer[lpos - i] = 0;
		ret = print_completions (is_filename, 1);
		errnum = ERR_NONE;

		if (ret >= 0)
		  {
		    /* Found, so insert COMPLETION_BUFFER.  */
		    cl_insert (completion_buffer + lpos - i);

		    if (ret > 0)
		      {
			/* There are more than one candidates, so print
			   the list.  */
			grub_putchar ('\n');
			print_completions (is_filename, 0);
			errnum = ERR_NONE;
		      }
		  }

		/* Restore the command-line.  */
		if (equal_pos >= 0)
		  buf[equal_pos] = '=';

		if (ret)
		  cl_init ();
	      }
	      break;
	    case 1:		/* C-a go to beginning of line */
	      cl_backward (lpos);
	      break;
	    case 5:		/* C-e go to end of line */
	      cl_forward (llen - lpos);
	      break;
	    case 6:		/* C-f forward one character */
	      if (lpos < llen)
		cl_forward (1);
	      break;
	    case 2:		/* C-b backward one character */
	      if (lpos > 0)
		cl_backward (1);
	      break;
	    case 21:		/* C-u kill to beginning of line */
	      if (lpos == 0)
		break;
	      /* Copy the string being deleted to KILL_BUF.  */
	      grub_memmove (kill_buf, buf, lpos);
	      kill_buf[lpos] = 0;
	      {
		/* XXX: Not very clever.  */
		
		int count = lpos;
		
		cl_backward (lpos);
		cl_delete (count);
	      }
	      break;
	    case 11:		/* C-k kill to end of line */
	      if (lpos == llen)
		break;
	      /* Copy the string being deleted to KILL_BUF.  */
	      grub_memmove (kill_buf, buf + lpos, llen - lpos + 1);
	      cl_delete (llen - lpos);
	      break;
	    case 25:		/* C-y yank the kill buffer */
	      cl_insert (kill_buf);
	      break;
	    case 16:		/* C-p fetch the previous command */
	      {
		char *p;

		if (history < 0)
		  /* Save the working buffer.  */
		  grub_strcpy (cmdline, buf);
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history++;
		p = get_history (history);
		if (! p)
		  {
		    history--;
		    break;
		  }

		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_refresh (1, 0);
	      }
	      break;
	    case 14:		/* C-n fetch the next command */
	      {
		char *p;

		if (history < 0)
		  {
		    break;
		  }
		else if (grub_strcmp (get_history (history), buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history (buf, history);

		history--;
		p = get_history (history);
		if (! p)
		  p = cmdline;

		grub_strcpy (buf, p);
		llen = grub_strlen (buf);
		lpos = llen;
		cl_refresh (1, 0);
	      }
	      break;
	    }
	}

      /* ESC, C-d and C-h are always handled. Actually C-d is not
	 functional if READLINE is zero, as the cursor cannot go
	 backward, but that's ok.  */
      switch (c)
	{
	case 27:	/* ESC immediately return 1 */
	  return 1;
	case 4:		/* C-d delete character under cursor */
	  if (lpos == llen)
	    break;
	  cl_delete (1);
	  break;
	case 8:		/* C-h backspace */
# ifdef GRUB_UTIL
	case 127:	/* also backspace */
# endif
	  if (lpos > 0)
	    {
	      cl_backward (1);
	      cl_delete (1);
	    }
	  break;
	default:		/* insert printable character into line */
	  if (c >= ' ' && c <= '~')
	    {
	      char str[2];

	      str[0] = c;
	      str[1] = 0;
	      cl_insert (str);
	    }
	}
    }

  grub_putchar ('\n');

  /* If ECHO_CHAR is NUL, remove the leading spaces.  */
  lpos = 0;
  if (! echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  /* Copy the working buffer to CMDLINE.  */
  grub_memmove (cmdline, buf + lpos, llen - lpos + 1);

  /* If the readline-like feature is turned on and CMDLINE is not
     empty, add it into the history list.  */
  if (readline && lpos < llen)
    add_history (cmdline, 0);

  return 0;
}

/* Don't use this with a MAXLEN greater than 1600 or so!  The problem
   is that GET_CMDLINE depends on the everything fitting on the screen
   at once.  So, the whole screen is about 2000 characters, minus the
   PROMPT, and space for error and status lines, etc.  MAXLEN must be
   at least 1, and PROMPT and CMDLINE must be valid strings (not NULL
   or zero-length).

   If ECHO_CHAR is nonzero, echo it instead of the typed character. */
int
get_cmdline (char *prompt, char *cmdline, int maxlen,
	     int echo_char, int readline)
{
  int old_cursor;
  int ret;

  old_cursor = console_setcursor (1);
  

  /* Complicated features are left to real_get_cmdline.  */
  ret = real_get_cmdline (prompt, cmdline, maxlen, echo_char, readline);
  console_setcursor (old_cursor);
  return ret;
}

int
safe_parse_maxint (char **str_ptr, int *myint_ptr)
{
  char *ptr = *str_ptr;
  int myint = 0;
  int mult = 10, found = 0;

  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      /* A bit tricky. This below makes use of the equivalence:
	 (A >= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      if (myint > ((MAXINT - digit) / mult))
	{
	  errnum = ERR_NUMBER_OVERFLOW;
	  return 0;
	}
      myint = (myint * mult) + digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = myint;

  return 1;
}
#endif /* STAGE1_5 */

#if !defined(STAGE1_5) || defined(FSYS_FAT)
int
grub_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return (c + ('a' - 'A'));

  return c;
}
#endif /* ! STAGE1_5 || FSYS_FAT */

int
grub_isspace (int c)
{
  switch (c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      return 1;
    default:
      break;
    }

  return 0;
}

#if !defined(STAGE1_5) || defined(FSYS_ISO9660)
int
grub_memcmp (const char *s1, const char *s2, int n)
{
  while (n)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1++;
      s2++;
      n--;
    }

  return 0;
}
#endif /* ! STAGE1_5 || FSYS_ISO9660 */


/* XXX: This below is an evil hack. Certainly, we should change the
   strategy to determine what should be defined and what shouldn't be
   defined for each image. For example, it would be better to create
   a static library supporting minimal standard C functions and link
   each image with the library. Complicated things should be left to
   computer, definitely. -okuji  */
#if !defined(STAGE1_5) || defined(FSYS_VSTAFS)
int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 || *s2)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1 ++;
      s2 ++;
    }

  return 0;
}
#endif /* ! STAGE1_5 || FSYS_VSTAFS */




/* Display an ASCII character.  */
void
grub_putchar (int c)
{
  if (c == '\n')
    grub_putchar ('\r');

  else if (c == '\t')
    {
      int n;
      
      n = 8 - ((console_getxy () >> 8) & 3);
      while (n--)
	grub_putchar (' ');
      
      return;
    }

  
  //grub_printf("\n");

  console_putchar (c);
  

}


int
substring (const char *s1, const char *s2)
{
  while (*s1 == *s2)
    {
      /* The strings match exactly. */
      if (! *(s1++))
	return 0;
      s2 ++;
    }

  /* S1 is a substring of S2. */
  if (*s1 == 0)
    return -1;

  /* S1 isn't a substring. */
  return 1;
}

#ifndef STAGE1_5



int
grub_strlen (const char *str)
{
  int len = 0;

  while (*str++)
    len++;

  return len;
}
#endif /* ! STAGE1_5 */

int
memcheck (unsigned long int addr, unsigned long int len)
{
#ifdef GRUB_UTIL
  auto unsigned long int start_addr (void);
  auto int unsigned long end_addr (void);
  
  auto unsigned long int start_addr (void)
    {
      int ret;
# if defined(HAVE_START_SYMBOL)
      asm volatile ("movl	$start, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_START_SYMBOL)
      asm volatile ("movl	$_start, %0" : "=a" (ret));
# endif
      return ret;
    }

  auto unsigned long int end_addr (void)
    {
      int ret;
# if defined(HAVE_END_SYMBOL)
      asm volatile ("movl	$end, %0" : "=a" (ret));
# elif defined(HAVE_USCORE_END_SYMBOL)
      asm volatile ("movl	$_end, %0" : "=a" (ret));
# endif
      return ret;
    }

  if (start_addr () <= addr && end_addr () > addr + len)
    return ! errnum;
#endif /* GRUB_UTIL */

  if ((addr < RAW_ADDR (0x1000)) )
//      || (addr < RAW_ADDR (0x100000)
//	  && RAW_ADDR (mbi.mem_lower * 1024) < (addr + len))
//      || (addr >= RAW_ADDR (0x100000)
//	  && RAW_ADDR (mbi.mem_upper * 1024) < ((addr - 0x100000) + len)))
    errnum = ERR_WONT_FIT;

  return ! errnum;
}



void *
grub_memmove (void *to, const void *from, int len)
{
   if (memcheck ((int) to, len))
     {
       /* This assembly code is stolen from
	  linux-2.2.2/include/asm-i386/string.h. This is not very fast
	  but compact.  */
       int d0, d1, d2;

       if (to < from)
	 {
	   asm volatile ("cld\n\t"
			 "rep\n\t"
			 "movsb"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),"1" (from),"2" (to)
			 : "memory");
	 }
       else
	 {
	   asm volatile ("std\n\t"
			 "rep\n\t"
			 "movsb\n\t"
			 "cld"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),
			 "1" (len - 1 + (const char *) from),
			 "2" (len - 1 + (char *) to)
			 : "memory");
	 }
     }

   return errnum ? NULL : to;
}

void *
grub_memset (void *start, int c, int len)
{
  char *p = start;

  if (memcheck ((int) start, len))
    {
      while (len -- > 0)
	*p ++ = c;
    }

  return errnum ? NULL : start;
}

#ifndef STAGE1_5
char *
grub_strcpy (char *dest, const char *src)
{
  grub_memmove (dest, src, grub_strlen (src) + 1);
  return dest;
}
#endif /* ! STAGE1_5 */

#ifndef GRUB_UTIL
# undef memcpy
/* GCC emits references to memcpy() for struct copies etc.  */
void *memcpy (void *dest, const void *src, int n)  __attribute__ ((alias ("grub_memmove")));
#endif
