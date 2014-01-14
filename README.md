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
