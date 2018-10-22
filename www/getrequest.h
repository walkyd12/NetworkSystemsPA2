int GetRequest(int sock, char* client_message);
int GetFileName(char* messageToParse, char* fileName);
int GetFileType(char* fileName, char* fileType);

/************************************************
 *            GET REQUEST FUNCTIONS             *
 ***********************************************/
// Main function to handle GET request
int GetRequest(int sock, char* client_message) {
    char message[MAX_MSG_SIZE];
    // int read_size;
    char fileName[MAX_FILE_NAME_SIZE];

    GetFileName( (client_message + 4), fileName );

    char fileType[50];
    GetFileType(fileName, fileType);
    printf("Requested file: %s\n", fileName);
    // Attempt to open the file with read only priveleges
    int fd = open( fileName + 1 , 'r');

    // if the file descriptor is less than 0, the file was not opened successfully
    if (fd < 0) {
        // Print client error message
        printf("File open failed! File: %s\n", fileName);
        
        printf("\nSERVER ERROR MESSAGE\n%s\n\n", serverErrorMsg);
        // Send file error signal to server
        write(sock, serverErrorMsg, strlen(serverErrorMsg));

        return 0;
    }
    // Seek to end of file to get file size
    int fileSize = lseek(fd, 0, SEEK_END);
    printf("File size: %d\n", fileSize);
    // Reset file pointer to start of file
    lseek(fd, 0, SEEK_SET);
    // Loop while reading from file
    int sizeRead = 0;
    while( ( sizeRead = read(fd, message, MAX_MSG_SIZE) ) > 0 ) { 
        // Put the header at the start of returnMessage
        char returnMessage[MAX_MSG_SIZE];
        sprintf(returnMessage, "HTTP/1.1 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", fileType, sizeRead);
        int headerLength = strlen(returnMessage);
        
        printf("Sending n bytes: %d\n", sizeRead);
        memcpy((returnMessage + headerLength), message, sizeRead);
        //Send the message back to client
        printf("\nRETURN MESSAGE\n\n%s\n", returnMessage);
        write(sock, returnMessage, (headerLength + sizeRead) );
    }
    close(fd);
    memset(message, 0, MAX_MSG_SIZE);
    memset(fileType, 0, 50);

    return 0;
}


// Extract the file name from the http message
int GetFileName(char* messageToParse, char* fileName) {
    for(int i = 0; i < MAX_FILE_NAME_SIZE; ++i) {
        if(messageToParse[i] != ' ') {
            fileName[i] = messageToParse[i];
        } else {
            fileName[i] = '\0';
            i = MAX_MSG_SIZE;
        }
    }
    if( (0 == memcmp(fileName, "/", 1)) && ( (0 == memcmp(fileName+1, " ", 1)) || (0 == memcmp(fileName+1, "\0", 1)) || (0 == memcmp(fileName+1, "\n", 1)) ) )  {
        memcpy(fileName, "/index.html", 11);
    }

    return 0;
}


// Extract the file type from the name of the file
int GetFileType(char* fileName, char* fileType) {
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
    } else {
        return 1;
    }
    return 0;
}