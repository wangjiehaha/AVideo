extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavutil/opt.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/common.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/samplefmt.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
}
#include <iostream>
#include <stdio.h>
#include <jni.h>
#include <wchar.h>

#define INBUF_SIZE 4096

FILE *pFin = NULL;
FILE *pFout = NULL;
AVCodec *pCodec = NULL; //编解码器实例指针
AVCodecContext *pCodecContext = NULL; //编解码参数信息
AVCodecParserContext *pCodecParserCtx = NULL; //用于解析码流，生成可以提供解码的包
AVFrame *frame = NULL; //用于保存解码生成的像素数据
AVPacket pkt; //用于保存码流解析出来的包

int open_input_output_file(char **argv)
{
    const char *inputFileName = argv[0];
    const char *outputFileName = argv[1];
    pFin = fopen(inputFileName, "rb+");
    if (pFin == NULL)
    {
        return -1;
    }
    pFout = fopen(outputFileName, "wb+");
    if (pFout == NULL)
    {
        return -1;
    }
    return 0;
}

int open_decoder()
{
    // 初始化全部组件
    avcodec_register_all();
    av_init_packet(&pkt);
    // 获取编码器
    pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec)
    {
        return -1;
    }
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext)
    {
        return -1;
    }
    if (pCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
    {
        pCodecContext->flags |= AV_CODEC_CAP_TRUNCATED;
    }
    // 初始化解析器
    pCodecParserCtx = av_parser_init(AV_CODEC_ID_H264);
    if (!pCodecParserCtx)
    {
        return -1;
    }
    // 打开编码器
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        return -1;
    }
    // 分配frame
    frame = av_frame_alloc();
    if (!frame)
    {
        return -1;
    }
    return 0;
}

void write_out_yuv_frame(AVFrame *frame)
{
    u_int8_t **pBuf = frame->data; //指向像素保存的地址，data是一个数组
    int *pStride = frame->linesize; //位宽
    for (int color_idx = 0; color_idx < 3; ++color_idx) {
        int nWidth = color_idx == 0 ? frame->width : frame->width / 2;
        int nHeight = color_idx == 0 ? frame->height : frame->height / 2;
        for (int idx = 0; idx < nHeight; ++idx)
        {
            fwrite(pBuf[color_idx], 1, nWidth, pFout);
            pBuf[color_idx] += pStride[color_idx];
        }
        fflush(pFout);
    }
}

void Colse()
{
    fclose(pFout);
    fclose(pFin);
    avcodec_close(pCodecContext);
    av_free(pCodecContext);
    av_frame_free(&frame);
}

