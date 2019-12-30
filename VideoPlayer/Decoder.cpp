#include "Decoder.h"

#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include <QAudioOutput>


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
}

const int maxImgs = 5;
const int minAudio = 4096;

struct Decoder::Data
{
	AVFormatContext *pFormatCtx = nullptr;
	AVCodecContext	*pCodecCtx = nullptr;
	AVCodec *pCodec = nullptr;
	AVCodecContext	*pCodecCtxA = nullptr;
	AVCodec *pCodecA = nullptr;
	AVCodecContext	*pCodecCtxS = nullptr;
	AVCodec *pCodecS = nullptr;
	AVFrame	*pFrame = nullptr, *pFrameRGB = nullptr;
	AVFrame *aFrame_ReSample = nullptr;
	SwsContext *img_convert_ctx = nullptr;
	SwrContext *au_convert_ctx = nullptr;
	AVPacket *packet = nullptr;
	void(*audioCallBack)(void *,Audio) = nullptr;
	void *audioArg = nullptr;
	void(*subtitleCallBack)(void *, Subtitle) = nullptr;
	void *subtitleArg = nullptr;
	char *audioBuffer = nullptr;
	int audioBufferSize = 0;
	unsigned char *out_buffer = nullptr;
	unsigned char *out_bufferA = nullptr;
	int videoindex = -1;
	int audioindex = -1;
	int subtitleindex = -1;
	int bufSize = -1;
	int bufSizeA = -1;
	bool isWorking = true;
	bool isImage1 = true;
	bool isSignaled = false;
	bool isStop = false;
	QVector<int> audios;
	QVector<QString> audioInfo;
	QVector<int> subtitles;
	QVector<QString> subtitleInfo;
	QQueue<Video> images1;
	QQueue<Video> images2;
};

Decoder::Decoder(QObject *parent):QObject(parent)
{
	data = new Data;
	av_register_all();
}

Decoder::~Decoder()
{
	delete data;
}

void Decoder::switchWorkState(bool work)
{
	data->isWorking = work;
}

QString Decoder::setFile(const QString & file)
{
	data->pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&data->pFormatCtx, file.toStdString().c_str(), nullptr, nullptr) != 0)
	{
		return QString(u8"无法打开文件："+file);
	}

	if (avformat_find_stream_info(data->pFormatCtx, nullptr) < 0)
	{
		return QString(u8"找不到媒体流信息：" + file);
	}

	data->videoindex = -1;
	data->audios.clear();
	data->subtitles.clear();
	data->audioInfo.clear();
	data->subtitleInfo.clear();

	for (int i = 0; i < data->pFormatCtx->nb_streams; i++)
	{
		int t = data->pFormatCtx->streams[i]->codec->codec_type;
		if (t == AVMEDIA_TYPE_VIDEO)
		{
			data->videoindex = i;
		}
		else if (t == AVMEDIA_TYPE_AUDIO)
		{
			data->audios.push_back(i);
			AVDictionaryEntry *t = av_dict_get(data->pFormatCtx->streams[i]->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX);
			if (t)
			{
				QString str(t->value);
				data->audioInfo.push_back(str);
			}
		}
		else if (t == AVMEDIA_TYPE_SUBTITLE)
		{
			data->subtitles.push_back(i);
			AVDictionaryEntry *t = av_dict_get(data->pFormatCtx->streams[i]->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX);
			if (t)
			{
				QString str(t->value);
				data->subtitleInfo.push_back(str);
			}
		}
	}

	openVideoDecodec(data->videoindex);
	if (data->audios.size())
	{
		openAudioDecodec(data->audios[2]);
	}
	if (data->subtitles.size())
	{
		openSubTitleDecodec(data->subtitles[2]);
	}

	data->pFrame = av_frame_alloc();
	data->packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	return QString();
}

QQueue<Video>* Decoder::getFrame()
{
	if (data->videoindex != 0)
	{
		return &data->images1;
	}

	data->isWorking = true;

	if (data->isImage1)
	{
		return &data->images2;
	}

	return &data->images1;
}

int Decoder::getVideo()
{
	return data->videoindex;
}

