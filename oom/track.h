//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: track.h,v 1.39.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TRACK_H__
#define __TRACK_H__

#include <QString>
#include <QHash>
#include <QPair>
#include <QUuid>

#include <vector>
#include <algorithm>

#include "part.h"
#include "key.h"
#include "node.h"
#include "route.h"
#include "ctrl.h"
#include "globaldefs.h"

class Pipeline;
class Xml;
class SndFile;
class MPEventList;
class BasePlugin;
class MidiAssignData;
class MidiPort;
class CCInfo;

class Track;
struct MonitorLog
{
	unsigned pos;
	int value;
};
struct MidiAssignData {/*{{{*/
	Track* track;
	QHash<int, CCInfo*> midimap;
	int port;
	int preset;
	int channel;
	bool enabled;
	void read(Xml&, Track*);
	//void write(int, Xml&);
	inline bool operator==(MidiAssignData mad)
	{
		return mad.port == port && mad.track == track;
	}
	inline uint qHash(MidiAssignData m)
	{
		return (m.channel ^ m.port)*qrand();
	}
};/*}}}*/
//---------------------------------------------------------
//   Track
//---------------------------------------------------------

static const int DEFAULT_TRACKHEIGHT = 78;
static const int MIN_TRACKHEIGHT = 50;
static const int MIN_TRACKHEIGHT_SLIDER = 68;
static const int MIN_TRACKHEIGHT_VU = 78;
static const int MIN_TRACKHEIGHT_TOOLS = 150;

class Track
{
public:

    enum TrackType
    {
        MIDI = 0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_BUSS,
        AUDIO_AUX, AUDIO_SOFTSYNTH
    };
private:
    TrackType _type;
    QString _comment;

    PartList _parts;

    void init();
	bool _reminder1;
	bool _reminder2;
	bool _reminder3;


protected:
	qint64 m_id;
    static unsigned int _soloRefCnt;
    static Track* _tmpSoloChainTrack;
    static bool _tmpSoloChainDoIns;
    static bool _tmpSoloChainNoDec;

    RouteList _inRoutes;
    RouteList _outRoutes;

    QString _name;
    int _partDefaultColor;
    bool _recordFlag;
    bool _mute;
    bool _solo;
    unsigned int _internalSolo;
    bool _off;
    int _channels; // 1 - mono, 2 - stereo

    bool _volumeEnCtrl;
    bool _volumeEn2Ctrl;
    bool _panEnCtrl;
    bool _panEn2Ctrl;
	bool _collapsed;
	int _mixerTab;
	int m_maxZIndex;

    int _activity;
    int _lastActivity;
    double _meter[MAX_CHANNELS];
    double _peak[MAX_CHANNELS];

    int _y;
    int _height; // visual height in Composer

    bool _locked;
    bool _selected;
	qint64 m_chainMaster;
	bool m_masterFlag;

    
    bool _wantsAutomation;

	MidiAssignData m_midiassign;
	QList<qint64> m_audioChain;
    
	bool readProperties(Xml& xml, const QString& tag);
    void writeProperties(int level, Xml& xml) const;

public:
    Track(TrackType);
    Track(const Track&, bool cloneParts);

    virtual ~Track()
    {
    };
    virtual Track & operator=(const Track& t);

    static const char* _cname[];
	
	bool getReminder1()
	{
		return _reminder1;
	}
	void setReminder1(bool r)
	{
		_reminder1 = r;
	}
	bool getReminder2()
	{
		return _reminder2;
	}
	void setReminder2(bool r)
	{
		_reminder2 = r;
	}
	bool getReminder3()
	{
		return _reminder3;
	}
	void setReminder3(bool r)
	{
		_reminder3 = r;
	}
	bool collapsed() { return _collapsed; }
	void setCollapsed(bool f) { _collapsed = f; }
	int mixerTab(){return _mixerTab;}
	void setMixerTab(int i){_mixerTab = i;}
	void setMaxZIndex(int i)
	{
		if(i > m_maxZIndex)
			m_maxZIndex = i;
	}
	int maxZIndex() { return m_maxZIndex; }

    QString comment() const
    {
        return _comment;
    }

    void setComment(const QString& s)
    {
        _comment = s;
    }

