/*
 * @Author: qibin
 * @Date: 2021-06-25 10:12:39
 * @LastEditTime: 2021-06-25 17:31:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /pingqibin/TinyWebServer_thread/ringbuffer/ringbuffer.h
 */

#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stddef.h>
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include "m_error.h"

class ringbuffer {
private:
    char buffer[8192];
    size_t capacity;
    size_t write_index;
    size_t read_index;
    
public:
    ringbuffer(int __cap=8192);
    ~ringbuffer() = default;
    
public:
    void readForward(size_t n);
    void writeForward(size_t n);
    int read_socket(int __fd, size_t __nbytes);
    int write_socket(int __fd, size_t __nbytes);
    int read_buf(char* buf, size_t __nbytes);
    int write_buf(char* buf, size_t __nbytes);
    char* writeAddr();
    char* readAddr();
    bool empty();
    bool full();
    size_t size();
    size_t freeSize();
    size_t Capacity();

    char& at(size_t index);
};


#endif