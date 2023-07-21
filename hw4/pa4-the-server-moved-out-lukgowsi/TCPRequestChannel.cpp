#include "TCPRequestChannel.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


using namespace std;


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    // if server
    //      create a socket on the specified
    //          - specity domain, type, and protocol
    //      bind the socket to addr set-ups listening
    //      connect socket to the IP addr of the server
    if(_ip_address == ""){
        //set up variables
        int status = 0;

        struct addrinfo hints;
        struct addrinfo *servinfo;

        memset(&hints, 0, sizeof hints);

        //AF_INET = IPv4
        //SOCK_STREAM = TCP
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        //socket - make socket - socket(int domain, int type, int protocol)
        status = getaddrinfo(NULL, _port_no.c_str(), &hints, &servinfo);

        //Normally only a single protocol exists to support a particular socket type
        //within a given protocol family, in which case protocol can be specified as 0
        if(status != 0){
            cout << "no get address info" << endl;
            exit(1);
        }
    
        //provide necessary machine info for sockaddr_in
        //address family, IPv4
        //IPv4 address, use current IPv4 address (INADDR_ANY)
        //convert short form host byte order to network byte order
        sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        
        //bind - assign address to socket - bind(int sockfd, const sturct sockaddr *addr, socklen_t addrlen)
        bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

        //listen - listen for client - linetn(intsockfd, int backlog)
        int backlog = 10;
        listen(sockfd, backlog);

        //accept - accept connection
        //written in a separate method

        freeaddrinfo(servinfo);
    } 
    // if client
    //      create a socket on the specified
    //          -specify domain, type, and protocol
    //      connect socket to the IP addr of the server
    else{
        int status = 0;

        struct addrinfo hints;
        struct addrinfo *servinfo;

        memset(&hints, 0, sizeof hints);

        //AF_INET = IPv4
        //SOCK_STREAM = TCP
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        //socket - make socket - socket(int domain, int type, int protocol)
        status = getaddrinfo(_ip_address.c_str(), _port_no.c_str(), &hints, &servinfo);

        if(status < 0){
            cout << "no get address info" << endl;
            exit(1);
        }

        //generate server's info based on parameters
        //address family, IPv4
        //connection port
        //connect short from host byte order to network byte order
        //convert ip address c-string to binary representation for sin_addr
        sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

        //connect - connect to listening socket - connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen)
        connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        
        freeaddrinfo(servinfo);
    }
}

TCPRequestChannel::TCPRequestChannel (int _sockfd) {
    //assign an existing socket to object's socket file descriptor
    this->sockfd = _sockfd;
}

TCPRequestChannel::~TCPRequestChannel () {
    // close the sockfd - close(this->sockfd)

    close(this->sockfd);
}

int TCPRequestChannel::accept_conn() {
    // struct sockaddr_storage
    // implementing accept(...) - retval the sockfd of client

    // accept - accept condition
    // socket file descriptor for accepted connection
    // accept connection - accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
    // return socket file descriptor

    //make socket storage
	struct sockaddr_storage addr;

	//gets size of socket and accepts connection from client
	socklen_t size = sizeof(addr);
	int connection = accept(sockfd,(struct sockaddr*)&addr, &size);

	return connection;
}

// read / write, recv/send
int TCPRequestChannel::cread (void* msgbuf, int msgsize) {
    // read from socket - read
    // return number of bytes read
    int numBytes = read(sockfd, msgbuf, msgsize);
    return numBytes;
}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    // write to socket - write(int fd, const void *buf, size_t count)
    // return number of bytes written
    int numBytes = write(sockfd, msgbuf, msgsize);
    return numBytes;
}
