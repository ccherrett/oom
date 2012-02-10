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
#include "TrackManager.h"

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
	QList<qint64> *tidlist = m_trackview->trackIndexList();
	QMap<qint64, TrackView::TrackViewTrack*> *tlist = m_trackview->tracks();
	for(int i = 0; i < tidlist->size(); ++i)
	{
		qint64 tid = tidlist->at(i);
		TrackView::TrackViewTrack* tvt = tlist->value(tid);
		if(tvt)
		{
			if(tvt->is_virtual)
			{
				VirtualTrack* vt = trackManager->virtualTracks().value(tid);
				if(vt)
				{
					if((vt->type == Track::MIDI || vt->type == Track::WAVE))
					{
						QListWidgetItem* item = new QListWidgetItem(QString(tr("Virtual: ")).append(vt->name));
						item->setData(Qt::UserRole+3, vt->id);
						list->addItem(item);
					}
				}
			}
			else
			{
				Track *it = song->findTrackById(tid);
				if(it)
				{
					if((it->type() == Track::MIDI || it->type() == Track::WAVE) && (it->parts()->empty() || m_viewall))
					{
						QListWidgetItem* item = new QListWidgetItem(it->name());
						item->setData(Qt::UserRole+3, it->id());
						list->addItem(item);
					}
				}
			}
		}
	}
	if(!list->count())
	{
		QListWidgetItem* item = new QListWidgetItem(tr("<No Empty Tracks>"));
		item->setData(Qt::UserRole+3, -1);
		list->addItem(item);
		w->setFixedHeight(90);
	}
	connect(list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	return w;//list;
}

void TrackViewMenu::updateData(QListWidgetItem *item)
{
	if(list && item)
	{
		//printf("Triggering Menu Action\n");
		setData(item->data(Qt::UserRole+3));
		
		//FIXME: This is a seriously brutal HACK but its the only way I can get it to dismiss the menu
		QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e);

		QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e2);
	}
}
