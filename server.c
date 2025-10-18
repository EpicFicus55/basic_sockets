#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_CONNECTIONS 10
#define MESSAGE_RECEIVED_RESP "Message received"
#define LOG_FILE_PATH "log.txt"
#define REMOTE_FILE_DIR "./remote"

#define LOG(...) do { \
    fprintf(logFile, __VA_ARGS__); \
    printf(__VA_ARGS__); \
} while(0)

typedef enum
    {
    STATUS_AVAILABLE,
    STATUS_CLIENT_CONNECTED,
    STATUS_CLIENT_TRANSFER_IN_PROGRESS,
    STATUS_CLIENT_TRANSFER_FAILED
    } ConnectionStatusType;

struct FileInfoType
    {
    char fileName[128];
    unsigned int sizeInBytes;
    };

typedef struct 
    {
    int socketFD;
    ConnectionStatusType status;
    struct FileInfoType fileInfo;
    FILE* file;
    } ConnectionType;

ConnectionType connections[ MAX_CONNECTIONS ];
FILE* logFile;

static int serverInit(int);
static int serverLoop(int);
static int serverClean(int);


int main(int argc, char** argv)
{

/* Argument checks */
if(argc < 2)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(EXIT_FAILURE);
    }

int serverSocket, portNr, result;

/* Get the port number */
portNr = atoi(argv[1]);

serverSocket = serverInit(portNr);
result = serverLoop(serverSocket);
if(result != 0)
    {
    serverClean(serverSocket);
    exit(EXIT_FAILURE);
    }

serverClean(serverSocket);
exit(EXIT_SUCCESS);

}


static int  serverInit(int port)
{
struct sockaddr_in serverAddr;
int serverSocket;

/* Initialiize the server address */
memset( &serverAddr, 0, sizeof(serverAddr));
serverAddr.sin_family = AF_INET; /* Code for the address family */
serverAddr.sin_port = htons(port); /* The port number, which must be converted to Network Byte Order */
serverAddr.sin_addr.s_addr = INADDR_ANY; /* THe IP address of the host */

/* Create the socket */
serverSocket = socket(AF_INET, SOCK_STREAM, 0);
if(serverSocket < 0)
    {
    perror("Server socket creation failed.\n");
    exit(EXIT_FAILURE);
    }

/* Bind the socket */
if(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
    perror("Unable to bind socket.\n");
    exit(EXIT_FAILURE);
    }

/* Set the socket to a passive state */
if(listen(serverSocket, 5) < 0)
    {
    perror("Failed to listen on the socket.\n");
    exit(EXIT_FAILURE);
    }

/* Initialize the client connections array */
memset(connections, 0, sizeof(connections));

/* Open the log file */
logFile = fopen("log.txt", "a+");
if(logFile == NULL)
    {
    fprintf(stderr, "Unable to open log file.\n");
    }
else
    {
    LOG("Starting server.\n");
    }

return serverSocket;
}


static int serverLoop(int serverSocket)
{
fd_set readfds;
struct timeval tval;
socklen_t clilen;
struct sockaddr_in clientAddr;
int clientSocket;

/* Clear the client information and wait for a connection */
memset(&clientAddr, 0, sizeof(clientAddr));

while(1)
    {
    int maxFD = serverSocket;

    FD_ZERO(&readfds);
    FD_SET(serverSocket, &readfds);

    tval.tv_sec = 5;
    tval.tv_usec = 0;

    /* Initialize the fd_set with all the currently open connections */
    for(int i = 0; i < MAX_CONNECTIONS; i++)
        {
        if((connections[i].status == STATUS_CLIENT_TRANSFER_IN_PROGRESS || connections[i].status == STATUS_CLIENT_CONNECTED)
            && connections[i].socketFD != 0)
            {
            FD_SET(connections[i].socketFD, &readfds);
            if(connections[i].socketFD > maxFD)
                {
                maxFD = connections[i].socketFD;
                }
            }
        }

    if(select(maxFD + 1, &readfds, NULL, NULL, &tval) < 0)
        {
        perror("Failed on select.\n");
        continue;
        }

    /* Check for new connections */
    if(FD_ISSET(serverSocket, &readfds))
        {
        clilen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clilen);
        LOG("New connection received.\n");
        
        /* Check the new socket */
        if(clientSocket < 0)
            {
            perror("Unable to connect to client.\n");
            return EXIT_FAILURE;
            }

        /* Register the newly connected client to the list of file descriptors to be read */
        for(int i = 0; i < MAX_CONNECTIONS; i++)
            {
            if(connections[i].status == STATUS_AVAILABLE)
                {
                connections[i].socketFD = clientSocket;
                connections[i].status = STATUS_CLIENT_CONNECTED;
                break;
                }
            }
        }

    /* Handle client requests */
    for(int i = 0; i < MAX_CONNECTIONS; i++)
        {
        if(connections[i].socketFD > 0 && FD_ISSET(connections[i].socketFD, &readfds))
            {
            char buffer[1024];
            ssize_t bytesRead;

            bytesRead = read(connections[i].socketFD, buffer, sizeof(buffer));
            if(bytesRead <= 0)
                {
                if(connections[i].status == STATUS_CLIENT_TRANSFER_IN_PROGRESS)
                    {
                    printf("Unable to transfer file. Try again.\n");
                    }
                close(connections[i].socketFD);
                connections[i].socketFD = 0;
                connections[i].status = STATUS_AVAILABLE;
                LOG("Closing a connection.\n");
                }
            else
                {
                /* The client is initiating a transfer */
                if(connections[i].status == STATUS_CLIENT_CONNECTED)
                    {
                    if(bytesRead != sizeof(struct FileInfoType))
                        {
                        printf("Could not read all the bytes: %ld / %ld.\n", bytesRead, sizeof(struct FileInfoType));
                        }
                    sprintf(connections[i].fileInfo.fileName, "%s/%s", REMOTE_FILE_DIR, ((struct FileInfoType*)buffer)->fileName);
                    connections[i].fileInfo.sizeInBytes = ((struct FileInfoType*)buffer)->sizeInBytes;

                    LOG("Received message:\n\tFile name:%s\n\tFile size: %d\n", connections[i].fileInfo.fileName, connections[i].fileInfo.sizeInBytes);
                    
                    strcpy(buffer, MESSAGE_RECEIVED_RESP);
                    bytesRead = write(connections[i].socketFD, buffer, strlen(buffer));
                    
                    connections[i].file = fopen(connections[i].fileInfo.fileName, "w+");
                    connections[i].status = STATUS_CLIENT_TRANSFER_IN_PROGRESS;
                    }
                /* The client has a transfer in progress */
                else
                    {
                    /* If we received less than the chunk size, we close the file */
                    if(bytesRead < 1024)
                        {
                        LOG("Received chunk of file.\n");

                        fprintf(connections[i].file, "%s", buffer);
                        fclose(connections[i].file);
                        memset(&connections[i], 0, sizeof(connections[i]));
                        }
                    }
                
                }

            }
        }
    fflush(logFile);
    }
}

static int serverClean(int serverSocket)
{

for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
    if(connections[i].socketFD != 0)
        {
        close(connections[i].socketFD);
        connections[i].status = STATUS_AVAILABLE;
        connections[i].socketFD = 0;
        }
    }

close(serverSocket);

LOG("Closing server.\n");

/* Close log file */
fclose(logFile);

return 0;
}