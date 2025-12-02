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
#include <sys/stat.h>
#include <errno.h>
#include "header.h"

#define MAX_CLIENTS 5

void server_init();


void* client_thread(void* arg)
{
    int fd = *(int*)arg;

    req_struct req;
    res_struct res;

    while(1)
    {
        char buf[1024];
        int n = read(fd, buf, sizeof(buf));
        if(n < 0)
        {
            perror("read");
            exit(1);
        }
        if(n == 0)
        {
            printf("Client closed the connection\n");
            break;
        }
        buf[n] = '\0';

        parser_req(buf, &req);
        if(strcmp(req.method, "GET") == 0)
        {
            request_get(&req, fd);
        }
        if(strcmp(req.method, "POST") == 0)
        {
            request_post(&req, fd);
        }
        if(strcmp(req.method, "PUT") == 0)
        {
            request_put(&req, fd);
        }
        if(strcmp(req.method, "ECHO") == 0)
        {
            request_echo(&req, fd);
        }
        if(strcmp(req.method, "OPTIONS") == 0)
        {
            request_options(&req, fd);
        }
    }

    close(fd);
    return NULL;
}

int main()
{
    server_init();
    
    return 0;
}

void server_init()
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (mkdir("files", 0777) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if(listen(listener, MAX_CLIENTS) < 0)
    {
        perror("listener");
        exit(1);
    }
    printf("Listening...\n");

    struct pollfd fds[2];
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while(1)
    {
        int ret = poll(fds, 2, -1);
        if(ret < 0)
        {
            perror("poll");
            exit(1);
        }

        if(fds[0].revents & POLLIN)
        {
            int* client = calloc(1, sizeof(int));
            *client = accept(listener, NULL, NULL);
            if(*client < 0)
            {
                perror("accept");
                free(client);
                continue;
            }
            printf("Client connected\n");

            pthread_t tid;
            if(pthread_create(&tid, NULL, client_thread, client) < 0)
            {
                perror("pthread_create");
                close(*client);
                free(client);
                break;
            }
            if(pthread_detach(tid) < 0)
            {
                perror("pthread_detach");
                exit(1);
            }
        }

        if(fds[1].revents & POLLIN)
        {
            char text[20];
            int n = read(STDIN_FILENO, text, sizeof(text));
            if(n < 0)
            {
                perror("read");
                exit(1);
            }
            text[n] = '\0';
            if(strcmp(text, "EXIT") == 0 || strcmp(text, "EXIT\n") == 0)
            {
                printf("Server shutting down...\n");
                break;
            }
        }
    }
    close(listener);
}