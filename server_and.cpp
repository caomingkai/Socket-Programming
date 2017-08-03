#include <sstream>
#include <iostream>
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


using std::string;
using std::vector;
using std::cin;
using std::cout;
using std::endl;

const char* LOCAL_HOST = "127.0.0.1";
const int TYPE_UDP = SOCK_DGRAM;
const unsigned short PORT_CLIENT = 24244;
const unsigned short PORT_SERVER = 22244;
const int    BUFSIZE  = 2048;


template <typename T>
std::string to_string(T value){
    std::ostringstream os;
    os << value;
    return os.str();
}

string clacResult(string msgEntry);


int main(){
    
    /*---------------------------------------
      Create UDP socket
     -----------------------------------------*/
    int socket_and = socket(AF_INET, TYPE_UDP, 0);// Beej code
    
    if ( socket_and < 0) {
        perror("socket_and cannot be created");
        close(socket_and);
        return 0;
    }

    /*---------------------------------------
       Bind socket with address and port
     -----------------------------------------*/
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    server_addr.sin_port = htons(PORT_SERVER);

    int bindFlag = bind(socket_and, (struct sockaddr*)&server_addr, sizeof(server_addr));// Beej code
    if (bindFlag < 0) {
        perror("socket_and cannot be bound ");
        close(socket_and);
        return 0;
    }else{
        cout<<"The Server AND is up and running using UDP on port "
            <<PORT_SERVER <<endl;
        cout<<"The Server AND start receiving lines from the edge server for AND computation. The computation results are:"
        <<endl;
    }
    
    
    /*---------------------------------------
     Keep receiving data
     Once receive an entry:
        - store the entry into the inbound data
        - calculate the result
        - store the result into the outbound data
     -----------------------------------------*/
    struct sockaddr_in client_addr;             // remote address
    socklen_t addrlen = sizeof(server_addr);    // length of addresses
    char* msgEntry = new char[BUFSIZE];         // receive buffer
    int lineTotal = 0;
    vector<string> dataIn;
    vector<string> dataOut;
    
    while (true) {
        
        int recvFlag = recvfrom(socket_and , msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
        if(recvFlag < 0 ){
            perror("socket_and cannot be bound ");
            close(socket_and);
            return 0;
        }
        if ( *msgEntry == '#' ) { // '#' from Edge:  means EOF
            string ack = "A";     // tell Edge: I receive your EOF
            sendto(socket_and ,ack.c_str(), BUFSIZE,
                   0, (struct sockaddr *)&client_addr, addrlen );// Beej code
            cout<< "The Server AND has successfully received "
                << to_string(lineTotal)
                << " lines from the edge server and finished all AND computations."
                <<endl;
        }else if( *msgEntry == '<' ){             // '<' from Edge: request result
            for (int i = 0; i < lineTotal; i++) { // send result to Edge
                string finalEntry = dataOut[i];
                sendto(socket_and ,finalEntry.c_str(), BUFSIZE,
                       0, (struct sockaddr *)&client_addr, addrlen );// Beej code
                
                // block next send() until receive 'E' from Edge
                int flgR = recvfrom(socket_and , msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
                if ( *msgEntry != 'E') {
                    cout<< to_string(i)<<"th Entry has no ACK from Edge";
                }
            }
            // 'a' to Edge: I have sent all my result
            sendto(socket_and ,"a", 1, 0, (struct sockaddr *)&client_addr, addrlen );// Beej code
            int flgR = recvfrom(socket_and , msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
            if ( *msgEntry != 'e') {
                cout<< "Last Result has no ACK from Edge";
            }
            cout << "The Server AND has successfully finished sending all computation results to the edge server." << endl;
            lineTotal = 0;  // There 3 lines IMPORTANT:
            dataIn.clear(); // after finishing this UDP task, reset
            dataOut.clear();// for next UDP communication
        }else{
            dataIn.push_back ( string(msgEntry) );  // inbound data
            string result = clacResult( msgEntry); // calculate data
            dataOut.push_back( result );            // outbound data
            lineTotal++;
            // until finishing all operation, send ACK to Edge
            // setting aside enough time to process current entry
            string ack = "A";
            sendto(socket_and ,ack.c_str(), BUFSIZE,
                   0, (struct sockaddr *)&client_addr, addrlen );// Beej code
        }
    }
}




/*---------------------------------------
    Calculate inbound data
        - segment data into operator and operand
        - calculate the result
 -----------------------------------------*/
string clacResult(string msgEntry){
    int i = 0;
    string fields[4];
    string buf;
    std::istringstream ss(msgEntry);
    while(getline(ss, buf, ',')){
        fields[i] = buf;
        i++;
    }
    
    int lengthOne = (int)fields[1].size();
    int lengthTwo = (int)fields[2].size();
    int lengthDif = lengthTwo - lengthOne > 0 ? lengthTwo - lengthOne : lengthOne - lengthTwo ;
    int lengthFnl = lengthOne > lengthTwo ? lengthOne :lengthTwo;
    int resultFnl[lengthFnl];
    
    for( int i = lengthFnl - 1 ; i >= lengthDif; i--){
        if(lengthOne > lengthTwo){
            if( fields[1].at(i) == '1' && fields[2].at(i-lengthDif) == '1'){
                resultFnl[i] = 1;
            }else{
                resultFnl[i] = 0;
            }
        }else{
            if( fields[1].at(i-lengthDif) == '1' && fields[2].at(i) == '1'){
                resultFnl[i] = 1;
            }else{
                resultFnl[i] = 0;
            }
        }
    }

    string resultFnlStr = "";
    for(int i = lengthDif; i <= lengthFnl - 1; i++){
        resultFnlStr += to_string(resultFnl[i]);
    }
    if(resultFnlStr.size() == 0){
        resultFnlStr = "0";
    }
    resultFnlStr = fields[1] + " and " + fields[2] + " = " + resultFnlStr ;
    cout << resultFnlStr << endl;
    
    //  1111 and 0000 = 0000,5
    return resultFnlStr + "," + fields[3];
}



