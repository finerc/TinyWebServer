/*
 * @Author: qibin
 * @Date: 2021-06-23 16:01:45
 * @LastEditTime: 2021-06-25 18:13:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /pingqibin/TinyWebServer/http/httpConn.cpp
 */

/*
1. 单连接情况下缓冲区满的问题——（环形缓冲区？）
*/

#include "httpConn.h"

// httpConn::httpConn(const httpConn& rhs) {
    
// }

// httpConn& httpConn::operator=(const httpConn& rhs) {

// }


// httpConn::~httpConn() {
//     std::cout << "deconstructor httpConn" << nn << std::endl;
//     // if(inbuffer) delete inbuffer;
//     // if(outbuffer) delete outbuffer;
// }

int httpConn::nn = 0;

void httpConn::init() {

    // inbuffer = new ringbuffer(INBUFSIZE);
    // outbuffer = new ringbuffer(OUTBUFSIZE);

    line_start = 0;
    check_idx = 0;
    check_state = CHECK_STATE_REQUESTLINE;
    method = GET;
    content_length = 0;
    version = HTTP1_1;
    linger = false;

    pack_id = 0;
    pack_index = 0;

    resource = NULL;
}

void httpConn::modfd(int ev) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = ev | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
}

int httpConn::m_read() {
    // block read
    int n = inbuffer.read_socket(sockfd, inbuffer.freeSize());
    // std::cout << inbuffer.size() << std::endl;
    return n;
}

int httpConn::m_write() {
    // block write
    int n = 0;
    if(!outbuffer.empty()) {
        n = outbuffer.write_socket(sockfd, outbuffer.size());
        if(n == -1 || n == 0)
            return n;
    }
    int m = write_resource();
    if(m == -1 || m == 0)
        return m;
    else 
        return n + m; 

}

bool httpConn::add_response(const char* format, ...) {
    if(outbuffer.full())
        return false;
    va_list arg_list;
    va_start(arg_list, format);

    char tempbuffer[100];

    int len = vsnprintf(tempbuffer, 100, format, arg_list);
    if (len >= 100 || len < 0)
    {
        va_end(arg_list);
        return false;
    }
    outbuffer.read_buf(tempbuffer, len);
    va_end(arg_list);

    return true;
}

bool httpConn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool httpConn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}

bool httpConn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}

bool httpConn::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool httpConn::add_linger()
{
    return add_response("Connection:%s\r\n", (linger == true) ? "keep-alive" : "close");
}

bool httpConn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool httpConn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool httpConn::add_resource()
{
    // if(!resource) return false;
    // int resfd = fileno(resource);
    // while(int n = read(resfd, outbuffer + write_remain, OUTBUFSIZE - 1 - write_remain))
    // {
    //     if(n<0) {
    //         sys_err("read resource error");
    //         return false;
    //     }
    //     else if(n == 0) {
    //         break;
    //     }
    //     else {
    //         write_remain += n;
    //     }
    // }
    // fclose(resource);

    return true;
}

int httpConn::write_resource()
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    // blocked read

    for(int i=pack_id;i<pack_num;i++) {
        int n = send(sockfd, pack+pack_index, 10-pack_index, 0);
        if(n == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) 
                break;
            sys_err("send pack error");
            return -1;
        }
        else if(n == 0){
            return 0;
        }
        else {
            pack_index = pack_index + n;
            if(pack_index<10) {
                i--;
                continue;
            }
            else {
                pack_index = 0;
            }
        }
    }

    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

