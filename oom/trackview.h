//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
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
#include "track.h"
#include <QMap>

class Xml;

struct TrackSettings {
	bool valid;
	int program;
	QString pname;
	int transpose;
	bool rec;
	Track* track;
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
		TrackList _tracks;
		QMap<QString, TrackSettings*> _tSettings;
		bool _recState;


	protected:
		QString _name;
		
		bool _selected;
		bool readProperties(Xml& xml, const QString& tag);
		void writeProperties(int level, Xml& xml) const;

	public:
		TrackView();
		~TrackView();
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
		void addTrack(Track*);
		void removeTrack(Track*);
		TrackList* tracks() { return &_tracks; } 
		bool record() { return _recState; }
		void setRecord(bool f) { _recState = f; }
		QMap<QString, TrackSettings*>* trackSettings() { return &_tSettings;}
		void addTrackSetting(QString tname, TrackSettings* settings) {
			_tSettings[tname] = settings;
		}
		void removeTrackSettings(QString name)
		{
			if(hasSettings(name))
			{
				_tSettings.erase(_tSettings.find(name));
			}
		}
		bool hasSettings(QString tname)
		{
			return _tSettings.contains(tname);
		}
		TrackSettings* getTrackSettings(QString tname)
		{
			if(hasSettings(tname))
				return _tSettings[tname];
			else
				return new TrackSettings;
		}
		virtual void write(int, Xml&) const;
		void read(Xml&);
};


//---------------------------------------------------------
//   TrackViewList
//---------------------------------------------------------

template<class T> class viewlist : public std::vector<TrackView*> {
      typedef std::vector<TrackView*> vlist;

   public:
      class iterator : public vlist::iterator {
         public:
            iterator() : vlist::iterator() {}
            iterator(vlist::iterator i) : vlist::iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::iterator*)this));
            }
            iterator operator++(int) {
                  return iterator ((*(vlist::iterator*)this).operator++(0));
            }
            iterator& operator++() {
                  return (iterator&) ((*(vlist::iterator*)this).operator++());
            }
      };

      class const_iterator : public vlist::const_iterator {
         public:
            const_iterator() : vlist::const_iterator() {}
            const_iterator(vlist::const_iterator i) : vlist::const_iterator(i) {}
            const_iterator(vlist::iterator i) : vlist::const_iterator(i) {}

            const T operator*() const {
                  return (T)(**((vlist::const_iterator*)this));
            }
      };

      class reverse_iterator : public vlist::reverse_iterator {
         public:
            reverse_iterator() : vlist::reverse_iterator() {}
            reverse_iterator(vlist::reverse_iterator i) : vlist::reverse_iterator(i) {}

            T operator*() {
                  return (T)(**((vlist::reverse_iterator*)this));
            }
      };

      viewlist() : vlist() {}
      virtual ~viewlist() {}

      void push_back(T v)             { vlist::push_back(v); }
      iterator begin()                { return vlist::begin(); }
      iterator end()                  { return vlist::end(); }
      const_iterator begin() const    { return vlist::begin(); }
      const_iterator end() const      { return vlist::end(); }
      reverse_iterator rbegin()       { return vlist::rbegin(); }
      reverse_iterator rend()         { return vlist::rend(); }
      T& back() const                 { return (T&)(vlist::back()); }
      T& front() const                { return (T&)(vlist::front()); }
      iterator find(const TrackView* t)       {
            return std::find(begin(), end(), t);
      }
      const_iterator find(const TrackView* t) const {
            return std::find(begin(), end(), t);
      }
      unsigned index(const TrackView* t) const {
            unsigned n = 0;
            for (vlist::const_iterator i = begin(); i != end(); ++i, ++n) {
                  if (*i == t)
                        return n;
            }
            return -1;
      }
      T index(int k) const           { return (*this)[k]; }
      iterator index2iterator(int k) {
            if ((unsigned)k >= size())
                  return end();
            return begin() + k;
      }
      void erase(TrackView* t)           { vlist::erase(find(t)); }

      void clearDelete() {
            for (vlist::iterator i = begin(); i != end(); ++i)
                  delete *i;
            vlist::clear();
      }
      void erase(vlist::iterator i) { vlist::erase(i); }
      void replace(TrackView* ot, TrackView* nt) {
            for (vlist::iterator i = begin(); i != end(); ++i) {
                  if (*i == ot) {
                        *i = nt;
                        return;
                  }
            }
      }
};

typedef viewlist<TrackView*> TrackViewList;
typedef TrackViewList::iterator iTrackView;
typedef TrackViewList::const_iterator ciTrackView;

#endif

