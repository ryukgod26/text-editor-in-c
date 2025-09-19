#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
    #define STDIN_FILENO 0
    #include<windows.h>
    #include<conio.h>
    void enableRowMode() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hIn, &mode);
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(hIn, mode);
}

#else
    #include <unistd.h>
    #include<termios.h>
    void enableRowMode(){
struct termios raw;

raw.c_lflag &= ~(ECHO);

tcsetattr(STDIN_FILENO, TCAFLUSH, & raw);
}
#endif





int main(){
    enableRowMode();
    char c;
    while (_read(STDIN_FILENO, &c ,1) == 1 && c!= 'q' );
    return 0;
}