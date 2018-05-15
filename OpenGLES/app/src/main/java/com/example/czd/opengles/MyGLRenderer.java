package com.example.czd.opengles;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by czd on 2018/5/15.
 */

public class MyGLRenderer implements GLSurfaceView.Renderer{
    private Triangle mTriangle;


    public void onSurfaceCreated (GL10 unused, EGLConfig config) {
        GLES20.glClearColor (0.f, 0.f, 0.f, 1.f);

        mTriangle = new Triangle();
    }

    public void onDrawFrame (GL10 unused) {
        GLES20.glClear (GLES20.GL_COLOR_BUFFER_BIT);
        mTriangle.draw ();
    }

    public void onSurfaceChanged (GL10 unused, int width, int height) {
        GLES20.glViewport (0, 0, width, height);
    }

    public static int loadShader (int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);

        // add the source code to the shader and compile it
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);

        return shader;
    }
}
