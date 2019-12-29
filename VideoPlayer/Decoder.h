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
	long long start;
	long long end;
};

class Decoder:public QObject
{
	Q_OBJECT
signals:
	void frameGetted();
public:
	Decoder(QObject *parent = nullptr);
	~Decoder();
	QString setFile(const QString &file);
	QQueue<Video>* getFrame();
	QVector<int> getAudio();
	QSize getVideoSize();
	int getVideo();
	void setAudioCallBack(void (*callBack)(void *,Audio),void *arg);
	void setSubtitleCallBack(void(*callBack)(void *, Subtitle), void *arg);
	void switchWorkState(bool work);
	void decode();
	void stop();
protected:
	bool decodeVideo();
	bool decodeAudio();
	bool decodeSubtitle();
	QString openVideoDecodec(int index);
	QString openAudioDecodec(int index);
	QString openSubTitleDecodec(int index);
	void cleanVideo();
	void cleanAudio();
	void cleanSubTitle();
protected:
	struct Data;
	Data *data = nullptr;
};

