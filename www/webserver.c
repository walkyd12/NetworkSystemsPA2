/*
    C socket server example, handles multiple clients using threads
*/
 
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <fcntl.h>

#define MAX_CONNECTIONS     10
#define MAX_CONSEC_FAILS    10
#define MAX_MSG_SIZE        64000

char* serverErrorMsg = { "HTTP/1.1 500 Internal Server Error" };
 
//the thread function
void *connectionHandler(void *);

struct TConnectionInformation {
    int *ciClientSock;
    int cConnections;
};
 
int main(int argc , char *argv[])
{
    int                     port, cConnections, cConsecFails;
    int                     socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in      server, client;
    pthread_t               connectionThreads[MAX_CONNECTIONS];
    // byte                    message[MAX_MSG_SIZE];
    char                    message[MAX_MSG_SIZE], client_message[MAX_MSG_SIZE];

    if(argc < 2) {
        printf("Please provide a socket to bind to.\n");
    }
    port = atoi(argv[1]);
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created.\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        printf("Bind failed. Error\n");
        return 1;
    }
    printf("Bind succeeded.\n");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");
    cConsecFails = 0;

    struct TConnectionInformation ci;
    ci.cConnections = 0;

    // Accept incoming connections
    while( 1 ) {
        c = sizeof(struct sockaddr_in);

        // Accept incoming connections
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

        // Check if connection correctly established
        if (client_sock < 0) {
            if(cConsecFails < MAX_CONSEC_FAILS) {
                printf("Accept failed. Trying again\n");
                ++cConsecFails;
                continue;
            } else {
                printf("Fail limit reached, aborting.\n");
                break;
            }
        }

        // Success, reset consecutive failures
        cConsecFails = 0;

        new_sock = malloc(1);
        *new_sock = client_sock;

        ci.ciClientSock = new_sock;
         
        // Launch a new thread for the connection we just created
        if( pthread_create(&connectionThreads[ci.cConnections], NULL, connectionHandler, (void*)&ci) < 0 ) {
            printf("Could not create thread.\n");
            return 1;
        }

        int curConnections = ci.cConnections;
        int newConnections = curConnections + 1;
        __sync_val_compare_and_swap(&(ci.cConnections), curConnections, newConnections);
        printf("Connection accepted. Current connections: %d\n", ci.cConnections);
    }

    /// wait all threads by joining them
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        pthread_join(connectionThreads[i], NULL);  
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connectionHandler(void *connectionInfo) {
    struct TConnectionInformation* ci = connectionInfo;

    //Get the socket descriptor
    int sock = *(int*)(ci->ciClientSock);
    int read_size;
    char message[MAX_MSG_SIZE], client_message[MAX_MSG_SIZE];

    //Receive a message from client
    while( (read_size = recv(sock, client_message, MAX_MSG_SIZE, 0)) > 0 ) {
        printf("RECIEVED %d bytes\n", read_size);
        int fileNameLen = 0;
        char* fileName;
        printf("%s\n", client_message);
        if(0 == memcmp(client_message, "GET", 3)) {
            fileName = client_message + 4;
            for(int tmpLen = 0; tmpLen < MAX_MSG_SIZE; ++tmpLen) {
                if(fileName[tmpLen] == ' ') {
                    fileName[tmpLen] = '\0';
                    fileNameLen = tmpLen;
                    tmpLen = MAX_MSG_SIZE;
                }
            }
        }

        if( (0 == memcmp(fileName, "/", 1)) && ( (0 == memcmp(fileName+1, " ", 1)) || (0 == memcmp(fileName+1, "\0", 1)) || (0 == memcmp(fileName+1, "\n", 1)) ) )  {
            fileName = "/index.html";
        }

        char fileType[50];
        if( (0 == memcmp((fileName+(strlen(fileName) - 5)), ".html", 5)) ) {
            memcpy(fileType, "text/html", 9);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 4)), ".txt", 4))) {
            memcpy(fileType, "text/plain", 10);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 4)), ".png", 4))) {
            memcpy(fileType, "image/png", 9);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 4)), ".gif", 4))) {
            memcpy(fileType, "image/gif", 9);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 4)), ".jpg", 4))) {
            memcpy(fileType, "image/jpg", 9);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 4)), ".css", 4))) {
            memcpy(fileType, "text/css", 8);
        } else if((0 == memcmp((fileName+(strlen(fileName) - 3)), ".js", 3))) {
            memcpy(fileType, "application/javascript", 22);
        }

        printf("%s\n", fileName);

        // Attempt to open the file with read only priveledges
        int fd = open( fileName + 1 , 'r');
    
        // if the file descriptor is less than 0, the file was not opened successfully
        if (fd < 0) {
            // Print client error message
            printf("File open failed! File: %s\n", fileName);
            // Send file error signal to server

            printf("\nSERVER ERROR MESSAGE\n%s\n\n", serverErrorMsg);
            write(sock, serverErrorMsg, strlen(serverErrorMsg));
            continue;
        }

        int fileSize = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        char returnMessage[MAX_MSG_SIZE];
        sprintf(returnMessage, "HTTP/1.1 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", fileType, fileSize);
        int headerLength = strlen(returnMessage);

        int sizeRead = 0;
        while( ( sizeRead = read(fd, message, MAX_MSG_SIZE) ) > 0 ) { 
            printf("Sending n bytes: %d\n", sizeRead);
            memcpy((returnMessage + headerLength), message, fileSize);
            
            // sprintf(returnMessage, "HTTP/1.1 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: 34466\r\n\r\n%s", fileType, message);
            //Send the message back to client
            printf("\nRETURN MESSAGE\n\n%s\n", returnMessage);
            write(sock, returnMessage, (headerLength + fileSize) );
        }
        close(fd);

        memset(message, 0, MAX_MSG_SIZE);
        memset(fileType, 0, 50);
    }
     
    if(read_size == 0) {
        int curConnections = (ci->cConnections);
        int newConnections = curConnections - 1;
        __sync_val_compare_and_swap(&(ci->cConnections), curConnections, newConnections);
        printf("Client disconnected. Current connections: %d\n", ci->cConnections);
        fflush(stdout);
    } else if(read_size == -1) {
        printf("recv failed\n");
    }
     
    printf("peace\n");
    return 0;
}