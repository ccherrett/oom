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

#include <QtGui>

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlEdit::setTool(int t)
{
	canvas->setTool(t);
}

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::CtrlEdit(QWidget* parent, AbstractMidiEditor* e, int xmag, bool expand, const char* name)
: QWidget(parent)
{
	setObjectName(name);
	setAttribute(Qt::WA_DeleteOnClose);

	m_collapsed = false;

	QHBoxLayout* hbox = new QHBoxLayout;
	panel = new CtrlPanel(0, e, "panel");
	canvas = new CtrlCanvas(e, 0, xmag, "ctrlcanvas", panel);
	vscale = new VScale;

	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(0);

	canvas->setOrigin(-(config.division / 4), 0);

	canvas->setMinimumHeight(50);

	panel->setFixedWidth(CTRL_PANEL_FIXED_WIDTH);
	hbox->addWidget(panel, expand ? 100 : 0, Qt::AlignRight);
	hbox->addWidget(canvas, 100);
	hbox->addWidget(vscale, 0);
	setLayout(hbox);

	connect(panel, SIGNAL(destroyPanel()), SLOT(destroyCalled()));
	connect(panel, SIGNAL(controllerChanged(int)), canvas, SLOT(setController(int)));
	connect(panel, SIGNAL(collapsed(bool)), SLOT(collapsedCalled(bool)));
	connect(panel, SIGNAL(collapsed(bool)), canvas, SLOT(toggleCollapsed(bool)));
	connect(canvas, SIGNAL(xposChanged(unsigned)), SIGNAL(timeChanged(unsigned)));
	connect(canvas, SIGNAL(yposChanged(int)), SIGNAL(yposChanged(int)));
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
		setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		setMaximumHeight(20);
		canvas->setMinimumHeight(20);
	}
	else
	{
		setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
		setMaximumHeight(400);
		canvas->setMinimumHeight(50);
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

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void CtrlEdit::writeStatus(int level, Xml& xml)
{
	if (canvas->controller())
	{
		xml.tag(level++, "ctrledit");
		xml.strTag(level, "ctrl", canvas->controller()->name());
		xml.tag(level, "/ctrledit");
	}
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void CtrlEdit::readStatus(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "ctrl")
				{
					QString name = xml.parse1();
					int portno = canvas->track()->outPort();
					MidiPort* port = &midiPorts[portno];
					MidiInstrument* instr = port->instrument();
					MidiControllerList* mcl = instr->controller();

					for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
					{
						if (ci->second->name() == name)
						{
							canvas->setController(ci->second->num());
							break;
						}
					}
				}
				else
					xml.unknown("CtrlEdit");
				break;
			case Xml::TagEnd:
				if (tag == "ctrledit")
					return;
			default:
				break;
		}
	}
}

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
