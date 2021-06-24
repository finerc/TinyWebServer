/*
 * @Author: qibin
 * @Date: 2021-06-23 14:47:38
 * @LastEditTime: 2021-06-24 15:36:14
 * @LastEditors: Please set LastEditors
 * @Description: main loop
 * @FilePath: /pingqibin/TinyWebServer/src/main.cpp
 */

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_map>
#include "m_error.h"
#include "httpConn.h"

const int PORT = 8888;
const int MAXFD = 10000;
const int MAX_EVENT_NUMBER = 5000;

int main(int args, char **argv) {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        sys_quit("create socket error");
    }

    struct sockaddr_in servaddr;
    memset((void*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    int flag = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
        sys_quit("setsockopt error");
    }

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        sys_quit("bind error");
    }

    if(listen(sockfd, 5) < 0) {
        sys_quit("listen error");
    }

    int epollfd = epoll_create(MAXFD);
    if(epollfd < 0) {
        sys_quit("epoll create error");
    }

    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLHUP;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        sys_quit("epoll_ctl error");
    }

    struct epoll_event events[MAX_EVENT_NUMBER];
    std::unordered_map<int, httpConn> clients;


    std::cout <<"webserver" << std::endl;

    for( ; ; ) {
        int epoll_ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(epoll_ret < 0) {
            sys_quit("epoll wait error");
        } 

        for(int i=0; i<epoll_ret; i++) {
            if(events[i].data.fd == sockfd) {
                // new connection
                struct sockaddr_in connaddr;
                socklen_t connaddr_len;
                int connfd = accept(sockfd, (struct sockaddr*)&connaddr, &connaddr_len);
                if(connfd < 0) {
                    sys_err("accept error");
                    continue;
                }

                // std::cout << "accept connection from client: " << connfd << std::endl;

                clients.insert({connfd, httpConn(connfd, epollfd)});
                clients[connfd].init();

                int flags = fcntl(connfd, F_GETFL, 0);
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                struct epoll_event ev;
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLHUP | EPOLLET;
                if(epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                    sys_err("epoll_ctl add error");
                }
            }
            else {
                int connfd = events[i].data.fd;

                if(clients.find(connfd) == clients.end())
                    continue;
                if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    std::cout << "bad event" << std::endl;
                    std::cout << "server close connection" << std::endl;

                    clients.erase(connfd);
                    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, 0) < 0) {
                        sys_err("epoll_ctl delete error");
                    }
                    close(connfd);
                    continue;
                }
                else if(events[i].events & EPOLLIN) {
                    // std::cout << "read event" << std::endl;
                    int n = clients[connfd].m_read();
                    if(n==0) {
                        // std::cout << "client close connection" << std::endl;

                        clients.erase(connfd);
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, 0) < 0) {
                            sys_err("epoll_ctl delete error");
                        }
                        close(connfd);
                        // std::cout << "close connection with client: " << connfd << std::endl;
                        continue;
                    }
                    else if(n == -1) {
                        std::cout << "server close connection" << std::endl;
                        
                        clients.erase(connfd);
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, 0) < 0) {
                            sys_err("epoll_ctl delete error");
                        }
                        if(close(connfd) < 0)
                            sys_err("close error");

                        continue;
                    }
                    if(!clients[connfd].process()) {
                        std::cout << "server close connection" << std::endl;
                        
                        clients.erase(connfd);
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, 0) < 0) {
                            sys_err("epoll_ctl delete error");
                        }
                        close(connfd);
                        continue;
                    }
                }
                else if(events[i].events & EPOLLOUT) {
                    // std::cout << "write event" << std::endl;
                    int n = clients[connfd].m_write();
                    if(n < 0) {
                        std::cout << "server close connection" << std::endl;
                        
                        clients.erase(connfd);
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, 0) < 0) {
                            sys_err("epoll_ctl delete error");
                        }
                        close(connfd);
                        continue;
                    }
                }
            }
        }
    }

    return 0;
}