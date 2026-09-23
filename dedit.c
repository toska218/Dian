#include <ncurses.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    FILE *fp;                /* 指向文件的指针 */
    char buf[1024]           /* 存放临时读到的每一行 */

    if(argc < 2){            /* 检查文件输入格式是否正确 */
    printf("错误，正确的输入格式为：.dedit <filename>\n");
    return 1;
    }

    fp = fopen(argv[1], "r");    /* 检查文件是否存在，是否能打开 */
    if(fp == NULL){
    printf("无法打开文件:%s\n",argv[1]);
    return 1;
    }



    int y = 0, x = 0;        /* 当前光标位置：行 y、列 x */
    int ch;

    initscr();               /* 初始化 ncurses，接管终端 */
    raw();                   /* 程序读取Ctrl-Q */
    cbreak;
    keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

    while (fgets(buf, sizeof(buf), fp) != NULL){
    printf("%s",buf);

    while ((ch = getch()) != 0x11) {   /* Ctrl-Q */
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
