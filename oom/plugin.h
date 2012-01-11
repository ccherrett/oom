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

//#include <vector>

//#include <QFileInfo>
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
//class PluginGui;

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

#include <ladspa.h>

#include <stdint.h>
#include <list>

class AudioTrack;
class Xml;

// plugin hints
const unsigned int PLUGIN_HAS_GUI            = 0x1;
const unsigned int PLUGIN_IS_SYNTH           = 0x2;
const unsigned int PLUGIN_IS_IN_PLACE_BROKEN = 0x4;
const unsigned int PLUGIN_USES_CHUNKS        = 0x8;

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
    int32_t rindex;
    int32_t hints;
    double value;

    struct {
        double def;
        float min;
        float max;
        float step;
        float step_small;
        float step_large;
    } ranges;

    bool enCtrl;
    bool en2Ctrl;
};

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin
{
public:
    Plugin()
    {
        m_type = PLUGIN_NONE;
        m_hints = 0;
        m_uniqueId = 0;

        m_active = false;
        m_active_before = false;

        m_audioInputCount = 0;
        m_audioOutputCount = 0;
        m_paramCount = 0;
        m_params = 0;

        m_id = -1;
        m_track = 0;

        _lib = 0;
    }

    ~Plugin()
    {
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

    QString filename(bool complete = true) const
    {
        if (m_type == PLUGIN_LV2)
            return m_filename;
        else if (complete)
            return m_filename;
        else
            // TODO
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

    uint32_t getAudioInputCount()
    {
        return m_audioInputCount;
    }

    uint32_t getAudioOutputCount()
    {
        return m_audioOutputCount;
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

    virtual bool readConfiguration(Xml& xml, bool readPreset = false);
    virtual void writeConfiguration(int level, Xml& xml);

    virtual QString getParameterName(uint32_t index);
    virtual void setNativeParameterValue(uint32_t index, double value);

    virtual void reload();
    virtual void reload_programs(bool init);
    virtual void process(uint32_t frames);
    virtual void buffer_size_changed(uint32_t bufferSize);

protected:
	PluginType m_type;
    int32_t m_hints;

    QString m_name;
    QString m_label;
    QString m_maker;
    QString m_copyright;
    QString m_filename; // URI for LV2
    long m_uniqueId;

    bool m_active;
    bool m_active_before;

    uint32_t m_audioInputCount;
    uint32_t m_audioOutputCount;

    uint32_t m_paramCount;
    ParameterPort* m_params;

    int m_id;
    AudioTrack* m_track;

private:
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

    bool lib_open(const char* filename)
    {
#ifdef __WINDOWS__
        _lib = LoadLibraryA(filename);
#else
        _lib = dlopen(filename, RTLD_NOW);
#endif
        return bool(_lib);
    }

    bool lib_close()
    {
        if (_lib)
#ifdef __WINDOWS__
            return FreeLibrary((HMODULE)_lib) != 0;
#else
            return dlclose(_lib) != 0;
#endif
        else
            return false;
    }

    void* lib_symbol(const char* symbol)
    {
        if (_lib)
#ifdef __WINDOWS__
            return (void*)GetProcAddress((HMODULE)_lib, symbol);
#else
            return dlsym(_lib, symbol);
#endif
        else
            return 0; //nullptr;
    }

    const char* lib_error()
    {
#ifdef __WINDOWS__
        return "Unknown error";
#else
        return dlerror();
#endif
    }
};

//---------------------------------------------------------
//   LADSPA Plugin
//---------------------------------------------------------

class LadspaPlugin : public Plugin
{
public:
    LadspaPlugin()
    {
        handle = 0;
        descriptor = 0;
    }

    LadspaPlugin(LadspaPlugin* plugin)
    {
    }

    QString getParameterName(uint32_t index)
    {
        if (index < m_paramCount)
            return QString("");
        else
            return QString("");
    }

    void setNativeParameterValue(uint32_t index, double value)
    {

    }

    void reload()
    {

    }

    void reload_programs(bool init)
    {

    }

    void process(uint32_t frames)
    {
    }

    void buffer_size_changed(uint32_t bufferSize)
    {
    }

    bool readConfiguration(Xml& xml, bool readPreset)
    {

    }

    void writeConfiguration(int level, Xml& xml)
    {

    }

protected:
    LADSPA_Handle handle;
    const LADSPA_Descriptor* descriptor;
};

//---------------------------------------------------------
//   LV2 Plugin
//---------------------------------------------------------

class Lv2Plugin : public Plugin
{
public:
    Lv2Plugin()
    {
    }

    Lv2Plugin(Lv2Plugin* plugin)
    {
    }

    QString getParameterName(uint32_t index)
    {
        if (index < m_paramCount)
            return QString("");
        else
            return QString("");
    }

    void setNativeParameterValue(uint32_t index, double value)
    {

    }

    void reload()
    {

    }

    void reload_programs(bool init)
    {

    }

    void process(uint32_t frames)
    {
    }

    void buffer_size_changed(uint32_t bufferSize)
    {
    }

    bool readConfiguration(Xml& xml, bool readPreset)
    {

    }

    void writeConfiguration(int level, Xml& xml)
    {

    }
};

//---------------------------------------------------------
//   VST Plugin
//---------------------------------------------------------

class VstPlugin : public Plugin
{
public:
    VstPlugin()
    {
        //effect = 0;
        events.numEvents = 0;
        events.reserved  = 0;
        is_oldsdk = false;
    }

    VstPlugin(VstPlugin* plugin)
    {
    }

    QString getParameterName(uint32_t index)
    {
        if (index < m_paramCount)
            return QString("");
        else
            return QString("");
    }

    void setNativeParameterValue(uint32_t index, double value)
    {

    }

    void reload()
    {

    }

    void reload_programs(bool init)
    {

    }

    void process(uint32_t frames)
    {
    }

    void buffer_size_changed(uint32_t bufferSize)
    {
    }

    bool readConfiguration(Xml& xml, bool readPreset)
    {

    }

    void writeConfiguration(int level, Xml& xml)
    {

    }

protected:
    //AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        //VstEvent* events[MAX_MIDI_BUFFER];
    } events;
    //VstMidiEvent midi_events[MAX_MIDI_BUFFER];
    bool is_oldsdk;
};

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

typedef std::list<Plugin>::iterator iPlugin;

class PluginList : public std::list<Plugin>
{
public:
    PluginList()
    {
    }

    void add(LadspaPlugin* plugin)
    {
        push_back(LadspaPlugin(plugin));
    }

    void add(Lv2Plugin* plugin)
    {
        push_back(Lv2Plugin(plugin));
    }

    void add(VstPlugin* plugin)
    {
        push_back(VstPlugin(plugin));
    }

    Plugin* find(const QString& filename, const QString& label)
    {
        for (iPlugin i = begin(); i != end(); ++i)
        {
            if ((i->filename() == filename) && (i->label() == label))
                return &*i;
        }
        return 0;
    }
};

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

const int PipelineDepth = 100;

class Pipeline : public std::vector<Plugin*>
{
public:
    Pipeline();
    ~Pipeline();

    void insert(Plugin* plugin, int index);
    void remove(int index);
    void removeAll();

    bool isActive(int idx) const;
    void setActive(int, bool);

    void setChannels(int);

    QString label(int idx) const;
    QString name(int idx) const;

    void showGui(int, bool);
    void showNativeGui(int, bool);
    void deleteGui(int idx);
    void deleteAllGuis();
    bool guiVisible(int);
    bool nativeGuiVisible(int);
    void updateNativeGui();

    void apply(int ports, uint32_t nframes, float** buffer);
    void move(int idx, bool up);
    bool empty(int idx) const;

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


extern void initPlugins();
//extern void initLV2();

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def);
//extern MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val);
#endif

#endif