	void setChainMaster(qint64 val){m_chainMaster = val;}
	qint64 chainMaster(){return m_chainMaster;}
	void setMasterFlag(bool val){m_masterFlag = val;}
	bool masterFlag() {return m_masterFlag;}
	void addManagedTrack(qint64 id)
	{
		if(!chainContains(id))
			m_audioChain.append(id);
	}
	QList<qint64>* audioChain() {return &m_audioChain;}
	bool chainContains(qint64 id) 
	{
		return (!m_audioChain.isEmpty() && m_audioChain.contains(id));
	}
	bool hasChildren()
	{
		return !m_audioChain.isEmpty();
	}
	Track* inputTrack();

	MidiAssignData* midiAssign() { return &m_midiassign; }

	qint64 id()
	{
		return  m_id;
	}

    int y() const;

    void setY(int n)
    {
        _y = n;
    }

    int height() const
    {
        return _height;
    }

    void setHeight(int n)
    {
        _height = n;
    }

    bool selected() const
    {
        return _selected;
    }

    void setSelected(bool f);

	void deselectParts();

    bool locked() const
    {
        return _locked;
    }

    void setLocked(bool b)
    {
        _locked = b;
    }

    bool volumeControllerEnabled() const
    {
        return _volumeEnCtrl;
    }

    bool volumeControllerEnabled2() const
    {
        return _volumeEn2Ctrl;
    }

    bool panControllerEnabled() const
    {
        return _panEnCtrl;
    }

    bool panControllerEnabled2() const
    {
        return _panEn2Ctrl;
    }

    void enableVolumeController(bool b)
    {
        _volumeEnCtrl = b;
    }

    void enable2VolumeController(bool b)
    {
        _volumeEn2Ctrl = b;
    }

    void enablePanController(bool b)
    {
        _panEnCtrl = b;
    }

    void enable2PanController(bool b)
    {
        _panEn2Ctrl = b;
    }
    void clearRecAutomation(bool clearList);

    const QString& name() const
    {
        return _name;
    }

    virtual void setName(const QString& s)
    {
        _name = s;
    }

    TrackType type() const
    {
        return _type;
    }

    void setType(TrackType t)
    {
        _type = t;
    }

    QString cname() const
    {
        int t = type();
        return QString(_cname[t]);
    }

    // routing

    RouteList* inRoutes()
    {
        return &_inRoutes;
    }

    RouteList* outRoutes()
    {
        return &_outRoutes;
    }

    bool noInRoute() const
    {
        return _inRoutes.empty();
    }

    bool noOutRoute() const
    {
        return _outRoutes.empty();
    }
    void writeRouting(int, Xml&) const;

    PartList* parts()
    {
        return &_parts;
    }

    const PartList* cparts() const
    {
        return &_parts;
    }
    Part* findPart(unsigned tick);
    iPart addPart(Part* p);

    virtual void write(int, Xml&) const = 0;

    virtual Track* newTrack() const = 0;
    virtual Track* clone(bool CloneParts) const = 0;

    virtual bool setRecordFlag1(bool f, bool monitor = false) = 0;
    virtual void setRecordFlag2(bool f, bool monitor = false) = 0;

    virtual Part* newPart(Part*p = 0, bool clone = false) = 0;
    void dump() const;
    virtual void splitPart(Part*, int, Part*&, Part*&);

    virtual void setMute(bool val, bool monitor = false);
    virtual void setOff(bool val);
    virtual void updateSoloStates(bool noDec) = 0;
    virtual void updateInternalSoloStates();
    void updateSoloState();
    void setInternalSolo(unsigned int val);
    static void clearSoloRefCounts();
    virtual void setSolo(bool val, bool monitor = false) = 0;
    virtual bool isMute() const = 0;

    unsigned int internalSolo() const
    {
        return _internalSolo;
    }

    bool soloMode() const
    {
        return _soloRefCnt;
    }

    bool solo() const
    {
        return _solo;
    }

    bool mute() const
    {
        return _mute;
    }

    bool off() const
    {
        return _off;
    }

    bool recordFlag() const
    {
        return _recordFlag;
    }
    
	void setDefaultPartColor(int pc)
    {
        _partDefaultColor = pc;
    }
	int getDefaultPartColor()
    {
        return _partDefaultColor;
    }

