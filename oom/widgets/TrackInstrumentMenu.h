//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _OOM_TRACK_INSTRUMENTMENU_
#define _OOM_TRACK_INSTRUMENTMENU_

#include <QMenu>
#include <QWidgetAction>
#include <QObject>

class QListView;
class QStandardItem;
class QStandardItemModel;
class Track;
class QModelIndex;


class TrackInstrumentMenu : public QWidgetAction
{
	Q_OBJECT

	QListView *list;
	QStandardItemModel *m_listModel;
	Track *m_track;

public:
	TrackInstrumentMenu(QMenu* parent, Track*);
	virtual QWidget* createWidget(QWidget* parent = 0);

private slots:
	void updateData(QStandardItem*);
	void itemClicked(const QModelIndex&);
	
signals:
	void triggered();
};

#endif
