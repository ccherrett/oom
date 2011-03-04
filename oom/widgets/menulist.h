//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _MENULIST_
#define _MENULIST_

#include <QMenu>
#include <QWidgetAction>

class QListWidget;
class QListWidgetItem;
class Track;

class MenuList : public QWidgetAction
{
	Q_OBJECT
	QListWidget *list;
	Track* _track;

	public:
		MenuList(QMenu* parent, Track* t);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void updateData(QListWidgetItem*);
	
	signals:
		void triggered();
};

#endif