    int activity()
    {
        return _activity;
    }

    void setActivity(int v)
    {
        _activity = v;
    }

    int lastActivity()
    {
        return _lastActivity;
    }

    void setLastActivity(int v)
    {
        _lastActivity = v;
    }

    void addActivity(int v)
    {
        _activity += v;
    }
    void resetPeaks();
    static void resetAllMeter();

    double meter(int ch) const
    {
        return _meter[ch];
    }

    double peak(int ch) const
    {
        return _peak[ch];
    }
    void resetMeter();

    bool readProperty(Xml& xml, const QString& tag);
    void setDefaultName();
	static QString getValidName(QString name, bool isdefault = false);

    int channels() const
    {
        return _channels;
    }
    virtual void setChannels(int n);

    bool isMidiTrack() const
    {
        return type() == MIDI || type() == DRUM;
    }

    virtual bool canRecord() const
    {
        return false;
    }
    virtual AutomationType automationType() const = 0;
    virtual void setAutomationType(AutomationType t) = 0;
    
    bool wantsAutomation()
    {
        return _wantsAutomation;
    }
    
    void setWantsAutomation(bool yesno)
    {
        _wantsAutomation = yesno;
    }
};

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class AudioTrack;

class MidiTrack : public Track
{
    int _outPort;
	qint64 _outPortId;
    int _outChannel;
    bool _recEcho; // For midi (and audio). Whether to echo incoming record events to output device.

    EventList* _events; // tmp Events during midi import
    MPEventList* _mpevents; // tmp Events druring recording
	QHash<int, QList<MonitorLog> > m_monitorBuffer;
	SamplerData* m_samplerData;

public:
    MidiTrack();
    MidiTrack(const MidiTrack&, bool cloneParts);
    virtual ~MidiTrack();

    void init();
    virtual AutomationType automationType() const;
    virtual void setAutomationType(AutomationType);

	bool transpose;
    int transposition;
    int velocity;
    int delay;
    int len;
    int compression;

	int getTransposition();
	QList<MonitorLog> getMonitorBuffer(int ctrl)
	{
		if(m_monitorBuffer.isEmpty() || !m_monitorBuffer.contains(ctrl))
		{
			QList<MonitorLog> list;
			m_monitorBuffer.insert(ctrl, list);
		}
		QList<MonitorLog> list1 = m_monitorBuffer.value(ctrl);
		return list1;
	}

    virtual bool setRecordFlag1(bool f, bool monitor = false);

    virtual void setRecordFlag2(bool, bool monitor = false);

    EventList* events() const
    {
        return _events;
    }

    MPEventList* mpevents() const
    {
        return _mpevents;
    }

    virtual void read(Xml&);
    virtual void write(int, Xml&) const;

    virtual MidiTrack* newTrack() const
    {
        return new MidiTrack();
    }

    virtual MidiTrack* clone(bool cloneParts) const
    {
        return new MidiTrack(*this, cloneParts);
    }
    virtual Part* newPart(Part*p = 0, bool clone = false);

    void setOutChannel(int i)
    {
        _outChannel = i;
    }

    void setOutPort(int i);
    void setOutPortId(qint64 i);
    void setOutChanAndUpdate(int i);
    void setOutPortAndUpdate(int i);
    void setOutPortIdAndUpdate(qint64 i);

    // Backward compatibility: For reading old songs.
    void setInPortAndChannelMask(unsigned int /*portmask*/, int /*chanmask*/);

    void setRecEcho(bool b)
    {
        _recEcho = b;
    }

    int outPort() const
    {
        return _outPort;
    }

	qint64 outPortId() const
	{
		return _outPortId;
	}

    int outChannel() const
    {
        return _outChannel;
    }

    bool recEcho() const
    {
        return _recEcho;
    }
	SamplerData* samplerData() {return m_samplerData;}
	void setSamplerData(SamplerData* data){ m_samplerData = data;}

    virtual bool isMute() const;
    virtual void setSolo(bool val, bool monitor = false);
    virtual void updateSoloStates(bool noDec);
    virtual void updateInternalSoloStates();

    virtual bool canRecord() const
    {
        return true;
    }
    
    AudioTrack* getAutomationTrack();
};

