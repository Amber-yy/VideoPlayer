#include "VideoRender.h"
#include "Decoder.h"

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

void VideoRender::paintEvent(QPaintEvent * e)
{
	QPixmap pixmap(size());
	pixmap.fill(Qt::black);
	QPainter painter(&pixmap);

	QRect re;
	int w = width();
	int h = height();
	int iw = data->img.width();
	int ih = data->img.height();

	double rateWin = (double)w / h;
	double rateImg = (double)iw / ih;

	//ͼ���߱ȴ��ڴ��ڿ�߱ȣ���ʱӦ�Կ��Ϊ׼
	if (rateImg > rateWin)
	{
		re = QRect(0, (h-w/rateImg)/2,w, w / rateImg);
	}
	else
	{
		re = QRect((w-h*rateImg)/2, 0, h*rateImg, h);
	}

	painter.drawImage(re, data->img);
	QString str;

	for (auto s : data->currentSubtitles)
	{
		str.append(s.text + "\n");
	}

	if (!str.isEmpty())
	{
		painter.setPen(Qt::white);
		QFont font(u8"΢���ź�");
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
		QFont font(u8"΢���ź�");
		font.setPixelSize(22);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, 100, 100), "loading");
		data->textIni = true;
	}

	QPainter t(this);
	t.drawPixmap(rect(),pixmap);
}
