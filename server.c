#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>

#include "common.h"

#define LOG(...) do { \
    fprintf(logFile, __VA_ARGS__); \
    printf(__VA_ARGS__); \
} while(0)

typedef enum
    {
    STATUS_AVAILABLE,
    STATUS_CLIENT_CONNECTED,
    STATUS_CLIENT_TRANSFER_IN_PROGRESS,
    STATUS_CLIENT_TRANSFER_COMPLETED,
    STATUS_CLIENT_TRANSFER_FAILED
    } ConnectionStatusType;

typedef struct connectionNode
    {
    int socketFD;
    ConnectionStatusType status;
    struct FileInfoType fileInfo;
    FILE* file;
    int receivedBytes;
    struct connectionNode* next; 
    } ConnectionNodeType;

typedef struct 
    {
    ConnectionNodeType* head;
    } ConnectionList;

ConnectionList connectionList;
FILE* logFile;

/* Linked list functionalities */
static inline ConnectionNodeType* allocateConnection(void) 
    {
    printf("Allocating connection.\n");
    ConnectionNodeType* connection = malloc(sizeof(ConnectionNodeType));
    return connection;
    }

static inline void deallocateConnection(ConnectionNodeType* connection)
    {
    printf("Freeing connection.\n");
    free(connection);
    }

static inline void addConnectionToList(ConnectionList* list, ConnectionNodeType* connection) 
    {
    if(!list) return;
    connection->next = list->head;
    list->head = connection;
    }
static inline void removeConnectionFromList(ConnectionList* list, ConnectionNodeType* connection)
    {
    if(!list) return;
    ConnectionNodeType** iterator = &list->head;
    while(*iterator)
        {
        if(*iterator == connection)
            {
            *iterator = (*iterator)->next;
            }
        else
            {
            iterator = &((*iterator)->next);
            }
        }
    }

static int serverInit(int);
static int serverLoop(int);
static int serverClean(int);
static int connectionInit(ConnectionNodeType*, int, char*);
static int connectionProcess(ConnectionNodeType*, int, char*);
static int connectionStop(ConnectionNodeType*);


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
serverClean(serverSocket);
if(result != 0)
    {
    exit(EXIT_FAILURE);
    }
    
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

printf("Entering loop.\n");

