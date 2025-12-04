#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL,0}
#define KILO_VERSION "1.0.1"
#define TAB_STOP 8
#define KILO_QUIT_TIMES 3

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
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

typedef struct erow{
int size;
int rsize;
char* chars;
char* render;
unsigned char *hl;
} erow;


struct editorConfig{
int cx,cy;
int rx;
int rowOff;
int colOff;
uint16_t screenRows;
uint16_t screenCols;
int numRows;
erow *row;
char* filename;
int dirty;
char statusmsg[80];
time_t statusmsg_time;
struct termios orig_termios;
};

typedef struct abuf{
char* b;
int len;
} abuf;

enum editorHighlight{
	HL_NORMAL = 0,
	HL_NUMBER,
	HL_MATCH,
	HL_SELECTED_MATCH,
	HL_COMMENT,
};

enum editorKey
{
	BACKSPACE=127,
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

int editorSyntaxToColor(int);
void editorUpdateSyntax(erow*);
void editorFindCallback(char*, int);
int editorRowRxToCx(erow *row, int rx);
void editorFind();
char* editorPrompt(char* ,void (*callback) (char *, int));
void editorInsertNewLine();
void editorRowAppendString(erow*,char*,size_t);
void editorFreeRow(erow*);
void editorDelRow(int);
void editorDelChar();
void editorRowDelChar(erow*,int);
void editorSave();
char* editorRowsToString(int*);
void editorInsertChar(int);
void editorRowInsertChar(erow*,int,int);
void editorDrawMessageBar(abuf*);
void editorSetStatusMessage(const char*,...);
void editorDrawStatusBar(abuf*);
int editorRowCxToRx(erow*,int);
void editorUpdateRow(erow*);
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
// void editorAppendRow(char* s,size_t len);
void editorInsertRow(int, char*,size_t);

int editorSyntaxToColor(int hl){
	switch(hl){
		case HL_NUMBER: return 31;
		case HL_MATCH: return 34;
		default: return 37;
	}
}

void editorUpdateSyntax(erow* row){
	row->hl = realloc(row->hl, row->rsize);
	memset(row->hl, HL_NORMAL, row->rsize);
	
	for(int i = 0; i < row->rsize; i++) {
		if (isdigit(row->render[i])){
		row->hl[i] = HL_NUMBER;
		}
	}
}

void editorFindCallback(char* query, int key){
	static int lastMatch = -1;
	static int direction = 1;
	static int saved_hl_line;
	static char *saved_hl = NULL;

	if(saved_hl){
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}
	if (key == '\r' || key == '\x1b'){
		lastMatch = -1;
		direction = 1;
		return;
	} else if (key == ARROW_RIGHT || key == ARROW_DOWN){
		direction = 1;
	}else if (key == ARROW_LEFT || key == ARROW_UP){
		direction = -1;
	} else{
		lastMatch = -1;
		direction = 1;
	}

	if (lastMatch == -1) direction = 1;
	int current = lastMatch;

	for(int i = 0; i < E.numRows; i++){
		current += direction;
		if (current == -1) current = E.numRows - 1;
		else if (current == E.numRows) current = 0;
		erow* row = &E.row[current];
		char* match = strstr(row->render, query);
		if (match){
			lastMatch = current;
			E.cy = current;
			E.cy = i;
			E.cx = editorRowCxToRx(row, match - row->render);
			E.rowOff = E.numRows;

			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			memset(&row->hl[match- row.render],HL_MATCH, strlen(query));
			break;
		}
	}
}

int editorRowRxToCx(erow *row, int rx) {
	int cur_rx = 0;
	int cx;
	for(cx = 0; cx < row->size; cx++){
		if (row->chars[cx] == '\t'){
			cur_rx += (TAB_STOP -1) - (cur_rx % TAB_STOP);
		}
		cur_rx++;
		if(cur_rx > rx) return cx;
	} 
	return cx;
}

void editorFind(){
	int prev_cx = E.cx;
	int prev_cy = E.cy;
	int prev_rowOff = E.rowOff;
	int prev_colOff = E.colOff;
	char* query = editorPrompt("Find [Esc to Cancel]: %s", editorFindCallback);
	if (query){
	free(query);
	}
	else{
		E.cx = prev_cx;
		E.cy = prev_cy;
		E.rowOff = prev_rowOff;
		E.colOff = prev_colOff;
	}
}

char* editorPrompt(char* prompt,void (*callback) (char *,int))
{
size_t bufSize = 128;
char* buf = malloc(bufSize);
size_t buflen = 0;
buf[bufSize] = '\0';

while(1){
	editorSetStatusMessage(prompt,buf);
	editorRefreshScreen();

	int c = editorReadKey();
	if(c == DEL_KEY || c == BACKSPACE || c == CTRL_KEY('h')){
		if(buflen != 0) buf[--buflen] = '\0';
	}
	else if(c == '\x1b'){
		editorSetStatusMessage("");
		if(callback) callback(buf, c);
		free(buf);
		return NULL;
	}
	else if(c == '\r'){
		if( buflen != 0){
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			return buf;
		}
	}else if(!iscntrl(c)  && c < 128){
		if ( buflen == bufSize -1){
			bufSize *= 2;
			buf = realloc(buf,bufSize);
		}
		buf[buflen++] = c;
		buf[buflen] = '\0';
	}
	if(callback) callback(buf, c);
}

}

void editorInsertNewLine()
{
if(E.cx == 0){
	editorInsertRow(E.cy,"",0);
}else{
	erow* row = &E.row[E.cy];
	editorInsertRow(E.cy+1,&row->chars[E.cx],row->size - E.cx);
	row = &E.row[E.cy];
	row->size = E.cx;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
}
E.cy++;
E.cx = 0;
}

void editorRowAppendString(erow* row,char* s, size_t len)
{
row->chars = realloc(row->chars,row->size + len + 1);
memcpy(&row->chars[row->size],s,len);
row->size += len;
row->chars[row->size] = '\0';
editorUpdateRow(row);
E.dirty++;
}

void editorDelRow(int at)
{
if(at < 0 || at >= E.numRows) return;
editorFreeRow(&E.row[at]);
memmove(&E.row[at],&E.row[at+1],sizeof(erow) * (E.numRows - at - 1));
E.numRows --;
E.dirty ++;
}

void editorFreeRow(erow* row)
{
free(row->chars);
free(row->render);
free(row->hl);
}

void editorDelChar()
{
if(E.cy == E.numRows) return;
if(E.cx == 0 && E.cy == 0) return;

erow* row = &E.row[E.cy];
if(E.cx > 0)
	{
		editorRowDelChar(row,E.cx-1);
		E.cx--;
	}
else
	{
		E.cx = E.row[E.cy -1].size;
		editorRowAppendString(&E.row[E.cy -1 ],row->chars,row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}

void editorRowDelChar(erow* row,int at)
{
if(at < 0 || at > row->size) return;
memmove(&row->chars[at],&row->chars[at+1],row->size-at);
row->size--;
editorUpdateRow(row);
E.dirty++;
}
void editorSave()
{
if (E.filename == NULL){
	E.filename = editorPrompt("Save as: %s [ESC to Cancel]",NULL);
	if(E.filename == NULL){
		editorSetStatusMessage("Save Aborted");
		return;
	}
}

int len;
char* buf = editorRowsToString(&len);

int fd = open(E.filename,O_RDWR | O_CREAT ,0644);
if(fd != -1){
	if(ftruncate(fd,len) != -1){
		if(write(fd,buf,len) == len){
			close(fd);
			free(buf);
			E.dirty = 0;
			editorSetStatusMessage("%d bytes Written to the Disk.",len);
			return;
		}
	}
	close(fd);
}

free(buf);
editorSetStatusMessage("Cannot Save I/O Error: %s",strerror(errno));
}

char* editorRowsToString(int* buflen)
{
int totalen = 0;
int j;
for(j=0;j<E.numRows;j++)
	totalen += E.row[j].size +1;
*buflen = totalen;

char* buf = malloc(totalen);
char* p = buf;
for(j=0; j<E.numRows; j++){
memcpy(p,E.row[j].chars,E.row[j].size);
p += E.row[j].size;
*p='\n';
p++;
}

return buf;
}

void editorInsertChar(int c)
{
if(E.cy == E.numRows)
	{	
		editorInsertRow(E.numRows,"",0);
	}
	editorRowInsertChar(&E.row[E.cy],E.cx,c);
	E.cx++;
}

void editorRowInsertChar(erow* row, int at, int c)
{
if(at <0 || at > row->size) at=row->size;
row->chars = realloc(row->chars,row->size+2);
memmove(&row->chars[at+1],&row->chars[at],row->size - at +1);
row->size++;
row->chars[at] = c;
editorUpdateRow(row);
E.dirty++;
}

void editorDrawMessageBar(abuf* ab)
{
abAppend(ab,"\x1b[K",3);
int msglen = strlen(E.statusmsg);
if(msglen > E.screenCols) msglen = E.screenCols;
if(msglen && time(NULL) - E.statusmsg_time < 5)
	abAppend(ab,E.statusmsg,msglen);

}

void editorSetStatusMessage(const char* fmt,...)
{
va_list ap;
va_start(ap,fmt);
vsnprintf(E.statusmsg,sizeof(E.statusmsg),fmt,ap);
va_end(ap);
E.statusmsg_time = time(NULL);
}

void editorDrawStatusBar(abuf* ab)
{
char status[80],rstatus[80];
//Changing to Inverted Colors
abAppend(ab,"\x1b[7m",4);
int len = snprintf(status,sizeof(status),"%.20s - %d lines %s",
		E.filename ? E.filename : "[No Name]",E.numRows,
		E.dirty ? "(modifies)" : ""
		);
int rlen = snprintf(rstatus,sizeof(rstatus),"%d %d",
		E.cy + 1,E.cx +1);
if (len > E.screenCols) len = E.screenCols;
abAppend(ab,status,len);
while (len < E.screenCols)
	{
		if(E.screenCols - len == rlen){
		abAppend(ab,rstatus,rlen);
		break;
		}
		else{
		abAppend(ab," ",1);
		len++;
		}
	}
//Changing to Normal Mode
abAppend(ab,"\x1b[m",3);
abAppend(ab,"\r\n",2);
}

int editorRowCxToRx(erow* row,int cx)
{
int rx= 0, j;
for(j=0; j<cx; j++)
	{
		if(row->chars[j] == '\t') rx += (TAB_STOP-1) - (rx%TAB_STOP);
		rx++;
	}
return rx;
}

void editorUpdateRow(erow* row)
{
int tabs = 0;
int j;
for(j=0;j<row->size;j++)
{
if (row->chars[j] == '\t') tabs++;
}
free(row->render);

row->render = malloc(row->size + tabs*(TAB_STOP-1) + 1);

int idx = 0;
for(j=0;j < row->size;j++)
	{
		if(row->chars[j] == '\t'){
		row->render[idx++] = ' ';
		while(idx % TAB_STOP != 0) row->render[idx++] = ' '; 
		}
		else{
		row->render[idx++] = row->chars[j];
		}
	}
row->render[idx] = '\0';
row->rsize = idx;
editorUpdateSyntax(row);
}

void editorScroll()
{
	E.rx = 0;
	if(E.cy < E.numRows)
	{
		E.rx = editorRowCxToRx(&E.row[E.cy],E.cx);
	}
	if(E.cy < E.rowOff)
	{
	E.rowOff = E.cy;
	}
	if(E.cy >= E.rowOff + E.screenRows)
	{
	E.rowOff = E.cy - E.screenRows +1;
	}
	if(E.rx < E.colOff)
	{
		E.colOff = E.rx;
	}
	if(E.rx >= E.colOff + E.screenCols)
	{
		E.colOff = E.rx - E.screenCols +1;
	}
}

void editorInsertRow(int at,char* s, size_t len)
{
if(at < 0 || at > E.numRows) return;

E.row = realloc(E.row,sizeof(erow) * (E.numRows +1));
memmove(&E.row[at+1],&E.row[at],sizeof(erow) * (E.numRows - at));
// int at = E.numRows;
E.row[at].size = len;
E.row[at].chars = malloc(len+1);
memcpy(E.row[at].chars,s,len);
E.row[at].chars[len] = '\0';
E.row[at].rsize = 0;
E.row[at].render = NULL;
E.row[at].hl = NULL;
editorUpdateRow(&E.row[at]);
E.numRows++;
E.dirty++;
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
	free(E.filename);
	E.filename = strdup(filename);
	char* line = NULL;
    size_t linecap = 0;
	ssize_t linelen;
    while( (linelen = getline(&line,&linecap,fp)) != -1){
        while(linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
        {
            linelen --;
        }
            editorInsertRow(E.numRows,line,linelen);
            }
    free(line);
    fclose(fp);
	E.dirty = 0;
}

void editorMoveCursor(int key)
{
	
erow* row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];
switch(key)
{

	case ARROW_LEFT:
		if (E.cx != 0){
		E.cx--;}

		else if(E.cy > 0)
		{
		E.cy--;
		E.cx = E.row[E.cy].size;
		}
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
		if (row && E.cx < row->size){
		E.cx++;}
		else if(row && E.cx == row->size){
		E.cy++;
		E.cx=0;
		}
		break;
}
row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];
int rowlen = row ? row->size : 0;
if(E.cx > rowlen)
{
E.cx = rowlen;
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
	int len = E.row[fileRow].rsize - E.colOff;
	if(len <0) len = 0;
	if(len > E.screenCols) len = E.screenCols;
	char* c = &E.row[fileRow].render[E.colOff];
	unsigned char *hl = &E.row[fileRow].hl[colOff];
	int current_color = -1;
	int j;
	for(j = 0; j<len;j++){
		if (hl[j] == HL_NORMAL){
			if (current_color = -1){
			abAppend(ab,"\x1b[39m",5);
			current_color = -1;
			}
			abAppend(ab, &c[j],1);	
		} else{
			int color = editorSyntaxToColor(hl[j]);
			if (color != current_color){
					current_color = color;
				char buf[16];
				int clen = snprintf(buf, sizeof(buf), "\x1b[%dm",color);
				abAppend(ab, buf, clen);
			}
			abAppend(ab, &c[j], 1);
		}
	}
	abAppend(ab, "\x1b[39m",5);
		}
	abAppend(ab,"\x1b[K",3);
//	if(y < E.screenRows - 1 ){
//	write(STDOUT_FILENO,"\r\n",2);
	abAppend(ab,"\r\n",2);
//	}
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
	static int quit_times = KILO_QUIT_TIMES;
    int c = editorReadKey();

    switch (c)
    {
    case '\r':
		editorInsertNewLine();
		break;
    case CTRL_KEY('q'):
		if(E.dirty && quit_times > 0){
			editorSetStatusMessage("Warning!!! File has Unsaved Changes. Press Ctrl-Q %d more times to Quit Without Saving." , quit_times);
			quit_times--;
			return;
		}
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
		exit(EXIT_SUCCESS);
    		break;
    case CTRL_KEY('f'):
	editorFind();
	break;

    case CTRL_KEY('s'):
	editorSave();
	break;
    case HOME_KEY:
	E.cx = 0;
	break;
    case END_KEY:
	if(E.cy < E.numRows)
		E.cx = E.row[E.cy].size;
	break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
		if(c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
		editorDelChar();
		break;
    case PAGE_UP:
    case PAGE_DOWN:
	{

	if(c == PAGE_UP)
	{
	E.cy = E.rowOff;
	}
	else if(c==PAGE_DOWN){
	E.cy = E.rowOff + E.screenRows-1;
	if(E.cy>E.numRows) E.cy = E.numRows;
	}
	
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
    case CTRL_KEY('l'):
    case '\x1b':
	break;
    default:
	editorInsertChar(c);
	break;
    }
	quit_times = KILO_QUIT_TIMES;
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
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",(E.cy - E.rowOff) +1,(E.rx - E.colOff)+1);
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
	E.rx = 0;
	E.numRows = 0;
	E.rowOff = 0;
	E.colOff = 0;
        E.row = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
    if(getWindowsSize(&E.screenRows,&E.screenCols) == -1) die("init error");
    //for status bar
    E.screenRows -= 2;
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
    editorSetStatusMessage("Help : Ctrl-q = Quit or Ctrl-S = Save or Ctrl-F: Find");
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
