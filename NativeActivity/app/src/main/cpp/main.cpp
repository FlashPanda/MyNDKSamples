/**
    一个完全不同的文件结构
*/
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <errno.h>
#include <cassert>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android_native_cpp_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native_activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native_activity", __VA_ARGS__))

/**
 * 我们保存状态的数据结构
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
}