while(1)
    {
    int maxFD = serverSocket;

    FD_ZERO(&readfds);
    FD_SET(serverSocket, &readfds);

    tval.tv_sec = 5;
    tval.tv_usec = 0;

    /* Initialize the fd_set with all the currently open connections */
    for(ConnectionNodeType* connection = connectionList.head; connection; connection = connection->next)
        {
        if((connection->status == STATUS_CLIENT_TRANSFER_IN_PROGRESS || connection->status == STATUS_CLIENT_CONNECTED)
            && connection->socketFD != 0)
            {
            FD_SET(connection->socketFD, &readfds);
            if(connection->socketFD > maxFD)
                {
                maxFD = connection->socketFD;
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
        ConnectionNodeType* incomingConnection = allocateConnection();
        incomingConnection->status = STATUS_CLIENT_CONNECTED;
        incomingConnection->socketFD = clientSocket;
        addConnectionToList(&connectionList, incomingConnection);
        }

    /* Handle client requests */
    ConnectionNodeType* connection = connectionList.head;
    ConnectionNodeType* next = connection ? connection->next : NULL;
    while(connection)
        {
        if(connection->socketFD > 0 && FD_ISSET(connection->socketFD, &readfds))
            {
            char buffer[CHUNK_SIZE];
            int bytesRead;

            /* Get the data from the client*/
            memset(buffer, 0, sizeof(buffer));
            bytesRead = read(connection->socketFD, buffer, sizeof(buffer));
            if(bytesRead <= 0)
                {
                if(connection->status == STATUS_CLIENT_TRANSFER_IN_PROGRESS)
                    {
                    LOG("Unable to transfer file. Try again.\n");
                    }
                close(connection->socketFD);
                connection->socketFD = 0;
                connection->status = STATUS_AVAILABLE;
                LOG("Closing a connection.\n");
                }
            else
                {
                /* Handle the data from the client */
                switch(connection->status)
                    {
                    /* The client is initiating a transfer */
                    case STATUS_CLIENT_CONNECTED:
                        /* Initialize the connection */
                        if(connectionInit(connection, bytesRead, buffer) == -1)
                            {
                            LOG("Client init error.\n");
                            }
                        connection = next;
                        next = next ? next->next : next;
                        break;
                    /* The client has a transfer in progress */
                    case STATUS_CLIENT_TRANSFER_IN_PROGRESS:
                        if(connectionProcess(connection, bytesRead, buffer) == -1)
                            {
                            LOG("Client transfer error.\n");
                            }
                        connection = next;
                        next = next ? next->next : next;
                        break;
                    /* The transfer from the client has been completed */
                    case STATUS_CLIENT_TRANSFER_COMPLETED:
                        if(connectionStop(connection) == -1)
                            {
                            connection = next;
                            next = next ? next->next : next;
                            }
                        break;
                    default:
                        LOG("Invalid connection.\n");
                            connection = next;
                            next = next ? next->next : next;
                        break;
                    }
                }
            }
        else 
            {
            connection = next;
            next = next ? next->next : next;
            }
        }
    fflush(logFile);
    }
}

static int serverClean(int serverSocket)
    {
    ConnectionNodeType* connection = connectionList.head;
    while(connection)
        {
        ConnectionNodeType* next = connection->next;
        deallocateConnection(connection);
        connection = next;
        }

    close(serverSocket);

    LOG("Closing server.\n");

    /* Close log file */
    fclose(logFile);

    return 0;
    }


static int connectionInit(ConnectionNodeType* connection, int bytesRead, char* buffer)
    {
    int bytesWritten;

    if(bytesRead != sizeof(struct FileInfoType))
        {
        printf("File information incomplete. Actual bytes %d / expected bytes %ld.\n", bytesRead, sizeof(struct FileInfoType));
        return -1;
        }
    sprintf(connection->fileInfo.fileName, "%s/%s", REMOTE_FILE_DIR, ((struct FileInfoType*)buffer)->fileName);
    connection->fileInfo.sizeInBytes = ((struct FileInfoType*)buffer)->sizeInBytes;

    LOG("Initializing file transfer:\n\tFile name:%s\n\tFile size: %d\n\tExpected chunks:%d\n", connection->fileInfo.fileName, connection->fileInfo.sizeInBytes, connection->fileInfo.sizeInBytes / CHUNK_SIZE);

    /* Send confirmation message to client */
    bytesWritten = write(connection->socketFD, MESSAGE_RECEIVED_RESP, strlen(MESSAGE_RECEIVED_RESP));
    if(bytesWritten <= 0)
        {
        LOG("Unable to send confirmation message to client.\n");
        connection->status = STATUS_CLIENT_TRANSFER_FAILED;
        return -1;
        }

    /* The connection was initiated, waiting for file contents */
    connection->file = fopen(connection->fileInfo.fileName, "w+");
    connection->status = STATUS_CLIENT_TRANSFER_IN_PROGRESS;

    return 0;
    }

static int connectionProcess(ConnectionNodeType* connection, int bytesRead, char* buffer)
{
    int bytesWritten;

    LOG("Received chunk of file. Processing %d bytes.\n", bytesRead);

    /* Append the incoming bytes to the file */
    fwrite(buffer, bytesRead, 1, connection->file);

    connection->receivedBytes += bytesRead;

    /* Send confirmation to client */
    bytesWritten = write(connection->socketFD, MESSAGE_RECEIVED_RESP, strlen(MESSAGE_RECEIVED_RESP));
    if(bytesWritten <= 0)
        {
        LOG("Unable to send confirmation message to client.\n");
        connection->status = STATUS_CLIENT_TRANSFER_FAILED;
        return -1;
        }

    if(connection->receivedBytes >= connection->fileInfo.sizeInBytes)
        {
        /* Close the file and the connection - transfer is finished */
        LOG("Finished transferring file: %s from connection.\n", connection->fileInfo.fileName);
        connection->status = STATUS_CLIENT_TRANSFER_COMPLETED;
        connectionStop(connection);
        fclose(connection->file);
        }

    return 0;
}

static int connectionStop(ConnectionNodeType* connection)
    {
    if(connection->socketFD != 0)
        {
        close(connection->socketFD);
    }
    removeConnectionFromList(&connectionList, connection);
    deallocateConnection(connection);

    return 0;
    }