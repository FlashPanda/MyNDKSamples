cmake_minimum_required(VERSION 3.4.1)

# 显示繁琐的构建信息，包括执行的每一条命令
set(CMAKE_VERBOSE_MAKEFILE on)

# 将源码路径设置成当前路径
set(lib_src_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 读取HOME环境变量
set(lib_build_DIR $ENV{HOME}/tmp)
# 创建目录
file(MAKE_DIRECTORY ${lib_build_DIR})

# 添加子目录
# add_subdirectory(source_dir [binary_dir]
# 				   [EXCLUDE_FROM_ALL])
add_subdirectory(${lib_src_DIR}/gmath ${lib_build_DIR}/gmath)
add_subdirectory(${lib_src_DIR}/gperf ${lib_build_DIR}/gperf)