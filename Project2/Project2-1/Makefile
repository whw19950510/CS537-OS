CC = gcc
CFLAGS = -g
%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

mysh: mysh.o 
	gcc -o mysh mysh.o
