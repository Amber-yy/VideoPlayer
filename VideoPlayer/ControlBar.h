#pragma once

#include <QWidget>

class ControlBar:public QWidget
{
	Q_OBJECT
signals:
	void sigOpenFile(QString &);
public:
	ControlBar(QWidget *parent);
	~ControlBar();
	void setDuration(int second);
	double getVolume();
protected:
	void onOpenFile();
	virtual void paintEvent(QPaintEvent *e) override;
protected:
	struct Data;
	Data *data = nullptr;

};

