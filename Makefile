all: clean client server

client:
	gcc -Wno-int-to-pointer-cast -Wno-implicit-function-declaration -o client client.c

server:
	gcc -lm -pthread -Wno-sizeof-array-argument -Wno-int-conversion -Wno-int-to-pointer-cast -Wno-implicit-function-declaration -o server server.c

.PHONY: clean

clean:
	rm -f client
	rm -f server
