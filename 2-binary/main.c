#include <sys/socket.h>  // defines socket, connect, ...
#include <netinet/in.h>  // defines sockaddr_in
#include <string.h>      // defines memset
#include <stdio.h>       // defines printf, perror, ...
#include <arpa/inet.h>   // inet_pton, ...
#include <unistd.h>      // read, ...
#include "template.h"

#define MAXLINE 80

int main(int argc, char **argv)
{
    int sockfd, n;
    char recvline[MAXLINE + 1];  // char buffer, where data will be read
    struct sockaddr_in servaddr;  // struct for server address
    // Address (ASCII)
    const char *address = "127.0.0.1";

    // Create a socket that uses IPv4 - protocol (AF_INET) and TCP-protocol (SOCK_STREAM)
    // Returns socket identifier or -1 if failed
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return 1;
    }

    
    // Initialize the struct representing the address, specify IPv4 and designate port on the server side
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(5000); 

    // Transfer IP-address to binary and save it to servaddr struct
    if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", address);
        return 1;
    }

    // Open TCP-connection using servaddr struct, if successful return 0 else negative value
    if (connect(sockfd,
                (struct sockaddr *) &servaddr,
                sizeof(servaddr)) < 0) {
        perror("connect error");
        return 1;
    }
    const char *information = "000000\n";
    if (write(sockfd, information, strlen(information)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    const char *name = "2-binary\n";
    if (write(sockfd, name, strlen(name)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    
    /*
        Read bytes from the socket, at most 80 (MAXLINE) or until a newline.
        Bytes are copied to recvline buffer
        n represents the amount of read bytes, or 0 if connection was closed, or negative if error
    */
    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {

        if ( recvline[n-1] == '\n' ){
            break;
        }
        if (fputs(recvline, stdout) == EOF) {
            fprintf(stderr, "fputs error\n");
            return 1;
        }
    }
    
    struct numbers a;
    memset(&a, 0, sizeof(a));
    //Parse information from the socket to struct.
    int num = parse_str(recvline, &a);

    //Write binary numbers to the socket, use network byte order
    if (write(sockfd, &a.a, 1) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    uint32_t hb = htonl(a.b);
    if (write(sockfd, &hb, 4) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    if (write(sockfd, &a.c, 1) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    uint16_t hd = htons(a.d);
    if (write(sockfd, &hd, 2) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    uint32_t he = htonl(a.e);
    if (write(sockfd, &he, 4) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    
    
    struct numbers b;
    memset(&b, 0, sizeof(b));
    
    //Read binary numbers from the socket and swap them to little-endian
    read(sockfd, &b.a, 1);
    read(sockfd, &b.b, 4);
    b.b = ntohl(b.b);
    read(sockfd, &b.c, 1);
    read(sockfd, &b.d, 2);
    b.d = ntohs(b.d);
    read(sockfd, &b.e, 4);
    b.e = ntohl(b.e);
    
    char lastString[82];
    output_str(lastString, 80, &b);
    strcat(lastString, "\n");
    // Format the binary numbers to string and write to the socket

    if (write(sockfd, lastString, strlen(lastString)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    
    if (n < 0) {
        perror("read error");
        return 1;
    }
    close(sockfd);
    return 0;
}