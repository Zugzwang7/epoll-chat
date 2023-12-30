/**
 * epoll聊天室 client端
 */
#include "chatroom.h"

#define error(msg) \
    do {perror(msg); exit(EXIT_FAILURE); } while (0)

using namespace std;

void* receive(void* arg) {
    int *temp = ((int*)arg);
    int sock = *temp;
    while (true) {
        char recvBuf[BUF_SIZE] = {};
        int reLen = recv(sock, recvBuf, BUF_SIZE, 0);
        cout<<endl<<recvBuf<<endl;
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    //创建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0) {
        error("socket error");
    }

    //设置服务器地址结构
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    //连接服务端
    if (connect(clientfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error("connect error");
    }

    string name;
    cout<<"Please enter your name:";
    getline(cin, name);
    write(clientfd, (char*)name.c_str(), name.length());

    void* temp = &clientfd;
    pthread_t th;
    pthread_create(&th, NULL, receive, temp);
    while (true) {
        string s;
        getline(cin, s);
        write(clientfd, (char*)s.c_str(), s.length());
    }

    close(clientfd);
    return 0;
}

