#define _DEFAULT_SOURCE

#include<termios.h>
#include<unistd.h>

struct termios orig_termios;

void disableRawMode(){
tcsetattr(STDIN_FILENO , TCAFLUSH , &orig_termios);
}

void enableRawMode(){
//struct termios raw;

tcgetattr(STDIN_FILENO,&orig_termios);
disableRawMode();

struct termios raw = orig_termios;
raw.c_lflag &= ~(ECHO);

tcsetattr(STDIN_FILENO, TCAFLUSH , &raw);

}


int main(){

enableRawMode();

char c;

while(read(STDIN_FILENO, &c , 1) == 1 && c != 'q');

return 0;

}
