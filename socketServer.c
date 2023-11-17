#include "socketutils.h"
#include <stdbool.h>
#include <pthread.h>

struct AcceptedSocket
{
    int socketFD;
    struct sockaddr_in address;
    bool acceptedSuccessfully;
    int errorNumber;
    char username[20];
};
void *Sendmsg(void *serverSocketFD);
void mutiClientHandler(int serverSocketFD);
struct AcceptedSocket *acceptSocket(int serverSocketFD);
void *RecvFromClient(void *acceptedSocket);
void threadWaitClient(struct AcceptedSocket *clientSocket);
void BroadcastToClient(char *buf, int socketFD);
void sendToClient(char *message, int socketFD);

struct AcceptedSocket acceptedSockets[10];
int acceptedSocketCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int i = 0;

int main()
{
    int serverSocketFD = createIPv4Socket();

    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int bindStatus = bind(serverSocketFD, (struct sockaddr *)serverAddress, sizeof(*serverAddress));

    if (bindStatus == 0)
    {
        printf("bind success!\n");
    }

    int listenStatus = listen(serverSocketFD, 10);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, Sendmsg, (void *)&serverSocketFD);
    mutiClientHandler(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);
}
// 服务器端主动发送消息
void *Sendmsg(void *serverSocketFD)
{
    char buf[1024];
    int serverSocket = *(int *)serverSocketFD;
    while (1)
    {
        fgets(buf, 1024, stdin);
        if (buf[0] != '\n' && buf[0] != '\0')
        {
            pthread_mutex_lock(&mutex);
            buf[strlen(buf) - 1] = 0;
            char buffer[1044];
            sprintf(buffer, "[Server] %s", buf);
            BroadcastToClient(buffer, serverSocket);
            pthread_mutex_unlock(&mutex);
        }
    }
}
// 服务器端多客户端处理
void mutiClientHandler(int serverSocketFD)
{
    while (1)
    {
        struct AcceptedSocket *clientSocket = acceptSocket(serverSocketFD);
        pthread_mutex_lock(&mutex);
        acceptedSockets[acceptedSocketCount++] = *clientSocket;
        pthread_mutex_unlock(&mutex);
        threadWaitClient(clientSocket);
    }
}
// 对于每个可用的客户端，创建一个线程用于接收消息
void threadWaitClient(struct AcceptedSocket *clientSocket)
{
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, RecvFromClient, (void *)clientSocket);
}
// 接收客户端消息
void *RecvFromClient(void *acceptedSocket)
{
    struct AcceptedSocket *clientSocket = (struct AcceptedSocket *)acceptedSocket;
    char buf[1024];
    // 接收客户端用户名，存入acceptedSockets[i].username
    ssize_t usernameLength = recv(clientSocket->socketFD, clientSocket->username, sizeof(clientSocket->username) - 1, 0);
    char *tempUsername = acceptedSockets[i++].username;
    strcpy(tempUsername, clientSocket->username);
    if (usernameLength > 0)
    {
        // 服务器端打印客户端连接信息
        clientSocket->username[usernameLength] = '\0';
        printf("Client %s:%d connected with username: %s\n",
               inet_ntoa(clientSocket->address.sin_addr), ntohs(clientSocket->address.sin_port), clientSocket->username);
        fflush(stdout);
        char broadcastMessage[1024];
        // 广播加入消息：
        sprintf(broadcastMessage, "[Server] %s has joined the chat", clientSocket->username);
        BroadcastToClient(broadcastMessage, clientSocket->socketFD);
    }
    while (1)
    {
        ssize_t amountReceived = recv(clientSocket->socketFD, buf, 1024, 0);
        if (amountReceived > 0)
        {
            buf[amountReceived - 1] = 0;
            printf("Received from client %s:%d (%s): %s\n",
                   inet_ntoa(clientSocket->address.sin_addr), ntohs(clientSocket->address.sin_port), clientSocket->username, buf);

            // @username开头 = 私聊：
            if (buf[0] == '@')
            {
                char recipientUsername[20];
                char message[1024];
                // 分离出用户名和消息
                sscanf(buf, "@%s %[^\n]", recipientUsername, message);
                pthread_mutex_lock(&mutex);
                // 遍历acceptedSockets，找到对应的socketFD，发送消息
                for (int i = 0; i < acceptedSocketCount; i++)
                {
                    // printf("acceptedSockets[%d].username: %s\n", i, acceptedSockets[i].username);
                    if (strcmp(acceptedSockets[i].username, recipientUsername) == 0)
                    {
                        char privateMsg[1044];
                        sprintf(privateMsg, "%s:%s", clientSocket->username, message);
                        sendToClient(privateMsg, acceptedSockets[i].socketFD);
                    }
                }
                pthread_mutex_unlock(&mutex);
            }
            else if (strcmp(buf, "exit") == 0)
            {
                // 广播退出消息：
                char exitMessage[1024];
                sprintf(exitMessage, "[Server] %s has left the chat", clientSocket->username);
                BroadcastToClient(exitMessage, clientSocket->socketFD);
                break;
            }
            else if (strcmp(buf, "$state") == 0)
            {
                // 广播当前在线人数：
                char stateMessage[1024];
                sprintf(stateMessage, "[Server] There are %d people online", acceptedSocketCount);
                sendToClient(stateMessage, clientSocket->socketFD);
            }
            else if (buf[0] != '\n' && buf[0] != '\0')
            {
                // 广播消息：
                pthread_mutex_lock(&mutex);
                char buffer[1044];
                sprintf(buffer, "%s:%s", clientSocket->username, buf);
                BroadcastToClient(buffer, clientSocket->socketFD);
                pthread_mutex_unlock(&mutex);
            }
        }

        if (amountReceived <= 0)
            break;
    }
    close(clientSocket->socketFD);
}
// 私聊给某个客户端
void sendToClient(char *message, int socketFD)
{
    send(socketFD, message, strlen(message), 0);
}
// 广播给所有客户端
void BroadcastToClient(char *buf, int socketFD)
{
    for (int i = 0; i < acceptedSocketCount; i++)
    {
        if (acceptedSockets[i].socketFD != socketFD)
        {
            send(acceptedSockets[i].socketFD, buf, strlen(buf), 0);
        }
    }
}
// 接收客户端连接
struct AcceptedSocket *acceptSocket(int serverSocketFD)
{
    struct sockaddr_in clientAddress;
    int clinetAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clinetAddressSize);

    struct AcceptedSocket *acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->socketFD = clientSocketFD;
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;
    if (!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->errorNumber = clientSocketFD;

    return acceptedSocket;
}