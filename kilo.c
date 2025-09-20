#define _DEFAULT_SOURCE

#include<stdbool.h>
#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
    #define STDIN_FILENO 0
    #include<windows.h>
    #include<conio.h>
	bool type_win = true;
    void enableRowMode() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hIn, &mode);
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(hIn, mode);
}

#else
//#define _DEFAULT_SOURCE
bool type_win = false;
    #include <unistd.h>
    #include<termios.h>
    void enableRowMode(){
struct termios raw;

tcgetattr(STDIN_FILENO,&raw);

raw.c_lflag &= ~(ECHO) ;

tcsetattr(STDIN_FILENO, TCAFLUSH, &raw);
}
#endif





int main(){
    enableRowMode();
    char c;
    if (type_win){
    while(_read(STDIN_FILENO,&c,1) == 1 && c != 'q');
    }
    else{
    while (read(STDIN_FILENO, &c ,1) == 1 && c!= 'q' );
    }
    return 0;
}