typedef QPair<bool, double> AuxInfo;
/*typedef struct _AuxInfo
{
	double value;
	bool pre;
	_AuxInfo(double v, bool p = false)
	{
		value = v;
		pre = p;
	}
} AuxInfo;
*/

//---------------------------------------------------------
//   AudioTrack
//    this track can hold audio automation data and can
//    hold tracktypes AUDIO, AUDIO_MASTER, AUDIO_BUSS,
//    AUDIO_INPUT, AUDIO_SOFTSYNTH, AUDIO_AUX
//---------------------------------------------------------

class AudioTrack : public Track
{
    bool _haveData;

    CtrlListList _controller;
    CtrlRecList _recEvents; // recorded automation events

    bool _prefader; // prefader metering
	QHash<qint64, AuxInfo> _auxSend;
	
    Pipeline* _efxPipe;

    AutomationType _automationType;

    bool _sendMetronome;

	QHash<int, qint64> m_auxControlList;
    void readAuxSend(Xml& xml);

protected:
    float** outBuffers;
    int _totalOutChannels;
    int _totalInChannels;

    unsigned bufferPos;
    virtual bool getData(unsigned, int, unsigned, float**);
    SndFile* _recFile;
    Fifo fifo; // fifo -> _recFile
    bool _processed;

public:
    AudioTrack(TrackType t);

    AudioTrack(const AudioTrack&, bool cloneParts);
    virtual ~AudioTrack();

    virtual bool setRecordFlag1(bool f, bool monitor = false);
    virtual void setRecordFlag2(bool f, bool monitor = false);
    bool prepareRecording();

    bool processed()
    {
        return _processed;
    }

	QHash<int, qint64>* auxControlList()
	{
		return &m_auxControlList;
	}
    //void setProcessed(bool v) { _processed = v; }

    void addController(CtrlList*);
    void removeController(int id);
    void swapControllerIDX(int idx1, int idx2);

    bool readProperties(Xml&, const QString&);
    void writeProperties(int, Xml&) const;

    void mapRackPluginsToControllers();
    void showPendingPluginNativeGuis();

    virtual AudioTrack* clone(bool cloneParts) const = 0;
    virtual Part* newPart(Part*p = 0, bool clone = false);

    SndFile* recFile() const
    {
        return _recFile;
    }

    void setRecFile(SndFile* sf)
    {
        _recFile = sf;
    }

    CtrlListList* controller()
    {
        return &_controller;
    }

    virtual void setChannels(int n);
    virtual void setTotalOutChannels(int num);

    virtual int totalOutChannels()
    {
        return _totalOutChannels;
    }
    virtual void setTotalInChannels(int num);

    virtual int totalInChannels()
    {
        return _totalInChannels;
    }

    virtual bool isMute() const;
    virtual void setSolo(bool val, bool monitor = false);
    virtual void updateSoloStates(bool noDec);
    virtual void updateInternalSoloStates();

    void putFifo(int channels, unsigned long n, float** bp);

    void record();

    virtual void setMute(bool val, bool monitor = false);
    virtual void setOff(bool val);

    void setSendMetronome(bool val)
    {
        _sendMetronome = val;
    }

    bool sendMetronome() const
    {
        return _sendMetronome;
    }

	bool volFromAutomation();
    double volume() const;
    double volume(unsigned) const;
    void setVolume(double val, bool monitor = false);
	bool panFromAutomation();
    double pan() const;
    void setPan(double val, bool monitor = false);

    bool prefader() const
    {
        return _prefader;
    }
	QHash<qint64, AuxInfo>* auxSends()
	{
		return &_auxSend;
	}
    double auxSend(qint64 idx) const;
    void setAuxSend(qint64 idx, double v, bool monitor = false);
    void addAuxSend();
	bool auxIsPrefader(qint64 idx);
	void setAuxPrefader(qint64 idx, bool);

    void setPrefader(bool val);

    Pipeline* efxPipe()
    {
        return _efxPipe;
    }
    void deleteAllEfxGuis();
    void clearEfxList();
    void addPlugin(BasePlugin* plugin, int idx);
    void idlePlugin(BasePlugin* plugin);

    double pluginCtrlVal(int ctlID) const;
    void setPluginCtrlVal(int param, double val);