void Decoder::setAudioCallBack(void(*callBack)(void *, Audio), void * arg)
{
	data->audioCallBack = callBack;
	data->audioArg = arg;
}

void Decoder::setSubtitleCallBack(void(*callBack)(void *, Subtitle), void * arg)
{
	data->subtitleCallBack = callBack;
	data->subtitleArg = arg;
}

QVector<int> Decoder::getAudio()
{
	return data->audios;
}

QSize Decoder::getVideoSize()
{
	if (data->videoindex == -1)
	{
		return QSize(640,480);
	}

	return QSize(data->pCodecCtx->width, data->pCodecCtx->height);
}

static void cleanUp(void *info)
{
	av_free(info);
}

void Decoder::decode()
{
	data->isWorking = true;
	data->isStop = false;

	while (true)
	{
		if (data->isStop)
		{
			break;
		}

		if (!data->isWorking)
		{
			QThread::msleep(10);
			continue;
		}

		int readed = -1;

		while ((readed = av_read_frame(data->pFormatCtx, data->packet)) >= 0)
		{
			bool ok = false;
			if (data->packet->stream_index == data->videoindex)
			{
				ok = decodeVideo();
			}
			else if (data->packet->stream_index == data->audioindex)
			{
				ok = decodeAudio();
			}
			else if (data->packet->stream_index == data->subtitleindex)
			{
				ok = decodeSubtitle();
			}

			av_free_packet(data->packet);

			if (ok)
			{
				break;
			}
		}
	
		if (readed < 0)
		{
			break;
		}
	}

	cleanVideo();
	cleanAudio();
	cleanSubTitle();

	avformat_close_input(&data->pFormatCtx);
	av_frame_free(&data->pFrame);
	av_free(&data->packet);
}

void Decoder::stop()
{
	data->isStop = true;
}

bool Decoder::decodeVideo()
{
	int got_picture;

	int ret = avcodec_decode_video2(data->pCodecCtx, data->pFrame, &got_picture, data->packet);
	
	if (ret < 0)
	{
		return false;
	}

	if (got_picture != 0)
	{
		sws_scale(data->img_convert_ctx, (const unsigned char* const*)data->pFrame->data, data->pFrame->linesize, 0, data->pCodecCtx->height,
			data->pFrameRGB->data, data->pFrameRGB->linesize);
	}
	else
	{
		return false;
	}

	QQueue<Video>* imgs;
	if (data->isImage1)
	{
		imgs = &data->images1;
	}
	else
	{
		imgs = &data->images2;
	}

	unsigned char *temp = (unsigned char *)av_malloc(data->bufSize);
	memcpy(temp, data->out_buffer, data->bufSize);
	QImage img = std::move(QImage((uchar *)temp, data->pCodecCtx->width, data->pCodecCtx->height, QImage::Format_RGB32, cleanUp, temp));

	double video_pts = 0;

	if (data->packet->dts == AV_NOPTS_VALUE && data->pFrame->opaque&& *(uint64_t*)data->pFrame->opaque != AV_NOPTS_VALUE)
	{
		video_pts = *(uint64_t *)data->pFrame->opaque;
	}
	else if (data->packet->dts != AV_NOPTS_VALUE)
	{
		video_pts = data->packet->dts;
	}
	else
	{
		video_pts = 0;
	}

	video_pts *= av_q2d(data->pFormatCtx->streams[data->videoindex]->time_base);
	video_pts *= 1000;

	if (video_pts >= 0)
	{
		imgs->push_back({ std::move(img) ,(__int64)video_pts });
		if (imgs->size() >= maxImgs)
		{
			data->isImage1 = !data->isImage1;
			data->isWorking = false;
		}

		if (!data->isSignaled)
		{
			data->isSignaled = true;
			emit frameGetted();
		}
	}

	return true;
}

