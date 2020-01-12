#pragma once

#include <QObject>
#include <QImage>

struct Video
{
	QImage img;
	long long pts;
};

struct Audio
{
	char *buffer;
	int size;
};

struct Subtitle
{
	QString text;
	int start;
	int end;
};

struct AVPacket;

#define MAX_WIDTH 3840
#define MAX_HEIGHT 2160

class Decoder:public QObject
{
	Q_OBJECT
signals:
	void frameGetted();
	void seeked(long long pts);
public:
	Decoder(QObject *parent = nullptr);
	~Decoder();
	QString setFile(const QString &file);
	QQueue<Video>* getFrame();
	QVector<int> getAudio();
	QVector<int> getSubtitle();
	QSize getVideoSize();
	long long getDuration();
	int getVideo();
	void setImageSize(int w, int h);
	void setAudioCallBack(void (*callBack)(void *,Audio),void *arg);
	void setSubtitleCallBack(void(*callBack)(void *, Subtitle), void *arg);
	void switchWorkState(bool work);
	void decode();
	void stop();
	void seek(int pos);
	QStringList getAudios();
	QStringList getSubtitles();
	QString openAudioDecodec(int index);
	QString openSubTitleDecodec(int index);
protected:
	bool decodeVideo(AVPacket &packet);
	bool decodeAudio(AVPacket &packet);
	bool decodeSubtitle(AVPacket &packet);
	QString openVideoDecodec(int index);
	void cleanVideo();
	void cleanAudio();
	void cleanSubTitle();
protected:
	struct Data;
	Data *data = nullptr;
};

