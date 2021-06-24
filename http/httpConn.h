/*
 * @Author: qibin
 * @Date: 2021-06-23 16:01:52
 * @LastEditTime: 2021-06-24 14:29:47
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /pingqibin/TinyWebServer/http/httpConn.h
 */
#ifndef __HTTPCONN_H
#define __HTTPCONN_H

#include <sys/socket.h>
#include "sys/epoll.h"
#include "cstring"
#include <errno.h>
#include "m_error.h"
#include "iostream"
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>

class httpConn {
public:
    static const int INBUFSIZE = 8192;
    static const int OUTBUFSIZE = 4096;
    const char *resource_path = "../htdocs";
    const char *ok_200_title = "OK";
    const char *error_400_title = "Bad Request";
    const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
    const char *error_403_title = "Forbidden";
    const char *error_403_form = "You do not have permission to get file form this server.\n";
    const char *error_404_title = "Not Found";
    const char *error_404_form = "The requested file was not found on this server.\n";
    const char *error_500_title = "Internal Error";
    const char *error_500_form = "There was an unusual problem serving the request file.\n";

    enum HTTP_CODE {
        MORE_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNL_ERROR,
        CLOSED_CONNECTION
    };

    enum HTTP_VERSION {
        HTTP1_0,
        HTTP1_1,
        HTTP2_0,
    };

    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        DELETE,
        PUT
    };

    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

private:
    int sockfd;
    int epollfd;
    char inbuffer[INBUFSIZE];
    char outbuffer[OUTBUFSIZE];
    int read_idx;
    int write_idx;
    int write_remain;
    int check_idx;
    int line_start;
    CHECK_STATE check_state;
    METHOD method;
    int content_length;
    char* url;
    HTTP_VERSION version;
    char* host;
    struct stat resource_stat;
    
    bool linger;

    FILE* resource;
    

public:
    httpConn() = default;
    ~httpConn() = default;

public:
    httpConn(int __fd, int __epollfd):sockfd(__fd), epollfd(__epollfd) {}
    void init();
    void modfd(int ev);
    int m_read();
    int m_write();
    bool add_response(const char* format, ...);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_content_type();
    bool add_linger();
    bool add_resource();
    bool add_blank_line();
    bool add_content(const char *content);
    LINE_STATUS parseLine();
    HTTP_CODE parseRequestLine(char *text);
    HTTP_CODE parseHeader(char *text);
    HTTP_CODE parseContent(char *text);
    HTTP_CODE processRead();
    HTTP_CODE processRequest();
    bool processWrite(httpConn::HTTP_CODE code);
    bool process();
};

#endif // __HTTPCONN_H