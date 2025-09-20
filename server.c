#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main(int argc, char** argv)
{

/* Argument checks */
if(argc < 2)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(1);
    }

int socketFD, newSocketFD, portNr, n;
socklen_t clilen;
char buffer[256];
struct sockaddr_in serverAddr, clientAddr;

/* Open the socket 
First parameter - address domain: AF_INET for internet domain, AF_UNIX for files on the same FS.
Second parameter - type of socket: SOCK_STREAM for continuous reading, or SOCK_DGRAM for chunk reading.
Third parameter - protocol: usually 0, which lets the OS will choose the appropriate one.

return - the file descriptor for the socket.
*/
socketFD = socket(AF_INET, SOCK_STREAM, 0);
if(socketFD < 0)
    {
    fprintf(stderr, "Socket creation failed.\n");
    exit(1);
    }
/* Get the port number */
portNr = atoi(argv[1]);

/* Initialiize the server address */
memset( &serverAddr, 0, sizeof(serverAddr));
serverAddr.sin_family = AF_INET; /* Code for the address family */
serverAddr.sin_port = htons(portNr); /* The port number, which must be converted to Network Byte Order */
serverAddr.sin_addr.s_addr = INADDR_ANY; /* THe IP address of the host */

/* Bind the socket */
if(bind(socketFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
    fprintf(stderr, "Unable to open socket.\n");
    exit(1);
    }

/* Listen to the socket */
if(listen(socketFD, 5) < 0)
    {
    fprintf(stderr, "Failed to listen on the socket.\n");
    exit(1);
    }

/* Clear the client information and wait for a connection */
memset(&clientAddr, 0, sizeof(clientAddr));
clilen = sizeof(clientAddr);
/*
First parameter - file descriptor of the original socket.
Second parameter - pointer of the client address, cast appropriately.
Third parameter - pointer to an int containing the size of the client length.

return - file descriptor of the new socket, to be used when communicating. */
newSocketFD = accept(socketFD, (struct sockaddr *)&clientAddr, &clilen);
if(newSocketFD < 0)
    {
    fprintf(stderr, "Failed to accept incoming connection.\n");
    exit(1);
    }

/* Receive data from the connection */
memset(&buffer[0], 0, sizeof(buffer));
n = read(newSocketFD, buffer, 255);
if(n < 0)
    {
    fprintf(stderr, "Failed to read from socket.\n");
    exit(1);
    }

/* Send received message to standard output */
fprintf(stdout, "Message: %s\n", buffer);

/* Send response to the client */
n = write(newSocketFD, "Message received.\n", 19);
if(n < 0)
    {
    fprintf(stderr, "Failed to write to socket.\n");
    exit(1);
    }

printf("%d\n", portNr);

return 0;
}