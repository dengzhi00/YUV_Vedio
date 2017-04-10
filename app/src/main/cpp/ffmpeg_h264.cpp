

extern "C"{
#include "ffmpeg_h264.h"

void ffmpeg_h264::start() {
    int in_w=480,in_h=272;
    const char* out_file = "/sdcard/ds.h264";

    av_register_all();

    pFormatCtx = avformat_alloc_context();



    fmt = av_guess_format(NULL,out_file,NULL);
    pFormatCtx->oformat = fmt;

    if(avio_open(&pFormatCtx->pb,out_file,AVIO_FLAG_READ_WRITE)<0){
        LOGD("写初始化失败");
        return;
    }
    video_st = avformat_new_stream(pFormatCtx,0);
    video_st->time_base.num = 1;
    video_st->time_base.den = 25;

    if(video_st == NULL){
        LOGD("video_st初始化失败");
        return;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    pCodecCtx->max_b_frames = 3;

    AVDictionary *param = 0;
    if(pCodecCtx->codec_id == AV_CODEC_ID_H264){
        av_dict_set(&param,"preset","slow",0);
        av_dict_set(&param,"tune","zerolatency",0);
    }

    if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
        av_dict_set(&param,"preset","ultrafast",0);
        av_dict_set(&param,"tune","zero-latency",0);
    }

    av_dump_format(pFormatCtx,0,out_file,1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if(!pCodec){
        LOGD("decoder失败");
        return;
    }

    if(avcodec_open2(pCodecCtx,pCodec,&param)<0){
        LOGD("open2 失败");
        return;
    }

    pFrame = av_frame_alloc();

    picture_size = avpicture_get_size(pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height);
    picture_buf = (uint8_t *) av_malloc((size_t) picture_size);
    avpicture_fill((AVPicture *) pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    avformat_write_header(pFormatCtx,NULL);
    av_new_packet(&pkt,picture_size);

    y_size = pCodecCtx->width * pCodecCtx->height;

    FILE* in_file = fopen("/sdcard/ds_480x272.yuv","r+");
    int i = 0;
    while(1){
        if(fread(picture_buf, 1, (size_t) (y_size * 3 / 2), in_file) <= 0){
            LOGD("文件读取完毕");
            break;
        }else if(feof(in_file)){
            break;
        }
        //y
        memcpy(pFrame->data[0], picture_buf, (size_t) y_size);
//        pFrame->data[0] = picture_buf;
        //u
        memcpy(pFrame->data[1],picture_buf + y_size, (size_t) (y_size / 4));
//        pFrame->data[1] = picture_buf + y_size;
        //v
        memcpy(pFrame->data[2],picture_buf+y_size*5/4, (size_t) (y_size / 4));
//        pFrame->data[2] = picture_buf + y_size*5/4;
        pFrame->pts = i;
        i++;

        int got_picture = 0;

        int ret = avcodec_encode_video2(pCodecCtx,&pkt,pFrame,&got_picture);
        if(ret<0){
            LOGD("解码失败");
            continue;
        }else{
            LOGD("解码成功");
        }
        if(got_picture == 1){
            LOGD("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
            framecnt++;
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx,&pkt);
            av_free_packet(&pkt);
        }
    }
    int ret = flush_encoder(pFormatCtx,0);
    if(ret<0){
        LOGD("flush_encoder 失败");
        return;
    }

    av_write_trailer(pFormatCtx);

    if(video_st){
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(in_file);
}

int ffmpeg_h264::flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
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
        ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                     NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame){
            ret=0;
            break;
        }
        LOGD("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

void ffmpeg_h264::start_encode(char *data) {
    uint8_t* ui = (uint8_t *) data;
    //y
    memcpy(pFrame->data[0], ui, (size_t) y_size);
//        pFrame->data[0] = picture_buf;
    //u
    memcpy(pFrame->data[1],ui + y_size, (size_t) (y_size / 4));
//        pFrame->data[1] = picture_buf + y_size;
    //v
    memcpy(pFrame->data[2],ui+y_size*5/4, (size_t) (y_size / 4));
//        pFrame->data[2] = picture_buf + y_size*5/4;
    pFrame->pts = i;
    i++;

    int got_picture = 0;

    int ret = avcodec_encode_video2(pCodecCtx,&pkt,pFrame,&got_picture);
    if(ret<0){
        LOGD("解码失败  %5d",ret);
    }else{
        LOGD("解码成功");
    }
    if(got_picture == 1){
        LOGD("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
        framecnt++;
        pkt.stream_index = video_st->index;
        ret = av_write_frame(pFormatCtx,&pkt);
        av_free_packet(&pkt);
    }
}

void ffmpeg_h264::end() {
    int ret = flush_encoder(pFormatCtx,0);
    if(ret<0){
        LOGD("flush_encoder 失败");
        return;
    }

    av_write_trailer(pFormatCtx);

    if(video_st){
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}

}
