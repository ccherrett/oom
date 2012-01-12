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

//#include <QUiLoader>

//#include "globaldefs.h"

//#include "stringparam.h"

//#include "config.h"

//class QAbstractButton;
//class QComboBox;
//class QRadioButton;
//class QScrollArea;
//class QToolButton;
//class QTreeWidget;

//class Slider;
//class DoubleLabel;
//class MidiController;

#if defined(__WIN32__) || defined(__WIN64__)
#include <windows.h>
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#else
#include <dlfcn.h>
#undef __cdecl
#define __cdecl
#endif

#include "audio.h"
#include "ctrl.h"
#include "globals.h"
#include "lib_functions.h"

// ladspa includes
#include "ladspa.h"

// lv2 includes
#include "lv2.h"
#include "lv2_ui.h"

#ifdef LILV_STATIC
#include "lilv/lilv.h"
#else
#include <lilv/lilv.h>
#endif

// vst includes
#include "pluginterfaces/vst2.x/aeffectx.h"

#include <list>
#include <vector>
#include <stdint.h>

#include <QFileInfo>

class AudioTrack;
class PluginI;
class PluginGui;
class Xml;

// plugin hints
const unsigned int PLUGIN_IS_SYNTH            = 0x01;
const unsigned int PLUGIN_IS_FX               = 0x02; // requires at least 1 audio port in and out
const unsigned int PLUGIN_HAS_NATIVE_GUI      = 0x04;
const unsigned int PLUGIN_HAS_IN_PLACE_BROKEN = 0x08;
const unsigned int PLUGIN_USES_CHUNKS         = 0x10;

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
        rindex = 0;
        hints  = 0;
        value  = 0.0;

        ranges.def = 0.0f;
        ranges.min = 0.0f;
        ranges.max = 1.0f;
        ranges.step = 0.001f;
        ranges.step_small = 0.0001f;
        ranges.step_large = 0.1f;

        enCtrl  = false;
        en2Ctrl = false;
    }

    void fix_current_value()
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
    }

    ParameterType type;
    unsigned int hints;
    int32_t rindex;
    double value;

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

        m_id = -1;
        m_track = 0;
        m_gui = 0;

        _lib = 0;
    }

    ~BasePlugin()
    {
        deleteGui();

        if (m_paramCount > 0)
            delete[] m_params;

        if (_lib)
            lib_close(_lib);
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

    uint32_t getParameterCount()
    {
        return m_paramCount;
    }

    ParameterPort* getParameter(uint32_t index)
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

    void setParameterValue(uint32_t index, double value)
    {
        if (index < m_paramCount)
        {
            m_params[index].value = value;

            setNativeParameterValue(index, value);
        }
    }

    void setId(int id)
    {
        m_id = id;
    }

    void setTrack(AudioTrack* track)
    {
        m_track = track;
    }

    void makeGui();
    void deleteGui();
    void showGui(bool yesno);
    bool guiVisible();

    // TODO - bool hasNativeGui, etc

    virtual QString getParameterName(uint32_t index) = 0;
    virtual void setNativeParameterValue(uint32_t index, double value) = 0;

    virtual bool init(QString filename, QString label) = 0;
    virtual void reload() = 0;
    virtual void reloadPrograms(bool init) = 0;

    virtual void process(uint32_t frames) = 0;
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

    int m_id;
    AudioTrack* m_track;

    PluginGui* m_gui;

    void* _lib;

#if 0
    // after here is all TODO
    void* _handle;
    int _references;
    int _instNo;
    QFileInfo fi;
    LADSPA_Descriptor_Function ladspa;
    const LADSPA_Descriptor *plugin;

    //LADSPA_PortDescriptor* _portDescriptors;
    unsigned long _portCount;
    unsigned long _inports;
    unsigned long _outports;
    unsigned long _controlInPorts;
    unsigned long _controlOutPorts;
    std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.

	virtual void initLadspa(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi = false);

public:
    QString lib(bool complete = true) /*const*/
    {
		QString rv;
		switch(m_type)
		{
            case PLUGIN_LV2:
				rv = m_uri;
			break;
			default:
        		if(complete)
					rv = fi.completeBaseName();
				else
					rv = fi.baseName();
			break;
		}
		return rv;
    } // ddskrjo const

    QString dirPath(bool complete = true) const
    {
        return complete ? fi.absolutePath() : fi.path();
    }

    QString filePath() const
    {
        return fi.filePath();
    }

    int references() const
    {
        return _references;
    }
    virtual int incReferences(int);

    int instNo()
    {
        return _instNo++;
    }

    LADSPA_Handle instantiate();

    void activate(LADSPA_Handle handle)
    {
        if (plugin && plugin->activate)
            plugin->activate(handle);
    }

    void deactivate(LADSPA_Handle handle)
    {
        if (plugin && plugin->deactivate)
            plugin->deactivate(handle);
    }

    void cleanup(LADSPA_Handle handle)
    {
        if (plugin && plugin->cleanup)
            plugin->cleanup(handle);
    }

    void connectPort(LADSPA_Handle handle, int port, float* value)
    {
        if (plugin)
            plugin->connect_port(handle, port, value);
    }

	//This is renamed to run(int n) for LV2Plugin::run();
    void apply(LADSPA_Handle handle, int n)
    {
        if (plugin)
            plugin->run(handle, n);
    }

    //int ports() { return plugin ? plugin->PortCount : 0; }

    unsigned long ports()
    {
        return _portCount;
    }

    LADSPA_PortDescriptor portd(unsigned long k) const
    {
        return plugin ? plugin->PortDescriptors[k] : 0;
        //return _portDescriptors[k];
    }

    LADSPA_PortRangeHint range(unsigned long i)
    {
        // FIXME:
        //return plugin ? plugin->PortRangeHints[i] : 0;
        return plugin->PortRangeHints[i];
    }

    virtual double defaultValue(unsigned long port) const;
    virtual void range(unsigned long i, float*, float*) const;

    virtual const char* portName(unsigned long i);

    // Returns (int)-1 if not an input control.

    virtual unsigned long port2InCtrl(unsigned long p)
    {
        return p >= rpIdx.size() ? (unsigned long) - 1 : rpIdx[p];
    }

    unsigned long inports() const
    {
        return _inports;
    }

    unsigned long outports() const
    {
        return _outports;
    }

    unsigned long controlInPorts() const
    {
        return _controlInPorts;
    }

    unsigned long controlOutPorts() const
    {
        return _controlOutPorts;
    }
#endif
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

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    QString getParameterName(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);

    void connect(int ports, float** src, float** dst);

    void process(uint32_t frames);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

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
    Lv2Plugin();
    ~Lv2Plugin();

    static void initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle);

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    QString getParameterName(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);

    void process(uint32_t frames);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

    uint32_t getCustomURIId(const char* uri);
    const char* getCustomURIString(int uri_id);

