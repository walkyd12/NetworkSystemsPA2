#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <pthread.h>
#include <fcntl.h>

#define MAX_CONNECTIONS     10
#define MAX_CONSEC_FAILS    10
#define HEADER_PADDING      1000
#define MAX_MSG_SIZE        64000
#define MAX_FILE_NAME_SIZE  64

// HTTP 500 error message
char* serverErrorMsg = { "HTTP/1.1 500 Internal Server Error" };
 
// Thread function
void *connectionHandler(void *);

// Structure that contains socket descriptor and a pointer to the global connection integer
struct TConnectionInformation {
    int *ciClientSock;
    int *cConnections;
};