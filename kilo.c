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

void die(const char*);
void disableRawMode();
void enableRawMode();
int editorReadKey();



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
