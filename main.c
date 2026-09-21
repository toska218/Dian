#include <ncurses.h>

int main(void) {
    initscr();
    printw("Hello, Dian Editor!");
    refresh();
    getch();
    endwin();
    return 0;
}
