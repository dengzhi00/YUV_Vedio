
extern "C"{
#include "jni.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "android/log.h"

#define LOGD(...) __android_log_print(3,"NDK",__VA_ARGS__)

class nv21_h264 {
private:
    int m_width;
    int m_height;
    int m_size;
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    //存储每一个视频/音频流信息的结构体
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVFrame* pFrame;
    int picture_size;
    uint8_t* picture_buf;
    AVPacket pkt;
    int index;
    FILE* file_y;
public:
    nv21_h264():m_width(0),m_height(0),m_size(0),pFormatCtx(NULL),fmt(NULL),video_st(NULL),pCodecCtx(NULL),index(0){};

public:
    int init(int width,int height);
    void add_nv21(char* data);
    void end();
private:
    int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);
    //nv21 转 420p格式
    void NV21ToI420(char* dstyuv,char* data, int imageWidth, int imageHeight);
    //逆时针旋转420p  90度
    void n420_spin(char* dstyuv,char* srcdata, int imageWidth, int imageHeight);
};
}





