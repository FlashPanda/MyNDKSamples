package com.example.czd.hello_gl2;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by czd on 2018/5/16.
 */

public class GL2JNIView extends GLSurfaceView {
    private static String TAG = "GL2JNIView";
    private static final boolean DEBUG = false;

    public GL2JNIView (Context context) {
        super (context);
        init (false, 0, 0);
    }

    public GL2JNIView (Context context, boolean translucent, int depth, int stencil) {
        super (context);
        init (translucent, depth, stencil);
    }

    private void init (boolean translucent, int depth, int stencil) {
        /*  默认情况下，GLSurfaceView()创建一个RGB_565格式的不透明表面
            如果我们需要一个半透明（或透明）表面，我们要在这里改变表面的格式
            使用PixelFormat.TRANSLUCENT枚举值设置表面，是它成为一个32位
            带有alpha通道服务端
         */
        if (translucent) {
            this.getHolder ().setFormat (PixelFormat.TRANSLUCENT);
        }

        /*
            需要一个context工厂来实现2.0的渲染
            ContextFactory类在下面定义
            这个函数必须在setRenderer()函数之前调用
            如果不用这个函数，默认的context是不共享的，而且没有属性列表
            函数声明：void setEGLContextFactory (GLSurfaceView.EGLContextFactory factory)
         */
        setEGLContextFactory (new ContextFactory());

        /*
            我们需要选择一个EGLConfig来匹配我们生成的表面格式。这一步操作
            会在我们的自定义配置选择器中完成。查看下面定义的ConfigChooser类。
         */
        setEGLConfigChooser (translucent?
                             new ConfigChooser (8, 8, 8, 8, depth, stencil) :
                             new ConfigChooser (5, 6, 5, 0, depth, stencil));

        /* 设置渲染器 */
        setRenderer (new Renderer());
    }

