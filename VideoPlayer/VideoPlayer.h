#pragma once

#include <QWidget>

class Audio;
class Subtitle;

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
	void OnSubtitleGetted(Subtitle audio);
	void OpenFile(const QString &file);
	void StopPlay();
	void switchAudio(int index);
	void switchSubtitle(int index);
	void setPause(bool p);
	bool isPlaying();
	void seek(int pos);
	void seeked(long long pts);
protected:
	QRect getControlRect();
	virtual bool eventFilter(QObject *obj, QEvent *e)override;
	virtual void keyPressEvent(QKeyEvent *e)override;
	virtual void mouseDoubleClickEvent(QMouseEvent *e)override;
	virtual void changeEvent(QEvent *e)override;
	virtual void resizeEvent(QResizeEvent *e)override;
	virtual void closeEvent(QCloseEvent *e)override;
protected:
	struct Data;
	Data *data = nullptr;
};
