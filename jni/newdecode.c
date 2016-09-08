/**
 功能：实现amr到wav格式的转换，并用jni被android调用！

 */
#include "com_android_jni_Decodec.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <win2linuxheader.h>

#include <android/log.h>
#include <jni.h>

#include "include/libavutil/avutil.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libswscale/swscale.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "lichao", __VA_ARGS__))

struct RIFF_HEADER {
	char szRiffID[4];
	DWORD dwRiffSize;
	char szRiffFormat[4];
};

struct FMT_BLOCK {
	char szFmtID[4];
	DWORD dwFmtSize;
	WORD wFormatTag;
	WORD wChannels;
	DWORD dwSamplesPerSec;
	DWORD dwAvgBytesPerSec;
	WORD wBlockAlign;
	WORD wBitsPerSample;
};

struct DATA_BLOCK {
	char szDataID[4];
	DWORD dwDataSize;
};
//JNI interface
JNIEXPORT jint JNICALL Java_com_android_jni_Decodec_newdecode(JNIEnv *env,
		jclass clz, jstring fileName) {
	AVFormatContext* pFormatCtx = NULL;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec = NULL;

	int AudioStream = -1;

	const char* SrcFile = (*env)->GetStringUTFChars(env, fileName, NULL);

	av_register_all();

	if (avformat_open_input(&pFormatCtx, SrcFile, NULL, NULL) != 0) {
		LOGI("Cannot open source file!");

		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGI( "Cannot find stream info!");

		return -1;
	}

	av_dump_format(pFormatCtx, 0, SrcFile, 0);
	int i;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			AudioStream = i;
			break;
		}
	}

	if (AudioStream == -1) {
		LOGI("Cannot find a audio stream!");

		return -1;
	}

	pCodecCtx = pFormatCtx->streams[AudioStream]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if (pCodec == NULL) {
		LOGI("Cannot find codec!");

		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGI("Cannot open codec!");

		return -1;
	}
	LOGI("比特率 %3d\n", pFormatCtx->bit_rate);
	LOGI("解码器名称 %s\n", pCodecCtx->codec->long_name);
	LOGI("time_base  %d \n", pCodecCtx->time_base);
	LOGI("声道数  %d \n", pCodecCtx->channels);
	LOGI("sample per second  %d \n", pCodecCtx->sample_rate);
	AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket));
	av_init_packet(packet);
	AVFrame *pFrame;
	pFrame = avcodec_alloc_frame(); //wtz

	int index = 0;
	int outSize;
	FILE* pWavFile;
	pWavFile = fopen("storage/sdcard0/lichao1.wav", "wb");
	if (!pWavFile) {
		LOGI("error while open lichao1.wav!");
		return -1;
	}
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == AudioStream){ //if is audiostream
			int len = avcodec_decode_audio4(pCodecCtx, pFrame, &outSize,packet);
			if (len < 0) {
				LOGI( "Error while decoding!");
				break;
			}
			if (outSize > 0) {

				/*********************解码过程 并将avcodec_decode_audio4（）得到的AV_SAMPLE_FMT_FLT转换成AV_SAMPLE_FMT_S16（32bits to 16bits）******************/
				int in_samples = pFrame->nb_samples;
				short *sample_buffer = (short *) malloc(
						pFrame->nb_samples * 2 * 2);
				memset(sample_buffer, 0, pFrame->nb_samples * 4);

				int i = 0;
				float* inputChannel0 = (float*) (pFrame->extended_data[0]);
//Mono
				if (pCodecCtx->channels == 1) {
					for (i = 0; i < in_samples; i++) {
						float sample = *inputChannel0++;
						// LOGI("sample is %f\n",sample);
						if (sample < -1.0f)
							sample = -1.0f;
						else if (sample > 1.0f)
							sample = 1.0f;
						sample_buffer[i] = (int16_t) (sample * 32767.0f);
					}
				}
//Stereo
				else {
					float* inputChannel1 = (float*) (pFrame->extended_data[1]);
					for (i = 0; i < in_samples; i++) {
						sample_buffer[i * 2] = (int16_t) ((*inputChannel0++)
								* 32767.0f);
						sample_buffer[i * 2 + 1] = (int16_t) ((*inputChannel1++)
								* 32767.0f);
					}
				}
				char data[1024 * 1024];
//memset(data,0,1024*1024);//重复申请内存 运行报错
				fwrite(sample_buffer, 1, in_samples * 2, pWavFile);
				/***************************************************/

				index++; //统计packet的数量
//LOGI("Write data into outfile……");

			}

		}
		av_free_packet(packet);
	}

//write wave header

	fseek(pWavFile, 0, SEEK_SET);

	int bits;
	switch (pCodecCtx->sample_fmt) {
	case AV_SAMPLE_FMT_S16:
		bits = 16;
		break;
	case AV_SAMPLE_FMT_S32:
		bits = 32;
		break;
	case AV_SAMPLE_FMT_U8:
		bits = 8;
		break;
	default:
		bits = 16;
		break;
	}
	LOGI("bits:%d sample_rate:%d channels:%d sample_fmt:%d\n", bits, pCodecCtx->sample_rate, pCodecCtx->channels, pCodecCtx->sample_fmt);

	struct RIFF_HEADER riffHeader;
	memcpy(riffHeader.szRiffID, "RIFF", 4);
	riffHeader.dwRiffSize = pFormatCtx->duration * (bits / 8)
			* pCodecCtx->sample_rate * pCodecCtx->channels + 36;
	memcpy(riffHeader.szRiffFormat, "WAVE", 4);
	fwrite(&riffHeader, 1, sizeof(riffHeader), pWavFile);
	LOGI("sizeof(riffHeader) is %d\n", sizeof(riffHeader));
	struct FMT_BLOCK fmtBlock;
	memcpy(fmtBlock.szFmtID, "fmt ", 4);
	fmtBlock.dwFmtSize = 0x10;
	fmtBlock.wFormatTag = 0x01;
	fmtBlock.wChannels = pCodecCtx->channels;
	fmtBlock.dwSamplesPerSec = pCodecCtx->sample_rate;
	fmtBlock.dwAvgBytesPerSec = (bits / 8) * pCodecCtx->sample_rate
			* pCodecCtx->channels;
	fmtBlock.wBlockAlign = (bits / 4) * pCodecCtx->channels;
	fmtBlock.wBitsPerSample = bits;
	fwrite(&fmtBlock, 1, sizeof(fmtBlock), pWavFile);
	LOGI("sizeof(fmtBlock) is %d\n", sizeof(fmtBlock));
	struct DATA_BLOCK dataBlock;
	memcpy(dataBlock.szDataID, "data", 4);
	dataBlock.dwDataSize = (bits / 8) * pFormatCtx->duration
			* pCodecCtx->sample_rate * pCodecCtx->channels;
	fwrite(&dataBlock, 1, sizeof(dataBlock), pWavFile);
	LOGI("sizeof(dataBlock) is %d\n", sizeof(dataBlock));
	fflush(pWavFile);
	fseek(pWavFile, 44, SEEK_SET);

	fclose(pWavFile);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	LOGI("Success!");

	return 0;
}
