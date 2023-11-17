#include "socketutils.h"
#include <pthread.h>

void threadWaitServer(int socketFD);
void *listenForMessage(void *socketFD);

int main()
{
    int socketFD = createIPv4Socket();

    // struct sockaddr_in *address = createIPv4Address("39.156.66.10", 80);
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);
    int connectStatus = connect(socketFD, (struct sockaddr *)address, sizeof(*address));

    if (connectStatus == 0)
    {
        printf("connecting to server!\n");
    }

    // 发送用户名到服务器
    char *name = NULL;
    size_t nameLen = 0;
    printf("Please enter your name for the chat: ");
    ssize_t nameCount = getline(&name, &nameLen, stdin);
    name[nameCount - 1] = 0;
    send(socketFD, name, strlen(name), 0);

    char *line = NULL;
    size_t lineLen = 0;
    printf("Please enter your message(or type \"exit\" to exit): \n");
    // threadWaitServer(socketFD);

    // 创建线程，监听服务器端的消息
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, listenForMessage, (void *)&socketFD);

    char buffer[1024];

    while (1)
    {
        ssize_t charCount = getline(&line, &lineLen, stdin);

        // 格式化字符串为：用户名:消息
        sprintf(buffer, "%s:%s", name, line);
        if (charCount > 0)
        {
            if (strcmp(line, "exit\n") == 0)
            {
                // line[charCount - 1] = 0;
                // char exitMessage[1024];
                // sprintf(exitMessage, "%s has left the chat", name);
                // send(socketFD, exitMessage, strlen(exitMessage), 0);
                send(socketFD, line, strlen(line), 0);
                break;
            }
            ssize_t amountWasSent = send(socketFD, line, strlen(line), 0);
        }
    }
    close(socketFD);

    pthread_exit(NULL);
}

void threadWaitServer(int socketFD)
{
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, listenForMessage, (void *)&socketFD);
}
// 线程处理函数，监听服务器端的消息
void *listenForMessage(void *socketFD)
{
    char buf[1024];
    int socket_FD = *(int *)socketFD;
    while (1)
    {
        ssize_t amountReceived = recv(socket_FD, buf, 1024, 0);
        if (amountReceived > 0)
        {
            buf[amountReceived] = 0;
            printf("%s\n", buf);
        }
        else
        {
            break;
        }
    }

    pthread_exit(NULL);
}