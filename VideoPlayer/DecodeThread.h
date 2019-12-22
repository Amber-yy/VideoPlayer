#pragma once

#include <QThread>

class Decoder;

class DecodeThread :public QThread
{
public:
	DecodeThread(QObject *parent = nullptr);
	~DecodeThread();
	void setDecoder(Decoder *d);
protected:
	virtual void run()override;
protected:
	Decoder *decoder = nullptr;
};

