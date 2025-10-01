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
int n;
char *sHostName, *sPortNr;
char buffer[256];
struct addrinfo hints, *res, *p;

if (argc < 3)
    {
    fprintf(stderr, "Invalid number of arguments.\n");
    exit(EXIT_FAILURE);
    }

/* Get the port number */
sHostName = argv[1];
sPortNr = argv[2];

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;

/* Resolve host name */
int status = getaddrinfo(sHostName, sPortNr, &hints, &res);
if(status != 0)
    {
    fprintf(stderr, "Failed to find server.\n");
    exit(EXIT_FAILURE);
    }

int socky = 0;
for (p = res; p != NULL; p = p->ai_next) {
    socky = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socky == -1) 
        {
        continue;  /* Skip to the next address */
        }

    if (connect(socky, p->ai_addr, p->ai_addrlen) == -1) 
        {
        fprintf(stderr, "Unable to connect socket. Retrying...\n");
        close(socky);
        socky = -1;
        continue;  /* Skip to the next address */
        }

    fprintf(stdout, "Connection succeeded!\n");
    break;  // Success: connected
}
if(socky == -1)
    {
    perror("Unable to find server.\n");
    exit(EXIT_FAILURE);
    }

freeaddrinfo(res);

/* Connection succeeded, read message from stdin and transmit to server */
printf("Message: ");
fgets(buffer, sizeof(buffer) - 1, stdin);
n = write(socky, buffer, strlen(buffer));
if(n < 0)
    {
    fprintf(stderr, "Failed to write to socket.\n");
    exit(EXIT_FAILURE);
    }

memset(&buffer[0], 0, sizeof(buffer));
n = read(socky, buffer, 255);
if(n < 0)
    {
    fprintf(stderr, "Failed to read to socket.\n");
    exit(EXIT_FAILURE);
    }
printf("%s\n", buffer);

return EXIT_SUCCESS;
}