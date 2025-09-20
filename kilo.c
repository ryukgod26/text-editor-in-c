#include<stdbool.h>
#include <io.h>
#include <fcntl.h>
#include<windows.h>
#include<conio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>

DWORD orig_mode;
HANDLE global_hIn;

void die(const char *s){
    perror(s);
    exit(EXIT_FAILURE);
}

void disableRawMode(){
if(SetConsoleMode(global_hIn,orig_mode) == -1) die("Error on Disable Raw Mode Set Console Mode");
}

void enableRawMode() {
    global_hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = orig_mode;
    if(GetConsoleMode(global_hIn, &mode) == -1) die('Error On Enable Raw Mode Get Console Mode ');
    atexit(disableRawMode);
    
    

    //To Turn Off ECHO and Special Inputs
    // mode &= ~(ENAB )
    
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT );
    if( SetConsoleMode(global_hIn, mode) == -1) die("Error on Enabling Raw Mode Set Console Mode");
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
