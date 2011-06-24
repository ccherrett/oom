//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ladspa_plugin.h $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett 
//  	(info@openoctave.org)
//=========================================================

//#ifdef LV2_SUPPORT
#ifndef _OOM_LV2_PLUGIN_H_
#define _OOM_LV2_PLUGIN_H_
#define OOM_LV2_MIDI_EVENT_ID 1
#include "plugin.h"
#include <lilv/lilv.h>
#include <suil/suil.h>
#include <QHash>
#include <QList>
#include <QObject>
#define LV2_FIFO_SIZE 1024

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
};/*}}}*/


class LV2Plugin : public Plugin
{
protected:
	void init(const char* uri);

private:
	//LV2_Feature*     m_features[9];
	//LV2_Feature m_data_access_feature;
	//LV2_Feature m_instance_access_feature;
	//LV2_Feature m_path_support_feature;
	//LV2_Feature m_new_file_support_feature;
	//LV2_Feature m_persist_feature;
	//LV2_Feature m_external_ui_feature;

	LilvPlugin* m_plugin;

public:
	LV2Plugin(const char* uri);
	const LilvPlugin* getPlugin();
	LilvInstance* instantiatelv2();
	//const LV2_Feature* const* features () { return m_features; }
    double defaultValue(unsigned int port) const;
    void lv2range(unsigned long i, float*, float*, float*) const;
    int updateReferences(int);
    const char* portName(unsigned long i);

	static const char* lv2_id_to_uri(uint32_t id);
	static uint32_t lv2_uri_to_id(const char *uri);
};

typedef std::list<LV2Plugin>::iterator iLV2Plugin;

//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class LV2PluginList : public std::list<LV2Plugin>/*{{{*/
{
public:

    void add(const char* uri)
    {
        push_back(LV2Plugin(uri));
    }

    LV2Plugin* find(const QString&, const QString&);

    LV2PluginList()
    {
    }
};/*}}}*/

class LV2PluginI : public PluginI
{
private:
	QList<LilvInstance*> m_instance;
	QList<SuilInstance*> m_uinstance;
	LV2Plugin* m_plugin;
	QWidget* m_nativeui;
	bool m_guiVisible;
	LV2ControlFifo* m_controlFifo;
	//QList<LilvInstance*> m_instance;
public:
	LV2PluginI();
	~LV2PluginI();
	LV2Plugin* plugin() {
		return m_plugin;
	}
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
	void makeGui();
	void makeNativeGui();
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
	void heartBeat();
};

extern LV2PluginList lv2plugins;
#endif
//#endif
