//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#include <QWidgetAction>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QtGui>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>

#include "track.h"
#include "instrumenttree.h"
#include "minstrument.h"
#include "globals.h"
#include "icons.h"
#include "song.h"
#include "node.h"
#include "audio.h"
#include "app.h"
#include "gconfig.h"
#include "config.h"
#include "instrumentmenu.h"

InstrumentMenu::InstrumentMenu(QMenu* parent, MidiTrack *track) : QWidgetAction(parent)
{
	m_track = track;
}

QWidget* InstrumentMenu::createWidget(QWidget* parent)
{
	if(!m_track)
		return 0;

	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	w->setFixedHeight(350);
	QString title(m_track->name());
	QLabel* plabel = new QLabel(title);
	plabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	plabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(plabel);
	QPushButton *btnClear = new QPushButton(tr("Clear Patch"));
	connect(btnClear, SIGNAL(clicked()), this, SLOT(clearPatch()));
	layout->addWidget(btnClear);

	QPushButton *btnClose = new QPushButton(tr("Dismiss"));
	connect(btnClose, SIGNAL(clicked()), this, SLOT(doClose()));

	m_tree = new InstrumentTree(w, m_track);
	m_tree->setObjectName("InstrumentMenuList");
	QString style = "InstrumentTree { background-color: #d8dbe8; border: 2px solid #29292a; border-radius: 0px; padding: 0px; color: #303033; font-size: 11x; alternate-background-color: #bec0cf; }";
	m_tree->setStyleSheet(style);
	m_tree->setAlternatingRowColors(true);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	connect(m_tree, SIGNAL(patchSelected(int, QString)), this, SIGNAL(patchSelected(int, QString)));
	connect(m_tree, SIGNAL(patchSelected(int, QString)), this, SLOT(updatePatch(int, QString)));
	layout->addWidget(m_tree);

	layout->addWidget(btnClose);

	w->setLayout(layout);
	return w;
}

void InstrumentMenu::updatePatch(int prog, QString name)
{
	m_program = prog;
	m_name = name;
	doClose();
}

void InstrumentMenu::doClose()
{
	//printf("InstrumentMenu::doClose() classed\n");
	setData(m_program);
	
	//FIXME: This is a seriously brutal HACK but its the only way it can get it done
	QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e);

	QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e2);
}

void InstrumentMenu::clearPatch()
{
	//printf("InstrumentMenu::clearPatch() called\n");
	m_program = -1;
	m_name = QString(tr("Select Patch"));
	emit patchSelected(m_program, m_name);
	doClose();
}
