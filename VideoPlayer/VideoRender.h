#pragma once

#include <QWidget>

class VideoRender : public QWidget
{
	Q_OBJECT
public:
	VideoRender(QWidget *parent = nullptr);
	~VideoRender();
};

