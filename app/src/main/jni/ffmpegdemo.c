#include <stdio.h>
#include "com_chaoyi_myapplication_MusicPlay.h"
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavfilter/avfilter.h"
#include "include/libswresample/swresample.h"


/* Log */
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGE( format, ... ) __android_log_print( ANDROID_LOG_ERROR, "(>_<)", format, ## __VA_ARGS__ )
#else
#define LOGE( format, ... ) printf( "(>_<) " format "\n", ## __VA_ARGS__ )
#endif


JNIEXPORT void JNICALL Java_com_chaoyi_myapplication_MusicPlay_playSound
	( JNIEnv *env, jobject instance, jstring input_ )
{
	const char *input = (*env)->GetStringUTFChars( env, input_, 0 );

	LOGE( "%s file : %s", "play sound begin....", input );


	av_register_all();
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	/* open */
	if ( avformat_open_input( &pFormatCtx, input, NULL, NULL ) != 0 )
	{
		LOGE( "%s", "打开输入文件失败" );
		return;
	} else {
		LOGE( "%s", "打开输入文件成功" );
	}

	/* 获取视频信息 */
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		LOGE( "%s", "获取流信息失败" );
		return;
	} else {
		LOGE( "%s", "获取流信息成功" );
	}

	int	audio_stream_idx	= -1;
	int	i			= 0;
	for ( int i = 0; i < pFormatCtx->nb_streams; ++i )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			LOGE( "找到音频" );
			audio_stream_idx = i;
			break;
		}
	}
	/* 获取解码器上下文 */
	AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
	/* 获取解码器 */
	AVCodec *pCodex = avcodec_find_decoder( pCodecCtx->codec_id );
	/* 打开解码器 */

	if ( avcodec_open2( pCodecCtx, pCodex, NULL ) < 0 )
	{
		LOGE( "%s", "打开解码器失败" );
		return;
	} else {
		LOGE( "%s : %s", "打开解码器成功", pCodex->name );
	}

	/* 申请avpakcet，装解码前的数据 */
	AVPacket *packet = (AVPacket *) av_malloc( sizeof(AVPacket) );
	/* 申请avframe，装解码后的数据 */
	AVFrame *frame = av_frame_alloc();


	/* 得到SwrContext ，进行重采样 */
	SwrContext *swrContext = swr_alloc();

	/* 缓存区 */
	uint8_t *out_buffer = (uint8_t *) av_malloc( 44100 * 2 );
	/* 输出的声道布局（立体声） */
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
	/* 输出采样位数  16位 */
	enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
	/* 输出的采样率必须与输入相同 */
	int out_sample_rate = pCodecCtx->sample_rate;

	/* swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式 */
	swr_alloc_set_opts( swrContext, out_ch_layout, out_formart, out_sample_rate,
			    pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
			    NULL );

	swr_init( swrContext );

/*    获取通道数  2 */
	int out_channer_nb = av_get_channel_layout_nb_channels( AV_CH_LAYOUT_STEREO );
/*    反射得到Class类型 */
	jclass david_player = (*env)->GetObjectClass( env, instance );
/*    反射得到createAudio方法 */
	jmethodID createAudio = (*env)->GetMethodID( env, david_player, "createTrack", "(II)V" );
/*    反射调用createAudio */
	(*env)->CallVoidMethod( env, instance, createAudio, 44100, out_channer_nb );
	jmethodID audio_write = (*env)->GetMethodID( env, david_player, "playTrack", "([BI)V" );


	int got_frame;
	while ( av_read_frame( pFormatCtx, packet ) >= 0 )
	{
		if ( packet->stream_index == audio_stream_idx )
		{
/*            解码  mp3   编码格式frame----pcm   frame */
			avcodec_decode_audio4( pCodecCtx, frame, &got_frame, packet );
			if ( got_frame )
			{
				LOGE( "解码" );
				swr_convert( swrContext, &out_buffer, 44100 * 2, (const uint8_t * *) frame->data, frame->nb_samples );
/*                缓冲区的大小 */
				int size = av_samples_get_buffer_size( NULL, out_channer_nb, frame->nb_samples,
								       AV_SAMPLE_FMT_S16, 1 );
				jbyteArray audio_sample_array = (*env)->NewByteArray( env, size );
				(*env)->SetByteArrayRegion( env, audio_sample_array, 0, size, (const jbyte *) out_buffer );
				(*env)->CallVoidMethod( env, instance, audio_write, audio_sample_array, size );
				(*env)->DeleteLocalRef( env, audio_sample_array );
			}
		}
	}
	av_frame_free( &frame );
	swr_free( &swrContext );
	avcodec_close( pCodecCtx );
	avformat_close_input( &pFormatCtx );
	(*env)->ReleaseStringUTFChars( env, input_, input );

	LOGE( "%s", "play sound end...." );
}


