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
- make the server more useful, maybe make it a system monitoring server that I can query for system information on my local network.
- Make it HTTP.
- Add multithreading to it. I've been meaning to use C11 threads for some time.
- Would be cool to make a server for a *very* basic game that would use a custom OpenGL rendering engine.
- More items incoming
- Maybe put it inside a Docker container?