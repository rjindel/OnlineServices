To find the most suitable data centre I wait for the first data centre to return
a certain number of packets. It is assumed that given a large enough sample size
this server will have the lowest latency and number of lost packets.
In the method I use, I send a packet and then wait for a certain amount of time
to receive a return packet before sending another packet.
I considered having two separate threads, for sending and receiving packets.
This would require additional synchronisation and be more complex, and so
I decided that this would be overkill.
Similarly I considered IOCP to also be overkill for the requirements.

There are a number of things that I would do differently in a production
environment. I have listed these below and the rationale behind them.

MessagePack
I have specified the include and library path in the project. In a production
environment I would either use an environment variable or put the directories
in the default user property sheets.

Hard coded values
I have mainly restricted these to the main file (OnlineService.cpp).
Normally hard coded value are avoided at all cost and this would be done by 
storing the values in some form of configuration file. In a production 
environment there would be a standard method to retrieve this data. While a 
simple implementation would be trivial it would be beyond the scope of this test 
and would raise several other potential issues, such as should the client secret 
be stored in a plain text file?

Error handling 
Again in a production environment there would be a standard method for error 
handling.

I throw a string on a few occasions. These represent exceptional situation that
shouldn't occur in production code and would require the application to exit. As 
such I throw string as it quickly and clearly expresses in code the error that 
has occurred. In production code I'd probably use a ASSERT_MSG style macro, which 
could be removed automatically from certain builds if needed. Similarly I might 
wrap the code in try\catch block that could handle the clean up, before exiting 
the application gracefully, depending on the defined error handling standard.

When other errors occur I display the error using printf. Ideally I would use a
debug function or class that would let me specify verbosity level and which
could avoid debug spam by only showing an error once and possibly showing an
error count.  
Additionally the debug output might be more verbose as I could filter the output
based on the verbosity level and redirect the output. 
If an error occurs which means the procedure must exit I return a false or
EXIT_FAILURE. This could be changed to an error code depending on the
requirements of the calling code.

Test code
Some code was written to test various functions including a simple server. This
has all been removed as a good test framework would be well beyond the scope of
this test and simple test framework would reduce the code quality.

I have left in a function which displays the Qos stats to the console as this
particularly useful. However, the console output is fairly basic. This should
ideally, as mentioned above, handle debug message better e.g. display an error
message only once with an error count.
It should buffer the input, so multiple thread can output debug messages to it
cleanly.
Also the function waits for user input, instead of exiting when a suitable data
centre is found.


KNOWN ISSUES

Naked pointer
When getting the QoS servers I pass a naked pointer to function which allocates
the data, which must then be manually deleted by the calling function. This
should generally be avoided. Ideally a vector class would be passed into this
function, however the QosConnection class has members which have a deleted copy
constructors, which is required by the vector class. 
Several other techniques were attempted, such as using move semantics and
allocating the required members in the constructor but these all had there own
issues.

Buffer full
When calling WSARecv, a WSAENOBUFS error is sometimes returned. After some
investigation, the cause of this couldn't be found, however the data seems to be
received correctly.
