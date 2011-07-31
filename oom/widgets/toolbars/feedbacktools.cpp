//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "icons.h"
#include "app.h"
#include "song.h"
#include "shortcuts.h"
#include "feedbacktools.h"

FeedbackTools::FeedbackTools(QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("feedbackToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	
	QAction* closeAction = oom->mixerDock()->toggleViewAction();
	closeAction->setShortcut(shortcuts[SHRT_OPEN_MIXER2].key);
	closeAction->setIcon(QIcon(*mixerIconSet3));

	m_btnShowMixer = new QToolButton(this);
	m_btnShowMixer->setDefaultAction(closeAction);
	m_btnShowMixer->setIconSize(QSize(29, 25));
	m_btnShowMixer->setFixedSize(QSize(29, 25));
	m_btnShowMixer->setAutoRaise(true);
	
	m_btnLoadProgramChange = new QToolButton(this);
	m_btnLoadProgramChange->setDefaultAction(pcloaderAction);
	m_btnLoadProgramChange->setIconSize(QSize(29, 25));
	m_btnLoadProgramChange->setFixedSize(QSize(29, 25));
	m_btnLoadProgramChange->setAutoRaise(true);
	
	m_btnMidiFeedback = new  QToolButton(this);
	m_btnMidiFeedback->setDefaultAction(feedbackAction);
	m_btnMidiFeedback->setIconSize(QSize(29, 25));
	m_btnMidiFeedback->setFixedSize(QSize(29, 25));
	m_btnMidiFeedback->setAutoRaise(true);

	m_layout->addWidget(m_btnMidiFeedback);
	m_layout->addWidget(m_btnLoadProgramChange);
	m_layout->addWidget(m_btnShowMixer);
	
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void FeedbackTools::songChanged(int)
{
}
