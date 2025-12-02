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
#include "header.h"



char* client_init();
char* req_init();

int main()
{
    client_init();

    return 0;
}


char* client_init()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        exit(1);
    }

    char* res = malloc(1024 * sizeof(char));

    while(1)
    {
        char* req = req_init();
        if(req == NULL)
        {
            continue;
        }
        write(sock, req, strlen(req));
        free(req);

        int n = read(sock, res, 1023);
        if(n < 0)
        {
            perror("read");
            exit(1);
        }
        if(n == 0)
        {
            printf("connection lost\n");
            break;
        }
        res[n] = '\0';

        printf("%s\n", res);

        printf("Do you want to send another request? (y/n): ");
        char choice[4];
        if(fgets(choice, sizeof(choice), stdin) == NULL)
        {
            printf("Input error, exiting.\n");
            break;
        }

        if(choice[0] == 'n' || choice[0] == 'N')
        {
            printf("Exiting...\n");
            break;
        }else{
            continue;
        }

    }
    
    close(sock);
    return res;
}