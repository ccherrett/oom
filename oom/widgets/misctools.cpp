#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "misctools.h"

MiscToolbar::MiscToolbar(QList<QAction*> actions, QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("miscToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
	
	if(!actions.isEmpty() && actions.size() == 2)
	{
		m_btnStep = new QToolButton(this);/*{{{*/
		m_btnStep->setDefaultAction(actions.at(0));
		m_btnStep->setIconSize(QSize(29, 25));
		m_btnStep->setFixedSize(QSize(29, 25));
		m_btnStep->setAutoRaise(true);
		m_layout->addWidget(m_btnStep);
		
		m_btnSpeaker = new QToolButton(this);
		m_btnSpeaker->setDefaultAction(actions.at(1));
		m_btnSpeaker->setIconSize(QSize(29, 25));
		m_btnSpeaker->setFixedSize(QSize(29, 25));
		m_btnSpeaker->setAutoRaise(true);
		m_layout->addWidget(m_btnSpeaker);/*}}}*/
	}
	
	m_btnPanic = new  QToolButton(this);
	m_btnPanic->setDefaultAction(panicAction);
	m_btnPanic->setIconSize(QSize(29, 25));
	m_btnPanic->setFixedSize(QSize(29, 25));
	m_btnPanic->setAutoRaise(true);
	m_layout->addWidget(m_btnPanic);
	
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void MiscToolbar::songChanged(int)
{
}
