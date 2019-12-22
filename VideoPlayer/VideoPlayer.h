#pragma once

#include <QWidget>

class Audio;

class VideoPlayer : public QWidget
{
	Q_OBJECT
public:
	VideoPlayer(QWidget *parent = nullptr);
	~VideoPlayer();
	void OnVideo();
	void OnAudio();
	void OnFrameGetted();
	void OnAudioGetted(Audio audio);
protected:
	virtual void paintEvent(QPaintEvent *e)override;
	virtual void closeEvent(QCloseEvent *e)override;
protected:
	struct Data;
	Data *data = nullptr;
};
