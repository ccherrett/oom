//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "audio.h"
#include "ctrl.h"
#include "globals.h"
#include "lib_functions.h"

#include "mididev.h"
#include "instruments/minstrument.h"

#ifdef __linux__
#undef __cdecl
#define __cdecl
#endif

#include <list>
#include <vector>
#include <math.h>
#include <stdint.h>
#include <QFileInfo>
#include <QMutex>

// ladspa includes
#include "ladspa.h"

// lv2 includes
#include "lv2.h"
#include "lv2_event.h"
#include "lv2_ui.h"

#ifdef LILV_STATIC
#include "lilv/lilv.h"
#else
#include <lilv/lilv.h>
#endif

// vst includes
#ifdef USE_OFFICIAL_VSTSDK
#define VST_FORCE_DEPRECATED 0
#include "pluginterfaces/vst2.x/aeffectx.h"
#else
#include "vestige/aeffectx.h"
// missing defines
#define effFlagsProgramChunks (1 << 5)
#define effGetParamLabel 6
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effGetProgramNameIndexed 29
#define effIdle 53
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kVstVersion 2400
#define kVstTransportChanged (1 << 0)
#endif

class AudioTrack;
class PluginI;
class PluginGui;
class Xml;

// plugin hints
const unsigned int PLUGIN_IS_SYNTH            = 0x01;
const unsigned int PLUGIN_IS_FX               = 0x02;
const unsigned int PLUGIN_HAS_NATIVE_GUI      = 0x04;
const unsigned int PLUGIN_HAS_IN_PLACE_BROKEN = 0x08;

// parameter hints
const unsigned int PARAMETER_IS_ENABLED        = 0x01;
const unsigned int PARAMETER_IS_AUTOMABLE      = 0x02;
const unsigned int PARAMETER_IS_TOGGLED        = 0x04;
const unsigned int PARAMETER_IS_INTEGER        = 0x08;
const unsigned int PARAMETER_IS_LOGARITHMIC    = 0x10;
const unsigned int PARAMETER_HAS_STRICT_BOUNDS = 0x20;
const unsigned int PARAMETER_USES_SCALEPOINTS  = 0x40;
const unsigned int PARAMETER_USES_SAMPLERATE   = 0x80;

enum PluginType {
    PLUGIN_NONE   = 0,
    PLUGIN_LADSPA = 1,
    PLUGIN_LV2    = 2,
    PLUGIN_VST    = 3
};

enum ParameterType {
    PARAMETER_UNKNOWN = 0,
    PARAMETER_INPUT   = 1,
    PARAMETER_OUTPUT  = 2
};

//---------------------------------------------------------
//   Control/Parameter Port
//---------------------------------------------------------

class ParameterPort
{
public:
    ParameterPort()
    {
        type   = PARAMETER_UNKNOWN;
        hints  = 0;
        rindex = 0;
        value  = tmpValue = 0.0;

        ranges.def = 0.0;
        ranges.min = 0.0;
        ranges.max = 1.0;
        ranges.step = 0.001;
        ranges.step_small = 0.0001;
        ranges.step_large = 0.1;

        enCtrl  = true;
        en2Ctrl = true;
        update  = false;
    }

