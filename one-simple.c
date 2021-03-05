#include <sys/socket.h>  // defines socket, connect, ...
#include <netinet/in.h>  // defines sockaddr_in
#include <string.h>      // defines memset
#include <stdio.h>       // defines printf, perror, ...
#include <arpa/inet.h>   // inet_pton, ...
#include <unistd.h>      // read, ...

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

    /*
        Read bytes from the socket, at most 80 (MAXLINE).
        Bytes are copied to recvline buffer
        n represents the amount of read bytes, or 0 if connection was closed, or negative if error
    */
    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0; // Add 0 for printing purposes

        if (fputs(recvline, stdout) == EOF) {
            fprintf(stderr, "fputs error\n");
            return 1;
        }
    }

    // If read return value was 0, loop terminates, without error
    if (n < 0) {
        perror("read error");
        return 1;
    }
    close(sockfd);
    return 0;
}