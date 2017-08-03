# build executables from respective c++ files

default: all edge


.PHONY: edge
edge: edge
	./edge

.PHONY: server_or
server_or: server_or
	./server_or

.PHONY: server_and
server_and: server_and
	./server_and


all: edge.cpp client.cpp server_or.cpp server_and.cpp
	g++ -o client client.cpp
	g++ -o edge edge.cpp
	g++ -o server_or server_or.cpp
	g++ -o server_and server_and.cpp
