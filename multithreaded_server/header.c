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


char* req_init()
{
    char* blueprint = "%s %s %s\r\n\r\n%s";

    char* vers = "HTTP/1.1";
    printf("Method: ");
    char method[10];
    while(1)
    {
        fgets(method, sizeof(method), stdin);
        method[strcspn(method, "\n")] = '\0';
        if(strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "ECHO") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "OPTIONS") == 0)
        {
            break;
        }else{
            printf("Try again\n");
            continue;
        }
    }

    char dest[50] = "";
    char body[1024] = "";

    if(strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0)
    {
        printf("Destination: ");
        fgets(dest, sizeof(dest), stdin);
        dest[strcspn(dest, "\n")] = '\0';
    }

    if(strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "ECHO") == 0)
    {
        printf("Body: ");
        fgets(body, sizeof(body), stdin);
        body[strcspn(body, "\n")] = '\0';
    }

    size_t len = strlen(method) + strlen(vers) + strlen(dest) + strlen(body) + 64;

    char* buffer = malloc(len);
    if(!buffer)
    {
        perror("malloc");
        return NULL;
    }

    snprintf(buffer, len, blueprint, method, dest, vers, body);
    
    return buffer;
}

char* res_init(int status, char* body)
{
    char* blueprint = "%s %d %s\r\n" "\r\n%s";
    char* vers = "HTTP/1.1";
    char msg[64];

    switch(status)
    {
        case 100:
            strcpy(msg, "OK");
            break;
        case 101:
            strcpy(msg, "File Not Found");
            break;
        case 102:
            strcpy(msg, "File is empty");
            break;
        default:
            strcpy(msg, "Unknown Status");
            break;
    }

    if(body == NULL)
    {
        body = "";
    }

    size_t len = strlen(vers) + strlen(msg) + strlen(body) + sizeof(status) + 8 + 16;
    char* buffer = malloc(len);
    if(buffer == NULL)
    {
        perror("malloc");
        exit(1);
    }
    
    snprintf(buffer, len, blueprint, vers, status, msg, body);

    return buffer;
}

void parser_req(const char* msg, req_struct* req)
{
    char header[1024];
    strncpy(header, msg, sizeof(header)-1);
    header[sizeof(header)-1] = '\0';

    char* method = strtok(header, " ");
    if(!method)
    {
        method = "";
    }
    char* dest = strtok(NULL, " ");
    if(!dest)
    {
        dest = "";
    }
    char* vers = strtok(NULL, " ");
    if(!vers)
    {
        vers = "";
    }
    char* body = strstr(msg, "\r\n\r\n");
    if(body)
    {
        body += 4;
    }else{
        body = "";
    }
    req->method = strdup(method);
    req->dest = strdup(dest);
    req->vers = strdup(vers);
    req->body = strdup(body);
}


void parser_res(const char* msg, res_struct* res)
{
    char header[1024];
    if(!msg)
    {
        return;
    }
    strncpy(header, msg, sizeof(header)-1);
    header[sizeof(header)-1] = '\0';


    char* vers = strtok(header, " ");
    char* code_str = strtok(NULL, " ");
    char* com = strtok(NULL, "\r\n");

    if (!vers || !code_str || !com) {
        res->vers = strdup("");
        res->com = strdup("");
        res->code = 0;
    } else {
        res->vers = strdup(vers);
        res->com = strdup(com);
        res->code = atoi(code_str);
    }

    char* delimiter = strstr(msg, "\r\n\r\n");
    if(!delimiter)
    {
        res->body = strdup("");
    }else{
        res->body = strdup(delimiter + 4);
    }

    res->vers = strdup(vers);
    res->com = strdup(com);
    
}

void request_get(req_struct* req, int client_fd)
{
    int status = 100;
    size_t size = 0;

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
        }else{
            perror("open");
            return;
        }
    }else{
        struct stat st;
        if(fstat(fd, &st) < 0)
        {
            perror("fstat");
            close(fd);
            exit(1);
        }

        size = st.st_size;
        if(size == 0)
        {
            status = 102;
            close(fd);
            return;
        }

        char* body = malloc(size + 1);
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
            close(fd);
            return;
        }
        body[n - 1] = '\0';

        char* res = res_init(status, body);
        if(res == NULL)
        {
            perror("res");
            close(fd);
            return;
        }
        write(client_fd, res, strlen(res));
        free(res);
        free(body);
        
    }

    free(req->dest);
    free(req->method);
    free(req->vers);
}

