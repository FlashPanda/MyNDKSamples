package com.example.czd.hello_gl2;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class GL2JNIActivity extends Activity {
    GL2JNIView mView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate (savedInstanceState);
        mView = new GL2JNIView(getApplication());
        setContentView (mView);
    }

    @Override
    protected void onPause () {
        super.onPause ();
        mView.onPause ();
    }

    @Override
    protected void onResume () {
        super.onResume ();
        mView.onResume ();
    }
}
