
#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "icons.h"
#include "shortcuts.h"
#include "transporttools.h"

TransportToolbar::TransportToolbar(QWidget* parent, bool showPanic, bool showMuteSolo)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("transportToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
	
	m_btnRewindEnd = new QToolButton(this);
	m_btnRewindEnd->setDefaultAction(startAction);
	m_btnRewindEnd->setIconSize(QSize(29, 25));
	m_btnRewindEnd->setFixedSize(QSize(29, 25));
	m_btnRewindEnd->setAutoRaise(true);
	m_layout->addWidget(m_btnRewindEnd);
	
	m_btnAudition = new QToolButton(this);
	m_btnAudition->setDefaultAction(replayAction);
	m_btnAudition->setIconSize(QSize(29, 25));
	m_btnAudition->setFixedSize(QSize(29, 25));
	m_btnAudition->setAutoRaise(true);
	m_layout->addWidget(m_btnAudition);
	
	m_btnRewind = new  QToolButton(this);
	m_btnRewind->setDefaultAction(rewindAction);
	m_btnRewind->setIconSize(QSize(29, 25));
	m_btnRewind->setFixedSize(QSize(29, 25));
	m_btnRewind->setAutoRaise(true);
	m_layout->addWidget(m_btnRewind);
	
	m_btnFFwd = new QToolButton(this);
	m_btnFFwd->setDefaultAction(forwardAction);
	m_btnFFwd->setIconSize(QSize(29, 25));
	m_btnFFwd->setFixedSize(QSize(29, 25));
	m_btnFFwd->setAutoRaise(true);
	m_layout->addWidget(m_btnFFwd);
	
	m_btnStop = new QToolButton(this);
	m_btnStop->setDefaultAction(stopAction);
	m_btnStop->setIconSize(QSize(29, 25));
	m_btnStop->setFixedSize(QSize(29, 25));
	m_btnStop->setAutoRaise(true);
	m_layout->addWidget(m_btnStop);
	
	m_btnPlay = new QToolButton(this);
	m_btnPlay->setDefaultAction(playAction);
	m_btnPlay->setIconSize(QSize(29, 25));
	m_btnPlay->setFixedSize(QSize(29, 25));
	m_btnPlay->setAutoRaise(true);
	m_layout->addWidget(m_btnPlay);
	
	m_btnClick = new QToolButton(this);
	m_btnClick->setIcon(QIcon(*metronomeIconSet3));
	m_btnClick->setIconSize(QSize(29, 25));
	m_btnClick->setFixedSize(QSize(29, 25));
	m_btnClick->setAutoRaise(true);
	m_btnClick->setCheckable(true);
	m_btnClick->setShortcut(shortcuts[SHRT_TOGGLE_METRO].key);
	m_btnClick->setToolTip(tr("Turn on/off metronome click"));
	m_btnClick->setChecked(song->click());
	m_layout->addWidget(m_btnClick);
	connect(m_btnClick, SIGNAL(toggled(bool)), song, SLOT(setClick(bool)));
	connect(song, SIGNAL(clickChanged(bool)), this, SLOT(updateClick(bool)));
	
	m_btnRecord = new QToolButton(this);
	m_btnRecord->setDefaultAction(recordAction);
	m_btnRecord->setIconSize(QSize(29, 25));
	m_btnRecord->setFixedSize(QSize(29, 25));
	m_btnRecord->setAutoRaise(true);
	m_layout->addWidget(m_btnRecord);
	
	if(showMuteSolo)
	{
		m_btnMute = new QToolButton(this);
		//m_btnMute->setDefaultAction();
		m_btnMute->setIconSize(QSize(29, 25));
		m_btnMute->setFixedSize(QSize(29, 25));
		m_btnMute->setAutoRaise(true);
		m_layout->addWidget(m_btnMute);
		
		m_btnSolo = new QToolButton(this);
		//m_btnSolo->setDefaultAction();
		m_btnSolo->setIconSize(QSize(29, 25));
		m_btnSolo->setFixedSize(QSize(29, 25));
		m_btnSolo->setAutoRaise(true);
		m_layout->addWidget(m_btnSolo);
		//NOTE: These are to isolate the Performer transport tools from global scope.
		connect(m_btnRecord, SIGNAL(clicked(bool)), SIGNAL(recordTriggered(bool)));
		connect(m_btnPlay, SIGNAL(clicked(bool)), SIGNAL(recordTriggered(bool)));
	}	
	
	if(showPanic)
	{
		m_btnPanic = new QToolButton(this);
		m_btnPanic->setDefaultAction(panicAction);
		m_btnPanic->setIconSize(QSize(29, 25));
		m_btnPanic->setFixedSize(QSize(29, 25));
		m_btnPanic->setAutoRaise(true);
		m_layout->addWidget(m_btnPanic);
	}
}

void TransportToolbar::songChanged(int)
{
}

void TransportToolbar::updateClick(bool state)
{
	m_btnClick->blockSignals(true);
	m_btnClick->setChecked(state);
	m_btnClick->blockSignals(false);
}

void TransportToolbar::setSoloAction(QAction* act)
{
	m_btnSolo->setDefaultAction(act);
}

void TransportToolbar::setMuteAction(QAction* act)
{
	m_btnMute->setDefaultAction(act);
}
