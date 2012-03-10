//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _OOM_AUTOMATION_MENU_
#define _OOM_AUTOMATION_MENU_

#include <QMenu>
#include <QWidgetAction>

class QListView;
class QStandardItem;
class QStandardItemModel;
class Track;
class QModelIndex;
class CtrlListList;

class AutomationMenu : public QWidgetAction
{
	Q_OBJECT
	QListView *list;
	QStandardItemModel *m_listModel;
	Track* m_track;
	CtrlListList* m_controllers;

	public:
		AutomationMenu(QMenu* parent, Track* t);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void itemClicked(const QModelIndex&);
	
	signals:
		void triggered();
};

#endif