httpConn::LINE_STATUS httpConn::parseLine() {
    char ch;
    for( ; check_idx < inbuffer.size(); check_idx++) {
        ch = inbuffer.at(check_idx);
        if(ch == '\r') {
            if(check_idx + 1 == inbuffer.size())
                return LINE_OPEN;
            else if(inbuffer.at(check_idx + 1) == '\n') {
                inbuffer.at(check_idx++) = '\0';
                inbuffer.at(check_idx++) = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(ch == '\n') {
            if(check_idx > 1 && inbuffer.at(check_idx - 1) == '\r') {
                inbuffer.at(check_idx-1) = '\0';
                inbuffer.at(check_idx++) = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

httpConn::HTTP_CODE httpConn::parseRequestLine(char* text) {
    url = strpbrk(text, " \t");
    if(!url)
        return BAD_REQUEST;
    *url++ = '\0';
    
    char *mtd = text;
    if(strcasecmp(mtd, "GET") == 0) {
        method = GET;
    }
    else if(strcasecmp(mtd, "POST") == 0) {
        method = POST;
    }
    else {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char *vsn = strpbrk(url, " \t");
    if(!vsn)
        return BAD_REQUEST;
    *vsn++ = '\0';
    
    if(strcasecmp(vsn, "HTTP/1.1") == 0) {
        version = HTTP1_1;
    }
    else if(strcasecmp(vsn, "HTTP/1.0") == 0) {
        version = HTTP1_0;
    }
    else if(strcasecmp(vsn, "HTTP/2.0") == 0) {
        version = HTTP2_0;
    }
    else {
        return BAD_REQUEST;
    }

    if(strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }
    
    if(strncasecmp(url, "https//", 8) == 0) {
        url += 8;
        url = strchr(url, '/');
    }

    if(!url || url[0] != '/')
        return BAD_REQUEST;
    
    if(strlen(url) == 1)
        strcat(url, "index.html");

    // std::cout << "url: " << url<< std::endl;
    // std::cout << "method: " << mtd << std::endl;
    // std::cout << "version: " << version << std::endl;

    check_state = CHECK_STATE_HEADER;
    return MORE_REQUEST;

}

httpConn::HTTP_CODE httpConn::parseHeader(char* text) {
    if(text[0] == '\0') {
        if(content_length != 0) {
            check_state = CHECK_STATE_CONTENT;
            return MORE_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0) {
            linger = true;
            std::cout << "Connection: " << "keep-alive" << std::endl;
        }
    }
    else if(strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, "\t");
        content_length = atoi(text);

        std::cout << "Content-length: " << content_length << std::endl;
    }
    else if(strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host = text;
    }
    else {
        return MORE_REQUEST;
        // std::cout << "Unknown request header: " << text << std::endl;
    }

    return MORE_REQUEST;
}

httpConn::HTTP_CODE httpConn::processRead() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = MORE_REQUEST;
    char* text = 0;


    while((check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status=parseLine()) == LINE_OK)) {
        text = inbuffer.readAddr() + line_start;
        line_start = check_idx;

        switch(check_state) {
            case CHECK_STATE_REQUESTLINE:
                ret = parseRequestLine(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            case CHECK_STATE_HEADER:
                ret = parseHeader(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else {
                    return processRequest();
                }
                break;
            case CHECK_STATE_CONTENT:
                return BAD_REQUEST;
                break;
            default:
                return INTERNL_ERROR;
        }
    }

    return MORE_REQUEST;
}

httpConn::HTTP_CODE httpConn::processRequest() {
    // char resource_root[100];
    // strcpy(resource_root, resource_path);
    // strcat(resource_root, url);
    // resource = fopen(resource_root, "r");
    // if(!resource) {
    //     return NO_RESOURCE;
    // }

    // if (stat(resource_root, &resource_stat) < 0)
    //     return NO_RESOURCE;

    return FILE_REQUEST;
}

bool httpConn::processWrite(httpConn::HTTP_CODE code) {
    switch(code) {
        case NO_RESOURCE:
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return false;
            if(m_write() < 0)
                return false;
            break;
        case BAD_REQUEST: {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if (!add_content(error_400_form))
                return false;
            if(m_write() < 0)
                return false;
            break;
        }
        case FILE_REQUEST:
            add_status_line(200, ok_200_title);
            add_headers(pack_num * pack_size);
            if(m_write() < 0)
                return false;
            // if(!write_resource())
            //     return false;
            break;
        default:
            return false;
    }

    return true;
}

bool httpConn::process()
{
    httpConn::HTTP_CODE read_ret = processRead();
    if (read_ret == MORE_REQUEST)
    {
        modfd(EPOLLIN);
        return true;
    }

    bool write_ret = processWrite(read_ret);
    modfd(EPOLLOUT);

    return write_ret;
}



