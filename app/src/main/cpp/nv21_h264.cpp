extern "C" {
#include "nv21_h264.h"

/**
 * 初始化参数
 */
int nv21_h264::init(int width, int height) {
    m_width = width;
    m_height = height;
    m_size = width * height;
    //存储 文件
    const char *out_file = "/sdcard/ds.h264";
    //ffmpeg 注册复用器，编码器
    av_register_all();

    pFormatCtx = avformat_alloc_context();

    fmt = av_guess_format(NULL, out_file, NULL);
    //编码流
    pFormatCtx->oformat = fmt;
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        LOGD("打开编码器失败");
        return -1;
    }
    video_st = avformat_new_stream(pFormatCtx, 0);
    //time_base 时基。通过该值可以把PTS，DTS转化为真正的时间
    video_st->time_base.num = 1;
    video_st->time_base.den = 25;

    if (video_st == NULL) {
        LOGD("video_st初始化失败");
        return -1;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //由于 nv21 数据是顺时针旋转了90度，所以要最终数据要逆时针旋转90度，长度变宽度，宽度变长度，
    //所以 将最终编码的宽度设置为源数据的高度，高度设置为源数据的宽度
    pCodecCtx->width = m_height;
    pCodecCtx->height = m_width;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    pCodecCtx->max_b_frames = 3;

    AVDictionary *param = 0;
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
    }

    if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }

    av_dump_format(pFormatCtx, 0, out_file, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

    if (!pCodec) {
        LOGD("decoder失败");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        LOGD("open2 失败");
        return -1;
    }

    pFrame = av_frame_alloc();

    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *) av_malloc((size_t) picture_size);
    avpicture_fill((AVPicture *) pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width,
                   pCodecCtx->height);

    avformat_write_header(pFormatCtx, NULL);
    av_new_packet(&pkt, picture_size);

    //y格式文件
    file_y = fopen("/sdcard/test_y.y", "w+");
    return 1;
}

void nv21_h264::add_nv21(char *data) {


//    memcpy(pFrame->data[0], data, (size_t) m_size);
//    char* u = (char *) pFrame->data[1];
//    char* v = (char *) pFrame->data[2];
//    int in = m_size>>2;
//    int j = 0;
//    for(int i = 0;i<in;i++){
//        *(u + j) = data[m_size+i+1];
//        *(v + j) = data[m_size+i];
//        j++;
//    }
//
//    char *y = (char *) pFrame->data[0];
//    for(int i = 0;i<m_height;i++){
//        for(int k = 0;k<m_width;k++){
//            y[i*k+k] = data[m_height*(k + 1) -(1 + i)];
//        }
//    }

    char *yuv_420 = new char[m_size * 3 / 2];
    //nv21 转 420p
    NV21ToI420(yuv_420, data, m_width, m_height);
    char *yuv_420_spin = new char[m_size * 3 / 2];
    //420p 逆时针旋转90度算法
    n420_spin(yuv_420_spin, yuv_420, m_width, m_height);
    //y
    memcpy(pFrame->data[0], yuv_420_spin, (size_t) m_size);
    //u
    memcpy(pFrame->data[1], yuv_420_spin + m_size, (size_t) (m_size >> 2));
    //v
    memcpy(pFrame->data[2], yuv_420_spin + (m_size >> 2), (size_t) (m_size >> 2));

    pFrame->pts = index;
    index++;

    int got_picture = 0;
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
    if (ret < 0) {
        LOGD("解码失败");
        return;
    }
    if (got_picture == 1) {
        LOGD("Succeed to encode frame: %5d\tsize:%5d\n", index, pkt.size);
        pkt.stream_index = video_st->index;
        ret = av_write_frame(pFormatCtx, &pkt);
        av_free_packet(&pkt);
    }
    free(yuv_420);
    free(yuv_420_spin);

}

void nv21_h264::end() {
    int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        LOGD("flush_encoder 失败");
        return;
    }

    av_write_trailer(pFormatCtx);

    if (video_st) {
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}

int nv21_h264::flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        LOGD("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;

}

void nv21_h264::NV21ToI420(char *dstyuv, char *data, int imageWidth, int imageHeight) {
    int Ustart = imageWidth * imageHeight;
    //y
    memcpy(dstyuv, data, (size_t) Ustart);
    //u v 长度
    int in = Ustart >> 2;
    for (int i = 0; i < in; i++) {
        //u
        dstyuv[Ustart + i] = data[Ustart + i + 1];
        //v
        dstyuv[Ustart + in + i] = data[Ustart + i];
    }
}

void nv21_h264::n420_spin(char *dstyuv, char *srcdata, int imageWidth, int imageHeight) {
    int i = 0, j = 0;
    int index = 0;
    int tempindex = 0;
    int div = 0;
    for (i = 0; i < imageWidth; i++) {
        div = i + 1;
        tempindex = 0;
        for (j = 0; j < imageHeight; j++) {
            tempindex += imageWidth;
            dstyuv[index++] = srcdata[tempindex - div];
        }
    }
    //写y 格式数据
    fwrite(dstyuv, 1, (size_t) m_size, file_y);

    //u起始位置
    int start = imageWidth * imageHeight;
    //u v 数据的长度
    int udiv = start >> 2;
    //u v 数据宽度
    int uWidth = imageWidth >> 1;
    //u v 数据高度
    int uHeight = imageHeight >> 1;
    //数据 下标位置
    index = start;
    for (i = 0; i < uWidth; i++) {
        div = i + 1;
        tempindex = start;
        for (j = 0; j < uHeight; j++) {
            tempindex += uHeight;
            dstyuv[index] = srcdata[tempindex - div];
            dstyuv[index + udiv] = srcdata[tempindex - div + udiv];
            index++;
        }
    }
}

}

