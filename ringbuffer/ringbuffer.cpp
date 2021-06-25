/*
 * @Author: qibin
 * @Date: 2021-06-25 10:12:46
 * @LastEditTime: 2021-06-25 17:50:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /pingqibin/TinyWebServer_thread/ringbuffer/ringbuffer.cpp
 */

#include "ringbuffer.h"

ringbuffer::ringbuffer(int __cap) {
    // try
    // {
    //     buffer  = new char[__cap];
    // }
    // catch(const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }

    capacity = __cap;
    read_index = 0;
    write_index = 0;
}

// ringbuffer::~ringbuffer() {
//     std::cout << "deconstructor ringbuffer" << std::endl;
//     if(buffer) {    
//         try
//         {
//             delete[] buffer;
//         }
//         catch(const std::exception& e)
//         {
//             std::cerr << e.what() << '\n';
//         }
//     }
// }

void ringbuffer::readForward(size_t n) {
    read_index = (read_index + n) % capacity;
}

void ringbuffer::writeForward(size_t n) {
    write_index = (write_index + n) % capacity;
}
char* ringbuffer::writeAddr() {
    return buffer + write_index;
}

char* ringbuffer::readAddr() {
    return buffer + read_index;
}

bool ringbuffer::empty() {
    if(read_index == write_index) return true;
    else return false;
}

bool ringbuffer::full() {
    if((write_index + 1) % capacity == read_index) return true;
    else return false;
}

size_t ringbuffer::size() {
    if(write_index >= read_index) return write_index - read_index;
    else return (write_index + capacity - read_index);
}

size_t ringbuffer::freeSize() {
    return capacity - 1 - size();
}

size_t ringbuffer::Capacity() {
    return capacity;
}

/* read __nbytes from socket __fd to ringbuffer */
int ringbuffer::read_socket(int __fd, size_t __nbytes) {
    if(__nbytes > freeSize()) {
        return -1;
    }

    int n;
    int read_count = 0;

    if(read_index >= write_index || capacity - write_index > __nbytes) {
        // continuous write
        while(__nbytes > 0) {
            n = recv(__fd, writeAddr(), __nbytes, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes = __nbytes - n;
            writeForward(n);
            read_count += n;
        }
    }
    else {
        size_t __nbytes1 = capacity - write_index;
        size_t __nbytes2 =  __nbytes - __nbytes1;
        while(__nbytes1 > 0) {
            n = recv(__fd, writeAddr(), __nbytes1, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes1 = __nbytes1 - n;
            writeForward(n);
            read_count += n;
        }
        
        while(__nbytes2 > 0) {
            n = recv(__fd, buffer, __nbytes2, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes1 = __nbytes2 - n;
            writeForward(n);
            read_count += n;
        }
    }

    return read_count;
}

/* write __nbytes to socket __fd from buffer */
int ringbuffer::write_socket(int __fd, size_t __nbytes) {
    
    if(__nbytes > size()) {
        return -1;
    }

    int n;
    int write_count = 0;

    if(write_index >= read_index || capacity - read_index > __nbytes) {
        // continuous write
        while(__nbytes > 0) {
            n = send(__fd, readAddr(), __nbytes, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes = __nbytes - n;
            readForward(n);
            write_count += n;
        }
    }
    else {
        size_t __nbytes1 = capacity - read_index;
        size_t __nbytes2 =  __nbytes - __nbytes1;
        while(__nbytes1 > 0) {
            n = send(__fd, readAddr(), __nbytes1, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes1 = __nbytes1 - n;
            readForward(n);
            write_count += n;
        }
        
        while(__nbytes2 > 0) {
            n = send(__fd, buffer, __nbytes2, MSG_DONTWAIT);
            if(n==0)
                return 0;
            else if(n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) 
                    break;
                sys_err("send error");
                return -1;
            }
            __nbytes1 = __nbytes2 - n;
            readForward(n);
            write_count += n;
        }
    }

    return write_count;
}

/* read __nbytes from buf to ringbuffer */
int ringbuffer::read_buf(char* buf, size_t __nbytes) {
    if(__nbytes > freeSize()) {
        return -1;
    }

    int n;

    if(read_index >= write_index || capacity - write_index > __nbytes) {
        // continuous write
        memcpy(writeAddr(), buf, __nbytes);
    }
    else {
        size_t __nbytes1 = capacity - write_index;
        size_t __nbytes2 =  __nbytes - __nbytes1;
        memcpy(writeAddr(), buf, __nbytes1);
        memcpy(buffer, buf, __nbytes2);
    }

    writeForward(__nbytes);

    return __nbytes;
}

/* write __nbytes to buf from ringbuffer */
int ringbuffer::write_buf(char* buf, size_t __nbytes) {
    return 0;
}

char& ringbuffer::at(size_t index) {
    if(index >= size())
        sys_err("ringbuffer index overflow");

    if(write_index > read_index || capacity - read_index > index) {
        return *(readAddr() + index);
    }
    else {
        index = index - (capacity - read_index);
        return *(buffer + index);
    }
}

