//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _TVDOCK_
#define _TVDOCK_

#include "ui_tviewdockbase.h"

class QStandardItemModel;
class QStandardItem;
class QModelIndex;

class TrackViewDock : public QFrame, public Ui::TViewDockBase {
    Q_OBJECT
	QStandardItemModel* _tableModel;
	QStandardItemModel* _autoTableModel;

	private slots:
		void btnUpClicked(bool);
		void btnDownClicked(bool);
		void btnDeleteClicked(bool);
		void btnTVClicked(bool);
		void trackviewInserted(QModelIndex, int, int);
		void trackviewRemoved(QModelIndex, int, int);
		void trackviewChanged(QStandardItem*);
		void autoTrackviewChanged(QStandardItem*);
		void updateTrackView(int, QStandardItem*);
	
	public slots:
		void populateTable(int);
	
	public:
		TrackViewDock(QWidget* parent = 0);
		~TrackViewDock();
	
	private:
		QList<int> getSelectedRows();
		void updateTableHeader();
};
#endif

