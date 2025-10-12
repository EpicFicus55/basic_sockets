#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define MAX_CONNECTIONS 10
#define MESSAGE_RECEIVED_RESP "Message received"

int connections[ MAX_CONNECTIONS ];

static int serverInit(int);
static int serverLoop(int);
static int serverClean(int);

int main(int argc, char** argv)
{

/* Argument checks */
if(argc < 2)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(1);
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
    fprintf(stderr, "Server socket creation failed.\n");
    exit(1);
    }

/* Bind the socket */
if(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
    fprintf(stderr, "Unable to bind socket.\n");
    exit(1);
    }

/* Set the socket to a passive state */
if(listen(serverSocket, 5) < 0)
    {
    fprintf(stderr, "Failed to listen on the socket.\n");
    exit(1);
    }

/* Initialize the client connections array */
memset(connections, 0, sizeof(connections));

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
        if(connections[i] > 0)
            {
            FD_SET(connections[i], &readfds);
            if(connections[i] > maxFD)
                {
                maxFD = connections[i];
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
        printf("New connection received.\n");
        
        /* Check the new socket */
        if(clientSocket < 0)
            {
            perror("Unable to connect to client.\n");
            return EXIT_FAILURE;
            }

        /* Register the newly connected client to the list of file descriptors to be read */
        for(int i = 0; i < MAX_CONNECTIONS; i++)
            {
            if(connections[i] == 0)
                {
                connections[i] = clientSocket;
                break;
                }
            }
        }

    /* Handle client requests */
    for(int i = 0; i < MAX_CONNECTIONS; i++)
        {
        if(connections[i] > 0 && FD_ISSET(connections[i], &readfds))
            {
            char buffer[1024];
            ssize_t bytesRead;

            bytesRead = read(connections[i], buffer, sizeof(buffer));
            if(bytesRead <= 0)
                {
                close(connections[i]);
                connections[i] = 0;
                printf("Closing a connection.\n");
                }
            else
                {
                buffer[bytesRead] = '\0';
                printf("Received message: %s\n", buffer);

                strcpy(buffer, MESSAGE_RECEIVED_RESP);
                bytesRead = write(connections[i], buffer, strlen(buffer));
                }

            }
        }
    
    }
}

static int serverClean(int serverSocket)
{

for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
    if(connections[i] != 0)
        {
        close(connections[i]);
        connections[i] = 0;
        }
    }

close(serverSocket);

return 0;
}