    void fix_current_value()
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        tmpValue = value;
    }

    ParameterType type;
    unsigned int hints;
    int32_t rindex;
    double value;
    double tmpValue;

    struct {
        double def;
        double min;
        double max;
        double step;
        double step_small;
        double step_large;
    } ranges;

    bool enCtrl;
    bool en2Ctrl;
    bool update;
};

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class BasePlugin
{
public:
    BasePlugin()
    {
        m_type = PLUGIN_NONE;
        m_hints = 0;
        m_uniqueId = 0;

        m_active = false;
        m_activeBefore = false;

        m_paramCount = 0;
        m_params = 0;
        m_currentProgram = -1;

        m_id = -1;
        m_channels = 0;
        m_track = 0;
        m_gui = 0;

        m_enabled = false; // wait for a reload() call
        m_lib = 0;

        // synths only
        m_ainsCount  = 0;
        m_aoutsCount = 0;
        m_portsIn = 0;
        m_portsOut = 0;
    }

    ~BasePlugin()
    {
        deleteGui();

        m_type = PLUGIN_NONE;
        m_hints = 0;
        m_uniqueId = 0;

        m_active = false;
        m_activeBefore = false;

        m_id = -1;
        m_channels = 0;
        m_track = 0;

        if (m_paramCount > 0)
        {
            delete[] m_params;
            m_paramCount = 0;
            m_params = 0;
        }

        if (m_lib)
        {
            lib_close(m_lib);
            m_lib = 0;
        }
        
        // synths only
        if (m_ainsCount > 0)
        {
            delete[] m_portsIn;
            m_ainsCount = 0;
            m_portsIn = 0;
        }

        if (m_aoutsCount > 0)
        {
            delete[] m_portsOut;
            m_aoutsCount = 0;
            m_portsOut = 0;
        }
    }

    PluginType type() const
    {
        return m_type;
    }

    int hints() const
    {
        return m_hints;
    }

    QString name() const
    {
        return m_name;
    }

    QString label() const
    {
        return m_label;
    }

    QString maker() const
    {
        return m_maker;
    }

    QString copyright() const
    {
        return m_copyright;
    }

    QString filename() const
    {
        return m_filename;
    }

    long uniqueId() const
    {
        return m_uniqueId;
    }

    bool active()
    {
        return m_active;
    }

    int id()
    {
        return m_id;
    }

    AudioTrack* track()
    {
        return m_track;
    }

    PluginGui* gui()
    {
        return m_gui;
    }

    bool enabled()
    {
        return m_enabled;
    }

    uint32_t getParameterCount()
    {
        return m_paramCount;
    }

    ParameterPort* getParameterPort(uint32_t index)
    {
        if (index < m_paramCount)
            return &m_params[index];
        else
            return 0;
    }

    double getParameterValue(uint32_t index)
    {
        if (index < m_paramCount)
            return m_params[index].value;
        else
            return 0.0;
    }

    double getParameterDefaultValue(uint32_t index)
    {
        if (index < m_paramCount)
            return m_params[index].ranges.def;
        else
            return 0.0;
    }

    int32_t getCurrentProgram()
    {
        return m_currentProgram;
    }

    bool controllerEnabled(uint32_t index)
    {
        if (index < m_paramCount)
            return m_params[index].enCtrl;
        else
            return false;
    }

    bool controllerEnabled2(uint32_t index)
    {
        if (index < m_paramCount)
            return m_params[index].en2Ctrl;
        else
            return false;
    }

    void enableController(uint32_t index, bool yesno = true)
    {
        if (index < m_paramCount)
            m_params[index].enCtrl = yesno;
    }

    void enable2Controller(uint32_t index, bool yesno = true)
    {
        if (index < m_paramCount)
            m_params[index].en2Ctrl = yesno;
    }

    void enableAllControllers(bool yesno = true)
    {
        for (uint32_t i = 0; i < m_paramCount; i++)
            m_params[i].enCtrl = yesno;
    }

    void enable2AllControllers(bool yesno = true)
    {
        for (uint32_t i = 0; i < m_paramCount; i++)
            m_params[i].en2Ctrl = yesno;
    }

    void updateControllers()
    {
        if (! m_track)
            return;

        for (uint32_t i = 0; i < m_paramCount; i++)
            audio->msgSetPluginCtrlVal(m_track, genACnum(m_id, i), m_params[i].value);
    }

    CtrlValueType valueType() const
    {
        return VAL_LINEAR;
    }

    void setActive(bool active)
    {
        m_active = active;
    }
    
    void setName(QString name)
    {
        m_name = name;
    }

    void setParameterValue(uint32_t index, double value)
    {
        if (index < m_paramCount)
        {
            if (m_params[index].hints & PARAMETER_IS_INTEGER)
                value = rint(value);

            m_params[index].value    = value;
            m_params[index].tmpValue = value;
            m_params[index].update   = true;
            setNativeParameterValue(index, value);
        }
    }

    void setId(int id)
    {
        m_id = id;
    }

    void setChannels(int n)
    {
        m_channels = n;
    }

    void setTrack(AudioTrack* track)
    {
        m_track = track;
    }

    void aboutToRemove()
    {
        bool blocked = m_proc_lock.tryLock();
        m_enabled = false;
        if (blocked) m_proc_lock.unlock();
    }
    
    // needed for synths
    QString getAudioOutputPortName(uint32_t index);
    void processSynth(MPEventList* eventList);

    void makeGui();
    void deleteGui();
    void showGui(bool yesno);
    bool guiVisible();

    virtual void deleteMe() = 0;
    virtual bool hasNativeGui() = 0;
    virtual void showNativeGui(bool yesno) = 0;
    virtual bool nativeGuiVisible() = 0;
    virtual void updateNativeGui() = 0;

    virtual QString getParameterName(uint32_t index) = 0;
    virtual QString getParameterUnit(uint32_t index) = 0;
    virtual void setNativeParameterValue(uint32_t index, double value) = 0;

    virtual uint32_t getProgramCount() = 0;
    virtual QString getProgramName(uint32_t index) = 0;
    virtual void setProgram(uint32_t index) = 0;

    virtual bool init(QString filename, QString label) = 0;
    virtual void reload() = 0;
    virtual void reloadPrograms(bool init) = 0;

    virtual void process(uint32_t frames, float** src, float** dst, MPEventList* eventList) = 0;
    virtual void bufferSizeChanged(uint32_t bufferSize) = 0;

    virtual bool readConfiguration(Xml& xml, bool readPreset = false) = 0;
    virtual void writeConfiguration(int level, Xml& xml) = 0;

protected:
	PluginType m_type;
    unsigned int m_hints;

    QString m_name;
    QString m_label;
    QString m_maker;
    QString m_copyright;
    QString m_filename; // URI for LV2
    long m_uniqueId;

    bool m_active;
    bool m_activeBefore;

    uint32_t m_paramCount;
    ParameterPort* m_params;
    int32_t m_currentProgram;

    int m_id;
    int m_channels;
    AudioTrack* m_track;
    PluginGui* m_gui;

    bool m_enabled;
    void* m_lib;
    QMutex m_proc_lock;

    // synths only
    uint32_t m_ainsCount;
    uint32_t m_aoutsCount;
    void** m_portsIn;
    void** m_portsOut;
};

