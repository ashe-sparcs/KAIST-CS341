all: test1 test2

test1 : client.c
	gcc -g -Wall -o client client.c	
		
test2 : server.c
	gcc -g -Wall -o server server.c

clean: 
	rm client
	rm server
