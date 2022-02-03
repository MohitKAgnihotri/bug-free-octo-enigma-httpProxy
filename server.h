#ifndef BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H
#define BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(char * hostname, long int port);

#define SUCCESS 0
#define FAILURE 1

#endif //BUG_FREE_OCTO_ENIGMA_HTTPPROXY_SERVER_H
