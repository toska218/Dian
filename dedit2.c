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
	b->cap = 1024;
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

  int Find_prev(Buffer *b, const char *term, int start_row, int start_col, int *found_row, int *found_col){		/* 向前找term */
	int i, j;
	int term_len = strlen(term);
	if(term_len == 0)
	  return 0;
	for(i = start_row; i >= 0; i--){
          int line_len = strlen(b->lines[i]);
          int limit = (i == start_row) ? start_col : line_len;
	  if(limit > line_len)
	    limit = line_len;
	  for(j = limit - term_len; j >= 0; j--){
            if(strncmp(b->lines[i] + j, term, term_len) == 0){
              *found_row = i;
              *found_col = j;
              return 1;
            }
          }
        }
        return 0;
    }

  int Find_next(Buffer *b, const char *term, int start_row, int start_col, int *found_row, int *found_col){		/* 向后找term */
	int i;
	int c = start_col;
	if(term[0] == '\0')
	return 0;
	for(i = start_row; i < b->count; i++){
          char *p;
          int line_len = strlen(b->lines[i]);
	  if(i > start_row)
	    c = 0;
          if(c <= line_len){
            p = strstr(b->lines[i] + c, term);
            if(p != NULL){
              *found_row = i;
              *found_col = p - b->lines[i];
              return 1;
            }
          }
        }
        return 0;
    }

  void Buffer_replace(Buffer *b, int row, int col, int old_len, const char *repl) {	/* 把第 row 行从 col 开始、长度为 old_len 的内容替换成 repl */
	int line_len = strlen(b->lines[row]);
	int repl_len = strlen(repl);
	int new_len = line_len - old_len + repl_len;
	char *newline = malloc(new_len + 1);
	memcpy(newline, b->lines[row], col);                			       /* 前面部分 */
	memcpy(newline + col, repl, repl_len);               			       /* 替换内容 */
	strcpy(newline + col + repl_len, b->lines[row] + col + old_len);               /* 后面部分 */
	free(b->lines[row]);
	b->lines[row] = newline;
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

  char *clipboard = NULL;

  void Clipboard_set(const char *text){
    if(clipboard != NULL){
        free(clipboard);
        clipboard = NULL;
    }
    if(text == NULL)
        return;
    clipboard = malloc(strlen(text) + 1);
    strcpy(clipboard, text);
  }
  void Sel_normalize(int *r1, int *c1, int *r2, int *c2){       /* 把选区规范成(r1,c1)在(r2,c2)之前 */
     if(*r1 > *r2 || (*r1 == *r2 && *c1 > *c2)){
        int tr = *r1, tc = *c1;
        *r1 = *r2; *c1 = *c2;
        *r2 = tr; *c2 = tc;
    }
  }

  char *Selection_get(Buffer *b, int r1, int c1, int r2, int c2){       /* 取出选区文本，跨行用\n连接 */
    char *out, *p;
    int total, r;

    Sel_normalize(&r1, &c1, &r2, &c2);

    if(r1 == r2){
        out = malloc(c2 - c1 + 1);
        memcpy(out, b->lines[r1] + c1, c2 - c1);
        out[c2 - c1] = '\0';
        return out;
    }

    total = strlen(b->lines[r1]) - c1 + 1;
    for(r = r1 + 1; r < r2; r++){
        total += strlen(b->lines[r]) + 1;
    }
    total += c2;

    out = malloc(total + 1);
    p = out;

    strcpy(p, b->lines[r1] + c1);
    p += strlen(b->lines[r1]) - c1;
    *p++ = '\n';

    for(r = r1 + 1; r < r2; r++){
        strcpy(p, b->lines[r]);
        p += strlen(b->lines[r]);
        *p++ = '\n';
    }

    memcpy(p, b->lines[r2], c2);
    p += c2;
    *p = '\0';
    return out;
  }

  void Buffer_insert_text(Buffer *b, int row, int col, const char *text){       /* 在(row, col)插入一串不含换行的文字 */
      int old_len = strlen(b->lines[row]);
      int add_len = strlen(text);
      b->lines[row] = realloc(b->lines[row], old_len + add_len + 1);
      memmove(b->lines[row] + col + add_len, b->lines[row] + col, old_len - col + 1);
      memcpy(b->lines[row] + col, text, add_len);
  }

  void Buffer_delete_range(Buffer *b, int r1, int c1, int r2, int c2){          /* 删除选区 */
      int k;

      Sel_normalize(&r1, &c1, &r2, &c2);

      if(r1 == r2){
        int len = strlen(b->lines[r1]);
        memmove(b->lines[r1] + c1, b->lines[r1] + c2, len - c2 + 1);
        return;
      }

    {
        int head_len = c1;
        int tail_len = strlen(b->lines[r2]) - c2;
        char *new_first = malloc(head_len + tail_len + 1);

        memcpy(new_first, b->lines[r1], head_len);
        strcpy(new_first + head_len, b->lines[r2] + c2);

        free(b->lines[r1]);
        b->lines[r1] = new_first;
    }

    for(k = r1 + 1; k <= r2; k++){
        Buffer_linedel(b, r1 + 1);
    }
  }

  void Buffer_paste(Buffer *b, int *row, int *col, const char *text) {
       int n = 0, i;
       char **parts;
       const char *p, *start;
       if(text == NULL || text[0] == '\0')
         return;
       for(p = text; *p; p++){
        if(*p == '\n')
          n++;
        }
        if(n == 0){
        Buffer_insert_text(b, *row, *col, text);
        *col += strlen(text);
        return;
        }

       parts = malloc((n + 1) * sizeof(char *));
       start = text;
       for(i = 0; i <= n; i++){
        const char *nl = strchr(start, '\n');
        int len = nl ? (int)(nl - start) : (int)strlen(start);
        parts[i] = malloc(len + 1);
        memcpy(parts[i], start, len);
        parts[i][len] = '\0';
        start = nl ? nl + 1 : start + len;
       }

    {
        int prefix_len = *col;
        char *suffix = malloc(strlen(b->lines[*row]) - prefix_len + 1);
        strcpy(suffix, b->lines[*row] + prefix_len);

        {
            char *newline = malloc(prefix_len + strlen(parts[0]) + 1);
            memcpy(newline, b->lines[*row], prefix_len);
            strcpy(newline + prefix_len, parts[0]);
            free(b->lines[*row]);
            b->lines[*row] = newline;
        }

        for(i = 1; i <= n; i++){
            char *line;
            if(i == n){
              line = malloc(strlen(parts[i]) + strlen(suffix) + 1);
              strcpy(line, parts[i]);
              strcat(line, suffix);
            }else{
              line = malloc(strlen(parts[i]) + 1);
              strcpy(line, parts[i]);
            }
            Buffer_lineinsert(b, *row + i, line);
            free(line);
        }

        *row += n;
        *col = strlen(parts[n]);
        free(suffix);
    }

    for (i = 0; i <= n; i++) free(parts[i]);
    free(parts);
  }
  typedef struct{
    Buffer buf;
    int row;
    int col;
   }Snapshot;

   Snapshot *snapshot_create(Buffer *b, int row, int col){
      Snapshot *s = malloc(sizeof(Snapshot));
      int i;
      Buffer_init(&s->buf);
      for (i = 0; i < b->count; i++){
        Buffer_lineadd(&s->buf, b->lines[i]);
      }
      s->row = row;
      s->col = col;
      return s;
   }

  void Snapshot_free(Snapshot *s){
    Buffer_free(&s->buf);
    free(s);
  }

  void Snapshot_restore(Buffer *b, Snapshot *s, int *row, int *col){
    int i;
    Buffer_free(b);
    Buffer_init(b);
    for(i = 0; i < s->buf.count; i++){
        Buffer_lineadd(b, s->buf.lines[i]);
    }
    *row = s->row;
    *col = s->col;
  }

  #define MAX_UNDO 1000
  Snapshot *undo_stack[MAX_UNDO];
  int undo_top = 0;
  Snapshot *redo_stack[MAX_UNDO];
  int redo_top = 0;

  void Undo_push(Buffer *b, int row, int col){
    if(undo_top >= MAX_UNDO)
      return;
    undo_stack[undo_top++] = snapshot_create(b, row, col);
  }

  void Undo_clear_redo(void){
    while(redo_top > 0){
        Snapshot_free(redo_stack[--redo_top]);
    }
  }

  void Undo_do(Buffer *b, int *row, int *col){
    if(undo_top == 0)
       return;

    redo_stack[redo_top++] = snapshot_create(b, *row, *col);

    Snapshot *s = undo_stack[--undo_top];
    Snapshot_restore(b, s, row, col);
    Snapshot_free(s);
    }

  void Redo_do(Buffer *b, int *row, int *col){
    if(redo_top == 0)
      return;

    undo_stack[undo_top++] = snapshot_create(b, *row, *col);

    Snapshot *s = redo_stack[--redo_top];
    Snapshot_restore(b, s, row, col);
    Snapshot_free(s);
  }



			/* 主函数 */

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
	int search_mode = 0;
	char search_buf[256] = "";
	int search_len = 0;
	int match_row = -1, match_col = -1;
	int search_start_row = 0, search_start_col = 0;
	int screen_h;
	int sr1 = 0, sc1 = 0, sr2 = 0, sc2 = 0;
	int replace_mode = 0;
	int replace_phase = 0;       /* 0=输入查找词 1=输入替换词 2=逐个确认 */
	char replace_search[256] = "";
	char replace_repl[256] = "";
	int replace_search_len = 0;
	int replace_repl_len = 0;
	int rep_match_row = -1, rep_match_col = -1;
	int rep_start_row = 0, rep_start_col = 0;
	int sel_active = 0;
        int sel_anchor_row = 0, sel_anchor_col = 0;

	setlocale(LC_ALL, "");

        initscr();               /* 初始化 ncurses，接管终端 */
        raw();                   /* 程序读取Ctrl-Q */
	noecho();
        keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

        while(1){
	  screen_h = (search_mode || replace_mode) ? LINES -2 : LINES - 1;

	  if(row < top){		/* 保证光标始终在屏幕内 */
	    top = row;
	  }
	  if(row >= top + screen_h){
	    top = row - screen_h + 1;
	  }
	  if(col < left){
	    left = col;
	  }
	  if(col >= left + COLS){
	    left = col - COLS + 1;
	  }

	  sr1 = 0; sc1 = 0; sr2 = 0; sc2 = 0;
          if(sel_active){
            sr1 = sel_anchor_row; sc1 = sel_anchor_col;
            sr2 = row; sc2 = col;
            Sel_normalize(&sr1, &sc1, &sr2, &sc2);
          }

	  erase();			/* 重绘屏幕 */
	  for(i = 0; i < screen_h && top + i < buf.count; i++){
	    int line_len = strlen(buf.lines[top + i]);

	    if(left < line_len){
	      mvaddnstr(i, 0, buf.lines[top + i] + left, COLS);
            }else{
	  mvaddnstr(i, 0, "", 0);
	    }

	  if(search_mode && match_row == top + i){			/* 高亮匹配项 */
            if(match_col >= left && match_col - left < COLS){
              attron(A_REVERSE);
              mvaddnstr(i, match_col - left, search_buf, search_len);
              attroff(A_REVERSE);
            }
          }
	 if(replace_mode && replace_phase == 2 && rep_match_row == top + i){
    	    if(rep_match_col >= left && rep_match_col - left < COLS){
              attron(A_REVERSE);
              mvaddnstr(i, rep_match_col - left, replace_search, replace_search_len);
              attroff(A_REVERSE);
             }
          }
	 if(sel_active && top + i >= sr1 && top + i <= sr2){
           int li = top + i;
           int hs = (li == sr1) ? sc1 : 0;
           int he = (li == sr2) ? sc2 : strlen(buf.lines[li]);
	   if(he > hs){
             int x = hs - left;
             int w = he - hs;
	     if(x < 0){
               w += x;
	       x = 0;
             }
             if(x < COLS && w > 0){
               attron(A_REVERSE);
               mvaddnstr(i, x, buf.lines[li] + hs, w);
               attroff(A_REVERSE);
             }
           }
         }
         }

	  if(replace_mode){
           if(replace_phase == 0){
             mvprintw(LINES - 2, 0, "查找: %s", replace_search);
           }else if (replace_phase == 1){
             mvprintw(LINES - 2, 0, "替换为: %s", replace_repl);
           }else{
             mvprintw(LINES - 2, 0, "Y=替换 N=跳过 A=全部 Enter/Ctrl-P=切换 Esc=退出");
           }
           clrtoeol();
         }

		/* 状态栏 */

         if(search_mode){						/* 搜索时，在状态栏上面一行显示搜索框 */
	   mvprintw(LINES - 2, 0, "搜索: %s", search_buf);
	   clrtoeol();
         }
	 if(confirm_quit){
           mvprintw(LINES - 1, 0, " 文件未保存！再按 Ctrl-Q 强制退出，其他键取消 ");
         }else if(replace_mode){
	   mvprintw(LINES - 1, 0, " REPLACE ");
	 }else if(search_mode){
	   mvprintw(LINES - 1, 0, " SEARCH ");
	 }else{
           mvprintw(LINES - 1, 0, " %s | %s | %d:%d ", filename, modified ? "Modified" : "Saved", row + 1 ,col + 1);
         }
	 clrtoeol();


	 move(row - top, col - left);
	 refresh();

	 ch = getch();

	 if(ch == 0x11){                        /* 判断退出时是否保存 */
         if(modified == 0 || confirm_quit == 1){
           break;
         }
         confirm_quit = 1;
           continue;
         }

         if(confirm_quit == 1){         /* 按其他任意键取消退出 */
           confirm_quit = 0;
           continue;
         }

	if(replace_mode){
	  if(ch == 0x1b || ch == 0x03){           /* Esc 或 Ctrl-C 退出 */
          replace_mode = 0;
          continue;
          }
    	  if(replace_phase == 0){
            if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){
              if(replace_search_len > 0)
	        replace_phase = 1;
              }else if(ch == KEY_BACKSPACE || ch == 127){
                if(replace_search_len > 0){
                  replace_search_len--;
                  replace_search[replace_search_len] = '\0';
                }
              }else if(ch >= 32 && ch <= 126){
                if(replace_search_len < 255){
                  replace_search[replace_search_len++] = ch;
                  replace_search[replace_search_len] = '\0';
                }
              }
       	      continue;
           }
	if(replace_phase == 1){
          if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){
            replace_phase = 2;
            rep_start_row = row;
            rep_start_col = col;
            rep_match_row = -1;
            rep_match_col = -1;
	    if(Find_next(&buf, replace_search, rep_start_row, rep_start_col, &rep_match_row, &rep_match_col)){
                row = rep_match_row;
                col = rep_match_col;
            }
          }else if(ch == KEY_BACKSPACE || ch == 127){
            if(replace_repl_len > 0){
              replace_repl_len--;
              replace_repl[replace_repl_len] = '\0';
            }
          }else if(ch >= 32 && ch <= 126){
            if(replace_repl_len < 255){
              replace_repl[replace_repl_len++] = ch;
              replace_repl[replace_repl_len] = '\0';
            }
          }
          continue;
        }
        if(replace_phase == 2){
          if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){
            int sr = (rep_match_row >= 0) ? rep_match_row : rep_start_row;
            int sc = (rep_match_col >= 0) ? rep_match_col + 1 : rep_start_col;
	    if(Find_next(&buf, replace_search, sr, sc, &rep_match_row, &rep_match_col)){
              row = rep_match_row;
              col = rep_match_col;
            }else{
              rep_match_row = -1;
              rep_match_col = -1;
            }
          }else if (ch == 0x10){                     /* Ctrl-P 上一个 */
            int pr = (rep_match_row >= 0) ? rep_match_row : rep_start_row;
            int pc = (rep_match_col >= 0) ? rep_match_col : rep_start_col;
	    if(Find_prev(&buf, replace_search, pr, pc, &rep_match_row, &rep_match_col)){
              row = rep_match_row;
              col = rep_match_col;
            }
          }else if(ch == 'Y' || ch == 'y'){
            if(rep_match_row >= 0){
	      Undo_push(&buf, row, col);
              Undo_clear_redo();
              Buffer_replace(&buf, rep_match_row, rep_match_col, replace_search_len, replace_repl);
              modified = 1;
              rep_start_row = rep_match_row;
              rep_start_col = rep_match_col + replace_repl_len;
            }
 	    if(Find_next(&buf, replace_search, rep_start_row, rep_start_col, &rep_match_row, &rep_match_col)){
              row = rep_match_row;
              col = rep_match_col;
            }else{
              rep_match_row = -1;
              rep_match_col = -1;
            }
          }else if(ch == 'N' || ch == 'n'){
            if(rep_match_row >= 0){
              rep_start_row = rep_match_row;
              rep_start_col = rep_match_col + 1;
            }
	    if(Find_next(&buf, replace_search, rep_start_row, rep_start_col, &rep_match_row, &rep_match_col)){
              row = rep_match_row;
              col = rep_match_col;
            }else{
              rep_match_row = -1;
              rep_match_col = -1;
            }
          }else if(ch == 'A' || ch == 'a'){
	    Undo_push(&buf, row, col);
            Undo_clear_redo();
            int r = rep_start_row, c = rep_start_col;
	    while(Find_next(&buf, replace_search, r, c, &r, &c)){
              Buffer_replace(&buf, r, c, replace_search_len, replace_repl);
              modified = 1;
              c += replace_repl_len;
            }
            replace_mode = 0;
          }
          continue;
        }
      }

	if(search_mode){
   	  mvprintw(LINES - 2, 0, "搜索: %s", search_buf);
          clrtoeol();  		 		 /* 搜索模式 */
         if(ch == 0x1b){                  		/* Esc 退出搜索 */
           search_mode = 0;
           match_row = -1;
           match_col = -1;
           continue;
         }else if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){
           if(search_len > 0 && Find_next(&buf, search_buf, search_start_row, search_start_col, &match_row, &match_col)){
             row = match_row;
             col = match_col;
             search_start_row = match_row;
             search_start_col = match_col + 1;
           }
           continue;
         }else if(ch == 0x10){           /* Ctrl-P 上一个 */
           if(search_len > 0){
             int pr = (match_row >= 0) ? match_row : search_start_row;
             int pc = (match_col >= 0) ? match_col : search_start_col;

             if(Find_prev(&buf, search_buf, pr, pc, &match_row, &match_col)){
               row = match_row;
               col = match_col;
               search_start_row = match_row;
               search_start_col = match_col + 1;
             }
          }
          continue;
          }else if(ch == KEY_BACKSPACE || ch == 127){
            if(search_len > 0){
              search_len--;
              search_buf[search_len] = '\0';
              match_row = -1;
              match_col = -1;
              search_start_row = row;
              search_start_col = col;
            }
          continue;
          }else if (ch >= 32 && ch <= 126){
            if(search_len < 255){
              search_buf[search_len++] = ch;
              search_buf[search_len] = '\0';
              match_row = -1;
              match_col = -1;
              search_start_row = row;
              search_start_col = col;
           }
           continue;
          }else{
           continue;
          }
         }

	 if(ch == 0x13){			/*  Ctrl+S保存 */
	   if(Buffer_save(&buf, filename) == 0){
	     modified = 0;
	   }
	 continue;
	 }

         if(ch == KEY_UP && row > 0){
           row--;
	   sel_active = 0;
         }else if(ch == KEY_DOWN && row < buf.count - 1){
           row++;
           sel_active = 0;
         }else if (ch == KEY_LEFT && col > 0){
           col--;
           sel_active = 0;
         }else if(ch == KEY_RIGHT && col < strlen(buf.lines[row])){
           col++;
           sel_active = 0;
         }else if(ch == KEY_RESIZE){		/* 缩放窗口 */
	   resizeterm(0, 0);			/* 重新读取终端大小 */
   	   clear();
	   if(col >= COLS){
             left = col - COLS + 1;
           }else{
             left = 0;
           }
	 }else if(ch == KEY_SLEFT || ch == KEY_SRIGHT || ch == KEY_SR || ch == KEY_SF){
           if(!sel_active){
             sel_active = 1;
             sel_anchor_row = row;
       	     sel_anchor_col = col;
           }
	   if(ch == KEY_SLEFT && col > 0){
             col--;
           }else if(ch == KEY_SRIGHT && col < strlen(buf.lines[row])){
             col++;
           }else if(ch == KEY_SR && row > 0){
             row--;
           }else if(ch == KEY_SF && row < buf.count - 1){
             row++;
           }

          if(col > strlen(buf.lines[row]))
	  col = strlen(buf.lines[row]);

        }else if(ch == 0x03){                    /* Ctrl-C 复制 */
          if(sel_active){
          int r1 = sel_anchor_row, c1 = sel_anchor_col, r2 = row, c2 = col;
          char *sel = Selection_get(&buf, r1, c1, r2, c2);
          Clipboard_set(sel);
          free(sel);
          sel_active = 0;
          }
        }else if(ch == 0x18){                    /* Ctrl-X 剪切 */
          if(sel_active){
          int r1 = sel_anchor_row, c1 = sel_anchor_col, r2 = row, c2 = col;
          char *sel = Selection_get(&buf, r1, c1, r2, c2);
          Clipboard_set(sel);
          free(sel);

          Undo_push(&buf, row, col);
          Undo_clear_redo();
          Buffer_delete_range(&buf, r1, c1, r2, c2);
          row = r1;
          col = c1;
          modified = 1;
          sel_active = 0;
          }
        }else if(ch == 0x16){                    /* Ctrl-V 粘贴 */
          if(clipboard != NULL){
	   Undo_push(&buf, row, col);
           Undo_clear_redo();
           Buffer_paste(&buf, &row, &col, clipboard);
           modified = 1;
          }
	}else if(ch == 0x12){               /* Ctrl-R 进入替换 */
    	  replace_mode = 1;
    	  replace_phase = 0;
          replace_search_len = 0;
   	  replace_search[0] = '\0';
          replace_repl_len = 0;
  	  replace_repl[0] = '\0';
     	  rep_match_row = -1;
          rep_match_col = -1;
	 }else if(ch == 0x06){                        /* Ctrl-F进入搜索模式 */
          search_mode = 1;
          search_len = 0;
          search_buf[0] = '\0';
          match_row = -1;
          match_col = -1;
          search_start_row = row;
          search_start_col = col;
         }else if(ch == '\n' || ch == '\r' || ch == KEY_ENTER){ 	/* 回车换行 */
	    Undo_push(&buf, row, col);
            Undo_clear_redo();
	    Buffer_line_enter(&buf, row, col);
	    row++;
	    col = 0;
	    modified = 1;
	 }else if(ch == KEY_BACKSPACE || ch == 127){		/* BACKSPACE键 */
	        Undo_push(&buf, row, col);
                Undo_clear_redo();
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
		Undo_push(&buf, row, col);
   	        Undo_clear_redo();
	        int len = strlen(buf.lines[row]);
	        if(col < len) {
                 Buffer_chardel(&buf, row, col);
		 modified = 1;
                }else if(row < buf.count - 1) {
                  Buffer_taildel(&buf, row);
		  modified = 1;
		}
	  }else if(ch >= 32 && ch <= 126){		/* 输入可打印字符 */
	        Undo_push(&buf, row, col);
                Undo_clear_redo();
         	Buffer_charinsert(&buf, row, col, ch);
		col++;
		modified = 1;
	  }else if(ch == 0x1a){        /* Ctrl-Z 撤回 */
                Undo_do(&buf, &row, &col);
                modified = 1;
          }else if(ch == 0x19){        /* Ctrl-Y 重做 */
                Redo_do(&buf, &row, &col);
                modified = 1;
          }

	if(col > strlen(buf.lines[row])){	/* 若此行字数太少，则缩减光标活动范围 */
	   col = strlen(buf.lines[row]);
	  }
        }

	endwin();
	while(undo_top > 0)				/* 退出时释放撤销栈和剪贴板 */
	 Snapshot_free(undo_stack[--undo_top]);
	while(redo_top > 0)
	 Snapshot_free(redo_stack[--redo_top]);
	if(clipboard != NULL)
	  free(clipboard);

	Buffer_free(&buf);
	return 0;
    }
