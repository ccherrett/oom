//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: tools.cpp,v 1.2 2004/04/28 21:56:13 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "tools.h"

#include <QActionGroup>
#include <QSpacerItem>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QIconSet>
#include <QIcon>

#include "icons.h"
#include "globals.h"
#include "action.h"

const char* infoPointer = QT_TRANSLATE_NOOP("@default", "select Pointer Tool:\n"
		"with the pointer tool you can:\n"
		"  select parts\n"
		"  move parts\n"
		"  copy parts");
const char* infoPencil = QT_TRANSLATE_NOOP("@default", "select Pencil Tool:\n"
		"with the pencil tool you can:\n"
		"  create new parts\n"
		"  modify length of parts");
const char* infoDel = QT_TRANSLATE_NOOP("@default", "select Delete Tool:\n"
		"with the delete tool you can delete parts");
const char* infoCut = QT_TRANSLATE_NOOP("@default", "select Cut Tool:\n"
		"with the cut tool you can split a part");
const char* infoGlue = QT_TRANSLATE_NOOP("@default", "select Glue Tool:\n"
		"with the glue tool you can glue two parts");
const char* infoScore = QT_TRANSLATE_NOOP("@default", "select Score Tool:\n");
const char* infoQuant = QT_TRANSLATE_NOOP("@default", "select Quantize Tool:\n"
		"insert display quantize event");
const char* infoDraw = QT_TRANSLATE_NOOP("@default", "select Drawing Tool");
const char* infoMute = QT_TRANSLATE_NOOP("@default", "select Muting Tool:\n"
		"click on part to mute/unmute");
const char* infoAutomation = QT_TRANSLATE_NOOP("@default", "Manipulate automation");
const char* infoMaster = QT_TRANSLATE_NOOP("@default", "Enable Tempo Editor to affect song");
const char* infoStretch = QT_TRANSLATE_NOOP("@default", "select Time Stretch Tool:\n"
		"insert time stretch");

ToolB toolList[] = {
	{&pointerIconSet3, QT_TRANSLATE_NOOP("@default", "Pointer"), infoPointer},
	{&pencilIconSet3, QT_TRANSLATE_NOOP("@default", "Pencil"), infoPencil},
	{&deleteIconSet3, QT_TRANSLATE_NOOP("@default", "Eraser"), infoDel},
	{&cutIconSet3, QT_TRANSLATE_NOOP("@default", "Cutter"), infoCut},
	{&note1IconSet3, QT_TRANSLATE_NOOP("@default", "Score"), infoScore},
	{&glueIconSet3, QT_TRANSLATE_NOOP("@default", "Glue"), infoGlue},
	{&quantIconSet3, QT_TRANSLATE_NOOP("@default", "Quantize"), infoQuant},
	{&drawIconSet3, QT_TRANSLATE_NOOP("@default", "Draw"), infoDraw},
	{&stretchIconSet3, QT_TRANSLATE_NOOP("@default", "Insert Time Stretch"), infoStretch},
	{&muteIconSet3, QT_TRANSLATE_NOOP("@default", "Mute parts"), infoMute},
	{&drawIconSet3, QT_TRANSLATE_NOOP("@default", "Edit automation"),    infoAutomation},
	{&drawIconSet3, QT_TRANSLATE_NOOP("@default", "Enable Tempo Editor"),    infoMaster},
};

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

EditToolBar::EditToolBar(QWidget* parent, int tools, bool addSpacer, const char*)
: QFrame(parent)
{
	m_layout = new QHBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	action = new QActionGroup(parent);
	action->setExclusive(true);

	nactions = 0;
	for (unsigned i = 0; i < sizeof (toolList) / sizeof (*toolList); ++i)
	{
		if ((tools & (1 << i)) == 0)
			continue;
		++nactions;
	}
	actions = new Action*[nactions];
	bool first = true;
	int n = 0;
	if(addSpacer)
		m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));
	bool addmaster = false;
	for (unsigned i = 0; i < sizeof (toolList) / sizeof (*toolList); ++i)
	{
		if ((tools & (1 << i)) == 0)
			continue;
		if((tools & (1 << i)) == MasterTool)
		{
			addmaster = true;
			continue;
		}
		ToolB* t = &toolList[i];

		Action* a = new Action(action, 1 << i, t->tip, true);
		actions[n] = a;
		a->setIcon(QIcon(**(t->icon)));
		//a->setIcon(QIcon(**(t->icon)));
		a->setToolTip(tr(t->tip));
		a->setWhatsThis(tr(t->ltip));
		if (first)
		{
			a->setChecked(true);
			first = false;
		}
		QToolButton* button = new QToolButton(this);
		button->setDefaultAction(a);
		button->setIconSize(QSize(29, 25));
		button->setFixedSize(QSize(29, 25));
		button->setAutoRaise(true);
		m_layout->addWidget(button);
		++n;
	}
	action->setVisible(true);
	if(addmaster)
	{
		QToolButton* button = new QToolButton(this);
		button->setDefaultAction(masterEnableAction);
		button->setIconSize(QSize(29, 25));
		button->setFixedSize(QSize(29, 25));
		button->setAutoRaise(true);
		m_layout->addWidget(button);
	}
	if(addSpacer)
		m_layout->addItem(new QSpacerItem(4, 2, QSizePolicy::Expanding, QSizePolicy::Minimum));

	connect(action, SIGNAL(selected(QAction*)), this, SLOT(toolChanged(QAction*)));
}

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(QAction* action)
{
	emit toolChanged(((Action*) action)->id());
}

QList<QAction*> EditToolBar::getActions()
{
	return action->actions();
}

//---------------------------------------------------------
//   ~EditToolBar
//---------------------------------------------------------

EditToolBar::~EditToolBar()
{
	delete actions;
}

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void EditToolBar::set(int id)/*{{{*/
{
	for (int i = 0; i < nactions; ++i)
	{
		Action* action = actions[i];
		if (action->id() == id)
		{
			action->setChecked(true);
			toolChanged(action);
			return;
		}
	}
}/*}}}*/

void EditToolBar::setNoUpdate(int id)/*{{{*/
{
	for (int i = 0; i < nactions; ++i)
	{
		Action* action = actions[i];
		if (action->id() == id)
		{
			action->setChecked(true);
			return;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   curTool
//---------------------------------------------------------

int EditToolBar::curTool()/*{{{*/
{
	for (int i = 0; i < nactions; ++i)
	{
		Action* action = actions[i];
		if (action->isChecked())
			return action->id();
	}
	return -1;
}/*}}}*/

