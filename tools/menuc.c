#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#define CFG_FILENAME "63s_grub_cfg"

struct cfg_sector
{
  int cfg_timeout;	//超时(秒)
  int cfg_default_no;   //默认菜单序号
  int cfg_bar_color;	//上下栏颜色
  int cfg_menu_color;   //菜单颜色
  int menuitems[6];	//菜单指针
};

static void
usage ()
{
	printf("Usage: 63s-menuc <filename> \n");
}

static void
checksize(FILE *fout, FILE *fin) {
    if (ftell(fout)>512) {
      printf("Compiled size greater than 512 bytes!\n");
      fclose(fout);
      fclose(fin);
      unlink(CFG_FILENAME);
      exit(1);
    }
}

int
is_whitespace(char c) {
  if(c == ' ') return 1;
  if(c == '\t') return 1;
  if(c == '\r' || c == '\n') return 1;
  if(c == '\0') return 1;
  return 0;
}

char *
trimstr(char *str) {
  char *src = str;
  char *dist = str;
  
  for(;*src!='\0' && is_whitespace(*src);++src); 

  if(*src=='\0') {
    dist[0] = '\0';
    return str;
  }

  for(;*src!='\0';++src,++dist)
    *dist = *src;

  while(is_whitespace(*src))
    --src, --dist;
  dist[1]='\0';

  return str;
}

int
readline(FILE *fin, char *buf, size_t size) {
  int len;

  if(fgets(buf, size, fin)==NULL)
    return -1;
  trimstr(buf);
  len = strlen(buf);
  bzero(buf+len,size-len);

  printf("read line (%d): %s\n", len, buf);
  return len;
}

void
overflow() {
  puts("size overflow\n");
  exit(2);
}

int
fill_menuitem(int *rpos, int *wpos, char *rbuf, char *wbuf) {
  int count, i;
  int wpos1;
  for(count = 0;count+*rpos<512 && rbuf[(count+*rpos)*512]!='\0';++count);

  if(*wpos+(count+1)*sizeof(int)>=512)
    overflow();

  wpos1 = *wpos;
  *wpos = *wpos + sizeof(int) * (count+1);
  for(i=0;i<count;++i){
    char *rstr = rbuf+(i+*rpos)*512;
    int l = strlen(rstr);
    if(l+*wpos+1>=512)
      overflow();
    strncpy(wbuf+*wpos, rbuf+(i+*rpos)*512, l+1);
    ((int *)(wbuf+wpos1))[i] = *wpos+0x8000;
    *wpos += l+1;
  }
  ++(*wpos);
  *rpos += count+1;
}

int
main (int argc, char *argv[]) {
  FILE *fin, *fout;
  long i,j;
  char rbuf[512*512];
  char wbuf[512];
  struct cfg_sector *cfg = (struct cfg_sector *)wbuf;
  int c=512;
  int current = sizeof(*cfg);
  int count;
  
  bzero(rbuf, sizeof(rbuf));
  bzero(wbuf, sizeof(wbuf));
  int title = 0;

  if(argc!=2)
	usage();
  fin = fopen(argv[1], "r");
  fout = fopen(CFG_FILENAME, "w+");
  
  fscanf(fin, "%d\n%d\n%d\n%d\n", 
	&cfg->cfg_timeout, &cfg->cfg_default_no,
	&cfg->cfg_bar_color, &cfg->cfg_menu_color);

  for(i=0;i<512;++i){
    j = readline(fin, rbuf+i*512, 512-4);
    if(j<0) break;
  }

  count = 0;
  for(j=0;j<i && j<6;++j){
    cfg->menuitems[j]=current + 0x8000;
    fill_menuitem(&count, &current, rbuf, wbuf);
  }

  fwrite(wbuf, sizeof(wbuf), 1, fout);
  fclose(fout);
  fclose(fin);
}
