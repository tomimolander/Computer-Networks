#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

// Helper function for printing an address
void print_address(const char *prefix, const struct addrinfo *res)
{
        char outbuf[80];
        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)res->ai_addr;
        void *address;

        if (res->ai_family == AF_INET)
                address = &(sin->sin_addr);
        else if (res->ai_family == AF_INET6)
                address = &(sin6->sin6_addr);
        else {
                printf("Unknown address\n");
                return;
        }

        const char *ret = inet_ntop(res->ai_family, address,
                                    outbuf, sizeof(outbuf));
        printf("%s %s\n", prefix, ret);
}


// host: Name or IP-address as a string
// serv: Port number as a string
int tcp_connect(const char *host, const char *serv)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    //Both IPv4 and IPv6, no restrictions
    hints.ai_family = AF_UNSPEC;
    // Only TCP connections
    hints.ai_socktype = SOCK_STREAM;

    // Query using hints-struct
    // Function reserves memory and returns the pointer
    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
            fprintf(stderr, "tcp_connect error for %s, %s: %s\n",
                    host, serv, gai_strerror(n));
            return -1;
    }
    ressave = res; // so that we can release the memory afterwards

    // res-structure points to a linked list. Iterate over it and try to connect.
    do {
           sockfd = socket(res->ai_family, res->ai_socktype,
                            res->ai_protocol);
            if (sockfd < 0)
                    continue;       /* ignore this one */

            print_address("Trying to connect", res);

            // If connection works exit the loop.
            if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                  break;          /* success */

            printf("connect failed\n");

            close(sockfd);  /* ignore this one */
    } while ( (res = res->ai_next) != NULL);

    // If we reached the end of the list but didn't connect ==> Error
    if (res == NULL) {      /* errno set from final connect() */
            fprintf(stderr, "tcp_connect error for %s, %s\n", host, serv);
            sockfd = -1;
    } else {
            print_address("We are using address", res);
    }

    // Free the memory of a linked list
    freeaddrinfo(ressave);

    return sockfd;
}

/*
    Helper function that reads server names and port numbers from the parameter sockfd socket.
    It then creates a new TCP connection to that address&port number and sends the IP address and port.
*/
int helper(int sockfd)
{
    int n;
    char *failing = "FAIL\n";
    char readArray[81];
    char outbuf[20];

    while ( (n = read(sockfd, readArray, 80)) > 0) {

        if ( readArray[n-1] == '\n' ){
            break;
        }
        if (fputs(readArray, stdout) == EOF) {
            fprintf(stderr, "fputs error\n");
            return 1;
        }
    }
    if (strstr(readArray, "OK") != NULL || strstr(readArray, "FAIL") != NULL){
        return 0;
    }
    
    char *serverName = strtok(readArray, " ");
    serverName= strtok(NULL, " ");
    char *portNum = strtok(NULL, " ");
    portNum[strlen(portNum)-1] = '\0';
    
    printf("Server name: %s, Port Number: %s\n", serverName, portNum);
    int sockfd_B = tcp_connect(serverName, portNum);
    if (sockfd_B >= 0) {
        printf("success!");
    } else {
        perror("no connection - fail: ");
        write(sockfd, failing, strlen(failing));
    }

    struct sockaddr_in own;
    socklen_t ownlen = sizeof(struct sockaddr_in);

    if (getsockname(sockfd_B, (struct sockaddr *)&own, &ownlen) < 0) {
        perror("getsockname error: ");
    }
    inet_ntop(AF_INET, &(own.sin_addr), outbuf, sizeof(outbuf));
    printf("\nOwn address: %s  port: %d\n", outbuf, ntohs(own.sin_port));

    char *info_B;
    
    info_B = (char*) malloc( 80* sizeof(char));
    sprintf(info_B, "ADDR %s %d 000000\n", outbuf,  ntohs(own.sin_port));

    if (write(sockfd_B, info_B, strlen(info_B)) < 0) {
        fprintf(stderr, "write error\n");
        //return 1;
    }
    free(info_B);
    return 1;
}


int main(void)
{
    // Skip SIGPIPE - Signal.
    signal(SIGPIPE, SIG_IGN);
    int sockfd_A, n;

    char recvline[80 + 1];  // char buffer, where data will be read
    struct sockaddr_in servaddr;  // struct for server address
    // Address (ASCII)
    const char *address = "127.0.0.1";

    // Create a socket that uses IPv4 - protocol (AF_INET) and TCP-protocol (SOCK_STREAM)
    // Returns socket identifier or -1 if failed
    if ( (sockfd_A = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
    if (connect(sockfd_A,
                (struct sockaddr *) &servaddr,
                sizeof(servaddr)) < 0) {
        perror("connect error");
        return 1;
    }
    const char *information = "000000\n";
    if (write(sockfd_A, information, strlen(information)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    const char *name = "4-names\n";
    if (write(sockfd_A, name, strlen(name)) < 0) {
        fprintf(stderr, "write error\n");
        return 1;
    }
    while (helper(sockfd_A)){}

    close(sockfd_A);
    return 0;

}