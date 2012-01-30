//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QList>
#include "tviewdock.h"
#include "track.h"

class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QPoint;

class MixerView : public TrackViewDock
{
	Q_OBJECT
	QList<qint64> m_selectList;
	TrackList m_tracklist;

	protected:
		virtual void trackviewChanged(QStandardItem*);
		virtual void autoTrackviewChanged(QStandardItem*);
	
	public slots:
		virtual void populateTable(int, bool startup = false);
		void updateTrackList();
	
	signals:
		void trackListChanged(TrackList*);

	public:
		MixerView(QWidget* parent = 0);
		~MixerView();
		TrackList* tracklist() { return &m_tracklist; }
		void addButton(QWidget *btn);
};