    // 上下文工厂
    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext (EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            Log.w (TAG, "createOpenGL ES 2.0 context");
            checkEglError ("Before eglCreateContext", egl);
            /**
             * 属性列表指定的格式就是这样，必须以EGL_CONTEXT_CLIENT_VERSION开头，后面紧跟一个整数值表示OpenGL ES的版本号
             */
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE};
            /**
             * 这是一个重度函数，它所做的事情非常重要
             * 函数的参数：
             * EGLDisplay display,  关联的显示器
             * EGLConfig config,    指定egl帧缓存的配置，这是上下文可以获取到的资源
             * EGLContext share_context,   要共享缓存给哪个EGL上下文。如果设置EGL_NO_CONTEXT表示要共享给谁
             * EGLint const * attrib_list   指定创建EGL所需的属性。只有EGL_CONTEXT_CLIENT_VERSION可以指定
             * eglCreateContext为当前渲染API创建了一个EGL渲染上下文。当前渲染API就是eglBindAPI绑定的东西。
             * 函数的返回值是一个上下文的句柄。这个上下文就可以用来渲染EGL绘制表面。如果创建失败，函数会
             * 返回一个EGL_NO_CONTEXT。
             */
            EGLContext context = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
            checkEglError("After eglContextContext", egl);
            return context;
        }

        public void destroyContext (EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext (display, context);
        }
    }

    private static void checkEglError (String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError ()) != EGL10.EGL_SUCCESS) {
            Log.e (TAG, String.format ("%s,: EGL error: 0x%x", prompt, error));
        }
    }

    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {
        public ConfigChooser (int r, int g, int b, int a, int depth, int stencil){
            mRedSize = r;
            mGreenSize = g;
            mBlueSize = b;
            mAlphaSize = a;
            mDepthSize = depth;
            mStencilSize = stencil;
        }

        /*
            EGL 配置指定，被用于2.0渲染
            我们使用最小的尺寸（4位）给R/G/B，但是会在chooseConfig()中提供真实的匹配
         */
        private static int EGL_OPENGL_ES2_BIT = 4;
        private static int[] s_configAttribs2 = {
                EGL10.EGL_RED_SIZE, 4,
                EGL10.EGL_GREEN_SIZE , 4,
                EGL10.EGL_BLUE_SIZE, 4,
                EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL10.EGL_NONE
        };

        public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display) {
            // 获取EGL最低配置
            int[] num_config = new int[1];
            /**
             * 这也是一个要重度用的函数。
             * 返回匹配指定属性的EGL帧缓存配置列表
             * 函数参数：
             * EGLDisplay display,      关联的显示器
             * EGLint const * attrib_list,      指定的属性
             * EGLConfig * configs,     输出的帧缓存配置列表
             * EGLint config_size,     帧缓存配置列表的大小
             * EGLint * num_config      符合条件的配置个数
             * 函数返回时，将所有的EGL帧缓存配置保存到configs中，config_size指定了配置列表最多可以接受多少个配置信息
             * num_config返回了符合条件的配置数量
             * 如果configs设置成NULL，config_size会被忽略
             * 一般使用的时候都是先将configs设置成NULL,调用一次函数，获得配置个数，再根据配置个数new一个相应大小的配置
             * 列表数组出来，再调用一次获取信息。
             *
             * 我们下面就是这么做的
             */
            // 获取满足指定属性的所有配置
            egl.eglChooseConfig (display, s_configAttribs2, null, 0, num_config);

            int numConfigs = num_config[0];
            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No configs match configSpec");
            }

            // 分配，读取EGL最低配置
            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, s_configAttribs2, configs, numConfigs, num_config);

            if (DEBUG){
                printConfigs (egl, display, configs);
            }

            return chooseConfig (egl, display, configs);
        }

        public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
            for (EGLConfig config : configs) {
                int d = findConfigAttrib (egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib (egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);

                // 检查深度合模板属性是否达到最低要求
                if (d < mDepthSize || s < mStencilSize) {
                    continue;
                }

                // rgba属性的检查
                int r = findConfigAttrib (egl, display, config, EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib (egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib (egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib (egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);

                if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize)
                    return config;
            }
            return null;
        }

        private int findConfigAttrib (EGL10 egl, EGLDisplay display,
                                      EGLConfig config, int attribute, int defaultValue) {
            /**
             * 返回EGL帧缓存配置的信息
             * 函数的参数：
             * EGLDisplay display,      相关联的显示器
             * EGLConfig config,        要查询的配置
             * EGLint attribute,        需要返回的属性
             * 	EGLint * value          返回值
             */
            if (egl.eglGetConfigAttrib (display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        private void printConfigs (EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
            int numConfigs = configs.length;
            Log.w (TAG, String.format ("%d configurations", numConfigs));
            for (int i = 0; i < numConfigs; ++i) {
                Log.w (TAG, String.format ("Configuration %d:\n", i));
                printConfig (egl, display, configs[i]);
            }
        }

        private void printConfig(EGL10 egl, EGLDisplay display,
                                 EGLConfig config) {
            int[] attributes = {
                    EGL10.EGL_BUFFER_SIZE,
                    EGL10.EGL_ALPHA_SIZE,
                    EGL10.EGL_BLUE_SIZE,
                    EGL10.EGL_GREEN_SIZE,
                    EGL10.EGL_RED_SIZE,
                    EGL10.EGL_DEPTH_SIZE,
                    EGL10.EGL_STENCIL_SIZE,
                    EGL10.EGL_CONFIG_CAVEAT,
                    EGL10.EGL_CONFIG_ID,
                    EGL10.EGL_LEVEL,
                    EGL10.EGL_MAX_PBUFFER_HEIGHT,
                    EGL10.EGL_MAX_PBUFFER_PIXELS,
                    EGL10.EGL_MAX_PBUFFER_WIDTH,
                    EGL10.EGL_NATIVE_RENDERABLE,
                    EGL10.EGL_NATIVE_VISUAL_ID,
                    EGL10.EGL_NATIVE_VISUAL_TYPE,
                    0x3030, // EGL10.EGL_PRESERVED_RESOURCES,
                    EGL10.EGL_SAMPLES,
                    EGL10.EGL_SAMPLE_BUFFERS,
                    EGL10.EGL_SURFACE_TYPE,
                    EGL10.EGL_TRANSPARENT_TYPE,
                    EGL10.EGL_TRANSPARENT_RED_VALUE,
                    EGL10.EGL_TRANSPARENT_GREEN_VALUE,
                    EGL10.EGL_TRANSPARENT_BLUE_VALUE,
                    0x3039, // EGL10.EGL_BIND_TO_TEXTURE_RGB,
                    0x303A, // EGL10.EGL_BIND_TO_TEXTURE_RGBA,
                    0x303B, // EGL10.EGL_MIN_SWAP_INTERVAL,
                    0x303C, // EGL10.EGL_MAX_SWAP_INTERVAL,
                    EGL10.EGL_LUMINANCE_SIZE,
                    EGL10.EGL_ALPHA_MASK_SIZE,
                    EGL10.EGL_COLOR_BUFFER_TYPE,
                    EGL10.EGL_RENDERABLE_TYPE,
                    0x3042 // EGL10.EGL_CONFORMANT
            };
            String[] names = {
                    "EGL_BUFFER_SIZE",
                    "EGL_ALPHA_SIZE",
                    "EGL_BLUE_SIZE",
                    "EGL_GREEN_SIZE",
                    "EGL_RED_SIZE",
                    "EGL_DEPTH_SIZE",
                    "EGL_STENCIL_SIZE",
                    "EGL_CONFIG_CAVEAT",
                    "EGL_CONFIG_ID",
                    "EGL_LEVEL",
                    "EGL_MAX_PBUFFER_HEIGHT",
                    "EGL_MAX_PBUFFER_PIXELS",
                    "EGL_MAX_PBUFFER_WIDTH",
                    "EGL_NATIVE_RENDERABLE",
                    "EGL_NATIVE_VISUAL_ID",
                    "EGL_NATIVE_VISUAL_TYPE",
                    "EGL_PRESERVED_RESOURCES",
                    "EGL_SAMPLES",
                    "EGL_SAMPLE_BUFFERS",
                    "EGL_SURFACE_TYPE",
                    "EGL_TRANSPARENT_TYPE",
                    "EGL_TRANSPARENT_RED_VALUE",
                    "EGL_TRANSPARENT_GREEN_VALUE",
                    "EGL_TRANSPARENT_BLUE_VALUE",
                    "EGL_BIND_TO_TEXTURE_RGB",
                    "EGL_BIND_TO_TEXTURE_RGBA",
                    "EGL_MIN_SWAP_INTERVAL",
                    "EGL_MAX_SWAP_INTERVAL",
                    "EGL_LUMINANCE_SIZE",
                    "EGL_ALPHA_MASK_SIZE",
                    "EGL_COLOR_BUFFER_TYPE",
                    "EGL_RENDERABLE_TYPE",
                    "EGL_CONFORMANT"
            };
            int[] value = new int[1];
            for (int i = 0; i < attributes.length; ++i) {
                int attribute = attributes[i];
                String name = names[i];
                if (egl.eglGetConfigAttrib (display, config, attribute, value)) {
                    Log.w (TAG, String.format ("  %s: %d\n", name, value[0]));
                }
                else {
                    while (egl.eglGetError () != EGL10.EGL_SUCCESS);
                }
            }
        }

        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
        private int [] mValue = new int [1];
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame (GL10 unused) {
            GL2JNILib.step();
        }

        public void onSurfaceChanged (GL10 unused, int width, int height) {
            GL2JNILib.init (width, height);
        }

        public void onSurfaceCreated (GL10 gl, EGLConfig config) {
            // 什么都不做
        }
    }
}
