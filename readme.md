# Socket web server and client in Linux
Basic Web server and client in Linux with C and Make, which handles file transfers.

I'm doing this for learning purposes, to find out more about low level networking, but I would like to eventually turn it into something useful that I can run on my home server.

# Server
Currently, the server has the following capabilities:
- exists
- is able to receive files and save them locally
- is able to handle connections to multiple clients at the same time
- performs the transfer in chunks if the file is larger than 1KB
- logs the information to a log file

It compiles with Make, so just running: `make server` should be enough.

To run it, specify the port number, like this: `./server 5000`.

# Client
This is mostly just a basic program to test the connection to the server. It requests a local file to transfer, performs the transfer, and exits.

Compile it with Make, like this: `make client`.
To run it, specify the server address and the port, like this: `./client localhost 5000`, and then specify the name of the file in the local directory.

# Future improvements
Since this is just a learning project, the possibilities are endless. Once I refine the code a little bit more, these are my objectives:
- be able to query the available files on server
- get more information about the state of the transfer
- more robust error handling
- retry the transfer on failure
- add multithreading to it.
- containerize the server.