private:
    float* m_paramsBuffer;
    std::vector<unsigned long> m_audioInIndexes;
    std::vector<unsigned long> m_audioOutIndexes;
    QList<const char*> m_customURIs;

    struct {
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        void* _lib;
    } ui;

    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    LV2_Feature* features[11]; //lv2_feature_count+1

    const LilvPlugin* lplug;
};

//---------------------------------------------------------
//   VST Plugin
//---------------------------------------------------------
#define MAX_VST_EVENTS 512

typedef AEffect* (*VST_Function)(audioMasterCallback);

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);

class VstPlugin : public BasePlugin
{
public:
    VstPlugin();
    ~VstPlugin();

    static void initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle);

    bool init(QString filename, QString label);
    void reload();
    void reloadPrograms(bool init);

    QString getParameterName(uint32_t index);
    void setNativeParameterValue(uint32_t index, double value);

    void process(uint32_t frames);
    void bufferSizeChanged(uint32_t bufferSize);

    bool readConfiguration(Xml& xml, bool readPreset);
    void writeConfiguration(int level, Xml& xml);

protected:
    AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_VST_EVENTS];
    } events;
    VstMidiEvent midiEvents[MAX_VST_EVENTS];
    bool isOldSdk;
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
        {
            LadspaPlugin::initPluginI(this, filename, label, nativeHandle);
        }
        else if (type == PLUGIN_LV2)
        {
            Lv2Plugin::initPluginI(this, filename, label, nativeHandle);
        }
        else if (type == PLUGIN_VST)
        {
            VstPlugin::initPluginI(this, filename, label, nativeHandle);
        }
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

    void insert(BasePlugin* plugin, int index);
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
    void showNativeGui(int, bool);
    void deleteGui(int idx);
    void deleteAllGuis();
    bool guiVisible(int);
    bool hasNativeGui();
    bool nativeGuiVisible(int);
    void updateNativeGui();

private:
    float* buffer[MAX_CHANNELS];
};

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;

extern PluginList plugins;

#if 0
//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port
{
    int idx;
    float val;
    float tmpVal;
    float lastGuiVal;

    bool enCtrl; // Enable controller stream.
    bool en2Ctrl; // Second enable controller stream (and'ed with enCtrl).
	bool logarithmic;
	bool isInt;
	bool toggle;
	bool samplerate;
	bool update;
	float min;
	float max;
	std::string name;
	/*Port(){
		name = "Unknown";
	}*/
};

class PluginI;

//---------------------------------------------------------
//   PluginIBase 
//---------------------------------------------------------

class PluginIBase
{
public:
    virtual long pluginUniqueId() const = 0;

    virtual bool on() const = 0;
    virtual void setOn(bool /*val*/) = 0;
    virtual int id() = 0;
    virtual QString pluginLabel() const = 0;
    virtual QString name() const = 0;

    virtual AudioTrack* track() = 0;

