//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#include <QWidgetAction>
#include <QListWidget>
#include <QListWidgetItem>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QtGui>

#include "track.h"
#include "trackview.h"
#include "globals.h"
#include "icons.h"
#include "song.h"
#include "node.h"
#include "audio.h"
#include "app.h"
#include "gconfig.h"
#include "config.h"
#include "tviewmenu.h"

TrackViewMenu::TrackViewMenu(QMenu* parent, TrackView *t, bool v) : QWidgetAction(parent)
{
	m_trackview = t;
	m_viewall = v;
}

QWidget* TrackViewMenu::createWidget(QWidget* parent)
{
	if(!m_trackview)
		return 0;

	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	w->setFixedHeight(350);
	QLabel* tvname = new QLabel(m_trackview->viewName());
	tvname->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tvname->setObjectName("TrackViewMenuLabel");
	layout->addWidget(tvname);
	list = new QListWidget();
	list->setObjectName("TrackViewMenuList");
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setAlternatingRowColors(true);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(list);
	w->setLayout(layout);
	//list->setFixedHeight(300);
	int r = 0;
	for (; r < m_trackview->tracks()->size(); ++r)
	{
		Track *it = song->findTrackById(m_trackview->tracks()->at(r));
		if(it)
		{
			if((it->type() == Track::MIDI || it->type() == Track::DRUM || it->type() == Track::WAVE) && (it->parts()->empty() || m_viewall))
			{
				list->insertItem(r, it->name());
			}
		}
	}
	if(!r)
	{
		list->insertItem(r, tr("<No Empty Tracks>"));
		//list->setFixedHeight(60);
		w->setFixedHeight(90);
	}
	connect(list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	return w;//list;
}

void TrackViewMenu::updateData(QListWidgetItem *item)
{
	if(list && item)
	{
		//printf("Triggering Menu Action\n");
		setData(item->text());
		
		//FIXME: This is a seriously brutal HACK but its the only way I can get it to dismiss the menu
		QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e);

		QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e2);
	}
}
