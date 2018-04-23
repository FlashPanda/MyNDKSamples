//
// Created by CZD on 2018/4/23.
//

#include <time.h>
#include "gperf.h"

#if defined(__GNUC__) && __GNUC__ >= 4
#define GPERF_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define GPERF_EXPORT __attribute__((visibility("default")))
#else
#define GPERF_EXPORT
#endif
	
// 返回当前时钟
GPERF_EXPORT uint64_t GetTicks(void) {
	struct timeval Time;
	uint64_t cur_tick = (uint64_t) 1000000;
	
	gettimeofday(&Time, NULL);
	cur_tick *= Time.tv_sec;
	
	return (cur_tick + Time.tv_usec);
}