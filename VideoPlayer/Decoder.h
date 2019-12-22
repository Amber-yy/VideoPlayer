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
	void switchWorkState(bool work);
	void decode();
	void stop();
protected:
	bool decodeVideo();
	bool decodeAudio();
protected:
	struct Data;
	Data *data = nullptr;
};

