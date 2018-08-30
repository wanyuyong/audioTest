#include <stdio.h>
#include "com_chaoyi_myapplication_OpenSLPlay.h"
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
#include "include/libavfilter/avfilter.h"
#include "include/libswresample/swresample.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


/* Log */
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGE( format, ... ) __android_log_print( ANDROID_LOG_ERROR, "(>_<)", format, ## __VA_ARGS__ )
#else
#define LOGE( format, ... ) printf( "(>_<) " format "\n", ## __VA_ARGS__ )
#endif

SLObjectItf	engineObject	= NULL;  /* 用SLObjectItf声明引擎接口对象 */
SLEngineItf	engineEngine	= NULL;  /* 声明具体的引擎对象 */

SLObjectItf	outputMixObject	= NULL;  /* 用SLObjectItf创建混音器接口对象 */
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;  /* 具体的混音器对象实例 */
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;  /* 默认情况 */

SLObjectItf	audioplayer	= NULL; /* 用SLObjectItf声明播放器接口对象 */
SLPlayItf slPlayItf	= NULL; /* 播放器接口 */
SLAndroidSimpleBufferQueueItf slBufferQueueItf	= NULL;  /* 缓冲区队列接口 */

size_t	buffersize = 0;
void	*buffer;

AVFormatContext *pFormatCtx;
AVCodecContext	*pCodecCtx;
AVCodec		*pCodex;
AVPacket	*packet;
AVFrame		*frame;
SwrContext	*swrContext;
uint8_t		*out_buffer;
int		out_channer_nb;
int		audio_stream_idx = -1;

/* opensl es调用 int * rate,int *channel */
int createFFmpeg( int *rate, int *channel, const char *input)
{
	av_register_all();
	pFormatCtx = avformat_alloc_context();
	int	error;
	char buf[] = "";
	/* 打开视频地址并获取里面的内容(解封装) */
	if ( (error = avformat_open_input( &pFormatCtx, input, NULL, NULL ) ) < 0 )
	{
		av_strerror( error, buf, 1024 );
		/* LOGE("%s" ,inputPath) */
		LOGE( "Couldn't open file %s: %d(%s)", input, error, buf );
		/* LOGE("%d",error) */
		LOGE( "打开视频失败" );
	}
	/* 3.获取视频信息 */
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		LOGE( "%s", "获取视频信息失败" );
		return(-1);
	}


	int i = 0;
	for ( int i = 0; i < pFormatCtx->nb_streams; ++i )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			LOGE( "  找到音频id %d", pFormatCtx->streams[i]->codec->codec_type );
			audio_stream_idx = i;
			break;
		}
	}
/* mp3的解码器 */

/*    获取音频编解码器 */
	pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
	LOGE( "获取视频编码器上下文 %p  ", pCodecCtx );

	pCodex = avcodec_find_decoder( pCodecCtx->codec_id );
	LOGE( "获取视频编码 %p", pCodex );

	if ( avcodec_open2( pCodecCtx, pCodex, NULL ) < 0 )
	{
	}
	packet = (AVPacket *) av_malloc( sizeof(AVPacket) );
/*
 *    av_init_packet(packet);
 *    音频数据
 */

	frame = av_frame_alloc();

/*    mp3  里面所包含的编码格式   转换成  pcm   SwcContext */
	swrContext = swr_alloc();

	int	length = 0;
	int	got_frame;
/*    44100*2 */
	out_buffer = (uint8_t *) av_malloc( 44100 * 2 );
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
/*    输出采样位数  16位 */
	enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
/* 输出的采样率必须与输入相同 */
	int out_sample_rate = pCodecCtx->sample_rate;


	swr_alloc_set_opts( swrContext, out_ch_layout, out_formart, out_sample_rate,
			    pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
			    NULL );

	swr_init( swrContext );
/*    获取通道数  2 */
	out_channer_nb	= av_get_channel_layout_nb_channels( AV_CH_LAYOUT_STEREO );
	*rate		= pCodecCtx->sample_rate;
	*channel	= pCodecCtx->channels;
	return(0);
}


/*  */
int getPcm( void **pcm, size_t *pcm_size )
{
	int	frameCount = 0;
	int	got_frame;
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
				*pcm		= out_buffer;
				*pcm_size	= size;
				break;
			}
		}
	}
	return(0);
}


