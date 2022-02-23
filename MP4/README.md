# Machine Problem 4

                                                  HTTP Proxy – Team_11

## Team Members:
1.	Weiting Lian
2.	Karthik Eswar Krishnamoorthy Natarajan

## Contributions:

1.	Weiting handled the server-side programming, cache implementation and Makefile organization.  
2.	The client programming, http request parsing, debugging, verifying the test cases and the README were done by Karthik Eswar.  

## Program Flow:
1)	Client

•	The Serverip, portnumber and the URL are collected from the user.    
•	The Socket function is called to establish a connection-oriented communication.     
•	The URL is parsed into hostname and filename and packed into the HTTP GET request format.    
•	The GET request is sent to the server and the response is read.  
•	The contents of the file are separated from the HTTP RESPONSE header and stored into a file.


2)	Proxy

•	The initial port number and IP address is set by the user.      
•	File descriptors are declared for listen and connect functions.    
•	The select function is used to actively listen for new connections and at the same time read data from existing clients.   
•	The received URL is checked with the contents of the cache by comparing host and file names.   
•	In case of a cache miss, the request is proxied to the remote HTTP server.   
•	The header of the response is parsed and the URL, Filename, Last Modified, Last Accessed, Expires field are stored in the cache. The content is then sent to the client.  
•	In case of a cache hit, the cached data is checked whether it is valid. If yes, then it is sent to the client. Else, the request is proxied to the remote HTTP server.    
•	The proxy supports the request headers with If-Modified-Since.    


## Program Usage:

	The Makefile in /MP4 has the commands to compile the TFTP server program and generate the executables.

To run the program, firstly you must generate the executables by running make all  

Server can be run by  
```bash
./Proxy <IP> <port_number>   
```
The Client can be run  by 
```bash
/Client_1 <IP> <port_number> <URL> 
```

Example:
```bash
./Proxy 127.0.0.1 5000
./Client_1 127.0.0.1 5000 https://web.mit.edu/dimitrib/www/datanets.html
```

## File Organization:  
**Server**\_**Socket**: Directory to store codes for server programming   
\-src: Source code (\*.c)   
\-\-Server\_MP4.c: Source code to implement the proxy server service.
\-\-Server\_http.c: Source code to implement cache and get and send response functionalities of Server.      
\-include: Header files corresponding to the source codes (\*.h)  
\-\-Server\_MP4.h  
\-\-Server\_http.h  

**Client**\_**Socket**: Directory to store codes for client programming 

**File_ Vault**: The text and binary files used to send to the client.  

**Makefile**: Makefile to generate executables for server.  






