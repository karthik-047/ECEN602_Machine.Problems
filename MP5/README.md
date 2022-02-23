# Machine Problem 5

                                                  NS-2 Simulation – Team_11

## Team Members:
1.	Weiting Lian
2.	Karthik Eswar Krishnamoorthy Natarajan

## Contributions:

1.	Weiting handled the initial node setup, input argument handling, and documentation.    
2.	The throughput logic, trace file generation, and README were done by Karthik Eswar.   

## Program Flow:

•	The TCP flavor and the case number are received from the user in stdin and both the arguments are checked for correctness.      
•	According to the case number, the end-to-end link delays between src2-R1 and R2-src are assigned.       
•	The nodes are declared, assigned labels, and necessary formatting is done.      
•	The links between the nodes are defined with the given bandwidth and delay values.    
•	The links are oriented for appropriate viewing in the network animator.  
•	The TCP agents and sinks are set to the src and rcv nodes respectively.        
•	An FTP application is layered over the TCP protocol, and the start and stop times are defined for the FTP application.      
•	The bytes received at src1 and src2 are stored in trace files and the average bytes received between 100th and 400th seconds gives the average throughput of the flows.     
•	The average throughput and the ratio between them are displayed on the terminal.     


## Program Usage:


The program ns2.tcl can be used to run the simulation.
```bash
	ns ns2.tcl <tcp_flavor> <case_no>
```

Example:
```bash
 ns ns2.tcl vegas 3
 ns ns2.tcl sack 1
```
