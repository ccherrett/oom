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

TrackViewMenu::TrackViewMenu(QMenu* parent, TrackView *t) : QWidgetAction(parent)
{
	m_trackview = t;
}

QWidget* TrackViewMenu::createWidget(QWidget* parent)
{
	if(!m_trackview)
		return 0;

	//QVBoxLayout* layout = new QVBoxLayout();
	//QWidget* w = new QWidget(parent);
	//w->setLayout(layout);
	//QLabel* tvname = new QLabel(m_trackview->viewName(), w);
	list = new QListWidget(parent);
	list->setObjectName("TrackViewMenuList");
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setAlternatingRowColors(true);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	list->setFixedHeight(300);
	int r = 0;
	for (iTrack it = m_trackview->tracks()->begin(); it != m_trackview->tracks()->end(); ++it)
	{
		if (((*it)->type() == Track::MIDI || (*it)->type() == Track::DRUM || (*it)->type() == Track::WAVE) && (*it)->parts()->empty())
		{
			list->insertItem(r, (*it)->name());
			++r;
		}
	}
	if(!r)
	{
		list->insertItem(r, tr("<No Empty Tracks>"));
		list->setFixedHeight(60);
	}
	connect(list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	return list;
}

void TrackViewMenu::updateData(QListWidgetItem *item)
{
	if(list && item)
	{
		//printf("Triggering Menu Action\n");
		setData(item->text());
		
		//FIXME: This is a seriously brutal HACK but its the only way it can get it done
		QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e);

		QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e2);
	}
}
