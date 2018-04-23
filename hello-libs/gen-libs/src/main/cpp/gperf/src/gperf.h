//
// Created by CZD on 2018/4/23.
//

#ifndef HELLO_LIBS_GPERF_H
#define HELLO_LIBS_GPERF_H

#include <sys/typed.h>

// 返回当前系统的时钟
#ifdef __cplusplus
extern "C"
#endif
uint64_t GetTicks(void);

#endif //HELLO_LIBS_GPERF_H
