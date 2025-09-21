#define _DEFAULT_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include<sys/ioctl.h>
#include<stdint.h>

struct editorConfig{
struct termios orig_termios;
};

struct editorConfig E;






int8_t getWindowsSize(int *x, int*y){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0)    {
        return -1;
    }
    else{
        *x = ws.ws_row;
        *y = ws.ws_col;
        return 0;
    }
}



void die(char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(EXIT_FAILURE);
}

void editoDrawRows()
{
    int rows,cols;
    int8_t result = getWindowsSize(&rows,&cols) ;
    if(result == -1){
        die("rows/cols");
    }
    int y;
    for (y = 0; y < rows; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("disableRow");
}

char editorReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}

void editorProcessKeyprocess()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);

        exit(EXIT_SUCCESS);
        break;
    }
}

void editorRefreshScreen()
{

    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editoDrawRows();

    write(STDOUT_FILENO,"\x1b[H",3);
}

void enableRawMode()
{
    // struct termios raw;

    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("RawModetcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | IXON | ICRNL | INPCK | ISTRIP);
    // raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("RawModetcsetattr");
}

int main()
{

    enableRawMode();

    while (1)
    {
        /*
        c = '\0';
        if( read(STDIN_FILENO, &c , 1) == -1 && errno != EAGAIN ) die("read");
        if(iscntrl(c)){
            printf("%d\r\n",c);
        }
        else{
            printf("%d (%c)\r\n",c,c);
        }
        if (c == CTRL_KEY('q')) break;
        */
        editorRefreshScreen();
        editorProcessKeyprocess();
    }

    return 0;
}
