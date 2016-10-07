There are a number of things that I would do differently in a production environment. I have listed these below and the rationale behind them.

Hard coded values
I have mainly restricted these to the main file (OnlineService.cpp).
Normally hard coded value are avoided at all cost and this would be done by storing the values in some form of configuration file. In a production environment there would be a standard method to retrieve this data. While a simple implementation would be trivial it would be beyond the scope of this test and would raise several other potential issues, such as should the client secret be stored in a plain text file?

Error handling 
Again in a production environment there would be a standard method for error handling.

I throw a string on a few occasions. These represent exceptional situation that
shouldn't occur in production code and would require the application to exit. As such I throw string as it quickly and clearly expresses in code the error that has occurred. In production code I'd probably use a ASSERT_MSG style macro, which could be removed automatically from certain builds if needed. Similarly I might wrap the code in try\catch block that could handle the clean up, before exiting the application gracefully, depending on the defined error handling standard.

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

Naked pointer
When getting the QoS servers I pass a naked pointer to function which allocates
the data, which must then be manually deleted by the calling function. This
should generally be avoided. Ideally a vector class would be passed into this
function, however the QosConnection class has members which have a deleted copy
constructors, which is required by the vector class. 
Several other techniques were attempted, such as using move semantics and
allocating the required members in the constructor but these all had there own
issues.



