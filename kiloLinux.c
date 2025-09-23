#define _DEFAULT_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL,0}

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include<sys/ioctl.h>
#include<stdint.h>
#include<inttypes.h>

struct editorConfig{
uint16_t screenRows;
uint16_t screenCols;
struct termios orig_termios;
};

struct ebuf{
char* b;
int len;
}


struct editorConfig E;


char editorReadKey();
void editoDrawRows();
void disableRawMode();
char editorReadKey();
void editorProcessKeyprocess();
void editorRefreshScreen();
void die(char *s);
void enableRawMode();
void initEditor();
int8_t getWindowsSize(uint16_t *x, uint16_t*y);
int8_t getCursorPosition(uint16_t* rows, uint16_t* cols);

int8_t getCursorPosition(uint16_t* rows, uint16_t* cols)
{
char buf[32];
uint8_t i = 0;

if(write(STDOUT_FILENO,"\x1b[6n",4) != 4) {printf("Crashed");return -1;}

while (i < sizeof(buf) -1 ){
    if(read(STDIN_FILENO,&buf[i],1) != 1) break;
    if(buf[i] == 'R') break;
    i++;
}

buf[i] = '\0';

if(buf[0] != '\x1b' || buf[1] != '[') return -1;
if(sscanf(&buf[2],"%hu;%hu",rows,cols) != 2) return -1;

//printf("\r\n&buf[1] '%s'\r\n",&buf[1]);

return 0;

// while(read(STDIN_FILENO,&c,1) == 1)
// {
// if(iscntrl(c))
// {
// printf("%d\r\n",c);
// }
// else
// {
// printf("%d (%c)\r\n",c,c);
// }
// }

}

int8_t getWindowsSize(uint16_t *x, uint16_t*y){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0) {
        if(write(STDOUT_FILENO,"\x1b[999C\x1b[999B",12) != 12) return -1;
        editorReadKey();
        return getCursorPosition(x,y);
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

    uint16_t y;
    for (y = 0; y < E.screenCols; y++)
    {
        write(STDOUT_FILENO, "~", 1);
	if(y < E.screenRows - 1 ){
	write(STDOUT_FILENO,"\r\n",2);
	}
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

void initEditor()
{
    if(getWindowsSize(&E.screenRows,&E.screenCols) == -1) die("init error");
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
    initEditor();
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
