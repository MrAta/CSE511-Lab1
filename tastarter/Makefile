all: server client client2 client3 client4

server: server.c server.h
	gcc -g -std=gnu99 server.c server.h -o server -pthread

client: client.c
	gcc -g -std=gnu99 client.c -o client -pthread

client2: client2.c
	gcc -g -std=gnu99 client2.c -o client2 -pthread

client3: client3.c
	gcc -g -std=gnu99 client3.c -o client3 -pthread

client4: client4.c
	gcc -g -std=gnu99 client4.c -o client4 -pthread

clean:
	rm server client client2 client3 client4


