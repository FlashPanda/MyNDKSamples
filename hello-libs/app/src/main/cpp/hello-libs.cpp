#include <cstring>
#include <jni.h>
#include <cinttypes>
#include <android/log.h>
// 这两个是自己的头文件
#include <gmath.h>
#include <gperf.h>

#define LOGI(...) \
	((void)__android_log_print)(ANDROID_LOG_INFO, "hello-libs::", __VA_ARGS__))

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_hellolibs_MainActivity_stringFronJNI (JNIEnv* env, jobject thiz) {
	// 越简单越好
	auto ticks = GetTicks();

	for (auto exp = 0; exp < 32; ++exp) {
		volatile unsigned val = gpower (exp);
		(void) val;
	}
	ticks = GetTicks() - ticks;

	LOGI("calculation time: %" PRIu64, ticks);

	return env->NewStringUTF("Hello from JNI LIBS!");
}