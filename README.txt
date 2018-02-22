a.
b.

c. Implemented:
	- TCP connection between a Client and an Edge Server;
	- UDP connections between an Edge Server and two backend servers.
	- AND/OR computations at backend servers

d. Files:
	- client.cpp
		+ Read working file into a vector, with blank lines trimmed
		+ Create a TCP socket, and connect to Edge Server
		+ Send file data to Edge Server, and suffix with ending character ‘#’
		+ Receive computation result from Edge Server
		+ Display necessary information
		+ Close socket

	- edge.cpp
		+ Create TCP socket, bind it to its own address, and listen to it
		+ Keep accept TCP connections
		+ Receive data from client via TCP
		+ Sequence data
		+ Create UDP 
		+ Send ‘AND’/ ’OR’ data to backend servers via UDP
		+ Call for result from backend server via UDP
		+ Merge result in previous order
		+ Send back final result to client via TCP

	- server_and.cpp
		+ Create UDP socket, bind it
		+ Keep receive data from Edge
		+ Calculate data
		+ Wait until Edge tell it to send back result

	- server_or.cpp
		+ Create UDP socket, bind it
		+ Keep receive data from Edge
		+ Calculate data
		+ Wait until Edge tell it to send back result

e. None

f. Format of messages exchanged:
	- Client to Edge
		+ use a loop to send each entry as it is， like “and,11,00”
		+ append a last ending signal “#”

	- Edge to Client: 
		+ use a loop to send each result entry
		+ like: “00”
	
	- Edge to Server_And
		+ use a loop to send precessed entry
		+ append data with sequence number,like “and,11,00,3”, “or,11,00,3”
		+ wait until server_and send back an ACK, and then send next entry
		+ this can set aside time for Server_and_or to do calculation.
		+ append an “#” to indicate no data to be sent

	- Server_And／Server_And to Edge
		+ use a loop to send result
		+ consist of data,result,sequence number, like “1111 and 0000 = 0000,5”
		+ wait until Edge send back an ACK, and then send next result entry
		+ this can set aside time for Edge to precess the result entry

g. none

h. Beej’s computer networking 
	- socket()
	- listen()
	- connect()
	- send()
	- receive()
	- sendto()
	- recvfrom()

i. From this project, I have searched a lot, and learned a lot.
   It is totally a great project!
   I have to say GOOD JOB!



