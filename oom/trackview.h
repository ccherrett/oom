//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef __TRACKVIEW_H__
#define __TRACKVIEW_H__

#include <QString>
#include <QObject>

#include <vector>
#include <algorithm>

#include "key.h"
#include "node.h"
#include "globaldefs.h"
#include <QMap>

class Xml;
class VirtualTrack;

struct TrackSettings {
	bool valid;
	int program;
	QString pname;
	int transpose;
	bool rec;
	qint64 tid;
	VirtualTrack* vtrack;
	virtual void write(int, Xml&) const;
	virtual void read(Xml&);
};

//---------------------------------------------------------
//   TrackView
//---------------------------------------------------------

class TrackView
{
	private:
		QString _comment;
		QList<qint64> m_tracks;
		QMap<qint64, TrackSettings*> _tSettings;
		QMap<qint64, TrackSettings*> m_vtrackSettings;
		bool _recState;
		qint64 m_id;

	protected:
		QString _name;
		
		bool _selected;
		bool readProperties(Xml& xml, const QString& tag);
		void writeProperties(int level, Xml& xml) const;

	public:
		TrackView();
		~TrackView();
		qint64 id() { return m_id; }
		QString getValidName(QString);
		TrackView& operator=(const TrackView& g);
		
		static const char* _cname[];
		
		QString comment() const         { return _comment; }
		void setComment(const QString& s) { _comment = s; }
		
		bool selected() const           { return _selected; }
		void setSelected(bool f);
		
		const QString& viewName() const     { return _name; }
		void setViewName(const QString& s)  { _name = s; }
		void setDefaultName();

		void addTrack(qint64);
		
		void removeTrack(qint64);
		
		QList<qint64>* tracks() { return &m_tracks; }

		bool record() { return _recState; }
		void setRecord(bool f) { _recState = f; }
		QMap<qint64, TrackSettings*>* trackSettings() { return &_tSettings;}
		QMap<qint64, TrackSettings*>* virtualTrackSettings() { return &m_vtrackSettings;}
		void addTrackSetting(qint64 id, TrackSettings* settings) {
			_tSettings[id] = settings;
		}
		void removeTrackSettings(qint64 id)
		{
			if(hasSettings(id))
			{
				_tSettings.erase(_tSettings.find(id));
			}
		}
		bool hasSettings(qint64 tid)
		{
			return _tSettings.contains(tid);
		}
		TrackSettings* getTrackSettings(qint64 id)
		{
			if(hasSettings(id))
				return _tSettings[id];
			else
				return 0;
		}
		virtual void write(int, Xml&) const;
		void read(Xml&);
};


//---------------------------------------------------------
//   TrackViewList
//---------------------------------------------------------

typedef QHash<qint64, TrackView*> TrackViewList;

typedef QHash<qint64, TrackView*>::iterator iTrackView;
typedef QHash<qint64, TrackView*>::const_iterator ciTrackView;

#endif

