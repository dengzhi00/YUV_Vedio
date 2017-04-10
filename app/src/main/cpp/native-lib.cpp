#include <jni.h>
#include <string>

extern "C"{

#include "ffmpeg_h264.h"
#include "nv21_h264.h"
FILE *file_y;
FILE *file_u;
FILE *file_v;
FILE *file_yuv;
FILE *file_yuv1;
FILE *file_yuv2;
ffmpeg_h264* ff;
nv21_h264* nv21;
int nv21_index;
void Java_com_dzm_yuv_YuvVedio_add(JNIEnv *env,jobject /*this*/,jbyteArray arrays){
    char* data = (char *) env->GetByteArrayElements(arrays, 0);
    if(!file_y){
        file_y = fopen("/sdcard/out_y.y","w+");
//        file_u = fopen("/sdcard/out_u.y","w+");
//        file_v = fopen("/sdcard/out_v.y","w+");
//        file_yuv = fopen("/sdcard/out_yuv.yuv","w+");
//        file_yuv1 = fopen("/sdcard/out_yuv1.yuv","w+");
        file_yuv2 = fopen("/sdcard/out_yuv2.yuv","w+");
    }
//    LOGD("数据开始编码1");
    int width = 640;
    int height = 480;
    int uv_height = height >> 1;
    int media_size = width*height;
    char *yuv = new char[media_size*3/2];
    int k_y = 0;
    int k_u = 0;
    int k_v = 0;
    int k = 0;
    char *y = new char[media_size];
    char *u = new char[media_size/4];
    char *v = new char[media_size/4];
    //y
    /**
     * y13  y9   y5  y1
     * y14  y10  y6  y2
     * y15  y11  y7  y3
     * y16  y12  y8  y4
     */
//    LOGD("数据开始编码2");
    for (int i = 0; i < width; i++) {
        int n_pos = width - 1 - i;
        for (int j = 0; j < height; j++) {
            y[k_y++] = data[n_pos];
            yuv[k++] = data[n_pos];
            n_pos+=width;
        }
    }
//    LOGD("数据开始编码3");
    //u v

    for (int i = 0; i < width; i += 2) {
        int nPos = media_size + width - 1;
        for (int j = 0; j < uv_height; j++) {
            u[k_u++] = data[nPos - i - 1];
            v[k_v++] = data[nPos - i];
            yuv[k] = data[nPos - i - 1];
            yuv[k+1] = data[nPos - i];
            k+=2;
            nPos += width;
        }
    }

    fwrite(yuv, 1, (size_t) media_size, file_y);
//    fwrite(data + media_size, 1, (size_t) (media_size / 4), file_u);
//    fwrite(data + media_size*5/4, 1, (size_t) (media_size / 4), file_v);

//    fwrite(y,1,(size_t) media_size,file_yuv);
//    fwrite(u,1,(size_t) (media_size / 4),file_yuv);
//    fwrite(v,1,(size_t) (media_size / 4),file_yuv);

//    fwrite(yuv, 1, (size_t) (media_size * 3 / 2), file_yuv1);

    int border = 10;
    for(int i = 0;i<height;i++){
        for(int j = 0;j<width;j++){
            if(i<border || i>(width - border) || j< border || j>(height - border)){
                data[i*width + j] = (char) 255;
            }
        }
    }

    fwrite(data,1,(size_t) (media_size * 3 / 2),file_yuv2);

    env->ReleaseByteArrayElements(arrays, (jbyte *) data, 0);
}

void Java_com_dzm_yuv_YuvVedio_close(JNIEnv *env,jobject /*this*/){
    fclose(file_y);
//    fclose(file_u);
//    fclose(file_v);
//    fclose(file_yuv);
//    fclose(file_yuv1);
    fclose(file_yuv2);
}

int Java_com_dzm_yuv_YuvVedio_addFile(JNIEnv *env,jobject /*this*/){
    int size = 0;
    if(!file_y){
        file_y = fopen("/sdcard/test.txt","w+");
    }
    char *chars = (char *) "this is";
    for (int i = 0; i <3 ; ++i) {
        fwrite(chars, sizeof(chars),1,file_y);
    }
    fclose(file_y);
    return sizeof(chars);
}

void Java_com_dzm_yuv_YuvVedio_test(JNIEnv *env,jobject /*this*/){

}

void Java_com_dzm_yuv_YuvVedio_encode(JNIEnv *env,jobject /*this*/){
    ff = new ffmpeg_h264();
    ff->start();
}

void Java_com_dzm_yuv_YuvVedio_startEncode(JNIEnv *env,jobject /*this*/,jbyteArray array){
    char * data = (char *) env->GetByteArrayElements(array, 0);
    ff->start_encode(data);
    env->ReleaseByteArrayElements(array, (jbyte *) data, 0);
}

void Java_com_dzm_yuv_YuvVedio_end(JNIEnv *env,jobject /*this*/){
    if(ff){
        ff->end();
    }
    if(nv21){
        nv21->end();
        free(nv21);
    }
}

void Java_com_dzm_yuv_YuvVedio_nv21Init(JNIEnv *env,jobject /*this*/,jint width,jint height){
    if(!nv21){
        nv21 = new nv21_h264();
        nv21_index = nv21->init(width,height);
    }
}

void Java_com_dzm_yuv_YuvVedio_nv21Data(JNIEnv *env,jobject /*this*/,jbyteArray array){
    if(nv21_index == 1){
        char* data = (char *) env->GetByteArrayElements(array, 0);
        nv21->add_nv21(data);
        env->ReleaseByteArrayElements(array, (jbyte *) data, 0);
    }
}


}





