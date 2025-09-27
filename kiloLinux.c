#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL,0}
#define KILO_VERSION "0.0.1"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>


typedef struct erow{
int size;
char* chars;
} erow;


struct editorConfig{
int cx,cy;
int rowOff;
uint16_t screenRows;
uint16_t screenCols;
int numRows;
erow *row;
struct termios orig_termios;
};

typedef struct abuf{
char* b;
int len;
} abuf;



enum editorKey
{
	ARROW_LEFT=1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	PAGE_UP,
	PAGE_DOWN,
	HOME_KEY,
	END_KEY,
	DEL_KEY
};

struct editorConfig E;

void editorScroll();
void editorOpen(char*);
void editorMoveCursor(int);
void abAppend(abuf* ,const char*, int len);
void abFree(abuf*);
int editorReadKey();
void editoDrawRows();
void disableRawMode();
void editorProcessKeyprocess();
void editorRefreshScreen();
void die(char *s);
void enableRawMode();
void initEditor();
int8_t getWindowsSize(uint16_t *x, uint16_t*y);
int8_t getCursorPosition(uint16_t* rows, uint16_t* cols);
void editorAppendRow(char* s,size_t len);

void editorScroll()
{
	if(E.cy < E.rowOff)
	{
	E.rowOff = E.cy;
	}
	if(E.cy >= E.rowOff + E.screenRows)
	{
	E.rowOff = E.cy - E.screenRows +1;
	}
}

void editorAppendRow(char* s, size_t len)
{
E.row = realloc(E.row,sizeof(erow) * (E.numRows +1));
int at = E.numRows;
E.row[at].size = len;
E.row[at].chars = malloc(len+1);
memcpy(E.row[at].chars,s,len);
E.row[at].chars[len] = '\0';
E.numRows++;
// E.row->size = len;
// E.row->chars = malloc(len +1 );
// memcpy(E.row->chars,s,len);
// E.row->chars[len] = '\0';
// E.numRows = 1;
}

void editorOpen(char* filename)
{
    FILE *fp = fopen(filename,"r");
    if(!fp) die("fopen");

	char* line = NULL;
    size_t linecap = 0;
	ssize_t linelen;
    while( (linelen = getline(&line,&linecap,fp)) != -1){
        while(linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
        {
            linelen --;
        }
            editorAppendRow(line,linelen);
            }
    free(line);
    fclose(fp);
}

void editorMoveCursor(int key)
{
switch(key)
{
	case ARROW_LEFT:
		if (E.cx != 0){
		E.cx--;}
		break;
	case ARROW_DOWN:
		if (E.cy < E.numRows){
		E.cy++;}
		break;
	case ARROW_UP:
		if (E.cy != 0){
		E.cy--;}
		break;
	case ARROW_RIGHT:
		if (E.cx != E.screenCols - 1){
		E.cx++;}
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
    for (y = 0; y < E.screenRows; y++)
    { 
	    int fileRow = y + E.rowOff;

	    if ( fileRow >= E.numRows){
	    if(E.numRows == 0 && y == E.screenRows/3){
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
	}
	else{
	int len = E.row[fileRow].size;
	if(len > E.screenCols) len = E.screenCols;
	abAppend(ab,E.row[fileRow].chars,len);
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

int editorReadKey()
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
		 if (seq[1] >= '0' && seq[1] <= '9')
		 {
			 if(read(STDIN_FILENO,&seq[2],1) != 1) return '\x1b';
			 if( seq[2] == '~')
			 {
			 	switch(seq[1])
				{
					case '1': return HOME_KEY;
					case '3': return DEL_KEY;
					case '4': return END_KEY;
					case '5': return PAGE_UP;
					case '6': return PAGE_DOWN;
					case '7': return HOME_KEY;
					case '8': return END_KEY;
				}
			 }
		 }
		 else if(seq[0] == '0')
		 {
			 switch(seq[1])
			 {
				 case 'H': return HOME_KEY;
				 case 'F': return END_KEY;
			 }
		 }
		 else{
    		switch(seq[1])
		{
		  	case 'A': return ARROW_UP;
			case 'B': return ARROW_DOWN;
			case 'C': return ARROW_RIGHT;		  
			case 'D': return ARROW_LEFT;
			case 'H': return HOME_KEY;
			case 'F': return END_KEY;
		}
		 }
 	 }
    }
    return c;
}

void editorProcessKeyprocess()
{
    int c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
	exit(EXIT_SUCCESS);
	break;
    case HOME_KEY:
	E.cx = 0;
	break;
    case END_KEY:
	E.cx = E.screenCols - 1;
	break;
    case PAGE_UP:
    case PAGE_DOWN:
	{
	int times = E.screenRows;
	while(times--)
		editorMoveCursor(c==PAGE_UP ? ARROW_UP : ARROW_DOWN);
	}
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
    editorScroll();
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
	E.numRows = 0;
	E.rowOff = 0;
        E.row = NULL;
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

int main(int argc, char* argv[])
{
    enableRawMode();
    initEditor();
    if(argc >= 2)
    {
        editorOpen(argv[1]);
    }
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
