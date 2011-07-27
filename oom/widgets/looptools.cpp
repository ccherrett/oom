#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "icons.h"
#include "looptools.h"

LoopToolbar::LoopToolbar(Qt::Orientation orient, QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("loopToolButtons");
	m_orient = orient;
	bool horizontal = (m_orient == Qt::Horizontal);

	if(horizontal)
		m_layout = new QHBoxLayout(this);
	else
		m_layout = new QVBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
	
	m_btnLoop = new QToolButton(this);
	m_btnLoop->setAutoRaise(true);
	if(horizontal)
	{
		m_btnLoop->setDefaultAction(loopAction);
		m_btnLoop->setIconSize(QSize(29, 25));
		m_btnLoop->setFixedSize(QSize(29, 25));
	}
	else
	{
		m_btnLoop->setCheckable(loopAction->isCheckable());
		m_btnLoop->setChecked(loopAction->isChecked());
		m_btnLoop->setToolTip(loopAction->toolTip());
		m_btnLoop->setIcon(QIcon(*loopVertIconSet3));
		m_btnLoop->setIconSize(QSize(25, 29));
		m_btnLoop->setFixedSize(QSize(25, 29));
		connect(m_btnLoop, SIGNAL(clicked(bool)), loopAction, SLOT(setChecked(bool)));
		connect(loopAction, SIGNAL(triggered(bool)), this, SLOT(setLoopSilent(bool)));
	}
	m_layout->addWidget(m_btnLoop);
	
	m_btnPunchin = new QToolButton(this);
	m_btnPunchin->setAutoRaise(true);
	if(horizontal)
	{
		m_btnPunchin->setDefaultAction(punchinAction);
		m_btnPunchin->setIconSize(QSize(29, 25));
		m_btnPunchin->setFixedSize(QSize(29, 25));
	}
	else
	{
		m_btnPunchin->setCheckable(punchinAction->isCheckable());
		m_btnPunchin->setChecked(loopAction->isChecked());
		m_btnPunchin->setToolTip(punchinAction->toolTip());
		m_btnPunchin->setIcon(QIcon(*punchinVertIconSet3));
		m_btnPunchin->setIconSize(QSize(25, 29));
		m_btnPunchin->setFixedSize(QSize(25, 29));
		connect(m_btnPunchin, SIGNAL(clicked(bool)), punchinAction, SLOT(setChecked(bool)));
		connect(punchinAction, SIGNAL(triggered(bool)), this, SLOT(setPunchinSilent(bool)));
	}
	m_layout->addWidget(m_btnPunchin);
	
	m_btnPunchout = new  QToolButton(this);
	m_btnPunchout->setAutoRaise(true);
	if(horizontal)
	{
		m_btnPunchout->setDefaultAction(punchoutAction);
		m_btnPunchout->setIconSize(QSize(29, 25));
		m_btnPunchout->setFixedSize(QSize(29, 25));
	}
	else
	{
		m_btnPunchout->setCheckable(punchoutAction->isCheckable());
		m_btnPunchout->setChecked(punchoutAction->isChecked());
		m_btnPunchout->setToolTip(punchoutAction->toolTip());
		m_btnPunchout->setIcon(QIcon(*punchoutVertIconSet3));
		m_btnPunchout->setIconSize(QSize(25, 29));
		m_btnPunchout->setFixedSize(QSize(25, 29));
		connect(m_btnPunchout, SIGNAL(clicked(bool)), punchoutAction, SLOT(setChecked(bool)));
		connect(punchoutAction, SIGNAL(triggered(bool)), this, SLOT(setPunchoutSilent(bool)));
	}
	m_layout->addWidget(m_btnPunchout);
	
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void LoopToolbar::songChanged(int)
{
}

void LoopToolbar::setLoopSilent(bool checked)
{
	if(m_btnLoop->isChecked() != checked)
	{
		m_btnLoop->blockSignals(true);
		m_btnLoop->click();
		m_btnLoop->blockSignals(false);
	}
}

void LoopToolbar::setPunchinSilent(bool checked)
{
	if(m_btnPunchin->isChecked() != checked)
	{
		m_btnPunchin->blockSignals(true);
		m_btnPunchin->click();
		m_btnPunchin->blockSignals(false);
	}
}

void LoopToolbar::setPunchoutSilent(bool checked)
{
	if(m_btnPunchout->isChecked() != checked)
	{
		m_btnPunchout->blockSignals(true);
		m_btnPunchout->click();
		m_btnPunchout->blockSignals(false);
	}
}

