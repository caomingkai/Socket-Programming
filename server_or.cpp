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
// parameters predefine
const char* LOCAL_HOST = "127.0.0.1";
const int TYPE_UDP = SOCK_DGRAM;
const unsigned short PORT_CLIENT = 24244;
const unsigned short PORT_SERVER = 21244;
const int    BUFSIZE  = 2048;

string clacResult(string msgEntry);


template <typename T> // cpp forum code
std::string to_string(T value){
    std::ostringstream os;
    os << value;
    return os.str();
}


int main(){
    
    /*  create socket
     */
    int socket_or = socket(AF_INET, TYPE_UDP, 0);// Beej code
    
    // if fail, return
    if ( socket_or < 0) {
        perror("socket_or cannot be created");
        close(socket_or);
        return 0;
    }
    
    /*    bind socket and address
     */
    
    // create server_addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    server_addr.sin_port = htons(PORT_SERVER);
    // bind server_addr
    int bindFlag = bind(socket_or, (struct sockaddr*)&server_addr, sizeof(server_addr));// Beej code
    
    if (bindFlag < 0) {
        perror("socket_or cannot be bound ");
        close(socket_or);// Beej code
        return 0;
    }else{
        cout<<"The Server OR is up and running using UDP on port "
        <<PORT_SERVER <<endl;
        cout<<"The Server OR start receiving lines from the edge server for OR computation. The computation results are:"
        <<endl;
    }
   
    /* receive data
     once receive an entry:
     - store the entry into the inbound data
     - calculate the result
     - store the result into the outbound data
     */
    struct sockaddr_in client_addr;     /* remote address */
    socklen_t addrlen = sizeof(server_addr);  /* length of addresses */
    unsigned char buf[BUFSIZE];         /* receive buffer */
    int lineTotal = 0;
    vector<string> dataIn;
    vector<string> dataOut;
    char* msgEntry = new char[BUFSIZE];
    
    while (true) {
        
        
        int recvFlag = recvfrom(socket_or, msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
        if(recvFlag < 0 ){
            perror("socket_or cannot be bound ");
            close(socket_or);
            return 0;
        }
        if ( *msgEntry == '#' ) { // '#' from Edge:  means EOF
            string ack = "O";  // ack Edge that I receive your EOF
            sendto(socket_or ,ack.c_str(), BUFSIZE,
                   0, (struct sockaddr *)&client_addr, addrlen );// Beej code
            cout<< "The Server OR has successfully received "
            << to_string(lineTotal)
            << " lines from the edge server and finished all OR computations."
            <<endl;
        }else if( *msgEntry == '<' ){ // '<': Edge request to sent back result
            for (int i = 0; i < lineTotal; i++) {
//
                string finalEntry = dataOut[i];
                sendto(socket_or ,finalEntry.c_str(), BUFSIZE,
                       0, (struct sockaddr *)&client_addr, addrlen );// Beej code
                int flgR = recvfrom(socket_or , msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
                if ( *msgEntry != 'E') { // indicate Edge has received last result
                    cout<< to_string(i)<<"th Entry has no ACK from Edge";
                }
            }
            // 'a': tell Edge 'I have sent all result'
            sendto(socket_or ,"o", 1, 0, (struct sockaddr *)&client_addr, addrlen );// Beej code
            int flgR = recvfrom(socket_or , msgEntry, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addrlen);// Beej code
            if ( *msgEntry != 'e') {
                cout<< "Last result has no ACK from Edge";
            }
            cout << "The Server OR has successfully finished sending all computation results to the edge server." << endl;
            lineTotal = 0;
            dataIn.clear();
            dataOut.clear();
            
        }else{
            dataIn.push_back ( string(msgEntry) );
            dataOut.push_back( clacResult( msgEntry ));
            lineTotal++;
            // send ACK to Edge to use receive to block next sent from Edge, setting aside enough time to process current entry
            string ack = "O";  // ack Edge that I receive your entry
            sendto(socket_or ,ack.c_str(), BUFSIZE,
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
            if( fields[1].at(i) == '0' && fields[2].at(i-lengthDif) == '0'){
                resultFnl[i] = 0;
            }else{
                resultFnl[i] = 1;
            }
        }else{
            if( fields[1].at(i-lengthDif) == '0' && fields[2].at(i) == '0'){
                resultFnl[i] = 0;
            }else{
                resultFnl[i] = 1;
            }
        }
    }
    
    for( int i = 0 ; i < lengthDif; i++){   // deal with first half '00000'
        if(lengthOne > lengthTwo){
            resultFnl[i] = (int)(fields[1].at(i)-48);
        }else{
            resultFnl[i] = (int)(fields[2].at(i)-48);
        }
    }
    
    
    string resultFnlStr = "";
    for(int i = 0; i <= lengthFnl - 1; i++){
        resultFnlStr += to_string(resultFnl[i]);
    }
    if(resultFnlStr.size() == 0){
        resultFnlStr = "0";
    }
    resultFnlStr = fields[1] + " or " + fields[2] + " = " + resultFnlStr;
    cout << resultFnlStr << endl;
    
    //  1111 or 0000 = 1111,8
    return resultFnlStr + "," + fields[3];
}



