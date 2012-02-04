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
#include "midiport.h"
#include "globals.h"
#include "icons.h"
#include "song.h"
#include "node.h"
#include "audio.h"
#include "app.h"
#include "gconfig.h"
#include "config.h"
#include "keymapmenu.h"

KeyMapMenu::KeyMapMenu(QMenu* parent, MidiTrack *track, KeyMap* map, Patch* p) : QWidgetAction(parent)
{
	m_keymap = map;
	m_track = track;
	m_patch = p;
	m_datachanged = false;
}

QWidget* KeyMapMenu::createWidget(QWidget* parent)
{
	if(!m_keymap)
		return 0;

	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	w->setFixedHeight(500);
	QString title(tr("Default Patch - Note: "));
	QLabel* plabel = new QLabel(title.append(song->key2note(m_keymap->key)));
	plabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	plabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(plabel);

	QHBoxLayout *hbox = new QHBoxLayout();
	m_kpatch = new QLineEdit();
	m_kpatch->setReadOnly(true);
	m_kpatch->setText(m_keymap->pname);
	//printf("Patch name in menu: %s, program: %d\n", m_keymap->pname.toUtf8().constData(), m_keymap->program);
	hbox->addWidget(m_kpatch);
	
	QPushButton *btnClear = new QPushButton();
	//btnClear->setFixedHeight(20);
	btnClear->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	btnClear->setToolTip(tr("Clear Patch"));
	btnClear->setIcon(*garbagePCIcon);
	btnClear->setIconSize(QSize(20, 20));
	btnClear->setFixedSize(QSize(24, 24));
	hbox->addWidget(btnClear);
	
	connect(btnClear, SIGNAL(clicked()), SLOT(clearPatch()));
	
	layout->addLayout(hbox);
	
	int outPort = ((MidiTrack*)m_track)->outPort();
	MidiInstrument* instr = midiPorts[outPort].instrument();
	
	m_tree = new InstrumentTree(w, instr);
	//m_tree->setObjectName("KeyMapMenuList");
	QString style = "InstrumentTree { background-color: #1e1e1e; selection-background-color: #2e2e2e; gridline-color:#343434; border: 2px solid #211f23; border-radius: 6px; padding: 0px; color: #bbbfbb; font-size: 11x; alternate-background-color: #1b1b1b; }";
	m_tree->setStyleSheet(style);
	m_tree->setAlternatingRowColors(true);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	connect(m_tree, SIGNAL(patchSelected(int, QString)), SLOT(updatePatch(int, QString)));
	layout->addWidget(m_tree);

	QLabel* clabel = new QLabel(tr("Key Comments"));
	clabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	clabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(clabel);

	m_comment = new QLineEdit();
	m_comment->setText(m_keymap->comment);
	//m_comment->setFixedHeight(150);
	layout->addWidget(m_comment);
	connect(m_comment, SIGNAL(textChanged(QString)), SLOT(updateComment()));
	connect(m_comment, SIGNAL(returnPressed()), SLOT(updateComment()));
	
	QLabel* pclabel = new QLabel(tr("Patch Key Comments"));
	pclabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	pclabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(pclabel);

	m_patchcomment = new QLineEdit();
	m_patchcomment->setReadOnly(false);
	if(m_patch)
	{
		//printf("Patch supplied, Name: %s\n", m_patch->name.toUtf8().constData());
		m_patchcomment->setText(m_patch->comments.value(m_keymap->key));
	}
	connect(m_patchcomment, SIGNAL(textChanged(QString)), SLOT(updatePatchComment()));
	connect(m_patchcomment, SIGNAL(returnPressed()), SLOT(updatePatchComment()));
	//printf("Patch name in menu: %s, program: %d\n", m_keymap->pname.toUtf8().constData(), m_keymap->program);
	layout->addWidget(m_patchcomment);

	QPushButton *btnClose = new QPushButton(tr("Save Settings"));
	layout->addWidget(btnClose);
	connect(btnClose, SIGNAL(clicked()), SLOT(doClose()));

	w->setLayout(layout);
	return w;
}

void KeyMapMenu::clearPatch()
{
	m_keymap->program = -1;
	m_keymap->hasProgram = false;
	m_keymap->pname = tr("Select Patch");
	m_kpatch->setText(m_keymap->pname);
}

void KeyMapMenu::updatePatch(int prog, QString name)
{
	m_keymap->program = prog;
	m_keymap->hasProgram = true;
	m_keymap->pname = name;
	m_kpatch->setText(name);
	m_datachanged = true;
}

void KeyMapMenu::doClose()
{
	setData(m_datachanged);
	
	//FIXME: This is a seriously brutal HACK but its the only way it can get it done
	QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e);

	QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e2);
}

void KeyMapMenu::updateComment()
{
	QString comment = m_comment->text();
	if(comment != m_keymap->comment)
	{
		m_datachanged = true;
		m_keymap->comment = comment;
		//song->dirty = true;
	}
}

void KeyMapMenu::updatePatchComment()
{
	if(m_patch)
	{
		m_datachanged = true;
		m_patch->comments[m_keymap->key] = m_patchcomment->text();
	}
}
