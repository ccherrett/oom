//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _TVIEWMENU_
#define _TVIEWMENU_

#include <QMenu>
#include <QWidgetAction>

class QListWidget;
class QListWidgetItem;
class TrackView;

class TrackViewMenu : public QWidgetAction
{
	Q_OBJECT
	QListWidget *list;
	TrackView* m_trackview;
	bool m_viewall;

	public:
		TrackViewMenu(QMenu* parent, TrackView* t, bool viewall = false);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void updateData(QListWidgetItem*);
	
	signals:
		void triggered();
};

#endif
