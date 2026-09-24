#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

  typedef struct{                               /* 结构体保存文件 */
        char **lines;
        int count;
        int cap;                                  /* 数组容量 */
   }Buffer;

  void Buffer_init(Buffer *b){        		          /* Buffer初始化 */
	b->cap = 30;
        b->lines = malloc(b->cap * sizeof(char*));
        b->count = 0;
    }

  void 	Buffer_lineadd(Buffer *b,const char *line){		/* 行数超出，容量增加 */
	if(b->count >= b->cap){
	  b->cap *= 2;
	  b->lines = realloc(b->lines, b->cap * sizeof(char*));
        }
	b->lines[b->count] = malloc(strlen(line) + 1);
	strcpy(b->lines[b->count], line);
	b->count++;
    }

  void Buffer_lineinsert(Buffer *b, int row, const char *text){	/* 在第row行插入新行 */
	int i;
	if(b->count >= b->cap){
          b->cap *= 2;
          b->lines = realloc(b->lines, b->cap * sizeof(char *));
        }
	for (i = b->count; i > row; i--){
        b->lines[i] = b->lines[i - 1];
        }
	b->lines[row] = malloc(strlen(text) + 1);
	strcpy(b->lines[row], text);
	b->count++;
    }

  void Buffer_linedel(Buffer *b, int row){			/* 删除第row行 */
	int i;
	free(b->lines[row]);
	for(i = row; i < b->count - 1; i++){
        b->lines[i] = b->lines[i + 1];
        }
	b->count--;
    }

  void Buffer_charinsert(Buffer *b, int row, int col, char ch){		/* 在(row,col)位置插入一个字符 */
	int len = strlen(b->lines[row]);
	b->lines[row] = realloc(b->lines[row], len + 2);
	memmove(b->lines[row] + col + 1, b->lines[row] + col, len - col + 1);
	b->lines[row][col] = ch;
    }

  void Buffer_chardel(Buffer *b, int row, int col){	 	/* 在(row,col)位置删除一个字符 */
	int len = strlen(b->lines[row]);
	memmove(b->lines[row] + col, b->lines[row] + col + 1, len - col);
    }

  void Buffer_line_enter(Buffer *b, int row, int col){		/* 按回车加一行 */
	char *right = malloc(strlen(b->lines[row]) - col + 1);
	strcpy(right, b->lines[row] + col);
	b->lines[row][col] = '\0';
	b->lines[row] = realloc(b->lines[row], col + 1);
	Buffer_lineinsert(b, row + 1, right);
	free(right);
    }

  void Buffer_headdel(Buffer *b, int row){	    /* 将第row行合并到上一行末尾 */
	int pre_len = strlen(b->lines[row - 1]);
	int cur_len = strlen(b->lines[row]);
	b->lines[row - 1] = realloc(b->lines[row - 1], pre_len + cur_len + 1);
	strcpy(b->lines[row - 1] + pre_len, b->lines[row]);
	Buffer_linedel(b, row);
    }

  void Buffer_taildel(Buffer *b, int row) {	    /* 将第row+1行合并到下一行末尾 */
	int cur_len = strlen(b->lines[row]);
	int next_len = strlen(b->lines[row + 1]);
	b->lines[row] = realloc(b->lines[row], cur_len + next_len + 1);
	strcpy(b->lines[row] + cur_len, b->lines[row + 1]);
	Buffer_linedel(b, row + 1);
    }

  int Buffer_save(Buffer *b, const char *filename){	/* 保存文件 */
	FILE *fp = fopen(filename, "w");
	int i;
	if(fp == NULL){
	 return -1;
	 }
	for(i = 0; i < b->count; i++){
	 fprintf(fp, "%s\n",b->lines[i]);
	 }
	fclose(fp);
	return 0;
    }

  int Buffer_read(Buffer *b,const char* filename){
        FILE *fp = fopen(filename, "r");              /* 读取文件 */
        char buf[1024];                              /* 暂存读取到的每一行 */
        int len;
        if(fp == NULL){
       	  return 1;
        }

        while(fgets(buf, sizeof(buf), fp) != NULL){
		len = strlen(buf);
		while(len > 0 && (buf[len -1] == '\n' || buf[len -1] == '\r')){
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
	for(i = 0; i<b->count; i++){
	  free(b->lines[i]);
	}
	free(b->lines);
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
	int left = 0;		 /* 决定屏幕第一列显示文件中哪一列 */
	int i;
	int modified = 0;	 /* 值为1时未保存 */
	char *filename = argv[1];
	int confirm_quit = 0;	 /* 是否正在等待确认退出 */

	setlocale(LC_ALL, "");

        initscr();               /* 初始化 ncurses，接管终端 */
        raw();                   /* 程序读取Ctrl-Q */
	noecho();
        keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

        while(1){
	  if(row < top){		/* 保证光标始终在屏幕内 */
	    top = row;
	  }
	  if(row >= top + LINES - 1){
	    top = row - (LINES - 1) + 1;
	  }
	  if(col < left){
	    left = col;
	  }
	  if(col >= left + COLS){
	    left = col - COLS + 1;
	  }

	  erase();			/* 重绘屏幕 */
	  for(i = 0; i < LINES-1 && top + i < buf.count; i++){
	    int line_len = strlen(buf.lines[top + i]);

	    if(left < line_len){
	      mvaddnstr(i, 0, buf.lines[top + i] + left, COLS);
           }else{
	  mvaddnstr(i, 0, "", 0);
	   }
	  }

	  if (confirm_quit){			/* 未保存退出 */
          mvprintw(LINES - 1, 0, " 文件未保存！再按 Ctrl-Q 强制退出，其他键取消 ");
          }else{
          mvprintw(LINES - 1, 0, " %s | %s | %d:%d ", filename, modified ? "Modified" : "Saved", row + 1 ,col + 1);
          }
	  clrtoeol();

	  move(row - top, col - left);
	  refresh();

	  ch = getch();
	  if(ch == 0x11){			/* 判断退出时是否保存 */
	    if(modified == 0 || confirm_quit == 1){
	      break;
	    }
	   confirm_quit = 1;
	   continue;
	  }

	  if(confirm_quit == 1){		/* 按其他任意键取消退出 */
	      confirm_quit = 0;
	      continue;
	  }

	  if(ch == 0x13){			/*  Ctrl+S保存 */
	    if(Buffer_save(&buf, filename) == 0){
	      modified = 0;
	    }
	    continue;
	  }

          if(ch == KEY_UP && row > 0){
              row--;
          }else if(ch == KEY_DOWN && row < buf.count - 1){
              row++;
          }else if (ch == KEY_LEFT && col > 0){
              col--;
          }else if(ch == KEY_RIGHT && col < strlen(buf.lines[row])){
              col++;
          }else if(ch == KEY_RESIZE){		/* 缩放窗口 */
          }else if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){
	       Buffer_line_enter(&buf, row, col);
	       row++;
	       col = 0;
	       modified = 1;
	  }else if(ch == KEY_BACKSPACE || ch == 127){		/* BACKSPACE键 */
	        if(col > 0){
	         Buffer_chardel(&buf, row, col - 1);
	         col--;
		 modified = 1;
		}else if(row > 0){
		 int pre_len = strlen(buf.lines[row - 1]);
		 Buffer_headdel(&buf, row);
		 row--;
		 col = pre_len;
		 modified = 1;
		}
	  }else if(ch == KEY_DC){			/* DELETE键位 */
	        int len = strlen(buf.lines[row]);
	        if(col < len) {
                 Buffer_chardel(&buf, row, col);
		 modified = 1;
                }else if(row < buf.count - 1) {
                  Buffer_taildel(&buf, row);
		  modified = 1;
		}
	  }else if(ch >= 32 && ch <= 126){		/* 输入可打印字符 */
		Buffer_charinsert(&buf, row, col, ch);
		col++;
		modified = 1;
	  }

	if(col > strlen(buf.lines[row])){	/* 若此行字数太少，则缩减光标活动范围 */
	   col = strlen(buf.lines[row]);
	  }
        }

	endwin();
	Buffer_free(&buf);
	return 0;
    }
