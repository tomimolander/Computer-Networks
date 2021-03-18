#include <stdlib.h>
#include <sys/socket.h>  // defines socket, connect, ...
#include <netinet/in.h>  // defines sockaddr_in
#include <string.h>      // defines memset
#include <stdio.h>       // defines printf, perror, ...
#include <arpa/inet.h>   // inet_pton, ...
#include <unistd.h>      // read, ...


int main(int argc, char **argv)
{
    int sockfd, n, m;
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
    const char *name = "3-large\n";
    if (write(sockfd, name, strlen(name)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    
    /*
        Read packets from the socket. Packet starts with 32-bit unsigned int that determines how many
        bytes the rest of the packet contains. Read the packets and respond to the server by sending the
        number of bytes read. If the length == 0, exit loop.
    */
    while ( 1 ) {
        uint32_t length;
        n = read(sockfd, &length, 4);

        length = ntohl(length);
        printf("Packet length in bytes: %d\n", length);
        if (length == 0 ){
            break;
        }

        char *str;
        str = (char*) malloc(length * sizeof(char));
        
        int remaining = length;
        m = read(sockfd, str, length);
        remaining -= m;
        while ( remaining > 0) {
            m = read(sockfd, str, remaining);
            remaining -= m;
        }
        printf("Reading packets done\n");

        length = htonl(length);
        if (write(sockfd, &length, 4) < 0) {
            fprintf(stderr, "write error\n");
            return 1;
        }

        free(str);
    }

    uint32_t end = 0;

    if (write(sockfd, &end, 4) < 0) {
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