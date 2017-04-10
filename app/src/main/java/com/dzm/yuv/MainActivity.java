package com.dzm.yuv;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import static android.R.attr.id;

public class MainActivity extends AppCompatActivity implements Runnable{

    // Used to load the 'native-lib' library on application startup.


    private CameraSurfacevView camera_surface;

    private YuvVedio yuvVedio;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        yuvVedio = new YuvVedio();
//        yuvVedio.encode();
        yuvVedio.nv21Init(640,480);
        camera_surface = (CameraSurfacevView) findViewById(R.id.camera_surface);
        camera_surface.setYuvVedio(yuvVedio);
//        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View v) {
//                yuvVedio.encode();
////                new Thread(MainActivity.this).start();
//
//            }
//        });
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        yuvVedio.close();
        yuvVedio.end();
    }

    @Override
    public void run() {
        File file = new File("/sdcard/ds_480x272.yuv");
        FileInputStream inputStream = null;
        try {
            inputStream = new FileInputStream(file);
            byte[] bytes = new byte[480*272*3/2];
            int bet;
            while ((bet = inputStream.read(bytes))>0){
                yuvVedio.startEncode(bytes);
                Log.d("bytes",bytesToHexString(bytes));
            }
            yuvVedio.end();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }finally {
            try {
                if(null != inputStream){
                    inputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public String bytesToHexString(byte[] src){
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
            stringBuilder.append(" ");
        }
        return stringBuilder.toString();
    }
}
