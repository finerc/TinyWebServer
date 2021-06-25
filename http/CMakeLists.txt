# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_WRAPPER_SRCS 变量
aux_source_directory(. DIR_HTTPCONN_SRCS)

include_directories(../error)
include_directories(../ringbuffer)

# 生成链接库
add_library (httpConn ${DIR_HTTPCONN_SRCS})
target_link_libraries(httpConn ringbuffer)