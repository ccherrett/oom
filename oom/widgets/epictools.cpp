#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "epictools.h"

EpicToolbar::EpicToolbar(QList<QAction*> actions, QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("epicToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	
	m_btnSelect = new QToolButton(this);
	m_btnSelect->setDefaultAction(multiPartSelectionAction);
	m_btnSelect->setIconSize(QSize(29, 25));
	m_btnSelect->setFixedSize(QSize(29, 25));
	m_btnSelect->setAutoRaise(true);
	m_layout->addWidget(m_btnSelect);
	
	if(!actions.isEmpty() && actions.size() == 2)
	{
		m_btnDraw = new QToolButton(this);
		m_btnDraw->setDefaultAction(actions.at(0));
		m_btnDraw->setIconSize(QSize(29, 25));
		m_btnDraw->setFixedSize(QSize(29, 25));
		m_btnDraw->setAutoRaise(true);
		m_layout->addWidget(m_btnDraw);
		
		m_btnRecord = new  QToolButton(this);
		m_btnRecord->setDefaultAction(actions.at(1));
		m_btnRecord->setIconSize(QSize(29, 25));
		m_btnRecord->setFixedSize(QSize(29, 25));
		m_btnRecord->setAutoRaise(true);
		m_layout->addWidget(m_btnRecord);
	}
	
	m_btnParts = new QToolButton(this);
	m_btnParts->setDefaultAction(noteAlphaAction);
	m_btnParts->setIconSize(QSize(29, 25));
	m_btnParts->setFixedSize(QSize(29, 25));
	m_btnParts->setAutoRaise(true);
	m_layout->addWidget(m_btnParts);
	
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void EpicToolbar::songChanged(int)
{
}
/*
void TransportToolbar::setSoloAction(QAction* act)
{
	m_btnSolo->setDefaultAction(act);
}

void TransportToolbar::setMuteAction(QAction* act)
{
	m_btnMute->setDefaultAction(act);
}
*/