    void readVolume(Xml& xml);

    virtual void preProcessAlways()
    {
        _processed = false;
    }
    virtual void addData(unsigned /*samplePos*/, int /*channels*/, int /*srcStartChan*/, int /*srcChannels*/, unsigned /*frames*/, float** /*buffer*/);
    virtual void copyData(unsigned /*samplePos*/, int /*channels*/, int /*srcStartChan*/, int /*srcChannels*/, unsigned /*frames*/, float** /*buffer*/);

    virtual bool hasAuxSend() const
    {
        return false;
    }

    // automation

    virtual AutomationType automationType() const
    {
        return _automationType;
    }
    virtual void setAutomationType(AutomationType t);
    void processAutomationEvents();

    CtrlRecList* recEvents()
    {
        return &_recEvents;
    }
    void recordAutomation(int n, double v);
    void startAutoRecord(int, double);
    void stopAutoRecord(int, double);
    void setControllerMode(int, CtrlList::Mode m);
    void clearControllerEvents(int);
    void seekPrevACEvent(int);
    void seekNextACEvent(int);
    void eraseACEvent(int, int);
    void eraseRangeACEvents(int, int, int);
    void addACEvent(int, int, double);
};

//---------------------------------------------------------
//   AudioInput
//---------------------------------------------------------

class AudioInput : public AudioTrack
{
	bool _slave;
	Track* _master;
    void* jackPorts[MAX_CHANNELS];
    virtual bool getData(unsigned, int, unsigned, float**);

public:
    AudioInput();
    AudioInput(const AudioInput&, bool cloneParts);
    virtual ~AudioInput();

    AudioInput* clone(bool cloneParts) const
    {
        return new AudioInput(*this, cloneParts);
    }

    virtual AudioInput* newTrack() const
    {
        return new AudioInput();
    }
    virtual void read(Xml&);
    virtual void write(int, Xml&) const;
    virtual void setName(const QString& s);

	void setSlave(bool s)
	{
		_slave = s;
	}
	bool isSlave()
	{
		return _slave;
	}
	void setMaster(Track* trk)
	{
		_master = trk;
	}
	Track* getMaster()
	{
		return _master;
	}
    void* jackPort(int channel)
    {
        return jackPorts[channel];
    }

    void setJackPort(int channel, void*p)
    {
        jackPorts[channel] = p;
    }
    virtual void setChannels(int n);

    virtual bool hasAuxSend() const
    {
        return true;
    }

    virtual bool isMute() const
    {
        return false;
    }

};

//---------------------------------------------------------
//   AudioOutput
//---------------------------------------------------------

class AudioOutput : public AudioTrack
{
	bool _slave;
	Track* _master;
    void* jackPorts[MAX_CHANNELS];
    float* buffer[MAX_CHANNELS];
    float* buffer1[MAX_CHANNELS];
    unsigned long _nframes;

    float* _monitorBuffer[MAX_CHANNELS];

public:
    AudioOutput();
    AudioOutput(const AudioOutput&, bool cloneParts);
    virtual ~AudioOutput();

    AudioOutput* clone(bool cloneParts) const
    {
        return new AudioOutput(*this, cloneParts);
    }

    virtual AudioOutput* newTrack() const
    {
        return new AudioOutput();
    }
    virtual void read(Xml&);
    virtual void write(int, Xml&) const;
    virtual void setName(const QString& s);

	void setSlave(bool s)
	{
		_slave = s;
	}
	bool isSlave()
	{
		return _slave;
	}
	void setMaster(Track* trk)
	{
		_master = trk;
	}
	Track* getMaster()
	{
		return _master;
	}
    void* jackPort(int channel)
    {
        return jackPorts[channel];
    }

    void setJackPort(int channel, void*p)
    {
        jackPorts[channel] = p;
    }
    virtual void setChannels(int n);

    void processInit(unsigned);
    void process(unsigned pos, unsigned offset, unsigned);
    void processWrite();
    void silence(unsigned);

    virtual bool canRecord() const
    {
        return true;
    }

    float** monitorBuffer()
    {
        return _monitorBuffer;
    }

    virtual bool isMute() const
    {
            return _mute;
    }

};

//---------------------------------------------------------
//   AudioBuss
//---------------------------------------------------------

