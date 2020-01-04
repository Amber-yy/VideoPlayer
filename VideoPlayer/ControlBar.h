#pragma once

#include <QWidget>

class ControlBar:public QWidget
{
public:
	ControlBar(QWidget *parent);
	~ControlBar();
	void setDuration(int second);
protected:
	virtual void paintEvent(QPaintEvent *e) override;
protected:
	struct Data;
	Data *data = nullptr;

};

