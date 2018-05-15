package com.example.czd.opengles;

import android.content.Context;
import android.opengl.GLSurfaceView;

/**
 * Created by czd on 2018/5/15.
 */

public class MyGLSurfaceView extends GLSurfaceView {
    private final MyGLRenderer mRenderer;

    public MyGLSurfaceView (Context context) {
        super (context);

        setEGLContextClientVersion (2);

        mRenderer = new MyGLRenderer ();

        setRenderer (mRenderer);
    }
}
