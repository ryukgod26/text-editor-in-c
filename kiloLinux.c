#define _DEFAULT_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL,0}
#define KILO_VERSION "0.0.1"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include<sys/ioctl.h>
#include<stdint.h>
#include<inttypes.h>
#include<string.h>

struct editorConfig{
int cx,cy;
uint16_t screenRows;
uint16_t screenCols;
struct termios orig_termios;
};

typedef struct abuf{
char* b;
int len;
} abuf;

enum editorKey
{
ARROW_UP = 'w',
ARROW_DOWN='s',
ARROW_RIGHT='d',
ARROW_LEFT='a'
}

struct editorConfig E;

void editorMoveCursor(char);
void abAppend(abuf* ,const char*, int len);
void abFree(abuf*);
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


void editorMoveCursor(char key)
{
switch(key)
{
	case ARROW_LEFT:
		E.cx--;
		break;
	case ARROW_DOWN:
		E.cy++;
		break;
	case ARROW_UP:
		E.cy--;
		break;
	case ARROW_RIGHT:
		E.cx++;
		break;
}
}

void abAppend(abuf* ab, const char* s, int len)
{
char* new = realloc(ab->b,ab->len + len);

if(new == NULL) return;
memcpy(&new[ab->len],s,len);
ab->b = new;
ab->len += len;
}

void abFree(abuf* ab)
{
free(ab->b);
}

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

void editoDrawRows(abuf* ab)
{
    uint16_t y;
    for (y = 0; y < E.screenCols; y++)
    {
	    if(y == E.screenRows/3){
 //       write(STDOUT_FILENO, "~", 1);
	char welcome[80];
	int welcomelen = snprintf(welcome,sizeof(welcome),"Kilo Editor -- version %s",KILO_VERSION);
if(welcomelen > E.screenCols) welcomelen = E.screenCols;
int padding = (E.screenCols - welcomelen) /2;
if(padding){
abAppend(ab,"~",1);
padding--;
}
while(padding--) abAppend(ab," " ,1);
abAppend(ab,welcome,welcomelen);

	    }
	    else{
	abAppend(ab,"~",1);
	    }
	abAppend(ab,"\x1b[K",3);
	if(y < E.screenRows - 1 ){
//	write(STDOUT_FILENO,"\r\n",2);
	abAppend(ab,"\r\n",2);
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
    if(c == '\x1b')
    {
    char seq[3];

    if(read(STDIN_FILENO,&seq[0],1) != 1) return '\x1b';
    if(read(STDIN_FILENO,&seq[1],1) != 1) return '\x1b';

    if (seq[0] == '[')
   	 {
    		switch(seq[1])
		{
		  	case 'A': return ARROW_UP;
			case 'B': return ARROW_DOWN;
			case 'C': return ARROW_RIGHT;		  
			case 'D': return ARROW_LEFT;
		}
    	}
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
    case ARROW_UP:
    case ARROW_LEFT:
    case ARROW_DOWN:
    case ARROW_RIGHT:
	editorMoveCursor(c);
	break;
    }
}

void editorRefreshScreen()
{
    abuf ab = ABUF_INIT;


    /*write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);*/

    //Hiding the cursor when Drawing Tildes on the Screen.
    abAppend(&ab,"\x1b[?25l",6);

//Used to Clear the screen at once but we now clear each line one by one.
//abAppend(&ab,"\x1b[2J",4);
  abAppend(&ab,"\x1b[H",3);

    editoDrawRows(&ab);

    char buf[32];
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",E.cy +1,E.cx+1);
    abAppend(&ab,buf,strlen(buf));


//    write(STDOUT_FILENO,"\x1b[H",3);
  //  abAppend(&ab,"\x1b[H",3);

    //Making the Cursor Visible Again After Drawing The Tildes
    abAppend(&ab,"\x1b[?25h",6);

    //Writing the whole Buffer
    write(STDOUT_FILENO,ab.b,ab.len);
    abFree(&ab);
}

void initEditor()
{
	E.cx = 0;
	E.cy = 0;
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
