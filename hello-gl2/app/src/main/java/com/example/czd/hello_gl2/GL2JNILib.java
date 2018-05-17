package com.example.czd.hello_gl2;

/**
 * Created by czd on 2018/5/16.
 */

public class GL2JNILib {
    static {
        System.loadLibrary ("gl2jni");
    }

    /**
     * 初始化
     * @param width 当前视图的宽度
     * @param height 当前视图的高度
     */
    public static native void init (int width, int height);
    public static native void step();
}
