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
class QPoint;

class TrackViewDock : public QFrame, public Ui::TViewDockBase {
    Q_OBJECT

	private:

	private slots:
		void btnUpClicked(bool);
		void btnDownClicked(bool);
		void btnDeleteClicked(bool);
		void btnTVClicked(bool);
		void trackviewInserted(QModelIndex, int, int);
		void trackviewRemoved(QModelIndex, int, int);
		void updateTrackView(int, QStandardItem*);
		void contextPopupMenu(QPoint);
		void templateContextPopupMenu(QPoint);
		void currentTabChanged(int);
		void populateInstrumentTemplates();
	
	protected:
		QStandardItemModel* _tableModel;
		QStandardItemModel* _autoTableModel;

		QStandardItemModel* _templateModel;
		
		QList<QIcon> m_icons;

		QList<int> getSelectedRows(int);
		void updateTableHeader();
	
	public slots:
		virtual void trackviewChanged(QStandardItem*);
		virtual void autoTrackviewChanged(QStandardItem*);
		virtual void populateTable(int, bool s = false);
		void selectStaticView(int);
		void toggleButtons(bool);
	
	public:
		TrackViewDock(QWidget* parent = 0);
		~TrackViewDock();
	
};
#endif

