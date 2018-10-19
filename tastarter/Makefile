all: server server_new client client2 client3 client4

server: server.c server.h
	gcc -g -std=gnu99 server.c server.h -o server -pthread

CFLAGS = -c -std=gnu99 -Wall -pthread -ggdb
LFLAGS = -std=gnu99 -Wall -ggdb
CC = gcc
DEPS = db.o cache.o
SERVERS = server_main.o
SERVERS += server-part1.o
#SERVERS += server-part2.c
#SERVERS += server-part3.c

server_new: $(SERVERS) $(DEPS)
	$(CC) $(LFLAGS) $^ -o $@

server_main.o: server_main.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	gcc -g -std=gnu99 client.c -o client -pthread

client2: client2.c
	gcc -g -std=gnu99 client2.c -o client2 -pthread

client3: client3.c
	gcc -g -std=gnu99 client3.c -o client3 -pthread

client4: client4.c
	gcc -g -std=gnu99 client4.c -o client4 -pthread

clean:
	rm server client client2 client3 client4 *.o server_new


