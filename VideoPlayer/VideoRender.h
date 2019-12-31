#pragma once

#include <QWidget>

struct Subtitle;

class VideoRender : public QWidget
{
	Q_OBJECT
public:
	VideoRender(QWidget *parent = nullptr);
	~VideoRender();
	void setImage(QImage &img);
	void addSubtitle(Subtitle title, clock_t cur);
protected:
	virtual void paintEvent(QPaintEvent *e)override;
protected:
	struct Data;
	Data *data = nullptr;
};

