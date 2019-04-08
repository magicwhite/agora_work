//
//  main.cpp
//  encoder_test
//
//  Created by rob on 2019/4/8.
//  Copyright © 2019 rob. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>

extern "C" {
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}


int main(int argc, char** argv) {
    
    int width = 1920;
    int height = 1080;
    int bitRate = 3000*1000;
    int frameNum = 0;
    
    std::string inputFileName = "input_yuv.yuv";
    std::string outputFileName = "out_264.h264";
    
    std::ifstream istrm(inputFileName, std::ios::binary);
    if (!istrm.is_open()) {
        std::cout << "failed to open input yuv file: input_yuv.yuv" << std::endl;
        exit(1);
    }
    std::ofstream ostrm(outputFileName, std::ios::binary);
    if (!ostrm.is_open()) {
        std::cout << "failed to open output 264 file: out_264.h264" << std::endl;
        exit(1);
    }
    
    avcodec_register_all();
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (codec == NULL) {
        std::cout << "failed when finding AV_CODEC_ID_H264" << std::endl;
        exit(1);
    }
    
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        std::cout << "failed when alloc codec context" << std::endl;
        exit(1);
    }
    
    codecCtx->width = width;
    codecCtx->height = height;
    codecCtx->bit_rate = bitRate;
    AVRational r = {1, 30};
    codecCtx->time_base = r;
    codecCtx->gop_size = 30;
    codecCtx->max_b_frames = 0;
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    av_opt_set(codecCtx->priv_data, "preset", "slow", 0);
    
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        std::cout << "failed when open codec" << std::endl;
        exit(1);
    }
    
    AVFrame *frame = av_frame_alloc();
    if (frame == NULL) {
        std::cout << "failed when alloc av frame" << std::endl;
        exit(2);
    }
    frame->width = codecCtx->width;
    frame->height = codecCtx->height;
    frame->format = codecCtx->pix_fmt;
    
    /*
    if (av_p_w_picpath_alloc(frame->data, frame->linesize, frame->width, frame->height, frame->format, 32) < 0) {
        std::cout << "failed when alloc picpath" << std::endl;
        exit(2);
    }
     */
    
    if (av_frame_get_buffer(frame, 0)) {
        std::cout << "failed when alloc plane buffer" << std::endl;
        exit(2);
    }
    
    AVPacket pkt;
    int got_packet;
    while (!istrm.eof()) {
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
        
        istrm.read(reinterpret_cast<char *>(frame->data[0]), frame->width * frame->height);
        istrm.read(reinterpret_cast<char *>(frame->data[1]), frame->width * frame->height / 4);
        istrm.read(reinterpret_cast<char *>(frame->data[2]), frame->width * frame->height / 4);
        frame->pts = frameNum;
        
        if (avcodec_encode_video2(codecCtx, &pkt, (const AVFrame *)frame, &got_packet) < 0) {
            std::cout << "failed when encode" << std::endl;
            exit(2);
        }
        if (got_packet) {
            std::cout << "encode [" << frameNum << "] frame, size=" << pkt.size << std::endl;
            ostrm.write(reinterpret_cast<char *>(pkt.data), pkt.size);
            av_packet_unref(&pkt);
        }
        frameNum++;
    }
    
    got_packet = 1;
    while (got_packet) {
        if (avcodec_encode_video2(codecCtx, &pkt, NULL, &got_packet) < 0) {
            std::cout << " failed when encode" << std::endl;
            exit(2);
        }
        if (got_packet) {
            std::cout << "write cache packet of frame[" << frameNum << "], size=" << pkt.size << std::endl;
            ostrm.write(reinterpret_cast<char *>(pkt.data), pkt.size);
            av_packet_unref(&pkt);
        }
    }
    
    istrm.close();
    ostrm.close();
    avcodec_close(codecCtx);
    av_free(codecCtx);
    //av_freep(&frame->data[0]);
    av_frame_free(&frame);
    
    return 0;
}


