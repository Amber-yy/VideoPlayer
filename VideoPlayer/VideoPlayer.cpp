#include "VideoPlayer.h"
#include "VideoRender.h"
#include "ControlBar.h"
#include "Decoder.h"
#include "DecodeThread.h"
#include <QPainter>
#include <QTimer>
#include <QQueue>
#include <QDebug>
#include <QMutex>
#include <QMessageBox>
#include <QFile>
#include <QAudioOutput>
#include <QResizeEvent>

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
	VideoRender *render = nullptr;
	ControlBar *control = nullptr;
	Audio currentAudio;
	int currentAudioIt;
	bool shown = true;
	Video tempVideo;
	clock_t start;
	QQueue<Audio> audios;
	QQueue<Subtitle> subtitles;
	QList<Subtitle> currentSubtitles;
	QMutex audioMutex;
	QMutex subtitleMutex;
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
	resize(640, 480);
	data = new Data;
	data->decoder = new Decoder(this);
	data->thread = new DecodeThread(this);
	data->render = new VideoRender(this);
	data->control = new ControlBar(this);
	data->render->stackUnder(data->control);

	QAudioFormat audioFormat;
	audioFormat.setSampleRate(44100);
	audioFormat.setChannelCount(2);
	audioFormat.setSampleSize(16);
	audioFormat.setCodec("audio/pcm");
	audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	audioFormat.setSampleType(QAudioFormat::UnSignedInt);

	data->audio = new QAudioOutput(audioFormat,this);
	data->audio->setBufferSize(audioBufferSize);

	data->timer = new QTimer(this);
	data->timerA = new QTimer(this);
	connect(data->control,&ControlBar::sigOpenFile,this,&VideoPlayer::OpenFile);
	connect(data->timer, &QTimer::timeout, this, &VideoPlayer::OnVideo);
	connect(data->timerA, &QTimer::timeout, this, &VideoPlayer::OnAudio);
	connect(data->decoder, &Decoder::frameGetted,this,&VideoPlayer::OnFrameGetted);
}

VideoPlayer::~VideoPlayer()
{
	delete data;
}

int t = 0;

void VideoPlayer::OnVideo()
{
	if (data->imgs == nullptr || data->imgs->isEmpty())
	{
		data->imgs = data->decoder->getFrame();
	}

	clock_t cur = clock() - data->start;

	if (data->subtitles.size() > 0)
	{
		if (data->subtitles.front().start <= cur)
		{
			data->subtitleMutex.lock();
			data->render->addSubtitle(data->subtitles.takeAt(0),cur);
			data->subtitleMutex.unlock();
		}
	}

	if (data->imgs->size())
	{
		if (data->shown)
		{
			data->tempVideo = data->imgs->takeFirst();
			data->render->setImage(data->tempVideo.img);
		}

		int delta = data->tempVideo.pts - cur;

		if (delta < 5)
		{
			if (delta - 2 > 0)
			{
				QThread::msleep(delta - 2);
			}
			data->shown = true;
			data->render->update();
		}
		else
		{
			data->shown = false;
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
		ShiftVolume(data->currentAudio.buffer+ data->currentAudioIt, size, data->control->getVolume());
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

void VideoPlayer::OpenFile(const QString & file)
{
	StopPlay();
	QString str = data->decoder->setFile(file);
	if (str.isEmpty())
	{
		data->thread->setDecoder(data->decoder);
		data->decoder->setAudioCallBack(AudioCallBack, this);
		data->decoder->setSubtitleCallBack(SubTitleCallBack, this);
		data->thread->start();
	}
	else
	{
		QMessageBox::warning(this,u8"´íÎó",str);
	}
}

void VideoPlayer::StopPlay()
{
	data->decoder->stop();
	data->thread->wait();

	for (Audio a : data->audios)
	{
		delete[] a.buffer;
	}

	data->audios.clear();
	data->shown = true;

	if (data->timer->isActive())
	{
		data->timer->stop();
		data->timerA->stop();
		QThread::msleep(200);
	}

	data->audio->stop();
	data->render->stopPlay();
}

void VideoPlayer::resizeEvent(QResizeEvent * e)
{
	data->render->setGeometry(rect());
	data->control->setGeometry(0,height()-85,width(),85);
	QSize s = data->render->getBestSize(e->size(),data->decoder->getVideoSize());
	data->decoder->setImageSize(s.width(),s.height());
}

void VideoPlayer::closeEvent(QCloseEvent * e)
{
	StopPlay();
	QWidget::closeEvent(e);
}
