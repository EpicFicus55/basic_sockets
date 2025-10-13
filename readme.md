# Socket web server and client in Linux
Basic Web server and client in Linux with C and Make.

I'm doing this for learning purposes, to find out more about low level networking, but I would like to eventually turn it into something useful that I can run on my home server.

# Server
Currently, the server has the following capabilities:
- exists
- handles one or more connections
- prints to stdin messages received from the clients

It compiles with Make, so just running: `make server` should be enough.

To run it, specify the port number, like this: `./server 5000`.

# Client
This is mostly just a basic program to test the connection to the server. It just sends a message and then dies.

Compile it with Make, like this: `Make client`.
To run it, specify the server address and the port, like this: `./client localhost 5000`.

# Future improvements
Since this is just a learning project, the possibilities are endless. Once I refine the code a little bit more, these are my objectives:
- turn the server into a file transfer server. It will be able to receive and transmit files between two computers.
- add multithreading to it.
- containerize the server.