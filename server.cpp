/**
 * epoll聊天室 server端
 */
#include "chatroom.h"

#define error(msg) \
    do {perror(msg); exit(EXIT_FAILURE); } while (0)

//处理私聊时所输入的字符串
string private_msg(string& name) {
    string msg = "";
    if (name[0] == '@'){
        for (int i = 1; i < name.length(); ++i) {
            if (name[i] == '@') {
                msg += name.substr(i + 1);
                name = name.substr(1, i - 1);
                break;
            }
        }
    }
    return msg;
}

//处理从客户端输入的信息:群聊or私聊or请求在线用户列表or退出聊天
int processClientMsg(int clientfd) {
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);

    printf("Received message from client #%d\n", clientfd);
    int len = recv(clientfd, buf, BUF_SIZE, 0);

    if (0 == len)
    {
        close(clientfd);
        clt_map.erase(clientfd);
        printf("Client #%d closed.\n Now there are %d clients in the chat room\n", clientfd, clt_map.size());
    }

    if (0 == strcmp(EXIT, buf))  //client输入'exit'
    {
        close(clientfd);
        clt_map.erase(clientfd);
        printf("Client #%d closed.\n Now there are %d clients in the chat room\n", clientfd, clt_map.size());
    }

    if (1 == clt_map.size()) {  //聊天室只剩一个client
        send(clientfd, CAUTION, strlen(CAUTION), 0);
        return len;
    }

    if (0 == strcmp("ls", buf)) {  //获取当前在线的client列表
        for (auto i : clt_map) {
            char t[BUF_SIZE] = {};
            sprintf(t, "ClientID#%d -> Client Name: %s", i.first, (char*)i.second.c_str());
            if (send(clientfd, t, BUF_SIZE, 0) < 0) {
                error("send error");
            }
        }
    }
    else  //发送消息
    {
        string name(buf);
        string msg = private_msg(name);
        if (name == clt_map[clientfd]) {  //输入的目标客户端名字就是该客户端的名字时
            string nouser = "Can't send message to yourself.";
            if (send(clientfd, (char*)nouser.c_str(), nouser.length(), 0) < 0) {
                error("send error");
            }
            return len;
        }

        int tarfd = -1;
        for (auto i : clt_map) {  //将客户端输入的私聊信息输入到目标客户端上
            if (i.second == name) {
                tarfd = i.first;
                sprintf(message, "Message from %s: %s", (char*)clt_map[clientfd].c_str(), (char*)msg.c_str());
                if (send(i.first, message, BUF_SIZE, 0) < 0) {
                    error("send error");
                }
                return len;
            }
        }

        if (tarfd == -1 && msg != "") {  //输入的目标客户端名字不存在时
            string nouser = "No client named " + name;
            if (send(clientfd, (char*)nouser.c_str(), nouser.length(), 0) < 0) {
                error("send error");
            }
            return len;
        }

        sprintf(message, SERVER_MESSAGE, clientfd, (char*)clt_map[clientfd].c_str(), buf);
        //当客户端直接进行群聊时进行广播
        list<int>::iterator it;
        for (auto it : clt_map) {
           if (it.first != clientfd) {
                if (send(it.first, message, BUF_SIZE, 0) < 0) {
                    error("send error");
                }
            }
        }
    }
    return len;
}

int main(int argc, char *argv[]) {
    //创建监听socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        error("socket error");
    }
    printf("server socket created.\n");

    //服务器IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    //地址复用
    const int reuseaddr = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    //绑定地址
    if (bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error("bind error");
    }

    //监听
    if (listen(listener, SOMAXCONN) < 0) {
        error("listen error");
    }
    printf("Start to listen: %s\n", SERVER_IP);

    //创建epoll instance
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        error("epfd error");
    }
    printf("epoll created, epollfd = %d\n", epfd);

    static struct epoll_event events[EPOLL_SIZE];

    //向epoll instance添加监听事件
    addfd(epfd, listener, true);

    printf("======Waiting for client's request======\n");
    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        //处理这epoll_events_count个就绪事件
        for (int i = 0; i < epoll_events_count; ++i) {
            int tmp_epoll_recv_fd = events[i].data.fd;
            //新用户连接
            if (tmp_epoll_recv_fd == listener) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listener, (struct sockaddr*)&client_address, &client_addrLength );
                char name[100] = {};
                recv(clientfd,name,sizeof(name), 0);  //接收客户端的名字信息
                printf("\n%s connected,clientID:%d\n", name, clientfd);

                //客户端连接文件描述符加入epoll interest list
                addfd(epfd, clientfd, true);

                clt_map[clientfd] = string(name);

                printf("Now there are %d clients int the chat room.\n", (int)clt_map.size());

                //服务端发送欢迎信息
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if (ret < 0) {
                    error("send error");
                }
            }
            //处理用户发来的消息
            else {
                int ret = processClientMsg(tmp_epoll_recv_fd);
                if (ret < 0) {
                    error("error");
                }
            }
        }
    }
    close(listener);
    close(epfd);
    return 0;
}