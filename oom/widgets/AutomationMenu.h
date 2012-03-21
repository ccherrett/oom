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
	QListView *m_inputList;
	QStandardItemModel *m_inputListModel;
	QStandardItemModel *m_listModel;
	Track* m_track;
	Track* m_synthTrack;
	Track* m_inputTrack;
	CtrlListList* m_inputControllers;
	CtrlListList* m_controllers;

	public:
		AutomationMenu(QMenu* parent, Track* t);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void inputItemClicked(const QModelIndex&);
		void itemClicked(const QModelIndex&, bool isInput = false);
	
	signals:
		void triggered();
};

#endif
