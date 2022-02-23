
# MP5: Network Simulation
	set ns [new Simulator]

#Checking Input arguments	
if { $argc != 2 } {
	puts "\[Error] Incorrect number of input arguments."
    puts "Proper syntax:"
    puts "ns ns2.tcl <TCP Flavor> <Case Number>"
    exit -1
	}
    
    set tcp_flavor [lindex $argv 0]
    set case_no    [lindex $argv 1]
    
#Checking case number is valid
    if {$case_no > 3 || $case_no < 1} {
		puts "\[Error] Invalid case number. Expects: 1, 2, 3;"
        exit
    }
       
#Checking flavor is valid
	global flavor
    if {$tcp_flavor == "SACK" || $tcp_flavor == "sack"} {
        set flavor "Sack1"
    } elseif {$tcp_flavor == "VEGAS" || $tcp_flavor == "vegas"} {
        set flavor "Vegas"
    } else {
        puts "\[Error] Invalid TCP flavor. Expects: VEGAS, SACK;"
        exit
    }
	
#Choose the input delay 
	global link2_delay
	set link2_delay 0
    switch $case_no {
        global link2_delay
        1 {set link2_delay "12.5ms"}
        2 {set link2_delay "20.0ms"}
        3 {set link2_delay "27.5ms"}
    }
    
    puts "\nTCP $tcp_flavor with End-to-end delay $link2_delay\n"    
#Store data in output files
    set file1 [open out1.tr w]
    set file2 [open out2.tr w]
    
#setup new simulator

    set file "out_$flavor$case_no"
    
#open tracefile
    set tracefile [open out.tr w]
    $ns trace-all $tracefile
    
#open NAM tracefile
    set namfile [open out.nam w]
    $ns namtrace-all $namfile
      
#Node declaration and color assignment 
    set src1 [$ns node]
    set src2 [$ns node]
    set R1   [$ns node]
    set R2   [$ns node]
    set rcv1 [$ns node]
    set rcv2 [$ns node]
#set label and shape  
	$src1 label src1
	$src2 label src2
	$R1   label R1
	$R2   label R2
	$rcv1 label rcv1
	$rcv2 label rcv2

	$R1 shape square
	$R2 shape square 

#Defining Links
    $ns duplex-link $R1   $R2 1.0Mb  5ms DropTail
    $ns duplex-link $src1 $R1 10.0Mb 5ms DropTail
    $ns duplex-link $rcv1 $R2 10.0Mb 5ms DropTail
    $ns duplex-link $src2 $R1 10.0Mb $link2_delay DropTail
    $ns duplex-link $rcv2 $R2 10.0Mb $link2_delay DropTail
    
#Defining Nam interfaces
    $ns duplex-link-op $R1   $R2 orient right
    $ns duplex-link-op $src1 $R1 orient right-down
    $ns duplex-link-op $src2 $R1 orient right-up
    $ns duplex-link-op $R2   $rcv1 orient right-up
    $ns duplex-link-op $R2   $rcv2 orient right-down
    
#Defining TCP Agents for connection and color codes assignment
    set tcp1 [new Agent/TCP/$flavor]
    set tcp2 [new Agent/TCP/$flavor]
    $ns attach-agent $src1 $tcp1
    $ns attach-agent $src2 $tcp2
    
#Defining TCP sink connection
    set sink1 [new Agent/TCPSink]
    set sink2 [new Agent/TCPSink]
    $ns attach-agent $rcv1 $sink1
    $ns attach-agent $rcv2 $sink2
    
    $ns connect $tcp1 $sink1
    $ns connect $tcp2 $sink2
    

#Creating FTP traffic on top of TCP connection
    set ftp1 [new Application/FTP]
    set ftp2 [new Application/FTP]
    
    $ftp1 attach-agent $tcp1
    $ftp2 attach-agent $tcp2
#assigning time slots for the flows	
    $ns at 0 "$ftp1 start"
    $ns at 0 "$ftp2 start"
    $ns at 400 "$ftp1 stop"
    $ns at 400 "$ftp2 stop"
	$tcp1 set fid_ 0
    $tcp2 set fid_ 1

# Assign data flow colors
    $ns color 0 Yellow
    $ns color 1 Blue
    
#initialize throughput variables	
	set tp1 0
    set tp2 0
    set count 0
	
#stat procedure to actually write data to output files
    proc stat {} {
        global ns sink1 sink2 file1 file2 tp1 tp2 count
        
        
#set sample time
        set time 1

#get current time
        set now [$ns now]
        
#bytes received by sinks
        set bw1 [$sink1 set bytes_]
        set bw2 [$sink2 set bytes_]
          
# calculate the bandwidth (in Mbps) and write it to the files
        puts $file1 "$now [expr $bw1/$time*8/1000000.0]"
        puts $file2 "$now [expr $bw2/$time*8/1000000.0]"
#start calculating throughputs after 100s
	if { $now >= 100 } {
		set tp1 [expr $tp1+ $bw1/$time*8/1000000.0]
		set tp2 [expr $tp2+ $bw2/$time*8/1000000.0]
    }
#increment sample counter	
	set count [expr $count + 1]
	
#reset bytes_ on the sinks
    $sink1 set bytes_ 0
    $sink2 set bytes_ 0
                
# re-schedule the procedure
    $ns at [expr $now+$time] "stat"
    }
    
#Finish procedure for terminating simulations
    proc finish {} {
        global ns tracefile namfile tp1 tp2 count
        $ns flush-trace
        puts "Avg throughput for Src1=[expr $tp1/($count-100)] MBits/sec\n"
        puts "Avg throughput for Src2=[expr $tp2/($count-100)] MBits/sec\n"
        puts "The ratio of throughputs src1/src2 = [expr $tp1/($tp2)]\n"
	    close $tracefile
        close $namfile
        #exec nam out.nam &
        exit 0
    }

#calling stat procedure to stat simulation
    $ns at 0 "stat"
	
#finish procedure
    $ns at 400 "finish"

    $ns run
