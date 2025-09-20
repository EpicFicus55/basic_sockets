#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
int socketFD, portNr, n;
struct sockaddr_in serverAddress;
struct hostent* server;
char buffer[256];

if (argc < 3)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(EXIT_FAILURE);
    }

/* Get the port number */
portNr = atoi(argv[2]);

/* Open the socket */
socketFD = socket(AF_INET, SOCK_STREAM, 0);
if(socketFD < 0)
    {
    fprintf(stderr, "Socket creation failed.\n");
    exit(EXIT_FAILURE);
    }

/* Get the server */
server = gethostbyname(argv[1]);
if(server == NULL)
    {
    fprintf(stderr, "Failed to find server.\n");
    exit(EXIT_FAILURE);
    }

memset(&serverAddress, 0, sizeof(serverAddress));
serverAddress.sin_family = AF_INET;
memcpy(&serverAddress.sin_addr.s_addr, server->h_addr, server->h_length);
serverAddress.sin_port = htons(portNr);

if(connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
    fprintf(stderr, "Failed to connect to server.\n");
    exit(EXIT_FAILURE);
    }

printf("Message: ");
fgets(buffer, sizeof(buffer) - 1, stdin);
n = write(socketFD, buffer, strlen(buffer));
if(n < 0)
    {
    fprintf(stderr, "Failed to write to socket.\n");
    exit(EXIT_FAILURE);
    }

memset(&buffer[0], 0, sizeof(buffer));
n = read(socketFD, buffer, 255);
if(n < 0)
    {
    fprintf(stderr, "Failed to read to socket.\n");
    exit(EXIT_FAILURE);
    }
printf("%s\n", buffer);

return EXIT_SUCCESS;
}