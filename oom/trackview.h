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

//---------------------------------------------------------
//   TrackView
//---------------------------------------------------------

class TrackView
{
public:
	struct TrackSettings {
		bool valid;
		int program;
		QString pname;
		int transpose;
		bool rec;
		//Kept for migrating old files
		qint64 tid;
		void write(int, Xml&) const;
		void read(Xml&);
	};

	struct TrackViewTrack {
		TrackViewTrack()
		{
			settings = 0;
			is_virtual = false;
		}
		TrackViewTrack(qint64 i)
		{
			id = i;
			settings = 0;
			is_virtual = false;
		}
		bool hasSettings()
		{
			return settings ? true : false; 
		}
		void setSettingsCopy(TrackSettings*);
		qint64 id;
		bool is_virtual;
		TrackSettings *settings;
		void write(int, Xml&) const;
		void read(Xml&);
	};
private:
	QString _comment;
	QList<qint64> m_tracksIndex;
	QMap<qint64, TrackViewTrack*> m_tracks;
	bool _recState;
	bool m_template;

	QString _name;
	qint64 m_id;
	
	bool _selected;
	bool readProperties(Xml& xml, const QString& tag);
	void writeProperties(int level, Xml& xml) const;

public:

	TrackView(bool istemplate = false);
	TrackView(const TrackView&);
	TrackView& operator=(const TrackView& g);
	//TrackView* clone();
	~TrackView();
	qint64 id() { return m_id; }
	static QString getValidName(QString, bool temp = false);
	
	static const char* _cname[];
	
	QString comment() const         { return _comment; }
	void setComment(const QString& s) { _comment = s; }
	
	bool selected() const           { return _selected; }
	void setSelected(bool f);
	
	const QString& viewName() const     { return _name; }
	void setViewName(const QString& s)  { _name = s; }
	void setDefaultName();

	void addTrack(qint64);
	void addTrack(TrackViewTrack*);
	
	void removeTrack(qint64);
	void removeTrack(TrackViewTrack*);
	
	QMap<qint64, TrackViewTrack*>* tracks() { return &m_tracks; }

	QList<qint64> *trackIndexList()
	{
		return &m_tracksIndex;
	}

	bool record() { return _recState; }
	void setRecord(bool f) { _recState = f; }

	bool hasVirtualTracks()
	{
		foreach(TrackViewTrack* t, m_tracks)
		{
			if(t->is_virtual)
				return true;
		}
		return false;
	}

	QList<qint64> virtualTracks()
	{
		QList<qint64> rv;
		foreach(TrackViewTrack* t, m_tracks)
		{
			if(t->is_virtual)
				rv.append(t->id);
		}
		return rv;
	}

	void clear();

	virtual void write(int, Xml&) const;
	void read(Xml&);
};


//---------------------------------------------------------
//   TrackViewList
//---------------------------------------------------------

typedef QHash<qint64, TrackView*> TrackViewList;

typedef QHash<qint64, TrackView*>::iterator iTrackView;
typedef QHash<qint64, TrackView*>::const_iterator ciTrackView;

typedef QMap<qint64, VirtualTrack*>::const_iterator ciVirtualTrack;

#endif

