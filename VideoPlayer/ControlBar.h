#pragma once

#include <QWidget>

class ControlBar:public QWidget
{
	Q_OBJECT
signals:
	void sigOpenFile(QString &);
	void sigSwitchAudio(int);
	void sigSwitchSubtitle(int);
	void sigPauseState(bool);
	void sigSeek(int);
public:
	ControlBar(QWidget *parent);
	~ControlBar();
	void setDuration(int second);
	double getVolume();
	void setAudios(QStringList audios);
	void setSubtitles(QStringList subtitles);
	void startPlay();
	void setProgress(int second);
protected:
	void onBack();
	void onForward();
	void onPauseState();
	void onAudioChange(int index);
	void onSubtitleChange(int index);
	void onOpenFile();
	virtual void paintEvent(QPaintEvent *e) override;
protected:
	struct Data;
	Data *data = nullptr;
};

