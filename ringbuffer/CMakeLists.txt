# 查找当前目录下的所有源文件
aux_source_directory(. DIR_RINGBUFFER_SRCS)

include_directories(../error)

# 生成链接库
add_library (ringbuffer ${DIR_RINGBUFFER_SRCS})