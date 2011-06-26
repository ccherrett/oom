//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <list>
#include <vector>

#include <QFileInfo>
#include <QUiLoader>

#include "ladspa.h"
#include "globals.h"
#include "globaldefs.h"
#include "ctrl.h"

//#include "stringparam.h"

#include "config.h"

#ifdef OSC_SUPPORT
//class OscIF;
#include "osc.h"
#endif

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif

class QAbstractButton;
class QComboBox;
class QRadioButton;
class QScrollArea;
class QToolButton;
class QTreeWidget;

class Xml;
class Slider;
class DoubleLabel;
class AudioTrack;
class MidiController;
class PluginGui;

//---------------------------------------------------------
//   PluginLoader
//---------------------------------------------------------

class PluginLoader : public QUiLoader
{
public:
    virtual QWidget* createWidget(const QString & className, QWidget * parent = 0, const QString & name = QString());

    PluginLoader(QObject * parent = 0) : QUiLoader(parent)
    {
    }
};

enum PluginType {
	LADSPA = 0,
	DSSI,
	LV2
};

typedef int PluginHint;
#define Toggle 0x4
#define SampleRate 0x8
#define Logarithmic 0x10
#define Integer 0x20

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin/*{{{*/
{
protected:
	PluginType m_type;
    void* _handle;
    int _references;
    int _instNo;
    QFileInfo fi;
    LADSPA_Descriptor_Function ladspa;
    const LADSPA_Descriptor *plugin;
    unsigned long _uniqueID;
    QString _label;
    QString _name;
    QString _maker;
    QString _copyright;
	QString m_uri;

    bool _isDssi;
#ifdef DSSI_SUPPORT
    const DSSI_Descriptor* dssi_descr;
#endif

    //LADSPA_PortDescriptor* _portDescriptors;
    unsigned long _portCount;
    unsigned long _inports;
    unsigned long _outports;
    unsigned long _controlInPorts;
    unsigned long _controlOutPorts;
    std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.

    bool _inPlaceCapable;
	virtual void initLadspa(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi = false);

public:
    Plugin(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi = false);
    Plugin(PluginType t, const char* a);
    ~Plugin();

    QString label() const
    {
        return _label;
    }

    QString name() const
    {
        return _name;
    }

    unsigned long id() const
    {
        return _uniqueID;
    }

    QString maker() const
    {
        return _maker;
    }

    QString copyright() const
    {
        return _copyright;
    }

	int type() {return m_type;}

	virtual QString uri()
	{
		return m_uri;
	}

    QString lib(bool complete = true) /*const*/
    {
		QString rv;
		switch(m_type)
		{
			case LV2:
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

    bool isDssiPlugin() const
    {
        return _isDssi;
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

#ifdef OSC_SUPPORT
    int oscConfigure(LADSPA_Handle /*handle*/, const char* /*key*/, const char* /*value*/);
#endif

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

    bool inPlaceCapable() const
    {
        return _inPlaceCapable;
    }
};/*}}}*/

typedef std::list<Plugin>::iterator iPlugin;

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<Plugin>/*{{{*/
{
public:

    void add(QFileInfo* fi, const LADSPA_Descriptor* d, bool isDssi = false)
    {
        push_back(Plugin(fi, d, isDssi));
    }

    Plugin* find(const QString&, const QString&);

    PluginList()
    {
    }
};/*}}}*/

//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port
{
    int idx;
    float val;
    float tmpVal;

    bool enCtrl; // Enable controller stream.
    bool en2Ctrl; // Second enable controller stream (and'ed with enCtrl).
	bool logarithmic;
	bool isInt;
	bool toggle;
	bool samplerate;
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

class PluginIBase/*{{{*/
{
public:
    virtual bool on() const = 0;
    virtual void setOn(bool /*val*/) = 0;
    virtual int pluginID() = 0;
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
};/*}}}*/

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

#define AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

class PluginI : public PluginIBase/*{{{*/
{
protected:
	int m_type; //redundant type until we get all the code moved to using type
    Plugin* _plugin;
    int channel;
    int instances;
    AudioTrack* _track;
    int _id;

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

#ifdef OSC_SUPPORT
    OscEffectIF _oscif;
#endif

public:
    PluginI();
    virtual ~PluginI();

    Plugin* plugin() const
    {
        return _plugin;
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

    int pluginID()
    {
        return _plugin->id();
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
    CtrlValueType valueType() const;

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

    void writeConfiguration(int level, Xml& xml);
    bool readConfiguration(Xml& xml, bool readPreset = false);
    bool loadControl(Xml& xml);
    bool setControl(const QString& s, double val);
    virtual void showGui();
    virtual void showGui(bool);

    bool isDssiPlugin() const
    {
        return _plugin->isDssiPlugin();
    }
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
		if(m_type == 2)
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

    bool inPlaceCapable() const
    {
        return _plugin->inPlaceCapable();
    }

    /*LADSPA_PortRangeHint range(int i)
    {
        return _plugin->range(controls[i].idx);
    }*/

	Port* getControlPort(int i)
	{
		return &controls[i];
	}
};/*}}}*/

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

const int PipelineDepth = 100;

class Pipeline : public std::vector<PluginI*>/*{{{*/
{
    float* buffer[MAX_CHANNELS];

public:
    Pipeline();
    ~Pipeline();

    void insert(PluginI* p, int index);
    void remove(int index);
    void removeAll();
    bool isOn(int idx) const;
    void setOn(int, bool);
    QString label(int idx) const;
    QString name(int idx) const;
    void showGui(int, bool);
    bool isDssiPlugin(int) const;
    void showNativeGui(int, bool);
    void deleteGui(int idx);
    void deleteAllGuis();
    bool guiVisible(int);
    bool nativeGuiVisible(int);
    void apply(int ports, unsigned long nframes, float** buffer);
    void move(int idx, bool up);
    bool empty(int idx) const;
    void setChannels(int);
	void updateNativeGui();
};/*}}}*/

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;


extern void initPlugins();
//extern void initLV2();
extern PluginList plugins;

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def);
//extern MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val);

#endif

