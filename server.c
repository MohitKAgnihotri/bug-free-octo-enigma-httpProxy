#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include<arpa/inet.h>
#include <errno.h>
#include "server.h"

#define BACKLOG 100
#define PORT_NUM 12345


int server_socket_fd;

int main(int argc, char *argv[])
{
    long int port, new_socket_fd;
    pthread_attr_t pthread_client_attr;
    pthread_t pthread;
    socklen_t client_address_len;
    struct sockaddr_in client_address;

    if (argc < 3)
    {
        printf("Usage: %s <server_name> <port>\n", argv[0]);
        exit(1);
    }

    char *hostname = argv[1];

    /* Get port from command line arguments or stdin. */
    char *endptr;
    port = PORT_NUM;
    port = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2]) {
        printf("Invalid number: %s \n",argv[2]);
        exit(2);
    } else if (*endptr) {
        printf("Trailing characters after number: %s \n",argv[2]);
        exit(1);
    } else if (errno == ERANGE) {
        printf("Number out of range: %s \n",argv[2]);
        exit(1);
    }

    /*Create the server socket */
    server_socket_fd = CreateServerSocket(hostname, port);

    /*Setup the signal handler*/
    SetupSignalHandler();

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_client_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

    while (1) {

        /* Accept connection to client. */
        client_address_len = sizeof (client_address);
        new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (new_socket_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Client connected\n");
        unsigned int *thread_arg = (unsigned int *) malloc(sizeof(unsigned int));
        *thread_arg = new_socket_fd;
        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread, &pthread_client_attr, pthread_routine, (void *)thread_arg) != 0) {
            perror("pthread_create");
            continue;
        }
    }

    return 0;
}


int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return FAILURE;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return SUCCESS;
    }

    return FAILURE;
}

int CreateServerSocket(char * hostname, long int port)
{
    struct sockaddr_in address;
    int socket_fd;
    char ip[16];

    if (hostname_to_ip(hostname , ip) != SUCCESS)
    {
        printf("Error in hostname_to_ip %s\n", hostname);
        exit(1);
    }

    /* Initialise IPv4 address. */
    memset(&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* Bind address to socket. */
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof (address)) == -1) {
        perror("bind");
        exit(1);
    }

    /* Listen on socket. */
    if (listen(socket_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Configure server socket
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    return socket_fd;
}

void SetupSignalHandler() {/* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
}

void *pthread_routine(void *arg)
{

    char buff[1024];
    time_t ticks;

    int client_socket = *(int*) arg;
    free(arg);

    ticks = time(NULL);
    snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
    write(client_socket, buff, strlen(buff));

    return NULL;
}

void signal_handler(int signal_number)
{
    close(server_socket_fd);
    exit(0);
}