# Machine Problem 1

                                         TCP _Echo server – Team_11

## Team Members:
1.	Weiting Lian
2.	Karthik Eswar Krishnamoorthy Natarajan

## Contributions:

1.	Weiting handled the server-side programming, Makefile organization and code updation for handling test cases.
2.	The Client side programming and verifying the test cases were done by Karthik Eswar.

## Program Flow:
1)	Client

•	The Socket function is called to establish a connection-oriented communication.  
•	The Server attributes like family, port number, and IPv4 address are set using sockaddr\_in structure.  
•	Connection is requested using the connect function.  
•	Upon successful connection, data is sent and is echoed back by the server.  

2)	Server

•	File descriptors are declared for listen and connect.  
•	Socket is created and initialized with the server properties.  
•	The server socket is binded with the listen file descriptor and made to listen for active clients.  
•	Incoming client requests are handled using the accept function.  
•	The Fork function is used to handle multiple clients without terminating existing connection.  
•	The Client terminations are detected, and corresponding zombie processes are killed.  

## Program Usage:

	The Makefile in /MP1 has the commands the to compile both the server and client programs and generate the executables.

To run the programs, firstly you have to generate the executables by running `make all`.  

Or you can generate executables for servers and clients separately by running `make Server` or `make Client`.  

**Note:** The makefile will generate 3 identical clients simultaneously (Client\_1, Client\_2, Client\_3). You can use any of them to test point to point communication.  

Server can be run by  
```bash
./Server <port_number>  
```
Client can be run by  
```bash
./Client\_1 <ipv4address> <port_number>  
```

## File Organization:  
**Server**\_**Socket**: Directory to store codes for TCP server programming  
\-src: Source code (\*.c)  
\-\-Server\_MP1.c: Source code to implement TCP server service  
\-\-Server\_sig\_handle.c: Source code to handle SIGCHLD signals to kill zombie process after clients terminated  
\-include: Header files corresponding to the source codes (\*.h)  
\-\-Server\_MP1.h  
\-\-Server\_sig\_handle.h  

**Client**\_**Socket**: Directory to store codes for TCP client programming  
\-src: Source code (\*.c)  
\-\-client.c: Source code to implement TCP server service  

**Common**: Directory to store common functions used by both server and client  
\-src: Source code (\*.c)  
\-\-common.c: Implement functions to detect and handle newline characters in data streams.  
\-include: Header files corresponding to the source codes (\*.h)  
\-\-common.h  

**Makefile**: Makefile to generate executables for both server and client