void realseFFmpeg()
{
	av_free_packet( packet );
	av_free( out_buffer );
	av_frame_free( &frame );
	swr_free( &swrContext );
	avcodec_close( pCodecCtx );
	avformat_close_input( &pFormatCtx );
}


/* 将pcm数据添加到缓冲区中 */
void getQueueCallBack( SLAndroidSimpleBufferQueueItf slBufferQueueItf, void* context )
{
	buffersize = 0;

	getPcm( &buffer, &buffersize );
	if ( buffer != NULL && buffersize != 0 )
	{
		/* 将得到的数据加入到队列中 */
		(*slBufferQueueItf)->Enqueue( slBufferQueueItf, buffer, buffersize );
	}
}


/* 创建引擎 */
void createEngine()
{
	slCreateEngine( &engineObject, 0, NULL, 0, NULL, NULL );                                                                                /* 创建引擎 */
	(*engineObject)->Realize( engineObject, SL_BOOLEAN_FALSE );                                                                             /* 实现engineObject接口对象 */
	(*engineObject)->GetInterface( engineObject, SL_IID_ENGINE, &engineEngine );                                                            /* 通过引擎调用接口初始化SLEngineItf */
}


/* 创建混音器 */
void createMixVolume()
{
	(*engineEngine)->CreateOutputMix( engineEngine, &outputMixObject, 0, 0, 0 );                                                            /* 用引擎对象创建混音器接口对象 */
	(*outputMixObject)->Realize( outputMixObject, SL_BOOLEAN_FALSE );                                                                       /* 实现混音器接口对象 */
	SLresult sLresult = (*outputMixObject)->GetInterface( outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb );     /* 利用混音器实例对象接口初始化具体的混音器对象 */
	/* 设置 */
	if ( SL_RESULT_SUCCESS == sLresult )
	{
		(*outputMixEnvironmentalReverb)->
		SetEnvironmentalReverbProperties( outputMixEnvironmentalReverb, &settings );
	}
}


/* 创建播放器 */
void createPlayer(const char *input)
{
	/* 初始化ffmpeg */
	int	rate;
	int	channels;
	createFFmpeg( &rate, &channels, input);

	LOGE( "rate is %d , channels is %d, file : %s", rate, channels, input );

	SLDataLocator_AndroidBufferQueue android_queue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };

	SLDataFormat_PCM pcm = { SL_DATAFORMAT_PCM, channels,
	                         rate * 1000,
	                         SL_PCMSAMPLEFORMAT_FIXED_16,
	                         SL_PCMSAMPLEFORMAT_FIXED_16,
	                         SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
	                         SL_BYTEORDER_LITTLEENDIAN };

	SLDataSource dataSource = { &android_queue, &pcm };

	SLDataLocator_OutputMix slDataLocator_outputMix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };

	SLDataSink slDataSink = { &slDataLocator_outputMix, NULL };

	const SLInterfaceID	ids[3]	= { SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME };
	const SLboolean		req[3]	= { SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE, SL_BOOLEAN_FALSE };

	(*engineEngine)->CreateAudioPlayer( engineEngine, &audioplayer, &dataSource, &slDataSink, 3, ids, req );
	(*audioplayer)->Realize( audioplayer, SL_BOOLEAN_FALSE );
	(*audioplayer)->GetInterface( audioplayer, SL_IID_PLAY, &slPlayItf ); /* 初始化播放器 */
	/* 注册缓冲区,通过缓冲区里面 的数据进行播放 */
	(*audioplayer)->GetInterface( audioplayer, SL_IID_BUFFERQUEUE, &slBufferQueueItf );
	/* 设置回调接口 */
	(*slBufferQueueItf)->RegisterCallback( slBufferQueueItf, getQueueCallBack, NULL );
	/* 播放 */
	(*slPlayItf)->SetPlayState( slPlayItf, SL_PLAYSTATE_PLAYING );

	/* 开始播放 */
	getQueueCallBack( slBufferQueueItf, NULL );
}


JNIEXPORT void JNICALL Java_com_chaoyi_myapplication_OpenSLPlay_playSound
	( JNIEnv *env, jobject instance, jstring input_ )
{
	const char *input = (*env)->GetStringUTFChars( env, input_, 0 );

	LOGE( "%s file : %s", "open sl play sound begin....", input );

	createEngine();
	createMixVolume();
	createPlayer(input);
}


