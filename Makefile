all: server client client2 client3 client4

server: server.c server.h
	gcc -g server.c server.h -o server -pthread

client: client.c
	gcc -g client.c -o client -pthread

client2: client2.c
	gcc -g client2.c -o client2 -pthread

client3: client3.c
	gcc -g client3.c -o client3 -pthread

client4: client4.c
	gcc -g client4.c -o client4 -pthread

clean:
	rm server client client2 client3 client4


