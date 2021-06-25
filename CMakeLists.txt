cmake_minimum_required(VERSION 3.0)
project(tinywebserver)

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)

# 添加lib子目录
# 由于echo依赖wrapper， 因此一定要把wrapper写到前面
add_subdirectory(error)
add_subdirectory(ringbuffer)
add_subdirectory(http)
add_subdirectory(src)
# add_subdirectory(clientConn)
# add_subdirectory(echo)
# add_subdirectory(sum)