#include <termios.h>
#include <stdio.h>

#include "../lib/input.h"

static struct termios old, current;

void initTermios()
{
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &current);
}

void resetTermios(void)
{
    tcsetattr(0, TCSANOW, &old);
}

char getch(void)
{
    char ch;

    initTermios();
    ch = getchar();
    resetTermios();

    return ch;
}