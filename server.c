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
#include <stdbool.h>
#include "server.h"

#define BACKLOG 100
#define PORT_NUM 12345
#define WEBSERVER_PORT 80


int proxy_server_socket_fd;
int webserver_socket_fd = 0;

char buffer_web_browser[BUFFER_SIZE];
char buffer_web_server[BUFFER_SIZE];
pthread_mutex_t mutex_request;

int main(int argc, char *argv[])
{
    long int port, client_socket_fd;
    socklen_t client_address_len;
    struct sockaddr_in client_address;
    pid_t childpid;

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
    proxy_server_socket_fd = CreateServerSocket(hostname, port);
    if (proxy_server_socket_fd < 0)
    {
        printf("Error creating server socket.\n");
        exit(1);
    }

    /*Setup the signal handler*/
    SetupSignalHandler();

    while (1) {

        /* Accept connection to client. */
        client_address_len = sizeof (client_address);
        client_socket_fd = accept(proxy_server_socket_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket_fd == -1) {
            perror("accept");
            continue;
        }

        printf("[%s] - Connected to server\n", inet_ntoa(client_address.sin_addr));

        if ((childpid = fork()) == 0)
        {
            while (1)
            {
                close(proxy_server_socket_fd);
                handle_client_routine(client_socket_fd);
            }
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


int ConnectToServer(char * hostname, long int port)
{
    struct sockaddr_in address;
    int socket_fd;
    char ip[16];

    if (hostname_to_ip(hostname , ip) != SUCCESS)
    {
        printf("Error in hostname_to_ip %s\n", hostname);
        return FAILURE;
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
        return FAILURE;
    }

    /* Connect to socket with server address. */
    if (connect(socket_fd, (struct sockaddr *)&address, sizeof address) == -1)
    {
        perror("connect");
        return FAILURE;
    }
    return socket_fd;
}

int CreateServerSocket(char * hostname, long int port)
{
    struct sockaddr_in address;
    int socket_fd;
    char ip[16];

    if (hostname_to_ip(hostname , ip) != SUCCESS)
    {
        printf("Error in hostname_to_ip %s\n", hostname);
        return FAILURE;
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
        return FAILURE;
    }

    /* Bind address to socket. */
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof (address)) == -1) {
        perror("bind");
        return FAILURE;
    }

    /* Listen on socket. */
    if (listen(socket_fd, BACKLOG) == -1) {
        perror("listen");
        return FAILURE;
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

void handle_client_routine(int client_socket)
{

    printf("[%d] - Client connected\n", client_socket);

    pthread_attr_t pthread_client_attr;
    pthread_t pthread_service_web_browser;
    pthread_t pthread_service_web_server;

    pthread_mutex_init(&mutex_request, NULL);

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_client_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }

    /* Create thread to serve connection to client. */
    if (pthread_create(&pthread_service_web_browser, &pthread_client_attr, pthread_web_browser_routine, (void *)&client_socket) != 0)
    {
        perror("pthread_create");
        exit(1);
    }

    /* Create thread to serve connection to client. */
    if (pthread_create(&pthread_service_web_server, &pthread_client_attr, pthread_web_server_routine, (void *)NULL) != 0)
    {
        perror("pthread_create");
        exit(1);
    }

    pthread_join(pthread_service_web_browser, NULL);
    pthread_join(pthread_service_web_server, NULL);

    printf("[Exit] - pthread_web_browser_routine\n");
    exit(0);
}


void *pthread_web_browser_routine( void *arg)
{

    printf("[Entry] - pthread_web_browser_routine\n");
    int client_socket = *(int *)arg;
    bool is_request_received = false;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while(1)
    {

        // Read from web browser
        memset(buffer_web_browser, 0, BUFFER_SIZE);
        pthread_mutex_lock(&mutex_request);

        if (!is_request_received) {
            ssize_t read_size = recv(client_socket, buffer_web_browser, BUFFER_SIZE, MSG_WAITALL);
            if (read_size == 0) {
                printf("[%s] - Disconnected from web browser\n", "Browser->Server");
                close(client_socket);
                break;
            }
            else if (read_size == -1)
            {
                if ( errno == EAGAIN)
                {
                    printf("[%s] - Timeout\n", "Browser->Server");
                    pthread_mutex_unlock(&mutex_request);
                    is_request_received = false;
                    continue;
                }
                else
                {
                    perror("read");
                    close(client_socket);
                    break;
                }
            }

            printf("[%s] - Received from web browser: %s\n", "Browser->Server", buffer_web_browser);
            if (!strstr(buffer_web_browser, "pages.cpsc.ucalgary.ca")) {
                printf("Request is not for this server\n");
                pthread_mutex_unlock(&mutex_request);
                is_request_received = false;
                continue;
            }
            is_request_received = true;
        }

        if (!webserver_socket_fd)
        {
            printf("[%s] - Webserver socket not created; Creating\n", "Browser->Server");
            webserver_socket_fd = ConnectToServer("pages.cpsc.ucalgary.ca", WEBSERVER_PORT);
            if (webserver_socket_fd == FAILURE)
            {
                printf("[%s] - Webserver socket creation failed\n", "Browser->Server");
                break;
            }
            setsockopt(webserver_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        }

        // Send to web server
        if (is_request_received) {
            ssize_t write_size = send(webserver_socket_fd, buffer_web_browser, strlen(buffer_web_browser), 0);
            if (write_size == -1) {
                perror("write");
                close(webserver_socket_fd);
                webserver_socket_fd = 0;
                break;
            } else if (write_size == 0) {
                printf("[%s] - Disconnected from web server\n", "Browser->Server");
                close(webserver_socket_fd);
                webserver_socket_fd = 0;
                break;
            } else {
                printf("[%s] - Sent to web server: %s\n", "Browser->Server", buffer_web_browser);
                is_request_received = false;
            }
        }
        pthread_mutex_unlock(&mutex_request);
    }
    pthread_mutex_unlock(&mutex_request);
    printf("[Exiting] - pthread_web_browser_routine\n");
    pthread_exit(NULL);
}

void *pthread_web_server_routine( void *arg)
{
    printf("[Entry] - pthread_web_server_routine\n");
    printf("[Exit] - pthread_web_server_routine\n");
    pthread_exit(NULL);
}



void signal_handler(int signal_number)
{
    close(proxy_server_socket_fd);
    exit(0);
}