# Machine Problem 3

                                                 TFTP – Team_11

## Team Members:
1.	Weiting Lian
2.	Karthik Eswar Krishnamoorthy Natarajan

## Contributions:

1.	Weiting handled the send-receive, error handling, octet mode, timeouts and Makefile organization
2.	The netascii mode, debugging, verifying the test cases and the README were done by Karthik Eswar.

## Program Flow:
1)	Client

•	The standard TFTP client is used as the client to send RRQ messages.  
•	The different parameters like mode and display can be changed in the terminal. (eg.: verbose, trace, status).   
•	File read is done using the get function.    
•	A valid read will store the contents read into a file specified by the get function.  


2)	Server

•	The initial ephemeral port number is set by the user.    
•	Socket is created and initialized with the server properties for UDP server.  
•	The server socket is bound with the listen file descriptor.  
•	listen() and accept() are not used in UDP. Instead, recvfrom() and sendto() are used for communication with the clients.  
•	A RRQ from the client is received and is checked for the correctness of opcode, filename and mode of transfer.  
•	If any mismatch is found, the error message with appropriate opcode and error number is sent to the client and the child (zombie) process is removed.  
•	If it matches the TFTP protocol format, a child process is created using fork() and a new socket id is created for the client.  
•	The file is opened and read based on the mode specified by the client and packed into blocks of 512 bytes with opcode and block number.  
•	If ACK arrives before the timer expires, the timer is reset.  
•	In case no ACK is received for a block, it is retransmitted after waiting for 5s. This process is repeated 5 times. If still there is no response from the client, transmission is stopped.  


## Program Usage:

	The Makefile in /MP3 has the commands to compile the TFTP server program and generate the executables.

To run the program, firstly you must generate the executables by running make all  

Server can be run by  
```bash
./Server <server_ip> <port_number> <max_number_clients>  
```

Example:
```bash
./Server 5000    
```

## File Organization:  
**Server**\_**Socket**: Directory to store codes for UDP server programming   
\-src: Source code (\*.c)   
\-\-Server\_MP3.c: Source code to implement UDP server service  
\-\-Server\_tftp.c: Source code to implement functionalities of Server (ERR, SEND_MSG, RECV_ACK)    
\-\-Server\_sig\_handle.c: Source code to handle SIGCHLD signals to kill zombie process after clients terminated   
\-include: Header files corresponding to the source codes (\*.h)  
\-\-Server\_MP2.h  
\-\-Server\_sbcp.h  
\-\-Server\_sig\_handle.h  


**File_ Vault**: The text and binary files used to send to the client.  

**Makefile**: Makefile to generate executables for server.  






