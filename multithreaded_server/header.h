#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>


typedef struct {
    char* method;
    char* dest;
    char* vers;
    char* body;
} req_struct;

typedef struct {
    char* vers;
    char* com;
    int code;
    char* body;
} res_struct;

void parser_req(const char* msg, req_struct* req);
void parser_res(const char* msg, res_struct* res);
void request_get(req_struct* req, int client_fd);
void request_post(req_struct* req, int client_fd);
void request_put(req_struct* req, int client_fd);
void request_echo(req_struct* req, int client_fd);
void request_options(req_struct* req, int client_fd);
char* res_init(int status, char* body);
char* req_init();




#endif
