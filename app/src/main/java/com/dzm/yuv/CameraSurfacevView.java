package com.dzm.yuv;

import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


import java.io.IOException;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * @author 邓治民
 *         date 2016/12/28 10:52
 *         获取摄像头 头像 承载surfaceview
 */

public class CameraSurfacevView extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback,Runnable {

    /** 相机 */
    private Camera mCamera;

    /** 是否打开camera */
    private boolean mPreviewRunning;

    /** 0代表前置摄像头，1代表后置摄像头 */
    private int cameraPosition = 1;

    /** 分辨率宽度 */
    private static final int CAMERAWIDTH = 640;
    /** 分辨率 */
    private static final int CAMERAHEIGHT = 480;

    private final static int SCREEN_PORTRAIT = 0;
    private final static int SCREEN_LANDSCAPE_LEFT = 90;
    private final static int SCREEN_LANDSCAPE_RIGHT = 270;
    private int screen;
    private byte[] raw;

    private int cameraId;
    private YuvVedio yuvVedio;

    private LinkedBlockingQueue<byte[]> queue;

    public CameraSurfacevView(Context context) {
        this(context, null);
    }

    public CameraSurfacevView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CameraSurfacevView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        queue = new LinkedBlockingQueue<>();
        new Thread(this).start();
        getHolder().addCallback(this);
        getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        raw = new byte[CAMERAWIDTH * CAMERAHEIGHT * 3 / 2];
    }


    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        mCamera = Camera.open(Camera.CameraInfo.CAMERA_FACING_FRONT);//代表摄像头的方位，CAMERA_FACING_FRONT前置
        setCameraDisplayOrientation((Activity) getContext(), Camera.CameraInfo.CAMERA_FACING_FRONT, mCamera);
        cameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
        cameraPosition = 0;

    }

    public void setYuvVedio(YuvVedio yuvVedio){
        this.yuvVedio = yuvVedio;
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        if (mPreviewRunning) {
            mCamera.stopPreview();
        }


        Camera.Parameters p = mCamera.getParameters();
//        List<Camera.Size> sizeList = p.getSupportedPreviewSizes();
//        for(Camera.Size ss:sizeList){
//            Log.d("size", ss.width+"::"+ss.height);
//        }
        p.setPreviewSize(CAMERAWIDTH, CAMERAHEIGHT);
//		p.setPreviewSize(720, 480);
//        p.setRotation(90);
//        mCamera.setDisplayOrientation();
        mCamera.setPreviewCallback(this);
        mCamera.setParameters(p);
        try {
            mCamera.setPreviewDisplay(getHolder());
        } catch (IOException e) {
            e.printStackTrace();
        }
        mCamera.startPreview();
        mPreviewRunning = true;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        if (null != mCamera) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if(null == data || null == yuvVedio){
            return;
        }
        try {
            queue.put(data);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }


    private void setCameraDisplayOrientation(Activity activity, int cameraId, android.hardware.Camera camera) {
        android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(cameraId, info);//得到每一个摄像头的信息
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                screen = SCREEN_PORTRAIT;
                degrees = 0;
                break;
            case Surface.ROTATION_90:// 横屏 左边是头部(home键在右边)
                screen = SCREEN_LANDSCAPE_LEFT;
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                screen = 180;
                degrees = 180;
                break;
            case Surface.ROTATION_270:// 横屏 头部在右边
                screen = SCREEN_LANDSCAPE_RIGHT;
                degrees = 270;
                break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;   // compensate the mirror
        } else {
            // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        camera.setDisplayOrientation(result);
    }


    @Override
    public void run() {
        while(true){
            try {
                byte[] bytes = queue.take();
                yuvVedio.nv21Data(bytes);
                Log.d("size   ", String.valueOf(queue.size()));
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