//---------------------------------------------------------
//   LADSPA Plugin
//---------------------------------------------------------

class LadspaPlugin : public BasePlugin
{
public:
    LadspaPlugin();
    ~LadspaPlugin();

    static void initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle);
    
    virtual void deleteMe()
    {
        delete this;
    }

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    QString getParameterName(uint32_t index);
    QString getParameterUnit(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);

    uint32_t getProgramCount();
    QString getProgramName(uint32_t index);
    void setProgram(uint32_t index);

    bool hasNativeGui();
    void showNativeGui(bool yesno);
    bool nativeGuiVisible();
    void updateNativeGui();

    void process(uint32_t frames, float** src, float** dst, MPEventList* eventList);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

    bool loadControl(Xml& xml);
    bool setControl(int32_t idx, QString oldName, double value);

protected:
    float* m_paramsBuffer;
    std::vector<unsigned long> m_audioInIndexes;
    std::vector<unsigned long> m_audioOutIndexes;

    LADSPA_Handle handle;
    const LADSPA_Descriptor* descriptor;
};

//---------------------------------------------------------
//   LV2 Plugin
//---------------------------------------------------------

class Lv2Plugin : public BasePlugin
{
public:    
    enum Lv2UiType {
        UI_NONE,
        UI_GTK2,
        UI_QT4,
        UI_X11,
        UI_EXTERNAL
    };

    // NOTE - we need to preserve this enum order
    // to be backwards compatible
    enum Lv2StateType {
        STATE_NULL   = 0,
        STATE_STRING = 1,
        STATE_BLOB   = 2
    };

    struct Lv2State {
        Lv2StateType type;
        const char* key;
        const char* value;
    };

    struct Lv2Event {
        uint32_t types;
        LV2_Event_Buffer* buffer;
    };

    Lv2Plugin();
    ~Lv2Plugin();

