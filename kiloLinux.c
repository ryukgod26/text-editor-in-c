#define _DEFAULT_SOURCE
#define CTRL_KEY(k) ((k) & 0x1f)


#include<termios.h>
#include<unistd.h>
#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<errno.h>

struct termios orig_termios;

void die(char* s){
    perror(s);
    exit(EXIT_FAILURE);
}

void disableRawMode(){
if(tcsetattr(STDIN_FILENO , TCSAFLUSH , &orig_termios) == -1)
die("disableRow");
}


char editorReadKey()
{
int nread;
char c;
while( (nread = read(STDIN_FILENO,&c,1)) != -1){
if(nread == -1 && errno != EAGAIN) die("read");
}	
return c;
}

void editorProcessKeyprocess()
{
char c = editorReadKey();
switch(c)
{
	case CTRL_KEY('q'):
		exit(0);
		break;
}
}

void editorRefreshScreen()
{
write(STDIN_FILENO,"\x1b[2J",4);
}
void enableRawMode(){
//struct termios raw;

if (tcgetattr(STDIN_FILENO,&orig_termios) == -1) die("RawModetcgetattr");
atexit(disableRawMode);

struct termios raw = orig_termios;
raw.c_iflag &= ~(BRKINT | IXON | ICRNL | INPCK | ISTRIP);
// raw.c_iflag &= ~(IXON | ICRNL);
raw.c_oflag &= ~(OPOST);
raw.c_cflag |= CS8;
raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
raw.c_cc[VMIN] = 0;
raw.c_cc[VTIME] = 1;

if(tcsetattr(STDIN_FILENO, TCSAFLUSH , &raw) == -1 ) die("RawModetcsetattr");
}

int main(){

enableRawMode();

while(1){
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
