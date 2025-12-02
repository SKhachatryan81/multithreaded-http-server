This project is created to mimic the workflow of a multithreaded http client-server architecture.
It contains a multithreaded server which the user doesn't interact with, a client code, which is made for the user and a library, where all of the functions needed are held.
The server supports several http methods which are present in real servers:
1. GET -> used to get the content of the file.
2. POST -> used to append to the content of the file, if the file doesn't exist, it is created.
3. PUT -> used to truncate the content of the file, if the file doesn't exist, it is created.
4. ECHO -> used to test the work of the server.
5. OPTIONS -> used to list all the methods supported by the server.

The server requires http requests and answers with http responses, as in real http server.
