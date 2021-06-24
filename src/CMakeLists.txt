message(STATUS "Build in src folder")
include_directories("../error")
include_directories("../http")

# 指定生成目标
add_executable(webserver main.cpp)

# 添加链接库
target_link_libraries(webserver error)
target_link_libraries(webserver httpConn)