    virtual void enableController(int /*i*/, bool v = true) = 0;
    virtual bool controllerEnabled(int /*i*/) const = 0;
    virtual bool controllerEnabled2(int /*i*/) const = 0;
    virtual void updateControllers() = 0;

    virtual void writeConfiguration(int /*level*/, Xml& /*xml*/) = 0;
    virtual bool readConfiguration(Xml& /*xml*/, bool readPreset = false) = 0;

    virtual int parameters() const = 0;
    virtual void setParam(int /*i*/, double /*val*/) = 0;
    virtual double param(int /*i*/) const = 0;
    virtual const char* paramName(int /*i*/) = 0;
    virtual Port* getControlPort(int) = 0;
	//virtual LADSPA_PortRangeHint range(int /*i*/) = 0;
};

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

#define AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

class PluginI : public PluginIBase
{
protected:
    Plugin* _plugin;
    AudioTrack* _track;
    int _id;

    int channel;
    int instances;

    QString _name;
    QString _label;

    int controlPorts;
    int controlOutPorts;
    bool _on;
    bool initControlValues;
    Port* controls;
    Port* controlsOut;

    PluginGui* _gui;
    bool _showNativeGuiPending;

    virtual void init();
    virtual void makeGui();

private:
    LADSPA_Handle* handle; // per instance

public:
    PluginI();
    virtual ~PluginI();

    Plugin* plugin() const
    {
        return _plugin;
    }

    int pluginHints() const
    {
        if (_plugin)
            return _plugin->hints();
        else
            return 0;
    }

    long pluginUniqueId() const
    {
        if (_plugin)
            return _plugin->uniqueId();
        else
            return 0;
    }

	int type()
	{
		return m_type;
	}

    bool on() const
    {
        return _on;
    }

    void setOn(bool val)
    {
        _on = val;
    }

    PluginGui* gui() const
    {
        return _gui;
    }
    void deleteGui();

    void setTrack(AudioTrack* t)
    {
        _track = t;
    }

    AudioTrack* track()
    {
        return _track;
    }

    void setID(int i);

    int id()
    {
        return _id;
    }
    void updateControllers();

    virtual bool initPluginInstance(Plugin*, int channels);
    virtual void setChannels(int);
    virtual void connect(int ports, float** src, float** dst);
    virtual void apply(int n);

    void enableController(int i, bool v = true)
    {
        controls[i].enCtrl = v;
    }

    bool controllerEnabled(int i) const
    {
        return controls[i].enCtrl;
    }

    void enable2Controller(int i, bool v = true)
    {
        controls[i].en2Ctrl = v;
    }

    bool controllerEnabled2(int i) const
    {
        return controls[i].en2Ctrl;
    }
    void enableAllControllers(bool v = true);
    void enable2AllControllers(bool v = true);

    virtual void activate();
    virtual void deactivate();

    QString pluginLabel() const
    {
        return _plugin->label();
    }

    QString label() const
    {
        return _label;
    }

    QString name() const
    {
        return _name;
    }
    virtual CtrlValueType valueType() const;

    QString lib() const
    {
        return _plugin->lib();
    }

#ifdef OSC_SUPPORT

    OscEffectIF& oscIF()
    {
        return _oscif;
    }

    int oscControl(unsigned long /*dssiPort*/, float /*val*/);
    int oscConfigure(const char */*key*/, const char */*val*/);
    int oscUpdate();
    //int oscExiting();
#endif

    virtual void writeConfiguration(int level, Xml& xml);
    virtual bool readConfiguration(Xml& xml, bool readPreset = false);
    virtual bool loadControl(Xml& xml);
    virtual bool setControl(const QString& s, double val);
    virtual void showGui();
    virtual void showGui(bool);

    virtual void showNativeGui();
    virtual void showNativeGui(bool);

    bool isShowNativeGuiPending()
    {
        return _showNativeGuiPending;
    }
    bool guiVisible();
    virtual bool nativeGuiVisible();

    int parameters() const
    {
        return controlPorts;
    }

    void setParam(int i, double val)
    {
        controls[i].tmpVal = val;
    }

    double param(int i) const
    {
        return controls[i].val;
    }
    virtual double defaultValue(unsigned int param) const;

    const char* paramName(int i)
    {
        if (m_type == PLUGIN_LV2)
        	return controls[i].name.c_str();
		else
			return _plugin->portName(controls[i].idx);
    }

    LADSPA_PortDescriptor portd(int i) const
    {
        return _plugin->portd(controls[i].idx);
    }

    virtual void range(int i, float* min, float* max) const
    {
        _plugin->range(controls[i].idx, min, max);
    }

    virtual bool isAudioIn(int k);

    virtual bool isAudioOut(int k);

    /*LADSPA_PortRangeHint range(int i)
    {
        return _plugin->range(controls[i].idx);
    }*/

	Port* getControlPort(int i)
	{
		return &controls[i];
	}
};/*}}}*/

extern void initPlugins(bool ladspa, bool lv2, bool vst);

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def);
//extern MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val);
#endif

#endif

