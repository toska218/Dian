#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

  typedef struct{                               /* 结构体保存文件 */
        char **lines;
        int row;
        int cap;                                  /* 数组容量 */
   }Buffer;

  void Buffer_init(Buffer *b){        		          /* Buffer初始化 */
        b->lines = malloc(b->cap * sizeof(char*));
        b->row = 0;
        b->cap = 30;
    }

  void 	Buffer_lineadd(Buffer *b,const char *line){		/* 行数超出，容量增加 */
	if(b->row >= b->cap){
	  b->cap =* 2;
	  b->lines = realloc(b->lines, b->cap * sizeof(char*));
        }
	b->lines[b->row] = malloc(strlen(line) + 1);
	strcpy(b->lines[b->row], line);
	b->row++;
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

	if(b->row == 0){
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

	int y = 0, x = 0;        /* 当前光标位置：行 y、列 x */
        int ch;
	int top = 0;		 /* 决定屏幕第一行显示文件中哪一行 */

        initscr();               /* 初始化 ncurses，接管终端 */
        raw();                   /* 程序读取Ctrl-Q */
        cbreak;
        keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

        while ((ch = getch()) != 0x11) {   /* Ctrl-Q */
		int line_len;
		int i;

		if(y < top){	 	/* 保证光标所在行在屏幕内 */
		  top = y;
		}
		if(y >= top + LINES){
		  top = y - LINES + 1;
		}

		erase();
		for(i = 0; i < LINES && top + i < buf.row; i++){
		  mvaddnstr(i, 0, buf.lines[top + i], COLS);
		}

		move(y - top, x)

                switch (ch) {
                  case KEY_UP:    if (y > 0)          y--; break;
                  case KEY_DOWN:  if (y < LINES - 1)  y++; break;
                  case KEY_LEFT:  if (x > 0)          x--; break;
                  case KEY_RIGHT: if (x < COLS - 1)   x++; break;
                  default:                            /* 打印字符 */
                    if (ch >= 32 && ch <= 126) {
                       mvaddch(y, x, ch);          /* 在当前位置显示字符 */
                       if (x < COLS - 1) x++;      /* 光标右移一位 */
                    }
                     break;
                }

            move(y, x);    /* 把光标放到更新后的位置 */
            refresh();     /* 刷新屏幕 */
        }

    endwin();          /* 退出 ncurses，恢复终端 */
    return 0;
}
