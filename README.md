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
    
    real	0m3.514s
    user	0m0.960s
    sys	    0m0.289s
and in the server two thread to handle message, in this case will reach 8000 tps.

**five client thread sync call, and each 10000 times**

    sails@xu:~/workspace/sails/example/echo_sync$ time ./client 8000 5
    clients thread:5
    
    real	0m18.417s
    user	0m4.767s
    sys	    0m1.443s

also 8000 tps
