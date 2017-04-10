
extern "C"{
#include "jni.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "android/log.h"

#define LOGD(...) __android_log_print(3,"NDK",__VA_ARGS__)

class ffmpeg_h264 {
private:
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVFrame* pFrame;
    int picture_size;
    uint8_t* picture_buf;
    AVPacket pkt;
    int y_size;
    int framecnt;
    int i;
public:
    ffmpeg_h264():pFormatCtx(NULL),fmt(NULL),video_st(NULL),pCodecCtx(NULL),pCodec(NULL),pFrame(NULL),framecnt(0),i(0){};

public:
    void start();
    void start_encode(char* data);
    void end();


private:
    int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index);
};

}




