package com.lichao.javawav;

import java.io.File;
import com.android.jni.Decodec;
import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;

/**
 * FFmpeg amr音频格式转换为wav格式
 * @author dell
 *
 */
public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }
    
    public void beginClick(View view){
    	String armpath = Environment.getExternalStorageDirectory().getAbsolutePath()+File.separatorChar+"lichao.amr";
    	Decodec.newdecode(armpath);
    	Log.d("lichao", "转换完成");
    }
}