    static void initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle);
    
    virtual void deleteMe()
    {
        delete this;
    }

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    QString getParameterName(uint32_t index);
    QString getParameterUnit(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);

    uint32_t getProgramCount();
    QString getProgramName(uint32_t index);
    void setProgram(uint32_t index);
    
    uint32_t getCustomURIId(const char* uri);
    const char* getCustomURIString(int uri_id);

    void saveState(Lv2StateType type, const char* uri_key, const char* value);
    Lv2State* getState(const char* uri_key);

    bool hasNativeGui();
    void showNativeGui(bool yesno);
    bool nativeGuiVisible();
    void updateNativeGui();

    void closeNativeGui(bool destroyed = false);
    void updateGuiPorts(bool onlyOutput = false);

    void ui_resize(int width, int height);
    void ui_write_function(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer);

    void process(uint32_t frames, float** src, float** dst, MPEventList* eventList);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

    bool loadControl(Xml& xml);
    bool loadState(Xml& xml);
    bool setControl(QString symbol, QString oldName, double value);

private:
    float* m_paramsBuffer;
    std::vector<uint32_t> m_audioInIndexes;
    std::vector<uint32_t> m_audioOutIndexes;
    std::vector<Lv2Event> m_events;
    QList<const char*> m_customURIs;
    QList<Lv2State> m_lv2States;

    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    LV2_Feature* features[11]; //lv2_feature_count+1
    
    struct {
        Lv2UiType type;
        bool visible;
        int width;
        int height;
        void* lib;
        void* nativeWidget;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        QString bundlePath;
    } ui;

    const LilvPlugin* lplug;
};

//---------------------------------------------------------
//   VST Plugin
//---------------------------------------------------------

#define MAX_VST_EVENTS   512
#define VST_SANITY_CHECK 0xdeadbeef

typedef AEffect* (*VST_Function)(audioMasterCallback);

bool VstPluginCanDo(AEffect* effect, const char* feature);
intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

class VstPlugin : public BasePlugin
{
public:
    VstPlugin();
    ~VstPlugin();

    static void initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle);
    
    virtual void deleteMe()
    {
        delete this;
    }

    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    bool hasNativeGui();
    void showNativeGui(bool yesno);
    bool nativeGuiVisible();
    void updateNativeGui();
    void resizeNativeGui(int width, int height);

    QString getParameterName(uint32_t index);
    QString getParameterUnit(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);
    void setAutomatedParameterValue(uint32_t index, double value);

    uint32_t getProgramCount();
    QString getProgramName(uint32_t index);
    void setProgram(uint32_t index);
    void updateCurrentProgram();

    void process(uint32_t frames, float** src, float** dst, MPEventList* eventList);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

    bool loadParameter(Xml& xml);
    
    // used to check if 'user' handle is valid
    unsigned int sanityCheck;

protected:
    bool isOldSdk;
    AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_VST_EVENTS];
    } events;
    VstMidiEvent midiEvents[MAX_VST_EVENTS];

    QStringList m_programNames;

    struct {
        int width;
        int height;
        QWidget* widget;
    } ui;
};

//---------------------------------------------------------
//   SynthPluginDevice
//---------------------------------------------------------

class SynthPluginDevice :
        public MidiDevice,
        public MidiInstrument
{
public:
    SynthPluginDevice(PluginType type, QString filename, QString name, QString label, bool duplicated = false);
    ~SynthPluginDevice();

    SynthPluginDevice* clone(QString newName = QString(""))
    {
        SynthPluginDevice* synth = new SynthPluginDevice(m_type, m_filename, m_name, m_label, true);
        synth->setName(newName.isEmpty() ? name() : newName);
        midiDevices.add(synth);
        return synth;
    }

    virtual int deviceType()
    {
        return MidiDevice::SYNTH_MIDI;
    }

    virtual bool isSynthPlugin() const
    {
        return true;
    }

    bool duplicated() const
    {
        return m_duplicated;
    }

    BasePlugin* plugin()
    {
        return m_plugin;
    }

    const QString& name() const
    {
        return m_name;
    }

    QString filename()
    {
        return m_filename;
    }

    QString label()
    {
        return m_label;
    }

    virtual bool hasGui() const
    {
        return true;
    }
    
    AudioTrack* audioTrack() const
    {
        return m_audioTrack;
    }
    
    void setTrackId(qint64 id);

    virtual QString open();
    virtual void close();
    virtual void setName(const QString& s);
    void setPluginName(const QString& s);
    QString getAudioOutputPortName(uint32_t index);
    int32_t getCurrentProgram();

    virtual void writeRouting(int, Xml&) const;

    virtual void collectMidiEvents();
    virtual void processMidi();

    virtual bool guiVisible() const;
    virtual void showGui(bool yesno);

    bool hasNativeGui() const;
    bool nativeGuiVisible();
    void showNativeGui(bool yesno);
    void updateNativeGui();

    virtual void reset(int, MType);
    virtual QString getPatchName(int, int, MType, bool);
    virtual Patch* getPatch(int, int, MType, bool);
    virtual void populatePatchPopup(QMenu*, int, MType, bool);
    virtual void populatePatchModel(QStandardItemModel*, int, MType, bool);

    void read(Xml&);
    void write(int level, Xml&);

    MidiPlayEvent receiveEvent();
    int eventsPending() const;
    
    void segmentSizeChanged(unsigned newSegmentSize);

protected:
    virtual bool putMidiEvent(const MidiPlayEvent&);

private:
    PluginType m_type;
    QString m_filename;
    QString m_name;
    QString m_label;
    BasePlugin* m_plugin;
    bool m_duplicated;
    AudioTrack* m_audioTrack;
};

