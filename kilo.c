#include<stdbool.h>
#include <io.h>
#include <fcntl.h>
#include<windows.h>
#include<conio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<stdint.h>

DWORD orig_mode;
HANDLE global_hIn;

static HANDLE g_hStdin = NULL;
static HANDLE g_hStdout = NULL;
static DWORD g_origInMode = 0;
static DWORD g_origOutMode = 0;

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
};

typedef struct abuf{
char* b;
int len;
} abuf;

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

void die(const char*);
void disableRawMode();
void enableRawMode();
int editorReadKey();

int editorReadKey(){
    INPUT_RECORD rec;
    DWORD cnt;
    while (1)
    {
        if (!ReadConsoleInput(g_hStdin, &rec, 1, &cnt)) die("ReadConsoleInput");
        if (rec.EventType != KEY_EVENT) continue;
        KEY_EVENT_RECORD key = rec.Event.KeyEvent;
        if (!key.bKeyDown) continue;

        char ch = key.uChar.AsciiChar;
        if(ch) {
            if((unsigned char)ch == 127) return BACKSPACE;
            return (int)ch;
        }

        switch (key.wVirtualKeyCode)
        {
        case VK_LEFT: return ARROW_LEFT;
        case VK_RIGHT: return ARROW_RIGHT;
        case VK_UP: return ARROW_UP;   
        case VK_DOWN: return ARROW_DOWN;
        case VK_HOME: return HOME_KEY;
        case VK_END: return END_KEY;
        case VK_PRIOR: return PAGE_UP;
        case VK_NEXT: return PAGE_DOWN;
        // case VK_BACK: return BACKSPACE;
        case VK_DELETE: return DEL_KEY;
        default:    break;
        }
    }
    
}

void die(const char *s){
    perror(s);
    exit(EXIT_FAILURE);
}

void disableRawMode(){
    if (g_hStdin && g_origInMode) SetConsoleMode(g_hStdin, g_origInMode);
    if (g_hStdout && g_origOutMode) SetConsoleMode(g_hStdout, g_origOutMode);
}

void enableRawMode() {
    g_hStdin = GetStdHandle(STD_INPUT_HANDLE);
    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if(g_hStdin == INVALID_HANDLE_VALUE || g_hStdout == INVALID_HANDLE_VALUE) die("GetStdHandle");
    if(!GetConsoleMode(g_hStdin, &g_origInMode)) die("GetConsoleGStdIn");
    if(!GetConsoleMode(g_hStdout, &g_origOutMode)) die("GetConsoleGStdOut");

    #ifndef ENABLE_QUICK_EDIT_MODE
    #define ENABLE_QUICK_EDIT_MODE 0x0040
    #endif
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
    
    DWORD inMode = g_origInMode;
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT | ENABLE_QUICK_EDIT_MODE);
    if (!SetConsoleMode(g_hStdin, inMode)) die("SetConsoleMode input");
    //global_hIn = GetStdHandle(STD_INPUT_HANDLE);
    // DWORD mode = orig_mode;
    DWORD outMode = g_origOutMode;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // if(GetConsoleMode(global_hIn, &mode) == -1) die('Error On Enable Raw Mode Get Console Mode ');
    // atexit(disableRawMode);
    SetConsoleMode(g_hStdout, outMode);
    atexit(disableRawMode);
    //To Turn Off ECHO and Special Inputs
    // mode &= ~(ENAB )
    
    // if( SetConsoleMode(global_hIn, mode) == -1) die("Error on Enabling Raw Mode Set Console Mode");
}


int main(){
    enableRawMode();
    char c;

    while(1){
        c = '\0';
       if(_read(STDIN_FILENO,&c,1) == -1 && errno != EAGAIN) die("read");
        if(iscntrl(c)){
            printf("%d\n",c);
        }
        else{
            printf("%d (%c) \n",c,c);
        }
        if(c == 'q') break;
    }
    
    return 0;
}
