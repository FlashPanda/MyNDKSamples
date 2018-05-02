#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window_jni.h>
#include <cpu-features.h>

#include "TeapotRenderer.h"
#include "NDKHelper.h"

// 预处理
#define HELPER_CLASS_NAME "com/sample/helper/NDKHelper"

// 需要共享的状态
struct android_app;
class Engine {
    TeapotRenderer renderer_;

    ndk_helper::GLContext* gl_context_;

    bool initialized_resources_;
    bool has_focus_;

    ndk_helper::DoubletapDetector doubletap_detector_;
    ndk_helper::PinchDetector pinch_detector_;
    ndk_helper::DragDetector drag_detector_;
    ndk_helper::PerfMonitor monitor_;

    ndk_helper::TapCamera tap_camera_;
    android_app* app_;

    ASensorManager* sensor_manager_;
    const ASensor* accelerometer_sensor_;
    ASensorEventQueue* sensor_event_queue_;

    void UpdateFPS(float fFPS);
    void ShowUI();
    void TransformPosition(ndk_helper::Vec2& vec);

public:
    static void HandleCmd(struct android_app* app, int32_t cmd);
    static int32_t HandleInput(android_app* app, AInputEvent* event);

    Engine();
    ~Engine();
    void SetState(android_app* app);
    int InitDisplay(android_app* app);
    void LoadResources();
    void UnloadResources();
    void DrawFrame();
    void TermDisplay();
    void TrimMemory();
    bool IsReady();

    void UpdatePosition(AInputEvent* event, int32_t iIndex, float& fX, float& fY);

    void InitSensors();
    void ProcessSensors(int32_t id);
    void SuspendSensors();
    void ResumeSensors();
};

/****************************************************
 * Ctor
 ****************************************************/
Engine::Engine()
    : initialized_resources_(false),
      has_focus_(false),
      app_(NULL),
      sensor_manager_(NULL),
      accelerometer_sensor_(NULL),
      sensor_event_queue_(NULL) {
    gl_context_ = ndk_helper::GLContext::GetInstance();
}

/****************************************************
 * Dtor
 ****************************************************/
Engine::~Engine() {}

/****************************************************
 * 加载资源
 ****************************************************/
void Engine::LoadResources() {
    renderer_.Init();
    renderer_.Bind(&tap_camera_);
}

/****************************************************
 * 卸载资源
 ****************************************************/
void Engine::UnloadResources() { renderer_.Unload(); }

/****************************************************
 * 初始化一个EGL上下文
 ****************************************************/
int Engine::InitDisplay(android_app* app) {
    if (!initialized_resources_) {
        gl_context_ -> Init(app_ -> window);
        LoadResources();
        initialized_resources_ = true;
    }
    else if (app -> window != gl_context_ -> GetANativeWindow()) {
        assert(gl_context_ -> GetANativeWindow());
        UnloadResources();
        gl_context_ -> Invalidate();
        app_ = app;
        gl_context_ -> Init(app -> window);
        LoadResources();
        initialized_resources_ = true;
    }
    else {
        // 初始化OpenGL ES和EGL
        if (gl_context_ -> Resume (app_ -> window) == EGL_SUCCESS) {
            UnloadResources();
            LoadResources();
        }
        else
            assert(0);
    }

    ShowUI();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glViewport(0, 0, gl_context_ -> GetScreenWidth(), gl_context_ -> GetScreenHeight());
    renderer_.UpdateViewport();

    tap_camera_.SetFlip(1.0f, -1.0f, -1.0f);
    tap_camera_.SetPinchTransformFactor(2.0f, 2.0f, 8.0f);

    return 0;
}

/****************************************************
 * 当前帧
 ****************************************************/
void Engine::DrawFrame() {
    float fps;
    if (monitor_.Update(fps))
        UpdateFPS(fps);
    renderer_.Update(monitor_.GetCurrentTime());

    // 用一个颜色填满屏幕
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer_.Render();

    if (gl_context_ -> Swap() != EGL_SUCCESS) {
        UnloadResources();
        LoadResources();
    }
}

/****************************************************
 * 终止EGL上下文
 ****************************************************/
void Engine::TermDisplay() { gl_context_->Suspend(); }

void Engine::TrimMemory() {
    LOGI("Trimming memory");
    gl_context_ -> Invalidate();
}

int32_t Engine::HandleInput(android_app* app, AInputEvent* event) {
    Engine* eng = (Engine*)app -> userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        ndk_helper::GESTURE_STATE doubleTapState = eng -> doubletap_detector_.Detect(event);
        ndk_helper::GESTURE_STATE dragState = eng -> drag_detector_.Detect(event);
        ndk_helper::GESTURE_STATE pinchState = eng -> pinch_detector_.Detect(event);

        if (doubleTapState == ndk_helper::GESTURE_STATE_ACTION)
            eng -> tap_camera_.Reset(true);
        else {
            if (dragState & ndk_helper::GESTURE_STATE_START) {
                ndk_helper::Vec2 v;
                eng -> drag_detector_.GetPointer(v);
                eng -> TransformPosition(v);
                eng -> tap_camera_.BeginDrag(v);
            }
            else if (dragState & ndk_helper::GESTURE_STATE_MOVE) {
                ndk_helper::Vec2 v;
                eng -> drag_detector_.GetPointer(v);
                eng -> TransformPosition(v);
                eng -> tap_camera_.Drag(v);
            }
            else if (dragState & ndk_helper::GESTURE_STATE_END) {
                eng -> tap_camera_.EndDrag();
            }

            if (pinchState & ndk_helper::GESTURE_STATE_START) {
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng -> pinch_detector_.GetPointers(v1, v2);
                eng -> TransformPosition(v1);
                eng -> TransformPosition(v2);
                eng -> tap_camera_.BeginPinch(v1, v2);
            }
            else if (pinchState & ndk_helper::GESTURE_STATE_MOVE) {
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng -> pinch_detector_.GetPointers(v1, v2);
                eng -> TransformPosition(v1);
                eng -> TransformPosition(v2);
                eng -> tap_camera_.Pinch(v1, v2);
            }
        }

        return 1;
    }
    return 0;
}

