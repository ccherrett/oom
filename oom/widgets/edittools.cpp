#include <QtGui>
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "song.h"
#include "edittools.h"

EditTools::EditTools(QList<QAction*> actions, QWidget* parent)
: QFrame(parent)	
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
 	setObjectName("editToolButtons");
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	
	foreach(QAction* act, actions)
	{
		addButton(act);
	}
	//m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void EditTools::addButton(QAction* act)
{
	QToolButton* m_btnPointer = new QToolButton(this);
	m_btnPointer->setDefaultAction(act);
	m_btnPointer->setIconSize(QSize(29, 25));
	m_btnPointer->setFixedSize(QSize(29, 25));
	m_btnPointer->setAutoRaise(true);
	m_layout->addWidget(m_btnPointer);
}

void EditTools::songChanged(int)
{
}

