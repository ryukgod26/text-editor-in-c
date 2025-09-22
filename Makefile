CC = gcc
CFLAGS = -Wall -Wextra -pedantic

all: kilo

ifeq ($(OS), Windows_NT)
    TARGET = kilo.exe
    SRC = kilo.c
else
    TARGET = kilo.out
    SRC = kiloLinux.c
endif


kilo: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f kilo.exe kilo.out


