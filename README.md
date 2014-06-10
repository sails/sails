sails
=====

sails is a platform for network applications. sails uses an event-driven I/O model.

c++

server
------

because of multiqueue netcard, so sails will make network threads number is configurable, make data queue no lock.


    +----------------------------------------------------------------------------------------------------------------+
	|			  			   	  			 					  		   										 	 |
	|			  			      	  	   	 					  		   		   +-----------+  				 	 |
	|			  			   	   +----------+	   	   	   	   	   	   	   	   	   |           |   	   	   		 	 |
	|             		  	   	   |  Decode -+	   	   	   	   	   	  	   		   |  Logic    +------------+	 	 |
	|				  	   	 	   +---+------+----------->+----+----+------------>+  thread1  |  			|	 	 |
	|				   	    	       	^      			 / |       	 \	   		   +-----------+  			|	 	 |
	|	   +------------+---------------+	 		 	/  |  Queue  |\    									|	 	 |
	|	   |   	  	    | 	  	 			 		   /   +---------+ \   									|	 	 |
	|	   | Recv/Send 	+<---------+----------+		  /   		  	    \  		   +------------+			|	 	 |
	|	   +------------+  	 	   |  Encode  |		 / 	  		  		 \  	   |   	   	 	+-----------+	 	 |
	|					   	   	   +----------+		/ 	  		  		  \  	   |  ........	|			|	 	 |
	|					      	       ^       	   / 	  		  		   \  	   +------------+			|	 	 |
	|                                  |          /       		  		   	\   							|	 	 |
	|					 	  	 	   +-------------------+---------+	   	 \    							|	 	 |
	|					 	   	 				/ 		   |   	   	 |	   	  \    +------------+			|	 	 |
	|					 	    			   / 		   |  Queue  |<----+   \   |  Logic	    |	    	|	 	 |
	|					 		   +----------/			   +---------+	   |	\  |  threadn   +-----------+	 	 |
	|					 		   |  Decode  |			  		  		   |	   +------------+			|	 	 |
	|      +------------+--------->+----------+                   		   |	   							|	 	 |
	|      |            |                                  +---------+     |								|	 	 |
	|	   | Recv/Send	|								   |     	 | 	   |								|	 	 |
	|	   +------------+ <--------+----------+	  		   |  Queue	 |<----+--------------------------------+	 	 |
	|	                 		   |  Encode  <------------+---------+ 										 	 	 |
	|							   +----------+			               											 	 |
	|							              																	 	 |
	|                                                                                                            	 |
	|                                                                                                                |
	|                                                                                                                |
	|                                                                                                                |
	|                                                                                                                |
	|																												 |
	|                                                                                                                |
	+----------------------------------------------------------------------------------------------------------------+



client
------

* sync(easy): each request must rebuild a new connect with server
* sync(hard): reuse a connect for many request in multithread, manager a struct{ticketid, condition_variable}, and interface wait for condition variable, when recv ticketid from server, and get this struct
and notify condition variable.

* async:manager a struct{ticketid, function(void *)}, when get ticketid from server, invoke function


server status
-------------

with ctemplate, sails build-in status report display by html file


build
=====

because the use of c++11 regex features, so need gcc 4.9 or above


performance
===========

test computer:

* cpu  :   T6400  @ 2.00GHz, cpu num:1, cpu cores:2

* memory : 5G

* server and client run on this computer

* build with -O2

**one client sync call method 10000 times**


    sails@xu:~/workspace/sails/example/echo_sync$ time ./client 
    clients thread:1
    
    real	0m1.286s
    user	0m0.159s
    sys	    0m0.254s
and in the server two thread to handle message, in this case will reach 7800 tps.

**two client sync call method 10000 times**


    sails@xu:~/workspace/sails/example/echo_sync$ time ./client 8000 2
    clients thread:2
    
    real	0m2.420s
    user	0m0.306s
    sys	    0m0.493s
8300 tps.


**five client thread sync call, and each 10000 times**

    sails@xu:~/workspace/sails/example/echo_sync$ time ./client 8000 5
    clients thread:5
    
    real	0m6.232s
    user	0m0.774s
    sys	    0m1.252s

8000 tps


test computer2:

* cpu  :   E5504  @ 2.00GHz, cpu num:2, cpu cores:4

* memory : 24G, 21G free

* server and client run on this computer

* build with debug

**one client sync call method 10000 times**

    clients thread:1
    
    real	0m-----s
    user	0m-----s
    sys	    0m-----s

**eight client sync call method 10000 times(8*10000)**

    clients thread:8
    
    real	0m-----s
    user	0m-----s
    sys	    0m-----s

