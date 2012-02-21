//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrledit.cpp,v 1.4.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "ctrledit.h"
#include "ctrlcanvas.h"
#include "AbstractMidiEditor.h"
#include "xml.h"
#include "vscale.h"
#include "ctrlpanel.h"
#include "globals.h"
#include "midiport.h"
#include "instruments/minstrument.h"
#include "gconfig.h"
#include "ResizeHandle.h"

#include <QtGui>

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::CtrlEdit(QWidget* parent, AbstractMidiEditor* e, int xmag, int height)
: QFrame(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0,0,0,0);
	mainLayout->setSpacing(0);
	ResizeHandle* handle = new ResizeHandle(this);
	mainLayout->addWidget(handle);

	connect(handle, SIGNAL(handleMoved(int)), this, SLOT(updateHeight(int)));
	connect(this, SIGNAL(minHeightChanged(int)), handle, SLOT(setMinHeight(int)));
	connect(this, SIGNAL(maxHeightChanged(int)), handle, SLOT(setMaxHeight(int)));
	connect(handle, SIGNAL(resizeEnded()), this, SIGNAL(resizeEnded()));
	connect(handle, SIGNAL(resizeStarted()), this, SIGNAL(resizeStarted()));

	//Set a high number because we allways set the height when we instantiate in the Performer
	//This will be updated as soon as you resize, so it should present no issues
	m_collapsedHeight = height;
	m_collapsed = false;
	m_maxheight = height;
	m_minheight = 80;

	QHBoxLayout* hbox = new QHBoxLayout;
	panel = new CtrlPanel(this, e, "panel");
	canvas = new CtrlCanvas(e, this, xmag, "ctrlcanvas", panel);

	vscale = new VScale;

	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(0);

	canvas->setOrigin(-(config.division / 4), 0);

	canvas->setMinimumHeight(20);
	canvas->setMinimumWidth(400);
	//canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	panel->setFixedWidth(CTRL_PANEL_FIXED_WIDTH);
	hbox->addWidget(panel, 0, Qt::AlignRight);
	hbox->addWidget(canvas, 1);
	hbox->addWidget(vscale, 0);
	mainLayout->addLayout(hbox);
	//setLayout(hbox);

	connect(panel, SIGNAL(destroyPanel()), SLOT(destroyCalled()));
	connect(panel, SIGNAL(controllerChanged(int)), canvas, SLOT(setController(int)));
	connect(panel, SIGNAL(collapsed(bool)), SLOT(collapsedCalled(bool)));
	connect(panel, SIGNAL(collapsed(bool)), handle, SLOT(setCollapsed(bool)));
	connect(panel, SIGNAL(collapsed(bool)), canvas, SLOT(toggleCollapsed(bool)));
	connect(canvas, SIGNAL(xposChanged(unsigned)), SIGNAL(timeChanged(unsigned)));
	connect(canvas, SIGNAL(yposChanged(int)), SIGNAL(yposChanged(int)));
	setFixedHeight(m_collapsedHeight);

	handle->setStartHeight(m_collapsedHeight);
}

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlEdit::setTool(int t)
{
	canvas->setTool(t);
}

void CtrlEdit::updateHeight(int h)
{
	//qDebug("CtrlEdit::updateHeight: %d", h);
	setFixedHeight(h);
	canvas->setFixedHeight(h);
	canvas->update();
	m_collapsedHeight = h;
}

void CtrlEdit::setMinHeight(int h)
{
	m_minheight = h;
//	qDebug("CtrlEdit::setMinHeight(%d)", h);
	setFixedHeight(h);
	canvas->setFixedHeight(h);
	canvas->update();
	
	//Notify the handle
	emit minHeightChanged(h);
}

void CtrlEdit::setMaxHeight(int h)
{
	//Just notify the handle
	if(!m_collapsed)
		m_maxheight = m_minheight+h;
	emit maxHeightChanged(h);
}

void CtrlEdit::setCollapsed(bool c)
{
	//Pass it through to the panel that has the button
	panel->toggleCollapsed(c);
}

void CtrlEdit::collapsedCalled(bool val)
{
	if(val)
	{
		if(!m_collapsed)
			m_collapsedHeight = height();
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		setFixedHeight(20);
		canvas->setFixedHeight(20);
		canvas->update();
	}
	else
	{
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
		if(m_collapsedHeight > m_maxheight)
		{
		//	qDebug("CtrlEdit::collapsedCalled: old val: %d setting: %d, maxheioght: %d", m_collapsedHeight, m_minheight, m_maxheight);
			setFixedHeight(m_maxheight);
			canvas->setFixedHeight(m_maxheight);
			canvas->update();
			//Update collapsedHeight?
			m_collapsedHeight =  m_maxheight;
		}
		else
		{
		//	qDebug("CtrlEdit::collapsedCalled using m_collapsedHeight~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~%d", m_collapsedHeight);
			setFixedHeight(m_collapsedHeight);
			canvas->setFixedHeight(m_collapsedHeight);
			canvas->update();
		}
	}
	m_collapsed = val;
}

bool CtrlEdit::setType(QString n)
{
	return panel->ctrlSetTypeByName(n);
}

QString CtrlEdit::type()
{
	return canvas->controller()->name();
}

/*QSize CtrlEdit::sizeHint()
{
	return QSize(400, 1);
}*/

//---------------------------------------------------------
//   destroy
//---------------------------------------------------------

void CtrlEdit::destroyCalled()
{
	emit destroyedCtrl(this);
}

//---------------------------------------------------------
//   setCanvasWidth
//---------------------------------------------------------

void CtrlEdit::setCanvasWidth(int w)
{
	canvas->setFixedWidth(w);
}

