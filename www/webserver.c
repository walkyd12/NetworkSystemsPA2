/********************************
 * Walker Schmidt
 * CSCI 4273: Network Systems
 * Programming Assignment #2
 ********************************/

#include "globals.h"
#include "getrequest.h"

/************************************************
 *                MAIN FUNCTION                 *
 ***********************************************/
int main(int argc , char *argv[]) {
    int                     port, cConnections, cConsecFails;
    int                     socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in      server, client;
    pthread_t               connectionThreads[MAX_CONNECTIONS];
    char                    message[MAX_MSG_SIZE], client_message[MAX_MSG_SIZE];

    if(argc < 2) {
        printf("Please provide a socket to bind to.\n");
    }
    port = atoi(argv[1]);
     
    // Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created.\n");
     
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    // Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        printf("Bind failed. Error\n");
        return 1;
    }
    printf("Bind succeeded.\n");
     
    //Listen
    listen(socket_desc, 3);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");

    struct TConnectionInformation ci;
    ci.cConnections = &cConnections;
    cConnections = 0;

    // Accept incoming connections
    while( 1 ) {
        c = sizeof(struct sockaddr_in);

        // Accept incoming connections
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

        // Check if connection correctly established
        if (client_sock < 0)
            break;

        // Success, reset consecutive failures
        cConsecFails = 0;

        new_sock = malloc(1);
        *new_sock = client_sock;

        ci.ciClientSock = new_sock;
         
        // Launch a new thread for the connection we just created
        if( pthread_create(&connectionThreads[*ci.cConnections], NULL, connectionHandler, (void*)&ci) < 0 ) {
            printf("Could not create thread.\n");
            return 1;
        }

        // Lockless multi-threaded programming :)
        int curConnections = *ci.cConnections;
        int newConnections = curConnections + 1;
        __sync_val_compare_and_swap(ci.cConnections, curConnections, newConnections);
        printf("Connection accepted. Current connections: %d\n", *ci.cConnections);
    }

    /// wait all threads by joining them
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        pthread_join(connectionThreads[i], NULL);  
    }
     
    return 0;
}

/************************************************
 *          CONNECTION HANDLER THREAD           *
 ***********************************************/
void *connectionHandler(void *connectionInfo) {
    struct TConnectionInformation* ci = connectionInfo;

    //Get the socket descriptor
    int sock = *(int*)(ci->ciClientSock);
    char client_message[MAX_MSG_SIZE];
    int read_size;
    // char message[MAX_MSG_SIZE], client_message[MAX_MSG_SIZE];

    //Receive a message from client
    while( (read_size = recv(sock, client_message, MAX_MSG_SIZE, 0)) > 0 ) {
        printf("RECIEVED %d bytes\n", read_size);
        printf("%s\n", client_message);
        if(0 == memcmp(client_message, "GET", 3)) {
            GetRequest(sock, client_message);
        }
    }
     
    if(read_size == 0) {
        // Lockless multi-threaded programming :)
        int curConnections = *(ci->cConnections);
        int newConnections = curConnections - 1;
        __sync_val_compare_and_swap((ci->cConnections), curConnections, newConnections);
        printf("Client disconnected. Current connections: %d\n", *ci->cConnections);
    } else if(read_size == -1) {
        // Recieving data failed
        printf("RECV FAILED\n");
    }

    return 0;
}
