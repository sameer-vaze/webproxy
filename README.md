# webproxy
Reverse proxy with caching enabled

webproxy.c 

Link to Video: https://youtu.be/8zBk48IELY0

This project submission contains a .c file that can be compiled to run a Webproxy.

---webproxy.c---

Compilation argument is:
gcc webproxy.c -o webproxy -lpthread


Once compiled an executable server is created. Arguments passed are:

	1. Port number for the webproxy
	2. Timeout value in seconds for cached items

Execution is done by the following way.
Example: ./webproxy <port number> <timeout in seconds>

HTTP Methods Implemented:

GET
This method is used to fetch webpages from the server and display them on the browser. The request
is of the form of:
	
	GET / HTTP/1.1
	Host: localhost:8097
	Connection: keep-alive
	Upgrade-Insecure-Requests: 1
	User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.89 Safari/537.36
	Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
	Accept-Encoding: gzip, deflate, sdch
	Accept-Language: en-US,en;q=0.8

All other methods are not required to be implemented.

Features Implemented:

1. Multithreaded proxy:

This feature has been implemented using pthreads. Its purpose is to handle multiple requests at the same time. The decision to
use pthreads was because of the requirement to have a common data structure(linked list) that is accessible by all threads and
editable by all threads too. These threads call the parseRequest function that takes in an integer value to identify the thread
that has been run.

2. Caching

This feature has been implemnted with the following features:
	
	1. Timeout: This a time value that is passed as a command line argument and is used to determine if a locally cached URL is
	still accessible. The reason to have a timeout is because data on the internet is not static, it may change quite often. So
	storing the contents of the URLs that are often accessed in the proxy saves time and also helps regulate traffic that the 
	webserver will recieve. 

	2. Hashing: Hashing is used with the URL that is being accessed to ease the search and access of URLs that have been locally
	cached. By using the djb2 hash function we make it possible to store the URL contents locally by mapping each URL to a long 
	integer value. So any future search for the cached URL does not require a search for the entire URL length but simply the 
	hash value. 

Whenever a new URL is tried to be accessed we store the contents locally and update the linked list with the following items:

	1. unsigned long hash: This is the hash corresponding to the URL being stored
	2. int hour, int minutes, and int seconds: These keep track of when page is cached and is used to check the time elapsed 
	based on the system time
	3. int available: It is an integer flag that is 1 if the cached page is accessible and 0 if the timeout has happened and 
	the cache is no longer valid.

The webproxy forwards the request that it recieved from the client to the actual webserver by creating a new socket. The
contents of the webpage are then relayed to the webproxt which stores the webpage locally as well as forwards it back to the client.

As the page is stored locally we update the linked list with the hash value of the URL and the time at which the cache was made. As
the proxy runs it regularly traverses theough the linked list and checks for each hash value corresponding to the webpages it has
stroed locally and changes the available flag as required. 

If the webpage is cached and the timeout has not elapsed the proxy returns the cached web page than retrieving the web page from the 
original webserver.

