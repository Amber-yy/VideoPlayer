#pragma once

#include <QWidget>

class DurationSlider:public QWidget
{
public:
	DurationSlider();
	~DurationSlider();
	void setRange(int min,int max);
	void setValue(int value);
protected:
	virtual void paintEvent(QPaintEvent *e)override;
	virtual void mousePressEvent(QMouseEvent *e)override;
	virtual void mouseMoveEvent(QMouseEvent *e)override;
	virtual void mouseReleaseEvent(QMouseEvent *e)override;
protected:
	struct Data;
	Data *data;
};

