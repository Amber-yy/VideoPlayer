#include "ControlBar.h"
#include "VideoPlayer.h"
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QFileDialog>
#include <QListView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QFile>

struct ControlBar::Data
{
	VideoPlayer *player;
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
	int audioIndex = -1;
	int subtitleIndex = -1;
};

ControlBar::ControlBar(QWidget *parent) :QWidget(parent)
{
	data = new Data;
	data->player = dynamic_cast<VideoPlayer *>(parent);
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
	data->sounds->setFixedWidth(120);
	data->sounds->setView(new QListView);
	data->subtitles->setFixedWidth(120);
	data->subtitles->setView(new QListView);
	comboLayout->addWidget(data->sounds);
	comboLayout->addWidget(data->subtitles);
	comboLayout->addSpacing(8);

	void (QComboBox::*ptr)(int) = &QComboBox::activated;

	connect(data->sounds, ptr,this,&ControlBar::onAudioChange);
	connect(data->subtitles, ptr, this, &ControlBar::onSubtitleChange);
	connect(data->pause,&QPushButton::clicked,this,&ControlBar::onPauseState);
	connect(data->play, &QPushButton::clicked, this, &ControlBar::onPauseState);

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

void ControlBar::setAudios(QStringList audios)
{
	data->sounds->clear();
	data->audioIndex = 0;
	data->sounds->addItems(audios);
}

void ControlBar::setSubtitles(QStringList subtitles)
{
	data->subtitles->clear();
	data->subtitleIndex = 0;
	data->subtitles->addItems(subtitles);
}

void ControlBar::startPlay()
{
	data->pause->setVisible(true);
	data->play->setVisible(false);
}

void ControlBar::onPauseState()
{
	if (data->player->isPlaying())
	{
		bool pause = data->pause->isVisible();

		data->pause->setVisible(!pause);
		data->play->setVisible(pause);

		emit sigPauseState(pause);
	}
}

void ControlBar::onAudioChange(int index)
{
	if (index != data->audioIndex)
	{
		data->audioIndex = index;
		emit sigSwitchAudio(index);
	}
}

void ControlBar::onSubtitleChange(int index)
{
	if (index != data->subtitleIndex)
	{
		data->subtitleIndex = index;
		emit sigSwitchSubtitle(index);
	}
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

 