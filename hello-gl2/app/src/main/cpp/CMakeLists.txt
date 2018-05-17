
cmake_minimum_required(VERSION 3.4.1)

add_library( gl2jni

             SHARED

             gl_code.cpp )

target_link_libraries( gl2jni
                       android
                       log
                       EGL
                       GLESv2)