#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "looptools.h"

LoopToolbar::LoopToolbar(QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("loopToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
	
	m_btnLoop = new QToolButton(this);
	m_btnLoop->setDefaultAction(loopAction);
	m_btnLoop->setIconSize(QSize(29, 25));
	m_btnLoop->setFixedSize(QSize(29, 25));
	m_btnLoop->setAutoRaise(true);
	m_layout->addWidget(m_btnLoop);
	
	m_btnPunchin = new QToolButton(this);
	m_btnPunchin->setDefaultAction(punchinAction);
	m_btnPunchin->setIconSize(QSize(29, 25));
	m_btnPunchin->setFixedSize(QSize(29, 25));
	m_btnPunchin->setAutoRaise(true);
	m_layout->addWidget(m_btnPunchin);
	
	m_btnPunchout = new  QToolButton(this);
	m_btnPunchout->setDefaultAction(punchoutAction);
	m_btnPunchout->setIconSize(QSize(29, 25));
	m_btnPunchout->setFixedSize(QSize(29, 25));
	m_btnPunchout->setAutoRaise(true);
	m_layout->addWidget(m_btnPunchout);
	
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void LoopToolbar::songChanged(int)
{
}
