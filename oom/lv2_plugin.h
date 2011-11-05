//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ladspa_plugin.h $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett 
//  	(info@openoctave.org)
//  Some functions in this file are
//  Copyright (C) 2005-2011, rncbc aka Rui Nuno Capela. All rights reserved.
//=========================================================

#include "plugin.h"
#ifdef LV2_SUPPORT
#ifndef _OOM_LV2_PLUGIN_H_
#define _OOM_LV2_PLUGIN_H_
#define OOM_LV2_MIDI_EVENT_ID 1
#ifdef LILV_SUPPORT
#ifdef LILV_STATIC
#include "lilv.h"
#else
#include <lilv/lilv.h>
#endif
#endif
#ifdef SLV2_SUPPORT
#include <slv2/slv2.h>
#endif
#include "lv2_data_access.h"
#include "lv2_ui.h"
#include "lv2_files.h"
#include "lv2_persist.h"
#include "lv2_external_ui.h"
#include <QHash>
#include <QList>
#include <QObject>
#define LV2_FIFO_SIZE 1024

#define UITYPE_QT4  1
#define UITYPE_GTK2 2
#define UITYPE_EXT  4

class QX11EmbedContainer;

struct LV2Data
{
	int frame;
	float value;
	unsigned time;
};

class LV2ControlFifo/*{{{*/
{
    LV2Data fifo[LV2_FIFO_SIZE];
    volatile int size;
    int wIndex;
    int rIndex;

public:

    LV2ControlFifo()
    {
        clear();
    }
    bool put(const LV2Data& event); // returns true on fifo overflow
    LV2Data get();
    const LV2Data& peek(int n = 0);
    void remove();

    bool isEmpty() const
    {
        return size == 0;
    }

    void clear()
    {
        size = 0, wIndex = 0, rIndex = 0;
    }

    int getSize() const
    {
        return size;
    }
};/*}}}*/

struct LV2World {/*{{{*/
#ifdef SLV2_SUPPORT
	SLV2World world;
	SLV2Plugins plugins;
	SLV2Instance instance;
	SLV2Value input_class;
	SLV2Value output_class;
	SLV2Value control_class;
	SLV2Value event_class;
	SLV2Value audio_class;
	SLV2Value midi_class;
	SLV2Value opt_class;
	SLV2Value qtui_class;
	SLV2Value gtkui_class;
	SLV2Value in_place_broken;
	SLV2Value toggle_prop;
	SLV2Value integer_prop;
	SLV2Value logarithmic_prop;
	SLV2Value samplerate_prop;
	SLV2Value units_prop;
	SLV2Value persist_prop;
#else
	LilvWorld* world;
	const LilvPlugins* plugins;
	LilvInstance* instance;
	LilvNode* input_class;
	LilvNode* output_class;
	LilvNode* control_class;
	LilvNode* event_class;
	LilvNode* audio_class;
	LilvNode* midi_class;
	LilvNode* opt_class;
	LilvNode* qtui_class;
	LilvNode* gtkui_class;
	LilvNode* in_place_broken;
	LilvNode* toggle_prop;
	LilvNode* integer_prop;
	LilvNode* logarithmic_prop;
	LilvNode* samplerate_prop;
	LilvNode* units_prop;
	LilvNode* persist_prop;
#endif
};/*}}}*/


class LV2Plugin : public Plugin
{
protected:
	void init(const char* uri);

private:
#ifdef SLV2_SUPPORT
	SLV2Plugin m_plugin;
#else
	LilvPlugin* m_plugin;
#endif
	bool m_config;
	int controlInputs;
	int controlOutputs;

public:
	LV2Plugin(const char* uri);
#ifdef SLV2_SUPPORT
	SLV2Plugin getPlugin();
#else
	const LilvPlugin* getPlugin();
#endif
	
	bool configSupport()
	{
		return m_config;
	}

	int inputPorts()
	{
		return controlInputs;
	}

	int outputPorts()
	{
		return controlOutputs;
	}

	//const LV2_Feature* const* features () { return m_features; }
    double defaultValue(unsigned int port) const;
    void lv2range(unsigned long i, float*, float*, float*) const;
    int updateReferences(int);
    const char* portName(unsigned long i);

	static const char* lv2_id_to_uri(uint32_t id);
	static uint32_t lv2_uri_to_id(const char *uri);
};

class LV2PluginI;

class LV2EventFilter : public QObject/*{{{*/
{
	Q_OBJECT
private slots:
	void closeWidget();

public:

	// Constructor.
	LV2EventFilter(LV2PluginI *p, QWidget *w);
	bool eventFilter(QObject*, QEvent*);

private:
	
	LV2PluginI *m_plugin;
	QWidget *m_widget;
};/*}}}*/

typedef QHash<QString, QString> Configs;
typedef QHash<QString, QString> ConfigTypes;
typedef QHash<unsigned long, float> Values;
typedef QList<LV2ControlFifo> ControlPipes;

