#include <sstream>
#include <iostream>
#include <fstream>
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

using std::ifstream;
using std::string;
using std::vector;
using std::cin;
using std::cout;
using std::endl;

// parameters predefine
const char* LOCAL_HOST = "127.0.0.1";
const int TYPE_TCP = SOCK_STREAM;
const unsigned short PORT_SERVER = 23244;
const int    BUFSIZE  = 2048;


int main(int argc, char *argv[]){
    
    /* ---------------------------------
     Read file
    ---------------------------------*/
    vector<string> data;
    bool inputOK = false;
    
    if (argc != 2 ) {
        cout << "Please input program name followed by file name, with space seperated." << endl;
        return 0;
    }else{
        ifstream fileReader( argv[1] );
        if (!fileReader.is_open() ) {
            cout << " Could not open file \n";
            return 0;
        }else{
            string line;
            int i = 0;
            while ( !fileReader.eof() ) {
                getline(fileReader, line);
                if(line != ""){    // trim blank lines
                    data.push_back(line);
                }
            }
            fileReader.close();
        }
    }
    
    
    /*--------------------------------------------
     Create socket
     --------------------------------------------*/
    int socket_client = socket(AF_INET, TYPE_TCP, 0);// Beej code
    
    if ( socket_client < 0) { // if fail, return
        perror("socket_client cannot be created");
        return 0;
    }
    
    /*--------------------------------------------
     Connect to Edge Server
     --------------------------------------------*/
    struct sockaddr_in edge_addr;
    edge_addr.sin_family = AF_INET;
    edge_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    edge_addr.sin_port = htons(PORT_SERVER);
    
    int connectFlag = connect(socket_client, (struct sockaddr*)&edge_addr, sizeof(edge_addr));// Beej code
    
    if (connectFlag < 0) {
        perror("socket_client cannot connect to Edge Server ");
        return 0;
    }
    cout << "The client is up and running. \n";
    
    
    /*-------------------------------------------- 
     Send data to Edge Server
     --------------------------------------------*/
    int numEntry = data.size();
    for (int i = 0; i < numEntry; i++) {
        send(socket_client, data[i].c_str(), BUFSIZE,
               0);// Beej code
    }
    // tell Edge Server sending process is done with '#'
    send(socket_client, "#", 1, 0);
    cout << "The client has successfully finished sending " << numEntry 
         <<" lines to the edge server." << endl;
    
    /*--------------------------------------------
     Receive data from Edge Server
     -------------------------------------------- */
    vector<string> dataRecv;
    
    for ( int i=0 ; i < numEntry ; i++ ) {
        char* buffer = new char[BUFSIZE];
        recv(socket_client, buffer, BUFSIZE , 0 );// Beej code
        dataRecv.push_back( string(buffer) );
    }
    
    cout << "The client has successfully finished receiving all computation results from the edge server." << endl;
    
    cout << "The final computation results are:" << endl;
    for (int i = 0; i < numEntry; i++) {
        cout << dataRecv[i] << endl;
    }
    
    close(socket_client);
    return 0;
}
    