int test(int argc, char **argv)
{
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    if (open_input_output_file(argv))
    {
        return -1;
    }
    if (open_decoder() < 0)
    {
        return -1;
    }
    int uDataSize = 0;//一次读入到我们定义的缓存的数据的长度
    int len = 0;
    uint8_t *pDataPtr = NULL;
    int got_frame = 0;//是否解码出完整的像素。
    while (1)
    {
        //读取码流数据到缓存
        uDataSize = fread(inbuf, 1, INBUF_SIZE, pFin);
        if (uDataSize == 0)
        {
            break;
        }
        pDataPtr = inbuf;//指向缓存的首地址
        while (uDataSize > 0)
        {
            //解析缓存里的码流数据
            len = av_parser_parse2(pCodecParserCtx, pCodecContext, &pkt.data, &pkt.size, pDataPtr, uDataSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
            pDataPtr += len;
            uDataSize -= len;
            if (pkt.size == 0)
            {
                continue;
            }
            //成功解析出一个packet的码流
            printf("Parse 1 packet.\n");
            int ret = avcodec_decode_video2(pCodecContext, frame, &got_frame, &pkt);
            if (ret < 0)
            {
                printf("Error:decode failed.\n");
                return -1;
            }
            if (got_frame)//解码出一帧完整的图像
            {
                printf("decode 1 frame OK Width*Height:(%d*%d).\n", frame->width, frame->height);
                //YUV像素数据写入文件
                write_out_yuv_frame(frame);
            }
        }
    }
    //解码解码器中剩余的数据
    pkt.data = NULL;
    pkt.size = 0;
    while (1)
    {
        int ret = avcodec_decode_video2(pCodecContext, frame, &got_frame, &pkt);
        if (ret < 0)
        {
            printf("Error:decode failed.\n");
            return -1;
        }
        if (got_frame)//解码出一帧完整的图像
        {
            printf("Flush decoder OK .\n");
            //YUV像素数据写入文件
            write_out_yuv_frame(frame);
        }
        else
        {
            break;
        }
    }
    Colse();
}

int main2();
int main1();
int encode_main();

jint FFmpeg_test(JNIEnv *env, jobject obj, jint argc, jobjectArray argv)
{
    const char *a;
    const char *b;
    jstring str1 = (jstring)env->GetObjectArrayElement(argv, 0);
    a = env->GetStringUTFChars(str1, NULL);
    jstring str2 = (jstring) env->GetObjectArrayElement(argv, 1);
    b = env->GetStringUTFChars(str2, NULL);
    const char *arg[] = {a,b};
    int c = argc;
    main2();
    return test(c, (char **)arg);
}

static JNINativeMethod gMethods[] =
        {
                {"ffTest", "(I[Ljava/lang/String;)I", (void *)FFmpeg_test}
        };

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *env = NULL;
    jint result = JNI_ERR;

    //获取env指针
    if (jvm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return result;
    }
    if (env == NULL) {
        return result;
    }

    //获取类引用，写类的全路径（包名+类名）。FindClass等JNI函数将在后面讲解
    jclass clazz = env->FindClass("etrans/com/avideo/TestActivity");
    if (clazz == NULL) {
        return result;
    }
    //注册方法
    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        return result;
    }
    //成功
    result = JNI_VERSION_1_6;
    return result;
}

int main2()
{
    AVFormatContext	*pFormatCtx;
    int	i = 0, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame	*pFrame, *pFrameYUV;
    unsigned char *out_buffer;
    AVPacket *packet;

    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    char filepath[] = "/storage/emulated/0/outPut.mp4";
    FILE *fp_yuv = fopen("/storage/emulated/0/xixixi.yuv", "wb+");
    if (!fp_yuv)
    {
        return -1;
    }
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    /** 打开一个视频文件，它会根据打开的文件信息填充AVFormatContext
     *  注意：只是读文件头，并不会填充流信息（也就是相当于读取文件的属性？？？）
     *  AVFormatContext必须为null，或者由avformat_alloc_context进行分配
     */
    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
    {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    /** 解码几帧视音频数据并且获得一些相关的信息，为的是防止文件头信息不全
     *  获取视频文件的流信息,填充到AVFormatContext
     */
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    //查找视频编码索引
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            // 首次读到了视频包？？？？
            videoindex = i;
            break;
        }
    }

    if (videoindex == -1)
    {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    //编解码上下文
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    //查找解码器
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Could not open codec.\n");
        return -1;
    }

    //申请AVFrame，用于原始视频
    pFrame = av_frame_alloc();
    //申请AVFrame，用于yuv视频
    pFrameYUV = av_frame_alloc();
    //分配内存，用于图像格式转换
    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    //手工调试函数，输出tbn、tbc、tbr、PAR、DAR的含义
    av_dump_format(pFormatCtx, 0, filepath, 0);
    printf("-------------------------------------------------\n");

    //申请转换上下文
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    //读取数据
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        if (packet->stream_index == videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0)
            {
                printf("Decode Error.\n");
                return -1;
            }

            if (got_picture >= 1)
            {
                //成功解码一帧
                sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);//转换图像格式

                y_size = pCodecCtx->width*pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
                printf("Succeed to decode 1 frame!\n");
            }
            else
            {
                //未解码到一帧，可能时结尾B帧或延迟帧，在后面做flush decoder处理
            }
        }
        av_free_packet(packet);
    }

    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (true)
    {
        if (!(pCodec->capabilities & CODEC_CAP_DELAY))
            return 0;

        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
        {
            break;
        }
        if (!got_picture)
        {
            break;
        }

        sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                  pFrameYUV->data, pFrameYUV->linesize);

        int y_size = pCodecCtx->width*pCodecCtx->height;
        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
        printf("Flush Decoder: Succeed to decode 1 frame!\n");
    }

    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    fclose(fp_yuv);

    return 0;
}

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &CODEC_CAP_DELAY))
    {
        return 0;
    }
    while (true)
    {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
        {
            break;
        }
        if (!got_frame)
        {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
        {
            break;
        }
    }
    return ret;
}

