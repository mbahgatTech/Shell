CFLAGS = -Wpedantic -std=gnu99
CC = gcc

myShell: myShell.h myShell.c
	$(CC) $(CFLAGS) myShell.c -o myShell

clean:
	rm myShell