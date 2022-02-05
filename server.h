#ifndef BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H
#define BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(char * hostname, long int port);

void handle_client_routine(int client_socket);
void *pthread_web_server_routine( void *arg );
void *pthread_web_browser_routine( void *arg);

#define SUCCESS 0
#define FAILURE (-1)

#define BUFFER_SIZE 65536

#endif //BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H