class AudioBuss : public AudioTrack
{
	AudioInput* _input;
	AudioOutput* _output;
public:

    AudioBuss() : AudioTrack(AUDIO_BUSS)
    {
    }

    AudioBuss* clone(bool /*cloneParts*/) const
    {
        return new AudioBuss(*this);
    }

    virtual AudioBuss* newTrack() const
    {
        return new AudioBuss();
    }
    virtual void read(Xml&);
    virtual void write(int, Xml&) const;

    virtual bool hasAuxSend() const
    {
        return true;
    }

    virtual bool isMute() const
    {
            return _mute;
    }

	void setOutputTrack(AudioOutput* out)
	{
		_output = out;
	}
	AudioOutput* output()
	{
		return _output;
	}
	void setInputTrack(AudioInput* in)
	{
		_input = in;
	}
	AudioInput* input()
	{
		return _input;
	}

};

//---------------------------------------------------------
//   AudioAux
//---------------------------------------------------------

class AudioAux : public AudioTrack
{
    float* buffer[MAX_CHANNELS];

public:
    AudioAux();

    AudioAux* clone(bool /*cloneParts*/) const
    {
        return new AudioAux(*this);
    }
    ~AudioAux();

    virtual AudioAux* newTrack() const
    {
        return new AudioAux();
    }
    virtual void read(Xml&);
    virtual void write(int, Xml&) const;
    virtual bool getData(unsigned, int, unsigned, float**);
    virtual void setChannels(int n);

    float** sendBuffer()
    {
        return buffer;
    }

    virtual bool isMute() const
    {
            return _mute;
    }
};

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

class WaveTrack : public AudioTrack
{
    Fifo _prefetchFifo; // prefetch Fifo
	AudioInput* _input;
	AudioOutput* _output;

public:
    static bool firstWaveTrack;

    WaveTrack() : AudioTrack(Track::WAVE)
    {
    }

    WaveTrack(const WaveTrack& wt, bool cloneParts) : AudioTrack(wt, cloneParts)
    {
    }

    virtual WaveTrack* clone(bool cloneParts) const
    {
        return new WaveTrack(*this, cloneParts);
    }

    virtual WaveTrack* newTrack() const
    {
        return new WaveTrack();
    }
    virtual Part* newPart(Part*p = 0, bool clone = false);

    virtual void read(Xml&);
    virtual void write(int, Xml&) const;

    virtual void fetchData(unsigned pos, unsigned frames, float** bp, bool doSeek);

    virtual bool getData(unsigned, int ch, unsigned, float** bp);

    void clearPrefetchFifo()
    {
        _prefetchFifo.clear();
    }

    Fifo* prefetchFifo()
    {
        return &_prefetchFifo;
    }
    virtual void setChannels(int n);

    virtual bool hasAuxSend() const
    {
        return true;
    }
    bool canEnableRecord() const;

    virtual bool canRecord() const
    {
        return true;
    }

	void setOutputTrack(AudioOutput* out)
	{
		_output = out;
	}
	AudioOutput* output()
	{
		return _output;
	}
	void setInputTrack(AudioInput* in)
	{
		_input = in;
	}
	AudioInput* input()
	{
		return _input;
	}

	void calculateCrossFades();

	bool leftEdgeOnTopOfPartBelow(WavePart* topPart, WavePart* bottomPart);
	bool rightEdgeOnTopOfPartBelow(WavePart* topPart, WavePart* bottomPart);
	bool leftAndRightEdgeOnTopOfPartBelow(WavePart* topPart, WavePart* bottomPart);
	QList<WavePart*> partsBelowLeftEdge(WavePart* part);
	QList<WavePart*> partsBelowRightEdge(WavePart* part);
};

//---------------------------------------------------------
//   TrackList
//---------------------------------------------------------

