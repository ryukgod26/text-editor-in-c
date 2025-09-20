#include<stdbool.h>
#include <io.h>
#include <fcntl.h>
#include<windows.h>
#include<conio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>

DWORD orig_mode;
HANDLE global_hIn;

void disableRawMode(){
SetConsoleMode(global_hIn,orig_mode);
}

void enableRawMode() {
    HANDLE global_hIn = GetStdHandle(STD_INPUT_HANDLE);
    atexit(disableRawMode);
    DWORD mode = orig_mode;
    GetConsoleMode(global_hIn, &mode);

    //To Turn Off ECHO and Special Inputs
    // mode &= ~(ENAB )
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT );
    SetConsoleMode(global_hIn, mode);
}


int main(){
    enableRawMode();
    char c;

    while(_read(STDIN_FILENO,&c,1) == 1 && c != 'q'){
        if(iscntrl(c)){
            printf("%d\n",c);
        }
        else{
            printf("%d (%c) \n",c,c);
        }
    }
    
    return 0;
}
