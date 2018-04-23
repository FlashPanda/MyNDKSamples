cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE on)

add_library(gperf
			SHARED
			src/gperf.c)
			
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../distribution)
set_target_properties(gperf
					  PROPERTIES
					  LIBRARY_OUTPUT_DIRECTORY
					  "${distribution_DIR}/gperf/lib/${ANDROID_ABI}")
					  
add_custom_command(TARGET 
				   gperf
				   POST_BUILD
				   COMMAND "${CMAKE_COMMAND}" -E
				   copy "${CMAKE_CURRENT_SOURCE_DIR}/src/gperf.h"
				   "${distribution_DIR}/gperf/include/gperf.h"
				   COMMENT "Copying gperf to output directory")
				   