void request_post(req_struct* req, int client_fd)
{
    int status = 100;
    size_t size = 0;
    size_t body_len = 0;
    char* buf = req->body;
    if(buf != NULL)
    {
        body_len = strlen(buf) + 1;
    }else{
        return;
    }
    char* body = malloc(body_len + 2);
    if(body == NULL)
    {
        perror("malloc");
        return;
    }
    memcpy(body, buf,  strlen(buf));
    body[body_len - 1] = '\n';
    body[body_len] = '\0';

    char* destination;
    size_t len = strlen("files/") + strlen(req->dest) + 1;
    destination = malloc(len);
    if (!destination) {
        perror("malloc failed");
        exit(1);
    }
    strcpy(destination, "files/");
    strcat(destination, req->dest);  
    
    int fd = open(destination, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if(fd < 0)
    {
        perror("open");
        return;
    }else{
        struct stat st;
        if(fstat(fd, &st) < 0)
        {
            perror("fstat");
            return;
        }
        size = st.st_size;

        size_t written = write(fd, body, body_len);
        if(written != body_len)
        {
            perror("write");
        }
        status = 100;
    }
    body[strcspn(body, "\n")] = '\0';
    char* res = res_init(status, body);
    if(res == NULL)
    {
        perror("res_init");
        close(fd);
        return;
    }
    write(client_fd, res, strlen(res));
    free(res);
    close(fd);

    free(req->dest);
    free(req->method);
    free(req->vers);
}

void request_put(req_struct* req, int client_fd)
{
    int status = 100;
    size_t size = 0;
    size_t body_len = 0;
    char* buf = req->body;
    if(buf != NULL)
    {
        body_len = strlen(buf) + 1;
    }else{
        return;
    }
    char* body = malloc(body_len + 2);
    if(body == NULL)
    {
        perror("malloc");
        return;
    }
    memcpy(body, buf,  strlen(buf));
    body[body_len - 1] = '\n';
    body[body_len] = '\0';

    char* destination;
    size_t len = strlen("files/") + strlen(req->dest) + 1;
    destination = malloc(len);
    if (!destination) {
        perror("malloc failed");
        exit(1);
    }
    strcpy(destination, "files/");
    strcat(destination, req->dest);  
    
    int fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
    if(fd < 0)
    {
        perror("open");
        return;
    }else{
        struct stat st;
        if(fstat(fd, &st) < 0)
        {
            perror("fstat");
            return;
        }
        size = st.st_size;

        size_t written = write(fd, body, body_len);
        if(written != body_len)
        {
            perror("write");
        }
        status = 100;
    }
    body[strcspn(body, "\n")] = '\0';
    char* res = res_init(status, body);
    if(res == NULL)
    {
        perror("res_init");
        close(fd);
        return;
    }
    write(client_fd, res, strlen(res));
    free(res);
    close(fd);

    free(req->dest);
    free(req->method);
    free(req->vers);
}

void request_echo(req_struct* req, int client_fd)
{
    int status = 100;
    size_t body_len = 0;
    char* buf = req->body;
    if(buf != NULL)
    {
        body_len = strlen(buf) + 1;
    }else{
        return;
    }
    char* body = malloc(body_len);
    if(body == NULL)
    {
        perror("malloc");
        return;
    }
    memcpy(body, buf,  body_len);
    body[body_len] = '\0';

    char* res = res_init(status, body);
    if(res == NULL)
    {
        perror("res_init");
        free(body);
        return;
    }
    free(body);
    write(client_fd, res, strlen(res));
    free(res);

    free(req->body);
    free(req->dest);
    free(req->method);
    free(req->vers);
}

void request_options(req_struct* req, int client_fd)
{
    int status = 100;
    char* options = "GET / POST / PUT / ECHO";
    char* res = res_init(status, options);
    size_t len = strlen(options) + sizeof(status) + 64;

    write(client_fd, res, len);
}