#include "ControlBar.h"

#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QFile>

struct ControlBar::Data
{
	QPushButton *left;
	QPushButton *right;
	QPushButton *pause;
	QPushButton *play;
	QPushButton *open;
	QPushButton *volumeBt;
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

	data->volumeBt = new QPushButton;
	controlLayout->addWidget(data->volumeBt);
	data->volumeBt->setObjectName("VolumeBt");

	data->volume = new QSlider(Qt::Horizontal);
	controlLayout->addWidget(data->volume);
	data->volume->setRange(0, 100);
	data->volume->setValue(50);
	data->volume->setFixedWidth(150);
	data->open = new QPushButton;
	controlLayout->addWidget(data->open);
	data->open->setObjectName("SearchBt");

	connect(data->open,&QPushButton::clicked,this,&ControlBar::onOpenFile);

	QSpacerItem *spItem1 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
	controlLayout->addSpacerItem(spItem1);

	QHBoxLayout *playLayout = new QHBoxLayout;
	controlLayout->addLayout(playLayout);
	data->left = new QPushButton;
	data->right = new QPushButton;
	data->pause = new QPushButton;
	data->play = new QPushButton;
	data->left->setObjectName("LeftBt");
	data->right->setObjectName("RightBt");
	data->pause->setObjectName("PauseBt");
	data->play->setObjectName("PlayBt");
	playLayout->addWidget(data->left);
	playLayout->addWidget(data->pause);
	playLayout->addWidget(data->play);
	playLayout->addWidget(data->right);
	data->play->hide();

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
	comboLayout->addSpacing(8);

	QFile file("data/ControlBar.css");

	if (file.open(QIODevice::ReadOnly))
	{
		setStyleSheet(file.readAll());
	}

}

ControlBar::~ControlBar()
{
	delete data;
}

void ControlBar::setDuration(int second)
{
	data->duration->setRange(0,second);
}

double ControlBar::getVolume()
{
	double v = data->volume->value();
	return v/100;
}

void ControlBar::onOpenFile()
{
	QString str = QFileDialog::getOpenFileName(this,u8"ÇëÑ¡ÔñÎÄ¼þ","");
	if (!str.isEmpty())
	{
		emit sigOpenFile(str);
	}
}

void ControlBar::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.fillRect(rect(), QColor(0, 0, 0, 100));
}

 