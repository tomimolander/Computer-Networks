#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define LISTENQ 5

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    int listenfd, connfd, firstfd, n;
    socklen_t len;
    struct sockaddr_in6 servaddr, cliaddr;
    char buff[80];
    struct sockaddr_in servaddr_first;
    const char *address = "127.0.0.1";

    if ( (firstfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return 1;
    }
    memset(&servaddr_first, 0, sizeof(servaddr_first));
    servaddr_first.sin_family = AF_INET;
    servaddr_first.sin_port   = htons(5000);
    if (inet_pton(AF_INET, address, &servaddr_first.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", address);
        return 1;
    }
    if (connect(firstfd,
                (struct sockaddr *) &servaddr_first,
                sizeof(servaddr_first)) < 0) {
        perror("connect error");
        return 1;
    }
    const char *information = "000000\n";
    if (write(firstfd, information, strlen(information)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    const char *name = "5-server\n";
    if (write(firstfd, name, strlen(name)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }

    // Create a listening socket
    if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Choose a port and bind the socket to it
    // Accept all connections
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(7207);

    if (bind(listenfd, (struct sockaddr *) &servaddr,
        sizeof(servaddr)) < 0) {
        perror("bind");
        return -1;
    }

    // Listen to the socket, set queue to 5
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen");
        return -1;
    }

    char outbuf[20];
    struct sockaddr_in own;
    socklen_t ownlen = sizeof(struct sockaddr_in);
    if (getsockname(firstfd, (struct sockaddr *)&own, &ownlen) < 0) {
        perror("getsockname error: ");
    }
    inet_ntop(AF_INET, &(own.sin_addr), outbuf, sizeof(outbuf));
    printf("\nOwn address: %s  port: %d\n", outbuf, ntohs(servaddr.sin6_port));

    // For accepting new connections
    while (1) {
        
        char recvline[80 + 1];  
        read(firstfd, recvline, 80);
        if ( recvline[0] != 'M' ){
            break;
        }
        printf("Incoming from main socket: %s\n", recvline);
        char *info_Conn;
        info_Conn = (char*) malloc( 80* sizeof(char));
        sprintf(info_Conn, "SERV %s %d\n", outbuf,  ntohs(servaddr.sin6_port));

        printf("Sending: %s", info_Conn);
        if (write(firstfd, info_Conn, strlen(info_Conn)) < 0) {
            fprintf(stderr, "write error\n");
        }

        len = sizeof(cliaddr);

        // New connection, connfd is a new socket that can be used
        // to transfer data between the server and client
        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr,
                             &len)) < 0) {
            perror("accept");
            return -1;
        }
        printf("connection from %s, port %d\n",
               inet_ntop(AF_INET6, &cliaddr.sin6_addr,
               buff, sizeof(buff)), ntohs(cliaddr.sin6_port));

        while(1) {

            
            // Accept connections, read the requested amount of bytes and respond to them

            int connNum;
            read(connfd, &connNum, 5);
            connNum = ntohl(connNum);
            if (connNum < 0 ){
                break;
            }         
            printf("Number of requested bytes: %d\n", connNum);
            char *responseArr;
            responseArr = (char*) malloc((connNum+1)* sizeof(char));
            memset(responseArr, 'A', connNum);

            if (write(connfd, responseArr, strlen(responseArr)) < 0) {
                fprintf(stderr, "write error\n");
                free(responseArr);
                break;
            }
            free(responseArr);
        } 

        free(info_Conn);
        close(connfd);  // close the socket
    }

    close(firstfd);
    return 0;
}