package com.dzm.yuv;

/**
 * Created by 83642 on 2017/3/30.
 */

public class YuvVedio {

    static {
//        System.loadLibrary("avcodec-57");
//        System.loadLibrary("avfilter-6");
//        System.loadLibrary("avformat-57");
//        System.loadLibrary("avutil-55");
//        System.loadLibrary("swresample-2");
//        System.loadLibrary("swscale-4");
        System.loadLibrary("native-lib");
    }

    public native int addFile();

    public native void close();

    public native void add(byte[] bytes);

    public native void encode();

    public native void startEncode(byte[] bytes);

    public native void end();

    public native void nv21Init(int width,int height);

    public native void nv21Data(byte[] bytes);
}