class LV2PluginI : public PluginI
{
private:
	LV2UI_Widget   m_lv2_ui_widget;
	lv2_external_ui_host m_lv2_ui_external;
	QWidget* m_nativeui;
#ifdef GTK2UI_SUPPORT
	struct _GtkWidget *m_gtkWindow;
#endif
#ifdef SLV2_SUPPORT
	QList<SLV2Instance> m_instance;
	SLV2UIInstance m_slv2_ui_instance;
	SLV2UIs        m_slv2_uis;
	SLV2UI         m_slv2_ui;
#else
	QList<LilvInstance*> m_instance;
    void* m_ui_lib;
    LV2UI_Handle            m_uihandle;
	const LV2UI_Descriptor* m_uidescriptor;
#endif

	Configs m_configs;

	ConfigTypes m_ctypes;

	Values m_values;

	LV2Plugin* m_plugin;
	bool m_guiVisible;
	bool m_stop_process;
	int m_ui_type;
	LV2ControlFifo* m_controlFifo;
	LV2EventFilter* m_eventFilter;

	LV2_Feature  **m_features;
	LV2_Feature    m_data_access_feature;
	LV2_Feature    m_instance_access_feature;
	LV2_Extension_Data_Feature m_data_access;

	LV2_Feature    m_ui_feature;
	LV2_Feature  **m_ui_features;

	QHash<QString, QByteArray> m_persist_configs;
	QHash<QString, uint32_t>   m_persist_ctypes;

	LV2_Feature                m_files_path_feature;
	LV2_Files_Path_Support     m_files_path_support;
	LV2_Feature                m_files_new_file_feature;
	LV2_Files_New_File_Support m_files_new_file_support;

public:
	LV2PluginI();
	~LV2PluginI();
	LV2Plugin* plugin() {
		return m_plugin;
	}
	void setStop(bool f)
	{
		m_stop_process = f;
	}
#ifdef SLV2_SUPPORT
	SLV2Instance instantiatelv2();
#else
	LilvInstance* instantiatelv2();
#endif
	const LV2UI_Descriptor *lv2_ui_descriptor() const;
	LV2UI_Handle lv2_ui_handle() const;
	LV2_Handle lv2_handle(unsigned short );
	const LV2_Persist *lv2_persist_descriptor(unsigned short) const;

	int lv2_persist_store(uint32_t, const void *, size_t, uint32_t, uint32_t);
	const void *lv2_persist_retrieve(uint32_t, size_t *, uint32_t *, uint32_t *);


	void setConfigs(const Configs& configs)/*{{{*/
	{ 
		m_configs = configs;
	}
	const Configs& configs() const
	{
		return m_configs; 
	}

	void setConfig(const QString& key, const QString& value)
	{
		m_configs[key] = value;
	}
	const QString& config(const QString& key)
	{
		return m_configs[key];
	}


	void setConfigTypes(const ConfigTypes& ctypes)
	{
		m_ctypes = ctypes;
	}
	const ConfigTypes& configTypes() const
	{
		return m_ctypes;
	}

	void setConfigType(const QString& sKey, const QString& sType)
	{
		m_ctypes[sKey] = sType;
	}
	const QString& configType(const QString& sKey)
	{
		return m_ctypes[sKey];
	}

	void realizeConfigs();
	void freezeConfigs();
	void releaseConfigs();

	void setValues(const Values& values)
	{
		m_values = values;
	}
	const Values& values() const
	{
		return m_values;
	}

	void setValue(unsigned long iIndex, float fValue)
	{
		m_values[iIndex] = fValue;
	}
	float value(unsigned long iIndex) const
	{
		return m_values[iIndex];
	}

	void clearConfigs()
	{
		m_configs.clear();
		m_ctypes.clear();
	}
	void clearValues()
	{
		m_values.clear();
	}

	/*}}}*/

	LV2ControlFifo* getControlFifo(unsigned long p)
	{
		if(!m_controlFifo)
			return 0;
		return &m_controlFifo[p];
	}
    virtual bool initPluginInstance(Plugin*, int channels);
    virtual void connect(int ports, float** src, float** dst);
    virtual void activate();
    virtual void deactivate();
	void cleanup();
    virtual void apply(int n);
    virtual void setChannels(int);
	void showGui();
	void showGui(bool);
    virtual void showNativeGui();
    virtual void showNativeGui(bool);
	virtual bool nativeGuiVisible()
	{
		return m_guiVisible;
	}
    virtual double defaultValue(unsigned int port)
	{
		if(m_plugin)
			return m_plugin->defaultValue(port);
		return 0.0;
	}
	bool canProcess()
	{
		return !m_stop_process;
	}
	void makeGui();
	void makeNativeGui();
	void closeNativeGui(bool cleanup = false);
    virtual bool isAudioIn(int k);
    virtual bool isAudioOut(int k);
    virtual void range(int i, float* min, float* max) const
    {
		float def;
        m_plugin->lv2range(controls[i].idx, &def, min, max);
    }
	int paramIndex(int i)
	{
		return controls[i].idx;
	}
	void idlePlugin();
	void heartBeat();
    virtual void writeConfiguration(int level, Xml& xml);
    virtual bool readConfiguration(Xml& xml, bool readPreset = false);
    virtual bool loadControl(Xml& xml);
    virtual bool setControl(const QString& s, float val);
    virtual CtrlValueType valueType() const;
};
#endif
#endif
