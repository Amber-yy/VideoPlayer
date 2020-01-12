#include "DurationSlider.h"

#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>

struct DurationSlider::Data
{
	int minValue = 0;
	int maxValue = 0;
	int curentValue= 0;
	bool isPressed = false;
};

static QString timeToStr(int t)
{
	int h = t / 3600;
	int min = (t - h * 3600) / 60;
	int s = (t - h * 3600 - min * 60);

	char buffer[1024];
	sprintf(buffer,"%02d:%02d:%02d",h,min,s);

	return QString(buffer);
}

DurationSlider::DurationSlider()
{
	data = new Data;
	setFixedHeight(22);
}

DurationSlider::~DurationSlider()
{
	delete data;
}

void DurationSlider::setRange(int min, int max)
{
	data->minValue = min;
	data->maxValue = max;
}

void DurationSlider::setValue(int value)
{
	if (!data->isPressed && data->curentValue != value)
	{
		data->curentValue = value;
		if (isVisible())
		{
			update();
		}
	}
}

int DurationSlider::getMax()
{
	return data->maxValue;
}

int DurationSlider::getValue()
{
	return data->curentValue;
}

void DurationSlider::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.fillRect(rect(),Qt::transparent);
	int h = height();
	int w = width() - 10;
	int cw = 5;
	if (data->maxValue)
	{
		cw = ((double)data->curentValue / data->maxValue)*w+5;
	}

	painter.setPen(Qt::white);
	painter.setBrush(Qt::white);

	painter.fillRect(QRect(5, 4, w, 2), Qt::gray);
	painter.fillRect(QRect(5, 4, cw, 2), QColor("#55f"));
	painter.drawEllipse(QPoint(cw,5),5,5);

	painter.drawText(QRect(0,10,w,20), timeToStr(data->curentValue)+"/"+ timeToStr(data->maxValue),Qt::AlignHCenter|Qt::AlignRight);
}

void DurationSlider::mousePressEvent(QMouseEvent * e)
{
	if (!data->maxValue)
	{
		return;
	}
	
	data->isPressed = true;
	int w = e->x();
	int cw = ((double)data->curentValue / data->maxValue)*w + 5;

	if (w<cw - 3 || w>cw + 10)
	{
		data->curentValue = (double)w / width()*data->maxValue;
		update();
	}

}

void DurationSlider::mouseMoveEvent(QMouseEvent * e)
{
	if (!data->maxValue || !data->isPressed)
	{
		return;
	}

	int w = e->x();
	int cw = ((double)data->curentValue / data->maxValue)*w + 5;
	data->curentValue = (double)w / width()*data->maxValue;
	update();
}

void DurationSlider::mouseReleaseEvent(QMouseEvent * e)
{
	data->isPressed = false;
	emit sigSeek(data->curentValue);
}
