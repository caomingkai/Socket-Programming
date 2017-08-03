#include <sstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <vector>

using std::size_t;
using std::cout;
using std::endl;
using std::string;
using std::vector;


const char* LOCAL_HOST = "127.0.0.1";
const unsigned short PORT_TCP = 23244;
const unsigned short PORT_UDP = 24244;
const unsigned short PORT_UDP_AND = 22244;
const unsigned short PORT_UDP_OR = 21244;
const int TYPE_TCP = SOCK_STREAM;
const int TYPE_UDP = SOCK_DGRAM;
const int MAX_CONNECTION = 5;
const int    BUFSIZE  = 2048;


template <typename T>
std::string to_string(T value){
    std::ostringstream os;
    os << value;
    return os.str();
}

int str2int(const string &str){
    std::stringstream ss(str);
    int num;
    ss >> num;
    return num;
}


int main() {
    
    /*---------------------------------------
     Create TCP socket
     -----------------------------------------*/
    // Create socket
    int socket_tcp = socket(AF_INET, TYPE_TCP, 0);
    if (socket_tcp < 0) {
        perror("socket_edge cannot be created");
        close(socket_tcp);
        return 0;
    }

    // Create local address for TCP
    struct sockaddr_in tcp_addr;
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    tcp_addr.sin_port = htons(PORT_TCP);
    
    /*---------------------------------------
      Bind the socket with the selected port
     -----------------------------------------*/
    int bindFlag = ::bind(socket_tcp, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr));
    if (bindFlag < 0) {
        perror("socket_edge cannot be bound ");
        close(socket_tcp);
        return 0;
    }
    
    cout << "The edge server is up and running." << endl;
    
    /*---------------------------------------
      Listen connections
     -----------------------------------------*/
    int listenFlag = listen(socket_tcp, MAX_CONNECTION);
    
    
    
    /*---------------------------------------
      1. Keep accepting TCP connections from client
      2. Receive data via TCP
      3. Process data
      4. Send 'AND' data to SERVER_AND via UDP
      5. Send 'OR' data to SERVER_OR   via UDP
      6. Call for result from SERVER_AND via UDP
      7. Call for result from SERVER_OR  via UDP
      8. Reassembly the final result
      9. Send final result to client via TCP
     -----------------------------------------*/
    while(true){
        socklen_t len = sizeof(tcp_addr);// Beej code
        int socket_server = accept(socket_tcp, (struct sockaddr *)&tcp_addr, &len);

        if (socket_server < 0) {
            perror("socket_edge cannot be bound ");
            close(socket_server);
            return 0;
        }
        
        // Receiving data from the client
        vector<string> data;
        int lineTotal = 0;
        bool receive_done = false;
        while (!receive_done) {
            char* msgEntry = new char[BUFSIZE];
            recv(socket_server, msgEntry, BUFSIZE, 0);
            if (*msgEntry != '#') { // ending character
                data.push_back(string(msgEntry));
                lineTotal++;
            } else {
                receive_done = true;
            }
        }
        
        cout << "The edge server has received " << lineTotal
        << " lines from the client using TCP over port " << PORT_TCP << "." << endl;
        
        
        
        // --------- Create UDP socket
        int socket_udp = socket(AF_INET, TYPE_UDP, 0);// Beej code
        if (socket_udp < 0) {
            perror("socket_edge cannot be created");
            close(socket_udp);
            return 0;
        }

        struct sockaddr_in udp_addr;
        udp_addr.sin_family = AF_INET;
        udp_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
        udp_addr.sin_port = htons(PORT_UDP);
        
        // --------- Bind UDP socket
        int bindFlag = bind(socket_udp, (struct sockaddr *)&udp_addr, sizeof(udp_addr));// Beej code
        if (bindFlag < 0) {
            perror("socket_edge cannot be bound ");
            close(socket_udp);
            return 0;
        }
        

        // --------- Process data
        // --------- Send data to two servers via UDP
        int line_and = 0;
        int line_or = 0;
        
        // SERVER_OR UDP address and port number
        struct sockaddr_in udp_or_addr;// Beej code
        udp_or_addr.sin_family = AF_INET;
        udp_or_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
        udp_or_addr.sin_port = htons(PORT_UDP_OR);
        socklen_t len_or = sizeof(udp_or_addr);
        char* resultAnd = new char[BUFSIZE];
        
        // SERVER_AND UDP address and port number
        struct sockaddr_in udp_and_addr;// Beej code
        udp_and_addr.sin_family = AF_INET;
        udp_and_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
        udp_and_addr.sin_port = htons(PORT_UDP_AND);
        socklen_t len_and = sizeof(udp_and_addr);
        char* resultOr = new char[BUFSIZE];
        
        // send data entries accordingly based on 'and' / 'or'
        for (int i = 0; i < data.size(); i++) {
            
            // obtain ith entry, suffix with sequencing num
            string entrySeq = data[i] + "," + to_string(i);
            char first_char = data[i].at(0);
            
            // check 'and' , 'or' via 1st character
            // send out to two backend servers
            if (first_char == 'a') {
                line_and++;
                sendto(socket_udp, entrySeq.c_str(), BUFSIZE, 0, (struct sockaddr *)&udp_and_addr, len_and);// Beej code
                
                // block next send() until ACK from SERVER_AND
                recvfrom(socket_udp, resultAnd, BUFSIZE, 0, (struct sockaddr *)&udp_and_addr, &len_and);// Beej code
                if( *resultAnd != 'A'){
                    cout<<to_string(i)<<"th entry has no ACK from SERVER_AND";
                }
            } else {
                line_or++;
                sendto(socket_udp, entrySeq.c_str(), BUFSIZE, 0, (struct sockaddr *)&udp_or_addr, len_or);
                // block next send() until ACK from SERVER_OR
                recvfrom(socket_udp, resultOr, BUFSIZE, 0, (struct sockaddr *)&udp_or_addr, &len_or);// Beej code
                if( *resultOr != 'O'){
                    cout<<to_string(i)<<"th entry has no ACK from SERVER_OR";
                }
            }
        }
        
        // tell SERVER_AND sending is done with ending signal "#"
        sendto(socket_udp, "#", 1, 0, (struct sockaddr *)&udp_and_addr, len_and);
        recvfrom(socket_udp, resultAnd, BUFSIZE, 0, (struct sockaddr *)&udp_and_addr, &len_and);// Beej code
        if( *resultAnd != 'A'){cout<<"the EOF signal '#' has no ACK from SERVER_AND";}
        cout << "The edge has successfully sent " + to_string(line_and) + " lines to Backend-Server AND." << endl;
        
        // tell SERVER_OR sending is done with ending signal "#"
        sendto(socket_udp, "#", 1, 0, (struct sockaddr *)&udp_or_addr, len_or);
        recvfrom(socket_udp, resultOr, BUFSIZE, 0, (struct sockaddr *)&udp_or_addr, &len_or);// Beej code
        if( *resultOr != 'O'){cout<<"the EOF signal '#' has no ACK from SERVER_OR";}
        cout << "The edge has successfully sent " + to_string(line_or) + " lines to Backend-Server OR." << endl;
        

        // --------- Receive result via UDP
        cout << "The edge server start receiving the computation results from Backend-Server OR "
        "and Backend-Server AND using UDP over port " + to_string(PORT_UDP) << endl;
        cout << "The computation results are:" << endl;
        
        vector<string> resultAndData;
        vector<string> resultOrData;
        vector<int> resultAndIndex;
        vector<int> resultOrIndex;
        string resultAndFields[2];
        string resultOrFields[2];
        int test_num_add = 0;
        int test_num_or = 0;
        
        // ---------  Call for result from SERVER_AND
        // ---------  send '<' to it saying: give me result
        sendto(socket_udp, "<", 1, 0, (struct sockaddr *)&udp_and_addr, len_and);
        do {
            
            recvfrom(socket_udp, resultAnd, BUFSIZE, 0, (struct sockaddr *)&udp_and_addr, &len_and);// Beej code
            if ( *resultAnd == 'a') { // receive EOF 'a'; give back a ACK 'e'
                sendto(socket_udp, "e", 1, 0, (struct sockaddr *)&udp_and_addr, len_and);         // Beej code
            }else{                    // receive regular result
                int i = 0;
                string buf;
                std::istringstream ss(resultAnd);
                while(getline(ss, buf, ',')){
                    resultAndFields[i] = buf;
                    i++;
                }
                cout<< resultAndFields[0] <<endl;
                resultAndData.push_back( resultAndFields[0] );
                resultAndIndex.push_back( str2int(resultAndFields[1]) );
                sendto(socket_udp, "E", 1, 0, (struct sockaddr *)&udp_and_addr, len_and);// Beej code
                test_num_add++;
            }
        } while ( *resultAnd != 'a');
        

        // ---------  Call for result from SERVER_OR
        // ---------  send '<' to it saying: give me result
        sendto(socket_udp, "<", 1, 0, (struct sockaddr *)&udp_or_addr, len_or);
        do {
            recvfrom(socket_udp, resultOr, BUFSIZE, 0, (struct sockaddr *)&udp_or_addr, &len_or);// Beej code
            if ( *resultOr == 'o') {  // receive EOF 'a'; give back a ACK 'e'
                sendto(socket_udp, "e", 1, 0, (struct sockaddr *)&udp_or_addr, len_or);          // Beej code
            }else{                    // receive regular resultback 'E'
                int i = 0;
                string buf;
                std::istringstream ss(resultOr);
                while(getline(ss, buf, ',')){
                    resultOrFields[i] = buf;
                    i++;
                }
                cout<< resultOrFields[0] <<endl;
                resultOrData.push_back( resultOrFields[0] );
                resultOrIndex.push_back( str2int(resultOrFields[1]) );
                sendto(socket_udp, "E", 1, 0, (struct sockaddr *)&udp_or_addr, len_or);// Beej code
                test_num_or++;
            }
        } while ( *resultOr != 'o');
        
        // ---------  Finish receiving all result
        // ---------  close socket
        close(socket_udp);// Beej code
        cout << "The edge server has successfully finished receiving all computation "
        "results from Backend-Server OR and Backend-Server And." << endl;
        
        
        // --------- Merge result from backend servers
        vector<string> resultFnl;
        int andLength = resultAndIndex.size();
        int orLength  =  resultOrIndex.size();
        int i = 0;
        int j = 0;

        while( i < andLength && j < orLength ){
            if( resultAndIndex[i] <= resultOrIndex[j] ){
                resultFnl.push_back( resultAndData[i] );
                i++;
            }else{
                resultFnl.push_back( resultOrData[j] );
                j++;
            }
        }
        
        while( i < andLength ){
            resultFnl.push_back( resultAndData[i] );
            i++;
        }
        
        while( j < orLength ){
            resultFnl.push_back( resultOrData[j] );
            j++;
        }
        
        int length = andLength + orLength;
        vector<string> resultOnly;
        string resultFields[2];
        
        for ( int i=0; i < length ; i++ ) {
            int j = 0;
            string buf;
            std::istringstream ss( resultFnl[i] );
            while(getline(ss, buf, '=')){
                resultFields[j] = buf;
                j++;
            }
            resultOnly.push_back( resultFields[1] );
        }
        
        
        
        /*-------------------------------------------------------
         TCP: Send Results back to Client
         ------------------------------------------------------*/
        for (int i = 0; i < length; i++) {
            send(socket_server, resultOnly[i].c_str(), BUFSIZE, 0);// Beej code
//            cout << resultFnl[i] << endl;
        }
        
        cout << "The edge server has successfully finished sending all computation results to the client." << endl;

    }
    
}



