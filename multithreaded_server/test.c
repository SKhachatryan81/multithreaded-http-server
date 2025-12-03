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
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "header.h"



// switch(status)
//     {
//         case 100:
//             strcpy(msg, "OK");
//             break;
//         case 101:
//             strcpy(msg, "File Not Found");
//             break;
//         case 102:
//             strcpy(msg, "File is empty");
//             break;
//         default:
//             strcpy(msg, "Unknown Status");
//             break;
//     }


void request_get(req_struct* req, int client_fd)
{
    int status = 100;
    size_t size = 0;
    char* body = NULL;

    char* destination;
    size_t len = strlen("files/") + strlen(req->dest) + 1;
    destination = malloc(len);
    if (!destination) {
        perror("malloc failed");
        exit(1);
    }
    strcpy(destination, "files/");
    strcat(destination, req->dest);  

   
    int fd = open(destination, O_RDONLY);
    if(fd < 0)
    {
        if(errno == ENOENT)
        {
            status = 101;
            body = NULL;
        }else{
            perror("open");
            free(destination);
            free(req->dest);
            free(req->method);
            free(req->vers);
            return;
        }
    }else{
        struct stat st;
        if(fstat(fd, &st) < 0)
        {
            perror("fstat");
            close(fd);
            return;
        }

        size = st.st_size;
        if(size == 0)
        {
            status = 102;
        }

        body = malloc(size + 1);
        if(!body)
        {
            perror("malloc");
            close(fd);
            return;
        }
        int n = read(fd, body, size);
        if(n < 0)
        {
            perror("read");
            close(fd);
            return;
        }
        if(n == 0)
        {
            status = 102;
            body = NULL;
        }else{
            body[n] = '\0';
        }
        close(fd);
    }
    
    char* res = res_init(status, body);
    if(res == NULL)
    {
        perror("res");
        return;
    }
    write(client_fd, res, strlen(res));
    free(res);
    free(body);
    free(destination);
        
    free(req->dest);
    free(req->method);
    free(req->vers);
}