#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define MESSAGE_RECEIVED_RESP "Message received"

struct FileInfoType
    {
    char fileName[128];
    unsigned int sizeInBytes;
    };

int serverSocket;

static int connectToServer(char*, char*);
static struct FileInfoType getFileInfo(char*);
static int sendFileToServer(int, struct FileInfoType*);

int main(int argc, char** argv)
{
char *sHostName, *sPortNr;
char fileName[128];
struct FileInfoType fileInfo;
// bool isFileTransmitted = false;

if (argc < 3)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(EXIT_FAILURE);
    }

/* Get the port number */
sHostName = argv[1];
sPortNr = argv[2];

serverSocket = connectToServer(sHostName, sPortNr);

/* Connection succeeded, read message from stdin and transmit to server */
printf("File to send: ");

memset(&fileName, 0, sizeof(fileName));
fgets(fileName, sizeof(fileName) - 1, stdin);
fileName[strcspn(fileName, "\n")] = '\0'; /* fgets return value also contains the \n... */

/* Get the file information */
fileInfo = getFileInfo(fileName);

/* Send the filename to the server */
printf("Sending message:\n\tFile name:%s\n\tFile size: %d\n", fileInfo.fileName, fileInfo.sizeInBytes);
sendFileToServer(serverSocket, &fileInfo);

return EXIT_SUCCESS;
}

static int connectToServer(char* hostName, char* port)
{
struct addrinfo hints, *res, *p;
int _socket = -1;

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;

/* Resolve host name */
int status = getaddrinfo(hostName, port, &hints, &res);
if(status != 0)
    {
    fprintf(stderr, "Failed to find server.\n");
    exit(EXIT_FAILURE);
    }


for (p = res; p != NULL; p = p->ai_next) {
    _socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (_socket == -1) 
        {
        continue;  /* Skip to the next address */
        }

    if (connect(_socket, p->ai_addr, p->ai_addrlen) == -1) 
        {
        fprintf(stderr, "Unable to connect socket. Retrying...\n");
        close(_socket);
        _socket = -1;
        continue;  /* Skip to the next address */
        }

    fprintf(stdout, "Connection succeeded!\n");
    break;  // Success: connected
}
if(_socket == -1)
    {
    perror("Unable to find server.\n");
    exit(EXIT_FAILURE);
    }

freeaddrinfo(res);

return _socket;
}


static struct FileInfoType getFileInfo(char* fileName)
{
struct FileInfoType fileInfo;
FILE* file;

memset(&fileInfo, 0, sizeof(fileInfo));


file = fopen(fileName, "r+");
if(!file)
{
    fprintf(stderr, "Unable to open %s %ld\n.", fileName, strlen(fileName));
    return fileInfo;
}

strcpy(fileInfo.fileName, fileName);

fseek(file, 0, SEEK_END);
fileInfo.sizeInBytes = ftell(file);

fclose(file);

return fileInfo;
}


static int sendFileToServer(int serverSocket, struct FileInfoType* fileInfo)
{
int n;
char buffer[1024];
FILE* file;

/* Send file information to the server and wait for response */
n = write(serverSocket, (void*)fileInfo, sizeof(*fileInfo));
if(n < 0)
    {
    perror("Failed to write fileinfo to socket.\n");
    exit(EXIT_FAILURE);
    }

memset(&buffer[0], 0, sizeof(buffer));
n = read(serverSocket, buffer, 255);
if(n < 0)
    {
    perror("Failed to read server response from socket.\n");
    exit(EXIT_FAILURE);
    }
printf("%s\n", buffer);

/* Send the file in chunks: */
file = fopen(fileInfo->fileName, "r");

int transferInProgress = true;
while(transferInProgress)
    {
    char serverResponse[128];
    size_t bytesRead;
    
    memset(buffer, 0, sizeof(buffer));
    memset(serverResponse, 0, sizeof(serverResponse));

    bytesRead = fread(buffer, sizeof(buffer)-1, 1, file);
    if(bytesRead == 0)
        {
        transferInProgress = false;
        }

    n = write(serverSocket, (void*)buffer, strlen(buffer));
    printf("Sending chunk of file: %d bytes received.\n", n);
    if(n < 0)
        {
        perror("Failed to write file data to socket.\n");
        fclose(file);
        exit(EXIT_FAILURE);
        }
    
    n = read(serverSocket, serverResponse, sizeof(serverResponse) - 1);
    if(n < 0)
        {
        perror("Failed to rget confirmation from server.\n");
        fclose(file);
        exit(EXIT_FAILURE);
        }
    else
        {
        printf("%s\n", serverResponse);
        }
    }

fclose(file);
return 0;
}