void Engine::HandleCmd(struct android_app* app, int32_t cmd) {
    Engine* eng = (Engine*)app -> userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            break;
        case APP_CMD_INIT_WINDOW:
            if (app -> window != NULL) {
                eng -> InitDisplay(app);
                eng -> DrawFrame();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            eng -> TermDisplay();
            eng -> has_focus_ = false;
            break;
        case APP_CMD_STOP:
            break;
        case APP_CMD_GAINED_FOCUS:
            eng -> ResumeSensors();
            eng -> has_focus_ = true;
            break;
        case APP_CMD_LOST_FOCUS:
            eng -> SuspendSensors();
            eng -> has_focus_ = false;
            eng -> DrawFrame();
            break;
        case APP_CMD_LOW_MEMORY:
            eng -> TrimMemory();
            break;
    }
}

void Engine::InitSensors() {
    sensor_manager_ = ndk_helper::AcquireASensorManagerInstance(app_);
    accelerometer_sensor_ = ASensorManager_getDefaultSensor(sensor_manager_, ASENSOR_TYPE_ACCELEROMETER);
    sensor_event_queue_ = ASensorManager_createEventQueue(sensor_manager_, app_ -> looper, LOOPER_ID_USER, NULL, NULL);
}

void Engine::ProcessSensors(int32_t id) {
    if (id == LOOPER_ID_USER) {
        ASensorEvent event;
        while (ASensorEventQueue_getEvents(sensor_event_queue_, &event, 1) > 0) {

        }
    }
}

void Engine::ResumeSensors() {
    if (accelerometer_sensor_ != NULL) {
        ASensorEventQueue_enableSensor(sensor_event_queue_, accelerometer_sensor_);
        ASensorEventQueue_setEventRate(sensor_event_queue_, accelerometer_sensor_, (1000L / 60) * 1000);
    }
}

void Engine::SuspendSensors() {
    if (accelerometer_sensor_ != NULL)
        ASensorEventQueue_disableSensor(sensor_event_queue_, accelerometer_sensor_);
}

void Engine::SetState(android_app* state) {
    app_ = state;
    doubletap_detector_.SetConfiguration(app_ -> config);
    drag_detector_.SetConfiguration(app_ -> config);
    pinch_detector_.SetConfiguration(app_ -> config);
}

bool Engine::IsReady() {
    if (has_focus_) return true;
    return false;
}

void Engine::TransformPosition (ndk_helper::Vec2& vec) {
    vec = ndk_helper::Vec2(2.0f, 2.0f) * vec /
                                         ndk_helper::Vec2(gl_context_ -> GetScreenWidth(),
                                                          gl_context_ -> GetScreenHeight()) -
          ndk_helper::Vec2(1.0f, 1.0f);
}

void Engine::ShowUI() {
    JNIEnv* jni;
    app_ -> activity -> vm -> AttachCurrentThread(&jni, NULL);

    jclass clazz = jni -> GetObjectClass(app_ -> activity -> clazz);
    jmethodID methodID = jni -> GetMethodID(clazz, "showUI", "()V");
    jni -> CallVoidMethod(app_ -> activity -> clazz, methodID);

    app_ -> activity -> vm -> DetachCurrentThread();
    return;
}

void Engine::UpdateFPS(float fFPS) {
    JNIEnv* jni;
    app_ -> activity -> vm -> AttachCurrentThread(&jni, NULL);
    jclass clazz = jni->GetObjectClass(app_ -> activity -> clazz);
    jmethodID methodID = jni -> GetMethodID(clazz, "updateFPS", "(F)V");
    jni -> CallVoidMethod(app_ -> activity -> clazz, methodID, fFPS);

    app_ -> activity -> vm -> DetachCurrentThread();
    return;
}

Engine g_engine;

void android_main (android_app* state) {
    g_engine.SetState(state);

    ndk_helper::JNIHelper::Init(state -> activity, HELPER_CLASS_NAME);
    state -> userData = &g_engine;
    state -> onAppCmd = Engine::HandleCmd;
    state -> onInputEvent = Engine::HandleInput;

#ifdef USE_NDK_PROFILER
    monstartup("libTeapotNativeActivity.so")
#endif

    g_engine.InitSensors();

    while (1) {
        int id;
        int events;
        android_poll_source* source;

        while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) source -> process(state, source);

            g_engine.ProcessSensors(id);

            if (state -> destroyRequested != 0) {
                g_engine.TermDisplay();
                return;
            }
        }

        if (g_engine.IsReady()) {
            g_engine.DrawFrame();
        }
    }
}