//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _ROUTE_MAP_DOCK_
#define _ROUTE_MAP_DOCK_

#include "ui_routemapbase.h"

class QStandardItemModel;
class QStandardItem;
class QModelIndex;

class RouteMapDock : public QFrame, public Ui::RouteMapBase
{
	Q_OBJECT
	QStandardItemModel* _listModel;

	public:
	RouteMapDock(QWidget* parent = 0);

	public slots:
		void populateTable(int);
	
	private slots:
		void btnDeleteClicked(bool);
		void btnAddClicked(bool);
		void btnEditClicked(bool);
		void btnLoadClicked(bool);
		void btnCopyClicked(bool);
		void btnLinkClicked(bool);
		void btnClearClicked(bool);
		void saveRouteMap(QString, QString);
		void updateRouteMap(QString, QString);
		void renameRouteMap(QStandardItem*);
		void songChanged(int);
	
	private:
		QList<int> getSelectedRows();
		void updateTableHeader();
};

#endif