bool Decoder::decodeAudio()
{
	int got_picture;

	int ret = avcodec_decode_audio4(data->pCodecCtxA, data->pFrame, &got_picture, data->packet);

	if (ret < 0)
	{
		return false;
	}

	if (got_picture != 0)
	{
		if (data->aFrame_ReSample->nb_samples != data->pFrame->nb_samples)
		{
			data->aFrame_ReSample->nb_samples = av_rescale_rnd(swr_get_delay(data->au_convert_ctx, 44100) + data->pFrame->nb_samples,
				44100, data->pFrame->sample_rate, AV_ROUND_UP);
			av_samples_fill_arrays(data->aFrame_ReSample->data, data->aFrame_ReSample->linesize, data->out_bufferA, AV_CH_LAYOUT_STEREO, data->aFrame_ReSample->nb_samples, AV_SAMPLE_FMT_S16, 0);
		}
		int len = swr_convert(data->au_convert_ctx, data->aFrame_ReSample->data, data->aFrame_ReSample->nb_samples, (const uint8_t **)data->pFrame->data, data->pFrame->nb_samples);
		if (len > 0)
		{
			int resampled_data_size = len * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
			char *buffer = new char[resampled_data_size];
			memcpy(buffer, data->out_bufferA, resampled_data_size);

			if (resampled_data_size < minAudio)
			{
				if (data->audioBuffer == nullptr)
				{
					data->audioBuffer = new char[2 * minAudio];
					data->audioBufferSize = 0;
				}

				memcpy(data->audioBuffer + data->audioBufferSize, buffer, resampled_data_size);
				delete[] buffer;
				data->audioBufferSize += resampled_data_size;

				if (data->audioBufferSize >= minAudio)
				{
					data->audioCallBack(data->audioArg, { data->audioBuffer, data->audioBufferSize });
					data->audioBuffer = nullptr;
				}
			}
			else
			{
				if (data->audioBuffer != nullptr&&data->audioBufferSize >= minAudio)
				{
					data->audioCallBack(data->audioArg, { data->audioBuffer, data->audioBufferSize });
					data->audioBuffer = nullptr;
				}
				data->audioCallBack(data->audioArg, { buffer, resampled_data_size });
			}

			if (!data->isSignaled)
			{
				data->isSignaled = true;
				emit frameGetted();
			}
		}

	}

	return true;
}

static int parseTime(char *ass, int i)
{
	int h = ass[i + 1] - '0';
	int mins = (ass[i + 3] - '0') * 10 + ass[i + 4] - '0';
	int s = (ass[i + 6] - '0') * 10 + ass[i + 7] - '0';
	int ms = ((ass[i + 9] - '0') * 10 + ass[i + 10] - '0') * 10;

	return h*3600 * 1000 + mins*60 * 1000 + s * 1000 + ms;
}

static QString parseText(char *ass)
{
	QString result;

	for (int i = 0; ass[i] != 0; ++i)
	{
		if (ass[i] == '[')
		{
			while (ass[i] != ']' && ass[i++] != 0);
		}
		else if (ass[i] == '{')
		{
			while (ass[i] != '}' && ass[i++] != 0);
		}
		else if (ass[i] == '\\')
		{
			++i;
		}
		else
		{
			result.append(ass[i]);
		}
	}

	return result.trimmed();
}

static Subtitle parseAss(char *ass)
{
	Subtitle result;  
	int counter = 0;

	for (int i = 0; ass[i] != 0; ++i)
	{
		if (ass[i] == ',')
		{
			++counter;
			//start
			if (counter == 1)
			{
				result.start = parseTime(ass, i);
			}

			//end
			if (counter == 2)
			{
				result.end = parseTime(ass, i);
			}

			//text
			if (counter == 9)
			{
				result.text = parseText(ass + i + 1);
			}
		}
	}

	return result;
}

bool Decoder::decodeSubtitle()
{
	int got_picture;
	AVSubtitle subtitle;
	int ret = avcodec_decode_subtitle2(data->pCodecCtxS, &subtitle, &got_picture, data->packet);

	if (ret < 0)
	{
		return false;
	}

	if (got_picture > 0)
	{
		int number = subtitle.num_rects;
		for (int i = 0; i < number; ++i)
		{
			Subtitle r = parseAss(subtitle.rects[i]->ass);
			if (r.text.length())
			{
				data->subtitleCallBack(data->subtitleArg,r);
			}
		}

		avsubtitle_free(&subtitle);
	}

	return true;
}

