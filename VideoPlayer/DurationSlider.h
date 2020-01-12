#pragma once

#include <QWidget>

class DurationSlider:public QWidget
{
	Q_OBJECT
signals:
	void sigSeek(int);
public:
	DurationSlider();
	~DurationSlider();
	void setRange(int min,int max);
	void setValue(int value);
	int getMax();
	int getValue();
protected:
	virtual void paintEvent(QPaintEvent *e)override;
	virtual void mousePressEvent(QMouseEvent *e)override;
	virtual void mouseMoveEvent(QMouseEvent *e)override;
	virtual void mouseReleaseEvent(QMouseEvent *e)override;
protected:
	struct Data;
	Data *data;
};

