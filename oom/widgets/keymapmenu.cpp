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
#include "keymapmenu.h"

KeyMapMenu::KeyMapMenu(QMenu* parent, MidiTrack *track, KeyMap* map) : QWidgetAction(parent)
{
	m_keymap = map;
	m_track = track;
}

QWidget* KeyMapMenu::createWidget(QWidget* parent)
{
	if(!m_keymap)
		return 0;

	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	w->setFixedHeight(350);
	QString title(tr("Default Patch - Note: "));
	QLabel* plabel = new QLabel(title.append(song->key2note(m_keymap->key)));
	plabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	plabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(plabel);

	QHBoxLayout *hbox = new QHBoxLayout();
	m_patch = new QLineEdit();
	m_patch->setReadOnly(true);
	m_patch->setText(m_keymap->pname);
	//printf("Patch name in menu: %s, program: %d\n", m_keymap->pname.toUtf8().constData(), m_keymap->program);
	hbox->addWidget(m_patch);
	
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
	
	m_tree = new InstrumentTree(w, m_track);
	//m_tree->setObjectName("KeyMapMenuList");
	QString style = "InstrumentTree { background-color: #d8dbe8; border: 2px solid #29292a; border-radius: 0px; padding: 0px; color: #303033; font-size: 11x; alternate-background-color: #bec0cf; }";
	m_tree->setStyleSheet(style);
	m_tree->setAlternatingRowColors(true);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	connect(m_tree, SIGNAL(patchSelected(int, QString)), SLOT(updatePatch(int, QString)));
	layout->addWidget(m_tree);

	QLabel* clabel = new QLabel(tr("Key Comments"));
	clabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	clabel->setObjectName("KeyMapMenuLabel");
	layout->addWidget(clabel);

	m_comment = new QTextEdit();
	m_comment->setText(m_keymap->comment);
	layout->addWidget(m_comment);
	connect(m_comment, SIGNAL(textChanged()), SLOT(updateComment()));
	
	QPushButton *btnClose = new QPushButton(tr("Dismiss"));
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
	m_patch->setText(m_keymap->pname);
}

void KeyMapMenu::updatePatch(int prog, QString name)
{
	m_keymap->program = prog;
	m_keymap->hasProgram = true;
	m_keymap->pname = name;
	m_patch->setText(name);
}

void KeyMapMenu::doClose()
{
	setData(m_keymap->program);
	
	//FIXME: This is a seriously brutal HACK but its the only way it can get it done
	QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e);

	QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent(this->parent(), e2);
}

void KeyMapMenu::updateComment()
{
	QString comment = m_comment->toPlainText();
	if(comment != m_keymap->comment)
	{
		m_keymap->comment = comment;
		//song->dirty = true;
	}
}