QString Decoder::openVideoDecodec(int index)
{
	data->videoindex = index;

	if (data->videoindex != -1)
	{
		data->pCodecCtx = data->pFormatCtx->streams[data->videoindex]->codec;
		data->pCodec = avcodec_find_decoder(data->pCodecCtx->codec_id);
		if (data->pCodec == nullptr)
		{
			data->videoindex = -1;
			return QString(u8"找不到对应的视频解码器");
		}

		if (avcodec_open2(data->pCodecCtx, data->pCodec, nullptr) < 0)
		{
			data->videoindex = -1;
			return QString(u8"无法打开对应的视频解码器");
		}

		data->pFrameRGB = av_frame_alloc();
		data->bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, data->pCodecCtx->width, data->pCodecCtx->height, 1);
		data->out_buffer = (unsigned char *)av_malloc(data->bufSize);

		av_image_fill_arrays(data->pFrameRGB->data, data->pFrameRGB->linesize, data->out_buffer,
			AV_PIX_FMT_RGB32, data->pCodecCtx->width, data->pCodecCtx->height, 1);

		data->img_convert_ctx = sws_getContext(data->pCodecCtx->width, data->pCodecCtx->height, data->pCodecCtx->pix_fmt,
			data->pCodecCtx->width, data->pCodecCtx->height, AV_PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}

	return QString();
}

QString Decoder::openAudioDecodec(int index)
{
	data->audioindex = index;

	if (data->audioindex != -1)
	{
		data->pCodecCtxA = data->pFormatCtx->streams[data->audioindex]->codec;
		data->pCodecA = avcodec_find_decoder(data->pCodecCtxA->codec_id);
		if (data->pCodecA == nullptr)
		{

			return QString(u8"找不到对应的音频解码器");
		}

		if (avcodec_open2(data->pCodecCtxA, data->pCodecA, nullptr) < 0)
		{
			data->audioindex = -1;
			return QString(u8"无法打开对应的音频解码器");
		}

		data->aFrame_ReSample = av_frame_alloc();
		data->bufSizeA = 192000 * 2;
		data->out_bufferA = (unsigned char *)av_malloc(data->bufSizeA);

		data->au_convert_ctx = swr_alloc();
		data->au_convert_ctx = swr_alloc_set_opts(data->au_convert_ctx, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
			av_get_default_channel_layout(data->pCodecCtxA->channels), data->pCodecCtxA->sample_fmt, data->pCodecCtxA->sample_rate, 0, NULL);
		swr_init(data->au_convert_ctx);
	}

	return QString();
}

QString Decoder::openSubTitleDecodec(int index)
{
	data->subtitleindex = index;

	if (data->subtitleindex != -1)
	{
		data->pCodecCtxS = data->pFormatCtx->streams[data->subtitleindex]->codec;
		data->pCodecS= avcodec_find_decoder(data->pCodecCtxS->codec_id);
		if (data->pCodecS == nullptr)
		{
			data->subtitleindex = -1;
			return QString(u8"找不到对应的字幕解码器");
		}

		if (avcodec_open2(data->pCodecCtxS, data->pCodecS, nullptr) < 0)
		{
			data->subtitleindex = -1;
			return QString(u8"无法打开对应的字幕解码器");
		}
	}

	return QString();
}

void Decoder::cleanVideo()
{
	av_free(data->out_buffer);
	data->out_buffer = nullptr;
	avcodec_close(data->pCodecCtx);
	sws_freeContext(data->img_convert_ctx);
	data->img_convert_ctx = nullptr;
	av_frame_free(&data->pFrameRGB);
}

void Decoder::cleanAudio()
{
	av_free(data->out_bufferA);
	data->out_bufferA = nullptr;
	avcodec_close(data->pCodecCtxA);
	swr_close(data->au_convert_ctx);
	swr_free(&data->au_convert_ctx);
	av_frame_free(&data->aFrame_ReSample);
	delete[] data->audioBuffer;
	data->audioBuffer = nullptr;
}

void Decoder::cleanSubTitle()
{
	avcodec_close(data->pCodecCtxS);
}
