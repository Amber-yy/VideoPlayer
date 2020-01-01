#include "VideoRender.h"
#include "Decoder.h"

#include <QDebug>
#include <QPainter>

struct VideoRender::Data
{
	QList<Subtitle> currentSubtitles;
	QImage img;
	bool textIni = false;
};

VideoRender::VideoRender(QWidget *parent) :QWidget(parent)
{
	data = new Data;
}

VideoRender::~VideoRender()
{
	delete data;
}

void VideoRender::setImage(QImage & img)
{
	data->img = img;
}

void VideoRender::addSubtitle(Subtitle title, clock_t cur)
{
	data->currentSubtitles.push_back(title);

	for (auto it = data->currentSubtitles.begin(); it != data->currentSubtitles.end(); ++it)
	{
		if (cur >= it->end)
		{
			auto temp = it++;
			data->currentSubtitles.erase(temp);
			if (it == data->currentSubtitles.end())
			{
				break;
			}
		}
	}
}

QSize VideoRender::getBestSize(QSize win,QSize img)
{
	int w = win.width();
	int h = win.height();
	int iw = img.width();
	int ih = img.height();

	double rateWin = (double)w / h;
	double rateImg = (double)iw / ih;

	if (rateImg > rateWin)
	{
		return QSize(w, w / rateImg);
	}

	return QSize(h*rateImg, h);
}

void VideoRender::paintEvent(QPaintEvent * e)
{
	clock_t start = clock();
	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);

	QRect re;
	int w = width();
	int h = height();
	int iw = data->img.width();
	int ih = data->img.height();

	double rateWin = (double)w / h;
	double rateImg = (double)iw / ih;

	//图像宽高比大于窗口宽高比，此时应以宽度为准
	if (rateImg > rateWin)
	{
		re = QRect(0, (h-w/rateImg)/2,w, w / rateImg);
	}
	else
	{
		re = QRect((w-h*rateImg)/2, 0, h*rateImg, h);
	}

	painter.drawImage(re, data->img.scaled(size()));
	QString str;

	for (auto s : data->currentSubtitles)
	{
		str.append(s.text + "\n");
	}

	if (!str.isEmpty())
	{
		painter.setPen(Qt::white);
		QFont font(u8"微软雅黑");
		font.setPixelSize(22);
		painter.setFont(font);
		QRect re = rect();
		re.setTop(re.height() - 80);
		re.setHeight(80);
		painter.drawText(re, Qt::AlignCenter, str);
	}

	if (!data->textIni)
	{
		painter.setPen(Qt::white);
		QFont font(u8"微软雅黑");
		font.setPixelSize(22);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, 100, 100), "loading");
		data->textIni = true;
	}

	clock_t end = clock();
	qDebug() << end - start;
}
