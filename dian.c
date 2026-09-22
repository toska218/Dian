#include <ncurses.h>

int main(void) {
    int y = 0, x = 0;        /* 当前光标位置：行 y、列 x */
    int ch;

    initscr();               /* 初始化 ncurses，接管终端 */
    cbreak;
    keypad(stdscr, TRUE);    /* 开启方向键等特殊按键 */

    while ((ch = getch()) != 27) {   /* Esc */
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
