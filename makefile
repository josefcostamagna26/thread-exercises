CC=gcc 
CFLAGS= -std=c89 -pedantic
TARGET=master

$(TARGET) : master.o nodo.o utente.o libroMastro.o
	$(CC) -o master *.o

nodo.o : nodo.c
	$(CC) $(CFLAGS) -c nodo.c

utente.o : utente.c
	$(CC) $(CFLAGS) -c utente.c

libroMastro.o : libroMastro.c
	$(CC) $(CFLAGS) -c libroMastro.c

master.o : master.c 
	$(CC) $(CFLAGS) -c master.c

clean : 
	rm -f *.o

run :
	./$(TARGET)
