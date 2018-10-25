all: server client client2 client3 client4 client5

server: server-part3 server-part2 server-part1 deps
	gcc -g -c -std=gnu99 server-main.c -o server-main.o -pthread
	gcc -g -std=gnu99 server-main.o server-part1.o server-part2.o server-part3.o db.o cache.o -o server -pthread

server-part3:
	gcc -g -c -std=gnu99 server-part3.c -o server-part3.o -pthread

server-part2:
	gcc -g -c -std=gnu99 server-part2.c -o server-part2.o -pthread

server-part1:
	gcc -g -c -std=gnu99 server-part1.c -o server-part1.o -pthread

deps:
	gcc -g -c -std=gnu99 db.c -o db.o
	gcc -g -c -std=gnu99 cache.c -o cache.o


client: client.c
	gcc -g -std=gnu99 client.c -o client -pthread -lm

client2: client2.c
	gcc -g -std=gnu99 client2.c -o client2 -pthread

client3: client3.c
	gcc -g -std=gnu99 client3.c -o client3 -pthread -lrt

client4: client4.c
	gcc -g -std=gnu99 client4.c -o client4 -pthread

client5: client5.c
	gcc -g -std=gnu99 client5.c -o client5 -pthread

clean:
	rm server client client2 client3 client4 client5 *.o
