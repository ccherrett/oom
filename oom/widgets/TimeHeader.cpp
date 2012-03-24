#include <stdio.h>
#include <values.h>

#include <QtGui>

#include "globals.h"
#include "TimeHeader.h"
#include "song.h"
#include "app.h"
#include "gconfig.h"
#include "poslabel.h"

extern int mtcType;

//
// the bigtime widget
// display is split into several parts to avoid flickering.
//

//---------------------------------------------------------
//   TimeHeader
//---------------------------------------------------------

TimeHeader::TimeHeader(QWidget* parent)
: QFrame(parent)
{
	setObjectName("timeHeader");
	tickmode = true;
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(0,0,0,0);
	m_layout->setSpacing(0);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	setFixedHeight(98);
	
	QHBoxLayout* timeBox = new QHBoxLayout;
	timeBox->setContentsMargins(0,0,0,0);
	timeBox->setSpacing(0);
	QHBoxLayout* infoBox = new QHBoxLayout;
	infoBox->setContentsMargins(0,0,0,0);
	infoBox->setSpacing(0);

	cursorPos = new PosLabel(this);
	cursorPos->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	cursorPos->setObjectName("thTimeLabel");
	
	timeLabel = new QLabel(this);
	timeLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	timeLabel->setObjectName("thLengthLabel");

	timeBox->addWidget(cursorPos);
	timeBox->addWidget(timeLabel);

	posLabel = new QLabel(this);
	posLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	posLabel->setObjectName("thBigTimeLabel");
	posLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	/*frameLabel = new QLabel(this);
	frameLabel->setAlignment(Qt::AlignLeft|Qt::AlignBottom);
	frameLabel->setObjectName("thSubFrameLabel");
	frameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);*/

	infoBox->addWidget(posLabel, 1);
	/*infoBox->addWidget(frameLabel);*/
	
	m_layout->addLayout(infoBox, 1);
	m_layout->addLayout(timeBox);
	
	setString(MAXINT);
}

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool TimeHeader::setString(unsigned v)
{
	if (v == MAXINT)
	{
		timeLabel->setText(QString("----.--.---"));
		posLabel->setText(QString("---:--"));
		//frameLabel->setText(QString("--:--"));
		return true;
	}

	unsigned absFrame = tempomap.tick2frame(v);
	int bar, beat;
	unsigned tick;
	AL::sigmap.tickValues(v, &bar, &beat, &tick);
	double time = double(absFrame) / double(sampleRate);
	int min = int(time) / 60;
	int sec = int(time) % 60;
	/*int mil = (int)(time % 1000);
	int sec = (int)((time/1000) % 60);
	int min = (int)((time/60000) % 60);
	int hours = (int)((time/3600000) % 24);*/
	double rest = time - (min * 60 + sec);
	switch (mtcType)
	{
		case 0: // 24 frames sec
			rest *= 24;
			break;
		case 1: // 25
			rest *= 25;
			break;
		case 2: // 30 drop frame
			rest *= 30;
			break;
		case 3: // 30 non drop frame
			rest *= 30;
			break;
	}
	int frame = int(rest);
	int subframe = int((rest - frame)*100);

	QString s;

	s.sprintf("%04d.%02d.%03d", bar + 1, beat + 1, tick);
	timeLabel->setText(s);

	s.sprintf("%d:%02d <font color='#565a56' size='12px'>%02d</font>", min, sec, frame);
	posLabel->setText(s);

	//s.sprintf("%02d:%02u", frame, subframe);
	//frameLabel->setText(s);


	//Q_UNUSED(frame);
	Q_UNUSED(subframe);
	return false;
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void TimeHeader::setPos(int idx, unsigned v, bool)
{
	if (idx == 0)
		setString(v);
}

void TimeHeader::setTime(unsigned tick)
{
	cursorPos->setValue(tick);
}
