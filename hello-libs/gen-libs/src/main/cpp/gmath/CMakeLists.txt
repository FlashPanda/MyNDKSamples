cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE on)

add_library(gmath STATIC src/gmath.c)

# 把库复制出去。需要留下一个静态库来通过gradle的检查
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution)
set_target_properties(gmath
					  PROPERTIES
					  ARCHIVE_OUTPUT_DIRECTIORY
					  "${distribution_DIR}/gmath/lib/${ANDROID_ABI}")
					  
# 复制库的头文件
add_custom_command(TARGET gmath POST_BUILD
				   COMMAND "${CMAKE_COMMAND}" -E
				   copy "${CMAKE_CURRENT_SOURCE_DIR}/src/gmath.h"
				   "${distribution_DIR}/gmath/include/gmath.h"
				   # 接下来两行是为了以后可能会有的调试目的
#				   COMMAND "${CMAKE_COMMAND}" -E
#				   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
				   COMMENT "Copying gmath to output directory")