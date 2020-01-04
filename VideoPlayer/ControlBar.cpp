#include "ControlBar.h"

#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>

struct ControlBar::Data
{
	QPushButton *left;
	QPushButton *right;
	QPushButton *pause;
	QPushButton *open;
	QComboBox *sounds;
	QComboBox *subtitles;
	QSlider *duration;
	QSlider *volume;
};

ControlBar::ControlBar(QWidget *parent) :QWidget(parent)
{
	data = new Data;
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(mainLayout);
	data->duration = new QSlider(Qt::Horizontal);
	mainLayout->addWidget(data->duration);

	QHBoxLayout *controlLayout = new QHBoxLayout;
	controlLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addLayout(controlLayout);

	data->volume = new QSlider(Qt::Horizontal);
	controlLayout->addWidget(data->volume);
	data->volume->setRange(0, 100);
	data->volume->setFixedWidth(150);
	data->open = new QPushButton;
	controlLayout->addWidget(data->open);
	data->open->setFixedSize(50, 50);

	QSpacerItem *spItem1 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
	controlLayout->addSpacerItem(spItem1);

	QHBoxLayout *playLayout = new QHBoxLayout;
	controlLayout->addLayout(playLayout);
	data->left = new QPushButton;
	data->right = new QPushButton;
	data->pause = new QPushButton;
	data->left->setFixedSize(50, 50);
	data->right->setFixedSize(50, 50);
	data->pause->setFixedSize(50, 50);
	playLayout->addWidget(data->left);
	playLayout->addWidget(data->right);
	playLayout->addWidget(data->pause);

	QSpacerItem *spItem2 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
	controlLayout->addSpacerItem(spItem2);


	QHBoxLayout *comboLayout = new QHBoxLayout;
	controlLayout->addLayout(comboLayout);
	data->sounds = new QComboBox;
	data->subtitles = new QComboBox;
	data->sounds->setFixedWidth(100);
	data->subtitles->setFixedWidth(100);
	comboLayout->addWidget(data->sounds);
	comboLayout->addWidget(data->subtitles);

}

ControlBar::~ControlBar()
{
	delete data;
}

void ControlBar::setDuration(int second)
{
	data->duration->setRange(0,second);
}

void ControlBar::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.fillRect(rect(), QColor(0, 0, 0, 100));
}

 