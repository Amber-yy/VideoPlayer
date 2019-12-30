#include "VideoPlayer.h"
#include "Decoder.h"
#include "DecodeThread.h"
#include <QPainter>
#include <QTimer>
#include <QQueue>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QAudioOutput>

extern void ShiftVolume(char* buf, int size, double vol);

const int audioBufferSize = 1024 * 1024 * 6;
const int maxAudioSize = 30;

struct VideoPlayer::Data
{
	Decoder *decoder;
	DecodeThread *thread;
	QTimer *timer;
	QTimer *timerA;
	QQueue<Video> *imgs = nullptr;
	QAudioOutput *audio = nullptr;
	QIODevice *audioDevice = nullptr;
	Audio currentAudio;
	int currentAudioIt;
	double volume = 0.5;
	bool shown = true;
	Video tempVideo;
	clock_t start;
	QQueue<Audio> audios;
	QQueue<Subtitle> subtitles;
	QList<Subtitle> currentSubtitles;
	QMutex audioMutex;
	QMutex subtitleMutex;
	QImage img;
};

static void AudioCallBack(void *arg,Audio audio)
{
	static_cast<VideoPlayer *>(arg)->OnAudioGetted(audio);
}

static void SubTitleCallBack(void *arg,Subtitle sub)
{
	static_cast<VideoPlayer *>(arg)->OnSubtitleGetted(sub);
}

VideoPlayer::VideoPlayer(QWidget *parent):QWidget(parent)
{
	data = new Data;
	data->decoder = new Decoder(this);
	data->thread = new DecodeThread(this);

	QAudioFormat audioFormat;
	audioFormat.setSampleRate(44100);
	audioFormat.setChannelCount(2);
	audioFormat.setSampleSize(16);
	audioFormat.setCodec("audio/pcm");
	audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	audioFormat.setSampleType(QAudioFormat::UnSignedInt);

	data->audio = new QAudioOutput(audioFormat,this);
	data->audio->setBufferSize(audioBufferSize);

	QString str = data->decoder->setFile("f:/r.mkv");
	data->thread->setDecoder(data->decoder);
	data->thread->start();
	data->decoder->setAudioCallBack(AudioCallBack,this);
	data->decoder->setSubtitleCallBack(SubTitleCallBack, this);

	data->timer = new QTimer(this);
	data->timerA = new QTimer(this);
	connect(data->timer, &QTimer::timeout, this, &VideoPlayer::OnVideo);
	connect(data->timerA, &QTimer::timeout, this, &VideoPlayer::OnAudio);
	connect(data->decoder, &Decoder::frameGetted,this,&VideoPlayer::OnFrameGetted);
}

VideoPlayer::~VideoPlayer()
{
	data->decoder->stop();
	data->thread->wait();

	for (Audio a : data->audios)
	{
		delete[] a.buffer;
	}

	delete data;
}

void VideoPlayer::OnVideo()
{
	if (data->imgs == nullptr || data->imgs->isEmpty())
	{
		data->imgs = data->decoder->getFrame();
	}

	clock_t cur = clock() - data->start;

	if (data->imgs->size())
	{
		if (data->shown)
		{
			data->tempVideo = data->imgs->takeFirst();
			data->img = data->tempVideo.img;
		}

		int delta = data->tempVideo.pts - cur;

		if (delta < 5)
		{
			if (delta - 2 > 0)
			{
				QThread::msleep(delta - 2);
			}
			data->shown = true;
			update();
		}
		else
		{
			data->shown = false;
		}
	}

	if (data->subtitles.size() > 0)
	{
		if (data->subtitles.front().start <= cur)
		{
			data->subtitleMutex.lock();
			data->currentSubtitles.push_back(data->subtitles.takeAt(0));
			data->subtitleMutex.unlock();
		}
	}

	for (auto it = data->currentSubtitles.begin(); it != data->currentSubtitles.end(); ++it)
	{
		if (cur >= it->end)
		{
			auto temp = it++;
			data->currentSubtitles.erase(temp);
			if (it == data->currentSubtitles.end())
			{
				break;
			}
		}
	}

}

void VideoPlayer::OnAudio()
{
	if (data->currentAudio.buffer == nullptr && data->audios.size())
	{
		data->audioMutex.lock();
		data->currentAudio = data->audios.takeFirst();
		data->audioMutex.unlock();
		data->currentAudioIt = 0;
		if (data->decoder->getVideo() != 0 && data->audios.size() < maxAudioSize/2)
		{
			data->decoder->switchWorkState(true);
		}
	}

	if (data->audio->bytesFree() && data->currentAudio.buffer)
	{
		int size = std::min(data->audio->bytesFree(), data->currentAudio.size - data->currentAudioIt);
		ShiftVolume(data->currentAudio.buffer+ data->currentAudioIt, size, data->volume);
		data->audioDevice->write(data->currentAudio.buffer + data->currentAudioIt, size);
		data->currentAudioIt += size;
		if (data->currentAudioIt >= data->currentAudio.size)
		{
			delete[] data->currentAudio.buffer;
			data->currentAudio.buffer = nullptr;
		}
	}
}

void VideoPlayer::OnFrameGetted()
{
	data->timer->start(10);
	data->timerA->start(5);
	data->start = clock();
	data->audioDevice = data->audio->start();
	data->currentAudio = { nullptr,0 };
	resize(data->decoder->getVideoSize());
}

void VideoPlayer::OnAudioGetted(Audio audio)
{
	data->audioMutex.lock();
	data->audios.push_back(audio);
	data->audioMutex.unlock();
	
	if (data->decoder->getVideo()!=0&&data->audios.size() >= maxAudioSize)
	{
		data->decoder->switchWorkState(false);
	}
}

void VideoPlayer::OnSubtitleGetted(Subtitle sub)
{
	data->subtitleMutex.lock();
	data->subtitles.push_back(sub);
	data->subtitleMutex.unlock();
}

void VideoPlayer::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.drawImage(rect(), data->img);
	QString str;

	for (auto s : data->currentSubtitles)
	{
		str.append(s.text + "\n");
	}

	if (!str.isEmpty())
	{
		painter.setPen(Qt::white);
		QFont font(u8"Î¢ÈíÑÅºÚ");
		font.setPixelSize(16);
		painter.setFont(font);
		painter.drawText(QRect(0,900,1920,100),Qt::AlignCenter,str);
	}

}

void VideoPlayer::closeEvent(QCloseEvent * e)
{
	data->timer->stop();
	data->timerA->stop();
	QThread::msleep(100);
	QWidget::closeEvent(e);
}
