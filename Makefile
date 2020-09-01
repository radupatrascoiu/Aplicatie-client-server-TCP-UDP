CC=g++ -std=c++11 -Wall -Wextra -g

build: server subscriber

server: helper_server.o server.o
	$(CC) $^ -o $@

server.o: server.cpp
	$(CC) $^ -c

helper_server.o: helper_server.cpp
	$(CC) $^ -c

subscriber: helper_subscriber.o subscriber.o
	$(CC) $^ -o $@

helper_subscriber.o: helper_subscriber.cpp
	$(CC) $^ -c

subscriber.o: subscriber.cpp
	$(CC) $^ -c

clean:
	rm *.o server subscriber
