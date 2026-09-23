#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

  typedef struct{                               /* 结构体保存文件 */
        char **lines;
        int count;
        int cap;                                  /* 数组容量 */
   }Buffer;

  void Buffer_init(Buffer *b){        		          /* Buffer初始化 */
        b->lines = malloc(b->cap * sizeof(char*));
        b->count = 0;
        b->cap = 30;
    }

  void 	Buffer_lineadd(Buffer *b,const char *line){		/* 行数超出，容量增加 */
	if(b->count >= b->cap){
	  b->cap =* 2;
	  b->lines = realloc(b->lines, b->cap * sizeof(char*));
        }
	b->lines[b->count] = malloc(strlen(line) + 1);
	strcpy(b->lines[b->count], line);
	b->count++;
    }


  void Buffer_read(Buffer *b,const char* filename){
        FILE *fp = fopen(filename, "r")              /* 读取文件 */
        char buf[1024];                              /* 暂存读取到的每一行 */
        int len;
        if(fp == NULL){
       	  return 1;
        }

        while(fgets(buf, sizeof(buf), fp) != NULL){
		len = strlen(buf);
		while(len > 0 && (buf[len -1] == '\n' || buf[len -1] == 'r')){
			buf[len-1] = '\0';
			len--;
		}
		Buffer_lineadd(b,buf);
        }

	fclose(fp);

	if(b->count == 0){
	  Buffer_lineadd(b, "");
	}
	return 0;
     }

  void Buffer_free(Buffer *b){				/* 释放Buffer */
	int i;
	for(i = 0; i<b->row; i++){
	  free(b->lines[i]);
     }

  int main(int argc, char **argv) {
	Buffer buf;
	if(argc < 2){           			 /* 检查文件输入格式是否正确 */
	  printf("错误，正确的输入格式为：.dedit <filename>\n");
	  return 1;
        }

	Buffer_init(&buf);
	if(Buffer_read(&buf, argv[1]) != 0){
	  printf("无法打开文件:%s\n",argv[1]);
	  return 1;
	}

	int row = 0, col = 0;        /* 当前光标位置：行 y、列 x */
        int ch;
	int top = 0;		 /* 决定屏幕第一行显示文件中哪一行 */
	int i;

        initscr();               /* 初始化 ncurses，接管终端 */
        raw();                   /* 程序读取Ctrl-Q */
        cbreak;
        keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

        while(1){
	  if(row < top){		/* 保证光标始终在屏幕内 */
	    top = row;
	  }
	  if(row >= top + LINES){
	    top = row - LINES + 1;
	  }

	  erase();			/* 重绘屏幕 */
	  for(i = 0; i < LINES && top + i < buf.count; i++){
	    mvaddnstr(i, 0, buf.lines[top + 1], COLS);
          }
	  move(row - top, col);
	  refresh();

	  ch = getch();
	  if(ch == 0x11) break;
          if(ch == KEY_UP && row > 0){
              row--;
          }else if(ch == KEY_DOWN && row < buf.count - 1){
              row++;
          }else if (ch == KEY_LEFT && col > 0){
              col--;
          }else if(ch == KEY_RIGHT && col < strlen(buf.lines[row])){
              col++;
          }else if (ch == KEY_RESIZE){		/* 缩放窗口 */
          }
        }

	endwin();
	Butter_free(&buf);
	return 0;
     }
