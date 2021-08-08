#include <winsock2.h>
#include <stdio.h>
#include <sys/stat.h>
#pragma comment(lib,"ws2_32.lib")

#define SERVER_PORT 5028       
#define HOSTLEN 256          
#define BACKLOG 10     
#define SERVER_IP 127.0.0.1

int sendall(int s, char* buf, int* len)
{
    int total;
    int bytesleft;
    int n;
    total = 0;
    bytesleft = *len;
    while (total < *len)
    {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total;
    return n == -1 ? -1 : 0;
}

void wrong_req(int sock)
{
    char* error_head = "HTTP/1.1 501 Not Implemented\r\n";
    char* error_type = "Content-type: text/plain\r\n";
    char* error_end = "\r\n";
    char* prompt_info = "The command is not yet completed\r\n";

    int len;
    len = strlen(error_head);
    if (sendall(sock, error_head, &len) == -1)
    {
        printf("Sending failed!");
        return;
    }

    len = strlen(error_type);
    if (sendall(sock, error_type, &len) == -1)
    {
        printf("Sending failed!");
        return;
    }

    len = strlen(error_end);
    if (sendall(sock, error_end, &len) == -1)
    {
        printf("Sending failed!");
        return;
    }

    len = strlen(prompt_info);
    if (sendall(sock, prompt_info, &len) == -1)
    {
        printf("Sending failed!");
        return;
    }
}

int not_exit(char* arguments)
{
    struct stat dir_info;
    if (stat(arguments, &dir_info) == -1)
        return 0;
    else
        return dir_info.st_size;
}

void file_not_found(char* arguments, int sock)
{
    char* error_head = "HTTP/1.1 404 Not Found\r\n";
    int len;
    char* error_type = "Content-type: text/plain\r\n";
    char* error_length = "Content-Length: %d\n\n";
    char* error_end = "\r\n";
    char prompt_info[50] = "Not found:  ";

    len = strlen(error_head);
    if (sendall(sock, error_head, &len) == -1)
    {
        printf("Sending error!");
        return;
    }
    len = strlen(error_type);
    if (sendall(sock, error_type, &len) == -1)
    {
        printf("Sending error!");
        return;
    }
    len = strlen(error_length);
    if (sendall(sock, error_length, &len) == -1)
    {
        printf("Sending error!");
        return;
    }

    len = strlen(error_end);
    if (sendall(sock, error_end, &len) == -1)
    {
        printf("Sending error!");
        return;
    }

    strcat(prompt_info, arguments);
    len = strlen(prompt_info);
    if (sendall(sock, prompt_info, &len) == -1)
    {
        printf("Sending error!");
        return;
    }
}

void send_header(int send_to, char* content_type)
{
    char* head = "HTTP/1.1 200 OK\r\n";
    int len;
    char temp_1[30] = "Content-type: ";
    len = strlen(head);

    if (sendall(send_to, head, &len) == -1)
    {
        printf("Sending error");
        return;
    }
    if (content_type)
    {
        strcat(temp_1, content_type);
        strcat(temp_1, "\r\n");
        len = strlen(temp_1);

        if (sendall(send_to, temp_1, &len) == -1)
        {
            printf("Sending error!");
            return;
        }


    }
}

char* file_type(char* arg)
{
    char* temp;
    if ((temp = strrchr(arg, '.')) != NULL)
    {
        return temp + 1;
    }
    return "";
}

void send_file(char* arguments, int sock)
{
    char* extension = file_type(arguments);
    char* content_type = "text/plain";
    char* body_length = "Content-Length: ";

    FILE* read_from;
    int readed = -1;
    struct stat statbuf;
    char read_buf[1024];
    int len;
    char length_buf[20];

    if (strcmp(extension, "html") == 0)
    {
        content_type = "text/html";
    }
    if (strcmp(extension, "gif") == 0)
    {
        content_type = "image/gif";
    }
    if (strcmp(extension, "jpg") == 0)
    {
        content_type = "image/jpg";
    }
    read_from = fopen(arguments, "rb");
    if (read_from != NULL)
    {
        send_header(sock, content_type);
        fstat(fileno(read_from), &statbuf);
        itoa(statbuf.st_size, length_buf, 10);
        send(sock, body_length, strlen(body_length), 0);
        send(sock, length_buf, strlen(length_buf), 0);
        printf("%s%s\n", body_length, length_buf);
        send(sock, "\n", 1, 0);
        send(sock, "\r\n", 2, 0);
        while (statbuf.st_size > 0)
        {
            if (statbuf.st_size > 1024)
            {
                len = fread(read_buf, 1, 1024, read_from);
                if (sendall(sock, read_buf, &len) == -1)
                {
                    printf("Sending error!");
                    continue;
                }
                statbuf.st_size -= 1024;
            }
            else
            {
                len = fread(read_buf, 1, statbuf.st_size, read_from);
                if (sendall(sock, read_buf, &len) == -1)
                {
                    printf("Sending error!");
                    continue;
                }
                statbuf.st_size = 0;
            }
        }

    }
    else
        printf("not find file!@\n");
}

void handle_req(char* request, int client_sock)
{
    char command[BUFSIZ];
    char arguments[BUFSIZ];
    strcpy(arguments, "./");

    if (sscanf(request, "%s%s", command, arguments + 2) != 2)
    {
        return;
    }

    printf("handle_cmd:    %s\n", command);
    printf("handle_path:   %s\n", arguments);

    if (strcmp(command, "GET") != 0)
    {
        wrong_req(client_sock);
        return;
    }
    if (not_exit(arguments) == 0)
    {
        printf("not such file!\n");
        file_not_found(arguments, client_sock);
        return;
    }
    send_file(arguments, client_sock);

    return;
}

int make_server_socket()
{

    const char* ip = NULL;
    struct sockaddr_in server_addr;
    int tempSockId;
    tempSockId = socket(PF_INET, SOCK_STREAM, 0);
    ip = (char*)malloc(sizeof(char));
error:
    printf("请输入服务器IP地址：");
    scanf("%s", ip);

    if (tempSockId == -1)
    {
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(tempSockId, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("IP地址绑定错误，请重新输入!\n");
        goto error;
    }
    if (listen(tempSockId, BACKLOG) == -1)
    {
        printf("监听错误，需要重启!\n");
        return -1;
    }
    return tempSockId;
}


void main(int argc, char* argv[])
{
    int server_socket;
    int acc_socket;
    int sock_size;
    int numbytes;
    char buf[512];
    struct sockaddr_in user_socket;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        fprintf(stderr, "error\n");
        exit(1);
    }
    printf("\n  ************* Simple Httpserver with C ***********\n\n\n");
    sock_size = sizeof(struct sockaddr_in);
    server_socket = make_server_socket();
    if (server_socket == -1)
    {
        printf("监听出错，需要重启\n");
        exit(2);
    }
    else
    {
        printf("服务器启动成功！默认端口号为：5028\n\n正在运行....\n");
    }

    while (1)
    {
        acc_socket = accept(server_socket, (struct sockaddr*)&user_socket, &sock_size);

        if ((numbytes = recv(acc_socket, buf, 512, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }
        handle_req(buf, acc_socket);
    }
}