//---------------------------------------------------------
//   PluginI, simple class for itenerator list
//---------------------------------------------------------

class PluginI
{
public:
    PluginI(PluginType type, const QString& filename, const QString& label, const void* nativeHandle)
    {
        m_type = type;
        m_hints = 0;
        m_filename = QString(filename);
        m_label = QString(label);
        m_audioInputCount = 0;
        m_audioOutputCount = 0;

        if (type == PLUGIN_LADSPA)
            LadspaPlugin::initPluginI(this, filename, label, nativeHandle);

        else if (type == PLUGIN_LV2)
            Lv2Plugin::initPluginI(this, filename, label, nativeHandle);

        else if (type == PLUGIN_VST)
            VstPlugin::initPluginI(this, filename, label, nativeHandle);
    }

    PluginType type()
    {
        return m_type;
    }

    unsigned int hints()
    {
        return m_hints;
    }

    QString filename(bool complete = true)
    {
        if (m_type == PLUGIN_LV2 || complete)
            return m_filename;
        else
        {
            QFileInfo fi(m_filename);
            return fi.completeBaseName();
        }
    }

    QString name()
    {
        return m_name;
    }

    QString label()
    {
        return m_label;
    }

    QString maker()
    {
        return m_maker;
    }

    uint32_t getAudioInputCount()
    {
        return m_audioInputCount;
    }

    uint32_t getAudioOutputCount()
    {
        return m_audioOutputCount;
    }

    // needs public access for xPlugin::initPluginI()
    unsigned int m_hints;
    QString m_name;
    QString m_maker;
    uint32_t m_audioInputCount;
    uint32_t m_audioOutputCount;

private:
    PluginType m_type;
    QString m_filename; // uri for LV2
    QString m_label;
};

typedef std::list<PluginI>::iterator iPlugin;

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<PluginI>
{
public:
    PluginList()
    {
    }

    void add(PluginType type, QString filename, QString label, const void* nativeHandle)
    {
        push_back(PluginI(type, filename, label, nativeHandle));
    }

    PluginI* find(const QString& baseFilename, const QString& label)
    {
        for (iPlugin i = begin(); i != end(); ++i)
        {
            if ((i->filename(false) == baseFilename) && (i->label() == label))
                return &*i;
        }
        return 0;
    }
};

//---------------------------------------------------------
//   Set error for last loaded plugin
//---------------------------------------------------------

const char* get_last_error();
void set_last_error(const char* error);

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

const int PipelineDepth = 100;

class Pipeline : public std::vector<BasePlugin*>
{
public:
    Pipeline();
    ~Pipeline();

    int addPlugin(BasePlugin* plugin, int index);
    void remove(int index);
    void removeAll();

    bool isActive(int idx) const;
    void setActive(int, bool);

    void setChannels(int);

    QString label(int idx) const;
    QString name(int idx) const;

    bool empty(int idx) const;
    void move(int idx, bool up);
    void apply(int ports, uint32_t nframes, float** buffer);

    void showGui(int, bool);
    void deleteGui(int idx);
    void deleteAllGuis();
    bool guiVisible(int);

    bool hasNativeGui(int);
    void showNativeGui(int, bool);
    bool nativeGuiVisible(int);

    void updateGuis();

private:
    float* buffer[MAX_CHANNELS];
};

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;

//---------------------------------------------------------

extern PluginList plugins;

#endif

