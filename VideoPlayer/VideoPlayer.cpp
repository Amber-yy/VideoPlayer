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
#include <QApplication>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QMouseEvent>

extern void ShiftVolume(char* buf, int size, double vol);

const int audioBufferSize = 1024 * 1024;
const int maxAudioSize = 30;
const int controlHide = 5000;

struct VideoPlayer::Data
{
	Decoder *decoder;
	DecodeThread *thread;
	QTimer *timer = nullptr;
	QTimer *timerA = nullptr;
	QQueue<Video> *imgs = nullptr;
	QAudioOutput *audio = nullptr;
	QIODevice *audioDevice = nullptr;
	VideoRender *render = nullptr;
	ControlBar *control = nullptr;
	Audio currentAudio;
	int currentAudioIt;
	bool shown = true;
	bool pause = false;
	bool controlHover = false;
	bool clearVideo = false;
	Video tempVideo;
	clock_t start;
	clock_t pauseTime;
	clock_t controlActive = 0;
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
	connect(data->decoder, &Decoder::seeked, this, &VideoPlayer::seeked);
	connect(data->control, &ControlBar::sigSwitchAudio, this, &VideoPlayer::switchAudio);
	connect(data->control, &ControlBar::sigSwitchSubtitle, this, &VideoPlayer::switchSubtitle);
	connect(data->control, &ControlBar::sigPauseState, this, &VideoPlayer::setPause);
	connect(data->control, &ControlBar::sigSeek, this, &VideoPlayer::seek);

	data->control->installEventFilter(this);
	data->control->setMouseTracking(true);
	data->render->installEventFilter(this);
	data->render->setMouseTracking(true);
}

VideoPlayer::~VideoPlayer()
{
	delete data;
}

void VideoPlayer::OnVideo()
{
	if (data->pause)
	{
		return;
	}

	if (data->imgs == nullptr || data->imgs->isEmpty())
	{
		data->imgs = data->decoder->getFrame();
	}

	clock_t cur = clock() - data->start;
	data->control->setProgress(cur / 1000);

	if (clock() - data->controlActive > controlHide && data->control->isVisible() && !data->controlHover)
	{
		data->control->hide();
	}

	if (data->subtitles.size() > 0)
	{
		if (data->subtitles.front().start <= cur)
		{
			data->subtitleMutex.lock();
			data->render->addSubtitle(data->subtitles.takeAt(0),cur);
			data->subtitleMutex.unlock();
		}
		if (data->clearVideo)
		{
			data->subtitleMutex.lock();
			data->subtitles.clear();
			data->subtitleMutex.unlock();
		}
	}

	if (data->imgs->size())
	{
		if (data->clearVideo)
		{
			data->tempVideo = data->imgs->takeFirst();
			data->render->cleanSubtitle();
			if (cur >= data->tempVideo.pts)
			{
				data->clearVideo = false;
			}
			else
			{
				return;
			}
		}
		else if (data->shown)
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
	else if(data->thread->isFinished())
	{
		StopPlay();
	}

}

void VideoPlayer::OnAudio()
{
	if (data->pause)
	{
		return;
	}

	bool isAudioOnly = (data->decoder->getVideo() != 0);
	if (isAudioOnly)
	{
		clock_t cur = clock() - data->start;
		data->control->setProgress(cur/1000);
	}

	if (data->currentAudio.buffer == nullptr && data->audios.size())
	{
		data->audioMutex.lock();
		data->currentAudio = data->audios.takeFirst();
		data->audioMutex.unlock();
		data->currentAudioIt = 0;
		if (isAudioOnly && data->audios.size() < maxAudioSize/2)
		{
			data->decoder->switchWorkState(true);
		}
	}

	if (!data->audios.size() && data->decoder->getVideo() != 0)
	{
		data->decoder->switchWorkState(true);
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
	else if (data->thread->isFinished())
	{
		StopPlay();
	}

}

void VideoPlayer::OnFrameGetted()
{
	if (data->decoder->getVideo() != -1)
	{
		data->timer->start(10);
	}

	data->timerA->start(5);
	data->start = clock();
	data->control->setAudios(data->decoder->getAudios());
	data->control->setSubtitles(data->decoder->getSubtitles());
	data->audioDevice = data->audio->start();
	data->currentAudio = { nullptr,0 };
	data->control->setDuration(data->decoder->getDuration());
	data->control->startPlay();

	QRect screenRect = QApplication::desktop()->screenGeometry();
	QSize imgSize = data->decoder->getVideoSize();

	if (imgSize.width() > screenRect.width() / 2 || imgSize.height() > screenRect.height() / 2)
	{
		int width = screenRect.width() / 2;
		int height = ((double)imgSize.height() / imgSize.width()) * width;
		imgSize = QSize(width,height);
	}

	resize(imgSize);

	data->pause = false;
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

	if (data->timer->isActive() || data->timerA->isActive())
	{
		data->timer->stop();
		data->timerA->stop();
		QThread::msleep(500);
	}

	data->audio->stop();
	data->render->stopPlay();
	data->control->show();
}

void VideoPlayer::switchAudio(int index)
{
	data->decoder->openAudioDecodec(data->decoder->getAudio()[index]);
}

void VideoPlayer::switchSubtitle(int index)
{
	data->decoder->openSubTitleDecodec(data->decoder->getSubtitle()[index]);
}

void VideoPlayer::setPause(bool p)
{
	data->pause = p;
	if (data->pause)
	{
		data->pauseTime = clock();
	}
	else
	{
		data->start += clock() - data->pauseTime;
	}
}

bool VideoPlayer::isPlaying()
{
	return data->timer->isActive()||data->timerA->isActive();
}

void VideoPlayer::seek(int pos)
{
	data->decoder->seek(pos);
	if (data->decoder->getVideo() != 0)
	{
		data->start = clock() - pos * 1000;
		data->audioMutex.lock();
		for (Audio a : data->audios)
		{
			delete[] a.buffer;
		}
		data->audios.clear();
		data->audioMutex.unlock();
		data->decoder->switchWorkState(true);
	}
}

void VideoPlayer::seeked(long long pts)
{
	data->clearVideo = true;

	if (data->decoder->getVideo() == 0)
	{
		data->start = clock() - pts;
	}
}

QRect VideoPlayer::getControlRect()
{
	return QRect(0, height() - 85, width(), 85);
}

bool VideoPlayer::eventFilter(QObject * obj, QEvent * e)
{
	QEvent::Type tp = e->type();

	if (obj == data->render && tp == QEvent::MouseMove)
	{
		QPoint pt = dynamic_cast<QMouseEvent *>(e)->pos();
		if (getControlRect().contains(pt))
		{
			data->controlActive = clock();
			data->controlHover = true;
			data->control->show();
		}
	}
	else if (obj == data->control)
	{
		if (tp == QEvent::Leave)
		{
			data->controlHover = false;
		}
		else if (tp == QEvent::MouseMove)
		{
			data->controlActive = clock();
			data->controlHover = true;
		}
	}

	return QWidget::eventFilter(obj, e);
}

void VideoPlayer::resizeEvent(QResizeEvent * e)
{
	data->render->setGeometry(rect());
	data->control->setGeometry(getControlRect());
	QSize s = data->render->getBestSize(e->size(),data->decoder->getVideoSize());
	data->decoder->setImageSize(s.width(),s.height());
}

void VideoPlayer::closeEvent(QCloseEvent * e)
{
	StopPlay();
	QWidget::closeEvent(e);
}
