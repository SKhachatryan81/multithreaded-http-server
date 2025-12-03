This project mimics the workflow of a multithreaded HTTP client–server architecture. It includes a multithreaded server (not directly interacted with by the user), a client program intended for user interaction, and a library containing all required helper functions.

The server supports several HTTP methods commonly found in real servers:

GET – retrieves the content of a file.

POST – appends data to a file; if the file does not exist, it is created.

PUT – replaces the entire content of a file; if the file does not exist, it is created.

ECHO – used for testing server functionality.

OPTIONS – returns a list of methods supported by the server.

The server expects well-formed HTTP requests and returns HTTP responses, following real HTTP server conventions.

The project provides a Bash script that enables the user to launch one server terminal and multiple client terminals for testing purposes.
