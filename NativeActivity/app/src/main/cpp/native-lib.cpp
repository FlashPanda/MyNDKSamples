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
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * 我们保存的状态信息
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * app的共享状态信息
*/
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

/**
 * 为显示器初始化EGL上下文
*/
static int engine_init_display(struct engine* engine) {
    // 初始化OpenGL ES和EGL

    /*
     * 配置属性
     * 这个例子中，我们选择了每个颜色分量8位的配置
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* 应用选择他的配置
     * 先适配最合适的，没有就用第一个配置
     */
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    assert(supportedConfigs);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
    assert(numConfigs);
    auto i = 0;
    for (; i < numConfigs; ++i) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r) &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b) &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 && d == 0) {
            config = supportedConfigs[i];
            break;
        }
    }
    if (i == numConfigs)
        config = supportedConfigs[0];

    /* EGL_NATIVE_VISUAL_ID是EGLConfig配置的一个属性，保证会被
     * ANativeWindow_setBuffersGeometry()接受。我们只要选择了一个EGLConfig
     * 就能将ANativeWindow重新设置以达到适配目的，使用的正是EGL_NATIVE_VISUAL_ID
     */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // 检查系统中的OpenGL
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }

    // 初始化GL状态
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

    return 0;
}

/**
 * 当前正在显示的帧
 */
static void engine_draw_frame(struct engine* engine) {
    if(engine->display == NULL)
        return; // 无显示

    // 将屏幕填充为单一颜色
    glClearColor(((float)engine->state.x) / engine->width, engine->state.angle,
                 ((float)engine->state.y) / engine->height, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);
}

/*
 * 卸下当前与显示器关联的EGL上下文
 */
static void engine_term_display(struct engine* engine) {
    if(engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if(engine->context != EGL_NO_CONTEXT)
            eglDestroyContext(engine->display, engine->context);
        if(engine->surface != EGL_NO_SURFACE)
            eglDestroySurface(engine->display, engine->surface);
        eglTerminate(engine->display);
    }

    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/*
 * 处理下一个输入
 */
static int32_t engine_handle_input (struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

/*
 * 处理下一个主要命令
 */
static void engine_handle_cmd (struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // 系统要我们保存当前的状态，照做
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // 窗口要显示了，做好准备
            if(engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // 窗口要切换到后台或者关闭了，做下清理
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // 当app得到焦点时，我们就开始监控加速计
            if(engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
                // 每秒要60个事件
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                                engine->accelerometerSensor,
                                               (1000L / 60) * 1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // 丢失焦点时，停止监控加速计
            // 这是为了防止电池的消耗
            if(engine->accelerometerSensor != NULL)
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);

            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

#include <dlfcn.h>
ASensorManager* AcquireASensorManagerInstance(android_app* app) {
    if(!app)
        return nullptr;

    typedef ASensorManager* (*PF_GETINSTANCEFORPACKAGE)(const char* name);
    void* androidHandle = dlopen("libandroid.so", RTLD_NOW);
    PF_GETINSTANCEFORPACKAGE getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)
            dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
    if(getInstanceForPackageFunc) {
        JNIEnv* env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, NULL);

        jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
        jmethodID midGetPackageName = env->GetMethodID(android_content_Context,
                                                       "getPackageName",
                                                       "()Ljava/lang/String;");
        jstring packageName = (jstring)env->CallObjectMethod(app->activity->clazz,
                                                             midGetPackageName);

        const char* nativePackageName = env->GetStringUTFChars(packageName, 0);
        ASensorManager* mgr = getInstanceForPackageFunc(nativePackageName);
        env->ReleaseStringUTFChars(packageName, nativePackageName);
        app->activity->vm->DetachCurrentThread();
        if(mgr) {
            dlclose(androidHandle);
            return mgr;
        }
    }

    typedef ASensorManager* (*PF_GETINSTANCE)();
    PF_GETINSTANCE getInstanceFunc = (PF_GETINSTANCE) dlsym(androidHandle, "ASensorManager_getInstance");
    // ASensorManager_getInstance必须是有效的
    assert(getInstanceFunc);
    dlclose(androidHandle);

    return getInstanceFunc();
}

/*
 * 这是原生app的入口函数，在android_native_app_glue中使用。
 * 它在自己的线程中运行，有它自己的事件循环来处理输入事件或是其他事情
 */
void android_main(struct android_app* state) {
    struct engine engine;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // 准备监控加速计
    engine.sensorManager = AcquireASensorManagerInstance(state);
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
            engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(
            engine.sensorManager,
            state->looper, LOOPER_ID_USER,
            NULL, NULL);

    if(state->savedState != NULL)
        // 从之前保存的状态开始
        engine.state = *(struct saved_state*)state->savedState;

    // 循环等待，直到有消息
    while (1) {
        // 读取所有挂起事件
        int ident;
        int events;
        struct android_poll_source* source;

        // 如果不在播放动画，我们就阻塞到有事件发生
        // 如果在动画，我们就循环直到所有事件都被读取再继续绘制动画的下一帧
        while ((ident = ALooper_pollAll(engine.animating? 0 : -1, NULL, &events,
                                        (void**)&source)) >= 0) {
            // 处理事件
            if(source != NULL)
                source->process(state, source);

            // 如果传感器有数据，处理它
            if(ident == LOOPER_ID_USER) {
                if(engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                        &event, 1) > 0) {
                        LOGI("accelerometer: x = %f, y = %f, z = %f",
                             event.acceleration.x, event.acceleration.y,
                             event.acceleration.z);
                    }
                }
            }

            // 检查我们是否在退出
            if(state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if(engine.animating) {
            // 处理完事件。绘制下一帧动画
            engine.state.angle += 0.01f;
            if(engine.state.angle > 1)
                engine.state.angle = 0;

            // 绘制受到屏幕刷新率限制，所以这里不用计时
            engine_draw_frame(&engine);
        }
    }
}