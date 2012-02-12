//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: minstrument.h,v 1.3.2.3 2009/03/09 02:05:18 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MINSTRUMENT_H__
#define __MINSTRUMENT_H__

#include "globaldefs.h"
#include <list>
#include <vector>
#include <QHash>

class MidiPort;
class QMenu;
class QStandardItemModel;
class MidiPlayEvent;
class Xml;
class EventList;
class MidiControllerList;
class QString;


//---------------------------------------------------------
//   Patch
//---------------------------------------------------------

struct Patch
{
    signed char typ; // 1 - GM  2 - GS  4 - XG
    signed char hbank, lbank, prog;
	QList<int> keys, keyswitches;
    QString name;
	int loadmode;
	QString engine;
	QString filename;
	int index;
	float volume;
    bool drum;
	QHash<int, QString> comments;
    void read(Xml&);
    void write(int level, Xml&);
};

typedef std::list<Patch*> PatchList;
typedef PatchList::iterator iPatch;
typedef PatchList::const_iterator ciPatch;

//---------------------------------------------------------
//   PatchGroup
//---------------------------------------------------------

struct PatchGroup
{
    QString name;
    PatchList patches;
	int id;
    void read(Xml&);
};

typedef std::vector<PatchGroup*> PatchGroupList;
typedef PatchGroupList::iterator iPatchGroup;
typedef PatchGroupList::const_iterator ciPatchGroup;

struct SysEx
{
    QString name;
    QString comment;
    int dataLen;
    unsigned char* data;
};

struct KeyMap
{
	int program;
	QString pname;
	int key;
	QString comment;
	bool hasProgram;
	inline bool operator==(KeyMap km)
	{
		return km.key == key && km.program == program && km.comment == comment;
	}
	inline uint qHash(KeyMap km)
	{
		return km.program ^ km.key;
	}
	void read(Xml&);
    void write(int level, Xml&);
};

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

class MidiInstrument
{
    PatchGroupList pg;
    MidiControllerList* _controller;
    QList<SysEx*> _sysex;
	QHash<int, KeyMap*> m_keymaps;
    bool _dirty;
    int _nullvalue;
	bool m_oomInstrument;
	double m_panValue;
	double m_verbValue;

    void init();

protected:
    EventList* _midiInit;
    EventList* _midiReset;
    EventList* _midiState;
    char* _initScript;
    QString _name;
    QString _filePath;

public:
    MidiInstrument();
    virtual ~MidiInstrument();
    MidiInstrument(const QString& txt);

    const QString& iname() const
    {
        return _name;
    }

    void setIName(const QString& txt)
    {
        _name = txt;
    }

	Patch* getDefaultPatch();

	KeyMap* newKeyMap(int key)
	{
		if(m_keymaps.contains(key))
		{
			return keymap(key);
		}
		else
		{
			KeyMap *km = new KeyMap;
			km->key = key;
			km->hasProgram = false;
			m_keymaps.insert(key, km);
			return km;
		}
	}
	bool hasMapping(int key)
	{
		if(m_keymaps.isEmpty())
		{
			return false;
		}
		return m_keymaps.contains(key);
	}
	KeyMap* keymap(int key)
	{
		if(hasMapping(key))
			return m_keymaps.value(key);
		else
		{
			return newKeyMap(key);;
		}
	}
	
	QHash<int, KeyMap*> *keymaps()
	{
		return &m_keymaps;
	}

	bool isOOMInstrument()
	{
		return m_oomInstrument;
	}

	void setOOMInstrument(bool val)
	{
		m_oomInstrument = val;
	}

    // Assign will 'delete' all existing patches and groups from the instrument.
    MidiInstrument& assign(const MidiInstrument&);

    QString filePath() const
    {
        return _filePath;
    }

    void setFilePath(const QString& s)
    {
        _filePath = s;
    }

	bool fileSave();

    bool dirty() const
    {
        return _dirty;
    }

    void setDirty(bool v)
    {
        _dirty = v;
    }

    const QList<SysEx*>& sysex() const
    {
        return _sysex;
    }

    void removeSysex(SysEx* sysex)
    {
        _sysex.removeAll(sysex);
    }

    void addSysex(SysEx* sysex)
    {
        _sysex.append(sysex);
    }

    EventList* midiInit() const
    {
        return _midiInit;
    }

    EventList* midiReset() const
    {
        return _midiReset;
    }

    EventList* midiState() const
    {
        return _midiState;
    }

    const char* initScript() const
    {
        return _initScript;
    }

    MidiControllerList* controller() const
    {
        return _controller;
    }

    int nullSendValue()
    {
        return _nullvalue;
    }

    void setNullSendValue(int v)
    {
        _nullvalue = v;
    }

    void readMidiState(Xml& xml);

    virtual bool guiVisible() const
    {
        return false;
    }

    virtual void showGui(bool)
    {
    }

    virtual bool hasGui() const
    {
        return false;
    }

    virtual void writeToGui(const MidiPlayEvent&)
    {
    }

    virtual void reset(int, MType);
    virtual QString getPatchName(int, int, MType, bool);
    virtual Patch* getPatch(int, int, MType, bool);
    virtual void populatePatchPopup(QMenu*, int, MType, bool);
	virtual void populatePatchModel(QStandardItemModel*, int, MType, bool);
    void read(Xml&);
    void write(int level, Xml&);

    PatchGroupList* groups()
    {
        return &pg;
    }
	void setDefaultPan(double p)
	{
		m_panValue = p;
	}
	void setDefaultVerb(double v)
	{
		m_verbValue = v;
	}
	double defaultPan()
	{
		return m_panValue;
	}
	double defaultVerb()
	{
		return m_verbValue;
	}
};

//---------------------------------------------------------
//   MidiInstrumentList
//---------------------------------------------------------

class MidiInstrumentList : public std::list<MidiInstrument*>
{
public:

    MidiInstrumentList()
    {
    }
};

typedef MidiInstrumentList::iterator iMidiInstrument;

extern MidiInstrumentList midiInstruments;
extern MidiInstrument* genericMidiInstrument;
extern void initMidiInstruments();
extern MidiInstrument* registerMidiInstrument(const QString&);
extern void removeMidiInstrument(const QString& name);
extern void removeMidiInstrument(const MidiInstrument* instr);

#endif