int main1()
{
    AVFormatContext* pFormatCtx;
    AVOutputFormat* pOutputFmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    uint8_t* picture_buf;
    AVFrame* pFrame;
    int picture_size;
    int y_size;
    int framecnt = 0;
    int in_w = 640, in_h = 272;					//视频图像宽高
    const char* out_file = "output.h264";

    int i, j;
    int num;
    int index_y, index_u, index_v;
    uint8_t *y_, *u_, *v_, *in;

    int got_picture = 0;
    int ret;

    av_register_all();
    //alloc avformat context
    pFormatCtx = avformat_alloc_context();
    //猜测类型,返回输出类型。
    pOutputFmt = av_guess_format(NULL, out_file, NULL);
    pFormatCtx->oformat = pOutputFmt;
    //打开FFmpeg的输入输出文件,成功之后创建的AVIOContext结构体。
    if (avio_open2(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE,NULL,NULL) < 0)
    {
        //Failed to open output file
        return -1;
    }

    //除了以下方法，另外还可以使用avcodec_find_encoder_by_name()来获取AVCodec
    pCodec = avcodec_find_encoder(pOutputFmt->video_codec);//获取编码器
    if (!pCodec)
    {
        //cannot find encoder
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);//申请AVCodecContext，并初始化。
    if (!pCodecCtx)
    {
        //failed get AVCodecContext
        return -1;
    }

    FILE * in_file = fopen("640-272.yuv", "rb");   //打开原始yuv数据文件

    /* 创建输出码流的AVStream */
    video_st = avformat_new_stream(pFormatCtx, 0);
    video_st->time_base.num = 1;
    video_st->time_base.den = 25;
    if (video_st == NULL)
    {
        return -1;
    }
    //Param that must set
    pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;  //H265
    pCodecCtx->codec_id = AV_CODEC_ID_H264;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->b_frame_strategy = true;
    /*
    码率
    bit_rate/-bt tolerance 设置视频码率容忍度kbit/s （固定误差）
    rc_max_rate/-maxrate bitrate设置最大视频码率容忍度 （可变误差）
    rc_min_rate/-minrate bitreate 设置最小视频码率容忍度（可变误差）
    rc_buffer_size/-bufsize size 设置码率控制缓冲区大小
    如何设置固定码率编码 ?
    bit_rate是平均码率，不一定能控制住
    c->bit_rate = 400000;
    c->rc_max_rate = 400000;
    c->rc_min_rate = 400000;
    提示  [libx264 @ 00c70be0] VBV maxrate specified, but no bufsize, ignored
    再设置  c->rc_buffer_size = 200000;  即可。如此控制后编码质量明显差了。
    */
    pCodecCtx->bit_rate = 400000;				//采样码率越大，目标文件越大
    //pCodecCtx->bit_rate_tolerance = 8000000; // 码率误差，允许的误差越大，视频越小
    //两个I帧之间的间隔
    pCodecCtx->gop_size = 15;

    //编码帧率，每秒多少帧。下面表示1秒25帧
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    //最小的量化因子
    pCodecCtx->qmin = 10;
    //最大的量化因子
    pCodecCtx->qmax = 30;

    //最大B帧数
    pCodecCtx->max_b_frames = 3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
    {
        /*
        preset的参数主要调节编码速度和质量的平衡，有ultrafast、superfast、veryfast、faster、fast、medium、slow、slower、veryslow、placebo这10个选项，从快到慢。
        */
        av_dict_set(NULL, "preset", "fast", 0);
        /*
        tune的参数主要配合视频类型和视觉优化的参数。
        tune的值有： film：  电影、真人类型；
        animation：  动画；
        grain：      需要保留大量的grain时用；
        stillimage：  静态图像编码时使用；
        psnr：      为提高psnr做了优化的参数；
        ssim：      为提高ssim做了优化的参数；
        fastdecode： 可以快速解码的参数；
        zerolatency：零延迟，用在需要非常低的延迟的情况下，比如电视电话会议的编码。
        */
        av_dict_set(NULL, "tune", "zerolatency", 0);
        /*
        画质,分别是baseline, extended, main, high
        1、Baseline Profile：基本画质。支持I/P 帧，只支持无交错（Progressive）和CAVLC；
        2、Extended profile：进阶画质。支持I/P/B/SP/SI 帧，只支持无交错（Progressive）和CAVLC；(用的少)
        3、Main profile：主流画质。提供I/P/B 帧，支持无交错（Progressive）和交错（Interlaced）， 也支持CAVLC 和CABAC 的支持；
        4、High profile：高级画质。在main Profile 的基础上增加了8x8内部预测、自定义量化、 无损视频编码和更多的YUV 格式；
　　	H.264 Baseline profile、Extended profile和Main profile都是针对8位样本数据、4:2:0格式(YUV)的视频序列。在相同配置情况下，
        High profile（HP）可以比Main profile（MP）降低10%的码率。 根据应用领域的不同，Baseline profile多应用于实时通信领域，
        Main profile多应用于流媒体领域，High profile则多应用于广电和存储领域。
        */
        //av_dict_set(¶m, "profile", "main", 0);
    }
    else if (pCodecCtx->codec_id == AV_CODEC_ID_H265 || pCodecCtx->codec_id == AV_CODEC_ID_HEVC)
    {
        av_dict_set(NULL, "preset", "fast", 0);
        av_dict_set(NULL, "tune", "zerolatency", 0);
    }

    //Output Info-----------------------------
    printf("--------------- out_file Information ----------------\n");
    //手工调试函数，输出tbn、tbc、tbr、PAR、DAR的含义
    av_dump_format(pFormatCtx, 0, out_file, 1);	//最后一个参数，如果是输出文件时，该值为1；如果是输入文件时，该值为0
    printf("-----------------------------------------------------\n");

    /* 查找编码器 */
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec)
    {
        printf("Can not find encoder! \n");
        return -1;
    }

    /* 打开编码器 */
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
    {
        printf("Failed to open encoder! \n");
        return -1;
    }

    pFrame = av_frame_alloc();
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(picture_size);
    avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    //Write File Header
    /* 写文件头（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS） */
    avformat_write_header(pFormatCtx, NULL);

    /* Allocate the payload of a packet and initialize its fields with default values.  */
    av_new_packet(&pkt, picture_size);

    j = 1;
    y_size = pCodecCtx->width * pCodecCtx->height;
    unsigned int uiReadSize = 0;
    while (true)
    {
        //Read raw YUV data
        if (feof(in_file))	//文件结束
        {
            break;
        }
        else
        {
            uiReadSize = fread(picture_buf, 1, y_size * 3 / 2, in_file);
            if (uiReadSize <= 0)
            {
                printf("Failed to read raw data! \n");
                break;
            }
            else if (uiReadSize < y_size * 3 / 2)
            {
                //读取数据不满足一帧YUV数据，表示数据读取完
                break;
            }
        }

        pFrame->data[0] = picture_buf;					 // Y
        pFrame->data[1] = picture_buf + y_size;			 // U
        pFrame->data[2] = picture_buf + y_size * 5 / 4;  // V

        //PTS
        pFrame->pts = j*(video_st->time_base.den) / ((video_st->time_base.num) * 25);

        //Encode
        /* 编码一帧视频。即将AVFrame（存储YUV像素数据）编码为AVPacket（存储H.264等格式的码流数据） */
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
        if (ret < 0)
        {
            printf("Failed to encode! \n");
            return -1;
        }
        if (got_picture == 1)
        {
            printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
            framecnt++;
            pkt.stream_index = video_st->index;

            /* 将编码后的视频码流写入文件 */
            ret = av_write_frame(pFormatCtx, &pkt);

            av_free_packet(&pkt);
        }

        j++;
    }

    //Flush Encoder
    /* 输入的像素数据读取完成后调用此函数。用于输出编码器中剩余的AVPacket */
    ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0)
    {
        printf("Flushing encoder failed\n");
        return -1;
    }

    //Write file trailer
    /* 写文件尾（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS） */
    av_write_trailer(pFormatCtx);

    //Clean
    if (video_st)
    {
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    av_free_packet(&pkt);

    fclose(in_file);

    return 0;
}

