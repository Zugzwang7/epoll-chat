#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <cstring>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define EPOLL_SIZE 5000
#define BUF_SIZE 0xFFFF
#define SERVER_WELCOME "======Welcome to the chat room! Your chatID is:Client #%d======"
#define SERVER_MESSAGE "ClientID #%d, name:%s say >> %s"
#define EXIT "exit"
#define CAUTION "It's just you in the chat room."

void addfd(int epollfd, int fd, bool enable_et);

using namespace std;

//储存客户信息：clientfd + clientname
map<int,string> clt_map;

void addfd(int epollfd, int fd, bool enable_et) {
    //设置非阻塞
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et) {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    printf("fd:%d add to epoll instance.\n", fd);
}

#endif //CHAT_COMMON_H