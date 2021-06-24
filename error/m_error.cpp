/*
 * @Author: qibin
 * @Date: 2021-06-23 14:54:23
 * @LastEditTime: 2021-06-23 16:23:16
 * @LastEditors: Please set LastEditors
 * @Description: error handler
 * @FilePath: /pingqibin/TinyWebServer/error/error.cpp
 */

#include "m_error.h"

void sys_err(const char* dsp) {
    perror(dsp);
}

void sys_quit(const char* dsp) {
    perror(dsp);
    exit(0);
}