int encode_main()
{
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    uint8_t* picture_buf;
    AVFrame* pFrame;
    int picture_size;
    int y_size;
    int framecnt=0;
    //FILE *in_file = fopen("src01_480x272.yuv", "rb");	//Input raw YUV data
    FILE *in_file = fopen("../ds_480x272.yuv", "rb");   //Input raw YUV data
    int in_w=480,in_h=272;                              //Input data's width and height
    int framenum=100;                                   //Frames to encode
    //const char* out_file = "src01.h264";              //Output Filepath
    //const char* out_file = "src01.ts";
    //const char* out_file = "src01.hevc";
    const char* out_file = "ds.h264";

    av_register_all();
    //Method1.
    pFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(NULL, out_file, NULL);
    pFormatCtx->oformat = fmt;

    //Method 2.
    //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
    //fmt = pFormatCtx->oformat;


    //Open output URL
    if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0){
        printf("Failed to open output file! \n");
        return -1;
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    //video_st->time_base.num = 1;
    //video_st->time_base.den = 25;

    if (video_st==NULL){
        return -1;
    }
    //Param that must set
    pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size=250;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    //pCodecCtx->qcompress = 0.6;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    //Optional Param
    pCodecCtx->max_b_frames=3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(NULL, "preset", "slow", 0);
        av_dict_set(NULL, "tune", "zerolatency", 0);
        //av_dict_set(¶m, "profile", "main", 0);
    }
    //H.265
    if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
        av_dict_set(NULL, "preset", "ultrafast", 0);
        av_dict_set(NULL, "tune", "zero-latency", 0);
    }

    //Show some Information
    av_dump_format(pFormatCtx, 0, out_file, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec){
        printf("Can not find encoder! \n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
        printf("Failed to open encoder! \n");
        return -1;
    }
    pFrame = av_frame_alloc();
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(picture_size);
    avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    //Write File Header
    avformat_write_header(pFormatCtx,NULL);

    // 应该是给pkt，分配一个空间
    av_new_packet(&pkt,picture_size);

    y_size = pCodecCtx->width * pCodecCtx->height;

    for (int i=0; i<framenum; i++){
        //Read raw YUV data
        if (fread(picture_buf, 1, y_size*3/2, in_file) <= 0){
            printf("Failed to read raw data! \n");
            return -1;
        }else if(feof(in_file)){
            break;
        }
        pFrame->data[0] = picture_buf;              // Y
        pFrame->data[1] = picture_buf+ y_size;      // U
        pFrame->data[2] = picture_buf+ y_size*5/4;  // V
        //PTS
        //pFrame->pts=i;
        pFrame->pts=i*(video_st->time_base.den)/((video_st->time_base.num)*25);
        int got_picture=0;
        //Encode
        int ret = avcodec_encode_video2(pCodecCtx, &pkt,pFrame, &got_picture);
        if(ret < 0){
            printf("Failed to encode! \n");
            return -1;
        }
        if (got_picture==1){
            printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
            framecnt++;
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx, &pkt);
            av_free_packet(&pkt);
        }
    }
    //Flush Encoder
    int ret = flush_encoder(pFormatCtx,0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
    }

    //Write file trailer
    av_write_trailer(pFormatCtx);

    //Clean
    if (video_st){
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(in_file);

    return 0;
}