template<class T> class tracklist : public std::vector<Track*>
{
    typedef std::vector<Track*> vlist;

public:

    class iterator : public vlist::iterator
    {
    public:

        iterator() : vlist::iterator()
        {
        }

        iterator(vlist::iterator i) : vlist::iterator(i)
        {
        }

        T operator*()
        {
            return (T) (**((vlist::iterator*)this));
        }

        iterator operator++(int)
        {
            return iterator((*(vlist::iterator*)this).operator++(0));
        }

        iterator & operator++()
        {
            return (iterator&) ((*(vlist::iterator*)this).operator++());
        }
    };

    class const_iterator : public vlist::const_iterator
    {
    public:

        const_iterator() : vlist::const_iterator()
        {
        }

        const_iterator(vlist::const_iterator i) : vlist::const_iterator(i)
        {
        }

        const_iterator(vlist::iterator i) : vlist::const_iterator(i)
        {
        }

        const T operator*() const
        {
            return (T) (**((vlist::const_iterator*)this));
        }
    };

    class reverse_iterator : public vlist::reverse_iterator
    {
    public:

        reverse_iterator() : vlist::reverse_iterator()
        {
        }

        reverse_iterator(vlist::reverse_iterator i) : vlist::reverse_iterator(i)
        {
        }

        T operator*()
        {
            return (T) (**((vlist::reverse_iterator*)this));
        }
    };

    tracklist() : vlist()
    {
    }

    virtual ~tracklist()
    {
    }

    void push_back(T v)
    {
        vlist::push_back(v);
    }

    iterator begin()
    {
        return vlist::begin();
    }

    iterator end()
    {
        return vlist::end();
    }

    const_iterator begin() const
    {
        return vlist::begin();
    }

    const_iterator end() const
    {
        return vlist::end();
    }

    reverse_iterator rbegin()
    {
        return vlist::rbegin();
    }

    reverse_iterator rend()
    {
        return vlist::rend();
    }

    T& back() const
    {
        return (T&) (vlist::back());
    }

    T& front() const
    {
        return (T&) (vlist::front());
    }

    iterator find(const Track* t)
    {
        return std::find(begin(), end(), t);
    }

    const_iterator find(const Track* t) const
    {
        return std::find(begin(), end(), t);
    }

    unsigned index(const Track* t) const
    {
        unsigned n = 0;
        for (vlist::const_iterator i = begin(); i != end(); ++i, ++n) {
            if (*i == t)
                return n;
        }
        return -1;
    }

    T index(int k) const
    {
        return (*this)[k];
    }

    iterator index2iterator(int k)
    {
        if ((unsigned) k >= size())
            return end();
        return begin() + k;
    }

    void erase(Track* t)
    {
        vlist::erase(find(t));
    }

    void clearDelete()
    {
        for (vlist::iterator i = begin(); i != end(); ++i)
            delete *i;
        vlist::clear();
    }

    void erase(vlist::iterator i)
    {
        vlist::erase(i);
    }

    void replace(Track* ot, Track* nt)
    {
        for (vlist::iterator i = begin(); i != end(); ++i) {
            if (*i == ot) {
                *i = nt;
                return;
            }
        }
    }
};

typedef tracklist<Track*> TrackList;
typedef TrackList::iterator iTrack;
typedef TrackList::const_iterator ciTrack;

typedef tracklist<MidiTrack*>::iterator iMidiTrack;
typedef tracklist<MidiTrack*>::const_iterator ciMidiTrack;
typedef tracklist<MidiTrack*> MidiTrackList;

typedef tracklist<WaveTrack*>::iterator iWaveTrack;
typedef tracklist<WaveTrack*>::const_iterator ciWaveTrack;
typedef tracklist<WaveTrack*> WaveTrackList;

typedef tracklist<AudioInput*>::iterator iAudioInput;
typedef tracklist<AudioInput*>::const_iterator ciAudioInput;
typedef tracklist<AudioInput*> InputList;

typedef tracklist<AudioOutput*>::iterator iAudioOutput;
typedef tracklist<AudioOutput*>::const_iterator ciAudioOutput;
typedef tracklist<AudioOutput*> OutputList;

typedef tracklist<AudioBuss*>::iterator iAudioBuss;
typedef tracklist<AudioBuss*>::const_iterator ciAudioBuss;
typedef tracklist<AudioBuss*> GroupList;

typedef tracklist<AudioAux*>::iterator iAudioAux;
typedef tracklist<AudioAux*>::const_iterator ciAudioAux;
typedef tracklist<AudioAux*> AuxList;

extern void addPortCtrlEvents(MidiTrack* t);
extern void removePortCtrlEvents(MidiTrack* t);

#endif

