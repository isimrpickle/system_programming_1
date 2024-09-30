CC = gcc
CFLAGS = -Wall  

.PHONY: all clean

all: jobExecutorServer jobCommander

jobExecutorServer: jobExecutorServer.c
	$(CC) $(CFLAGS) -o $@ $<

jobCommander: jobCommander.c
	$(CC) $(CFLAGS) -o $@ $<

run: jobCommander
	./jobCommander

clean:
	rm -f jobExecutorServer jobCommander jobExecutorServer.txt Commander_to_Server Server_to_Commander
