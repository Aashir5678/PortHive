CC = gcc
CFLAGS = -Wall -pthread

honeypot: main.o server.o flask_client.o
	$(CC) $(CFLAGS) main.o server.o flask_client.o -o honeypot

main.o: main.c server.h
	$(CC) $(CFLAGS) -c main.c -o main.o

server.o: server.c server.h flask_client.h
	$(CC) $(CFLAGS) -c server.c -o server.o

flask_client.o: flask_client.c flask_client.h
	$(CC) $(CFLAGS) -c flask_client.c -o flask_client.o

clean:
	rm -f *.o honeypot