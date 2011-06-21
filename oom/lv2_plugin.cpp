//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ladspa_plugin.cpp $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett 
//  	(info@openoctave.org)
//=========================================================

#include <QDialog>
#include <string.h>

#include "track.h"
#include "lv2_plugin.h"
#include "plugingui.h"

#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/files/files.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/uri-unmap/uri-unmap.h"
#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"
#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#define LV2_GTK_UI_URI "http://lv2plug.in/ns/extensions/ui#GtkUI"
#define LV2_QT4_UI_URI "http://lv2plug.in/ns/extensions/ui#Qt4UI"
#define LV2_NS_UI   "http://lv2plug.in/ns/extensions/ui#"

static QHash<QString, uint32_t>    uri_map;
static QHash<uint32_t, QByteArray> ids_map;

LV2PluginList lv2plugins;

static uint32_t lv2_uri_to_id(LV2_URI_Map_Callback_Data /*data*/, const char *map, const char *uri )
{
	if ((map && strcmp(map, LV2_EVENT_URI) == 0) && strcmp(uri, LILV_URI_MIDI_EVENT) == 0)
	{
	    return OOM_LV2_MIDI_EVENT_ID;
	}
	return LV2Plugin::lv2_uri_to_id(uri);
}

static LV2_URI_Map_Feature lv2_uri_map = { NULL, lv2_uri_to_id };
static LV2_Feature lv2_uri_map_feature = { LV2_URI_MAP_URI, & lv2_uri_map };

static const char *lv2_id_to_uri (LV2_URI_Unmap_Callback_Data /*data*/, const char *map, uint32_t id )
{
	if ((map && strcmp(map, LV2_EVENT_URI) == 0) && id == OOM_LV2_MIDI_EVENT_ID)
	    return LILV_URI_MIDI_EVENT;

	return LV2Plugin::lv2_id_to_uri(id);
}

static LV2_URI_Unmap_Feature lv2_uri_unmap = { NULL, lv2_id_to_uri };
static LV2_Feature lv2_uri_unmap_feature = { LV2_URI_UNMAP_URI, &lv2_uri_unmap };

//static LV2_Feature data_access_feature = {"http://lv2plug.in/ns/ext/data-access", NULL};
static LV2_Feature instance_access_feature = {"http://lv2plug.in/ns/ext/instance-access", NULL};
//static LV2_Feature path_support_feature = {LV2_FILES_PATH_SUPPORT_URI, NULL};
//static LV2_Feature new_file_support_feature = {LV2_FILES_NEW_FILE_SUPPORT_URI, NULL};
//static LV2_Feature persist_feature = {"http://lv2plug.in/ns/ext/persist", NULL};
static LV2_Feature external_ui_feature = {LV2_NS_UI "external", NULL};
		
static const LV2_Feature* features[5] = { 
	//&data_access_feature,
	&instance_access_feature,
	//&path_support_feature,
	//&new_file_support_feature,
	//&persist_feature,
	&lv2_uri_map_feature,
	&lv2_uri_unmap_feature,
	&external_ui_feature,
    NULL
};

static LV2World* lv2world;

void initLV2()/*{{{*/
{
	lv2world = new LV2World;
	lv2world->world = lilv_world_new();
	lilv_world_load_all(lv2world->world);

	lv2world->plugins = lilv_world_get_all_plugins(lv2world->world);

	lv2world->input_class = lilv_new_uri(lv2world->world, LILV_URI_INPUT_PORT);
	lv2world->output_class = lilv_new_uri(lv2world->world, LILV_URI_OUTPUT_PORT);
	lv2world->control_class = lilv_new_uri(lv2world->world, LILV_URI_CONTROL_PORT);
	lv2world->event_class = lilv_new_uri(lv2world->world, LILV_URI_EVENT_PORT);
	lv2world->audio_class = lilv_new_uri(lv2world->world, LILV_URI_AUDIO_PORT);
	lv2world->midi_class = lilv_new_uri(lv2world->world, LILV_URI_MIDI_EVENT);

	lv2world->opt_class = lilv_new_uri(lv2world->world, LILV_NS_LV2 "connectionOptional");
	lv2world->qtui_class = lilv_new_uri(lv2world->world, LV2_QT4_UI_URI);
	lv2world->gtkui_class = lilv_new_uri(lv2world->world, LV2_GTK_UI_URI);
	lv2world->in_place_broken = lilv_new_uri(lv2world->world, LILV_NS_LV2 "inPlaceBroken");

	lv2world->toggle_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "toggled");    
	lv2world->integer_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "integer");
	lv2world->samplerate_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "sampleRate");
	lv2world->logarithmic_prop = lilv_new_uri(lv2world->world, "http://lv2plug.in/ns/dev/extportinfo#logarithmic");

	//printf("initLV2()\n");
	printf("Found %d LV2 Plugins\n", lilv_plugins_size(lv2world->plugins));
	LILV_FOREACH(plugins, i, lv2world->plugins) 
	{
		const LilvPlugin* p = lilv_plugins_get(lv2world->plugins, i);
		//lv2world->plugin_list.insert(QString(lilv_node_as_string(lilv_plugin_get_uri(p))), p);
		const char* curi = lilv_node_as_string(lilv_plugin_get_uri(p));
		//printf(" uri: %s\n", curi);
        plugins.push_back(LV2Plugin(curi));
		//lv2plugins.add(curi);
	}
	printf("Master plugin list contains %d plugins", (int)plugins.size());
}/*}}}*/


LV2Plugin::LV2Plugin(const char* uri)
: Plugin(LV2, uri)
{
	init(uri);
}

void LV2Plugin::init(const char* uri)/*{{{*/
{
	//plugin = NULL;
	//ladspa = NULL;
	_handle = 0;
	_references = 0;
	_instNo = 0;
	_copyright = QString("Unknown");

	_portCount = 0; 
	_inports = 0;
	_outports = 0;
	_controlInPorts = 0;
	_controlOutPorts = 0;
	m_uri = QString(uri);

	_inPlaceCapable = false;
	if(lv2world)
	{
		//const LilvPlugin *p = lv2world->plugin_list.value(QString(uri));
		LilvNode* plugin_uri = lilv_new_uri(lv2world->world, uri);
		const LilvPlugin *p = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
		lilv_node_free(plugin_uri);
		if(p)
		{
			m_plugin = const_cast<LilvPlugin*>(p);
			if(m_plugin && debugMsg)
				printf("LV2Plugin::init Found LilvPlugin, setting m_plugin\n");
			LilvNode* name = lilv_plugin_get_name(m_plugin);
			if(name)
			{
				_name = QString(lilv_node_as_string(name));
				lilv_node_free(name);
				//printf("Found plugin: %s\n", _name.toLatin1().constData());
			}
			LilvNode* lvmaker = lilv_plugin_get_author_name(m_plugin);
			_maker = QString(lvmaker ? lilv_node_as_string(lvmaker) : "Unknown");
			lilv_node_free(lvmaker);
			//QString uri(lilv_node_as_string(lilv_plugin_get_uri(p)));
			const LilvPluginClass* pclass = lilv_plugin_get_class(m_plugin);
			const LilvNode* clabel = lilv_plugin_class_get_label(pclass);
			_label = QString(lilv_node_as_string(clabel));
			//printf("Found label: %s\n", _label.toLatin1().constData());
			//lilv_node_free(clabel);
			_uniqueID = (unsigned long)lv2_uri_to_id(lilv_node_as_string(lilv_plugin_get_uri(m_plugin)));

			_portCount = lilv_plugin_get_num_ports(m_plugin);
			
			for(int i = 0; i < (int)_portCount; ++i)
			{
				const LilvPort* port = lilv_plugin_get_port_by_index(m_plugin, i);
				if(port)
				{
					if(lilv_port_is_a(m_plugin, port, lv2world->control_class))//Controll
					{
						if(lilv_port_is_a(m_plugin, port, lv2world->input_class))
						{
							++_controlInPorts;;
						}
						else if(lilv_port_is_a(m_plugin, port, lv2world->output_class))
						{
							++_controlOutPorts;
						}
					}
					else if(lilv_port_is_a(m_plugin, port, lv2world->audio_class))//Audio I/O
					{
						if(lilv_port_is_a(m_plugin, port, lv2world->input_class))
						{
							++_inports;;
						}
						else if(lilv_port_is_a(m_plugin, port, lv2world->output_class))
						{
							++_outports;
						}
					}
					//I donf think we currently have a machanism to deal with this outside of the synths
					else if(lilv_port_is_a(m_plugin, port, lv2world->event_class)) //Midi I/O
					{
						if(lilv_port_is_a(m_plugin, port, lv2world->input_class))
						{
							//++_inports;;
						}
						else if(lilv_port_is_a(m_plugin, port, lv2world->output_class) || lilv_port_is_a(m_plugin, port, lv2world->midi_class))
						{
							//++_outports;
						}
					}
				}
			}
		}
	}
}/*}}}*/

const LilvPlugin* LV2Plugin::getPlugin()/*{{{*/
{
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());
	const LilvPlugin *p = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	if(p == NULL)
	{
		return NULL;
	}
	return p;
}/*}}}*/

LilvInstance* LV2Plugin::instantiatelv2()/*{{{*/
{
	printf("LV2Plugin::instantiatelv2()\n");
	if(m_plugin == NULL)
	{
		printf("m_plugin is NULL\n");
		return NULL;
	}
	//LilvInstance* lvinstance = lilv_plugin_instantiate(m_plugin, sampleRate, m_features);
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());
	const LilvPlugin *p = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	LilvInstance* lvinstance = lilv_plugin_instantiate(p, sampleRate, features);//NULL);
	printf("After instantiate\n");
	if(lvinstance == NULL)
	{
		fprintf(stderr, "Plugin::instantiate() Error: plugin:%s instantiate failed!\n", _label.toUtf8().constData());
		return NULL;
	}
	//Set the pointer the shared plugin handle
	if(instance_access_feature.data == NULL)
		instance_access_feature.data = lilv_instance_get_handle(lvinstance);
	return lvinstance;
}/*}}}*/

void LV2Plugin::lv2range(unsigned long i, float* min, float* max) const/*{{{*/
{
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());
	const LilvPlugin *plug = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	const LilvPort* port = lilv_plugin_get_port_by_index(plug, i);
	if(port)
	{
		LilvNode* lmin;
		LilvNode* lmax;
		LilvNode* ldef;
		lilv_port_get_range(plug, port, &ldef, &lmin, &lmax);

		if(lilv_port_has_property(plug, port, lv2world->toggle_prop))
		{
			*min = 0.0f;
			*max = 1.0f;
			if(lmin) lilv_node_free(lmin);
			if(lmax) lilv_node_free(lmax);
			if(ldef) lilv_node_free(ldef);
			return;
		}
		//if(lilv_port_has_property(m_plugin, port, lv2world->samplerate_prop))
		if(lmin)
			*min = lilv_node_as_float(lmin);
		else
			*min = 0.0f;
		if(lmax)
			*max = lilv_node_as_float(lmax);
		else
			*max = 1.0f;

		if(lmin) lilv_node_free(lmin);
		if(lmax) lilv_node_free(lmax);
		if(ldef) lilv_node_free(ldef);
	}
}/*}}}*/

double LV2Plugin::defaultValue(unsigned int p) const/*{{{*/
{
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());
	const LilvPlugin *plug = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	if (p >= _portCount)//lilv_plugin_get_num_ports(plug))
		return 0.0;

	double val = 1.0;
	const LilvPort* port = lilv_plugin_get_port_by_index(plug, p);/*{{{*/
	if(port)
	{
		LilvNode* lmin;
		LilvNode* lmax;
		LilvNode* ldef;
		lilv_port_get_range(plug, port, &ldef, &lmin, &lmax);

		//TODO: implement proper logarithmic support
		/*if(lilv_port_has_property(m_plugin, port, lv2world->logarithmic_prop))
		{
		}
		else
		{
		}*/
		if(ldef)
			val = (double)lilv_node_as_float(ldef);
		if(lmin) lilv_node_free(lmin);
		if(lmax) lilv_node_free(lmax);
		if(ldef) lilv_node_free(ldef);
	}/*}}}*/

	return val;
}/*}}}*/

int LV2Plugin::updateReferences(int val)/*{{{*/
{
#ifdef PLUGIN_DEBUGIN 
	fprintf(stderr, "Plugin::incReferences _references:%d val:%d\n", _references, val);
#endif

	int newref = _references + val;

	if (newref == 0)
	{
		_references = 0;
		//TODO:Posibly shutdown instance here
		rpIdx.clear();
		return 0;
	}

	if (getPlugin() != NULL)
	{
		unsigned long cin = 0;
		for(int i = 0; i < (int)_portCount; ++i)
		{
			const LilvPort* port = lilv_plugin_get_port_by_index(getPlugin(), i);
			if(port)
			{
				if(lilv_port_is_a(getPlugin(), port, lv2world->control_class))//Controll
				{
					if(lilv_port_is_a(getPlugin(), port, lv2world->input_class))
					{
						rpIdx.push_back(cin);
						++cin;;
					}
					else if(lilv_port_is_a(getPlugin(), port, lv2world->output_class))
					{
						rpIdx.push_back((unsigned long) - 1);
					}
				}
				else if(lilv_port_is_a(getPlugin(), port, lv2world->audio_class))//Audio I/O
				{
					rpIdx.push_back((unsigned long) - 1);
				}
				_inPlaceCapable = !lilv_port_has_property(getPlugin(), port, lv2world->in_place_broken);
			}
		}
	}

	if (getPlugin() == NULL)
	{
		_references = 0;
		return 0;
	}

	_references = newref;

	return _references;
}/*}}}*/

const char* LV2Plugin::portName(unsigned long i)/*{{{*/
{
    //return plugin ? plugin->PortNames[i] : 0;

	const char* rv = 0;
	const LilvPort* port = lilv_plugin_get_port_by_index(getPlugin(), i);
	if(port)
	{
		LilvNode* pname = lilv_port_get_name(getPlugin(), port);
		if(pname)
		{
			rv = lilv_node_as_string(pname);
			lilv_node_free(pname);
		}
	}
	return rv;
}/*}}}*/

//The following two functions borrowed for qtractor
uint32_t LV2Plugin::lv2_uri_to_id(const char *uri)/*{{{*/
{
	const QString sUri(uri);

	QHash<QString, uint32_t>::ConstIterator iter = uri_map.constFind(sUri);
	if (iter == uri_map.constEnd()) {
		uint32_t id = uri_map.size() + 10000;
		uri_map.insert(sUri, id);
		ids_map.insert(id, sUri.toUtf8());
		return id;
	}

	return iter.value();
}/*}}}*/

const char *LV2Plugin::lv2_id_to_uri(uint32_t id)/*{{{*/
{
	QHash<uint32_t, QByteArray>::ConstIterator iter = ids_map.constFind(id);
	if (iter == ids_map.constEnd())
		return NULL;
	return iter.value().constData();
}/*}}}*/

LV2PluginI::LV2PluginI()
: PluginI()
{
	m_type = LV2;
	m_nativeui = 0;
}

LV2PluginI::~LV2PluginI()
{
	//We need to free the resources from lilv here
	if(m_plugin)
	{
		//deactivate();
		for(int j = 0; j < instances; ++j)
		{
			lilv_instance_deactivate((LilvInstance*)m_instance[j]);
			lilv_instance_free((LilvInstance*)m_instance[j]);
		}
		m_plugin->updateReferences(-1);
		if(m_nativeui)
			delete m_nativeui;
	}
}

//---------------------------------------------------------
//   initPluginInstance
//---------------------------------------------------------

bool LV2PluginI::initPluginInstance(Plugin* plug, int c)/*{{{*/
{
	//channel = c;
	if (!plug)
	{
		printf("initPluginInstance: plugin is null\n");
		return false;
	}
	m_plugin = 0;
	_plugin = plug;
	LV2Plugin* lv2plugin = (LV2Plugin*)plug;
	if(lv2plugin)
		m_plugin = lv2plugin;
	else
		return false;

	QString inst("-" + QString::number(m_plugin->instNo()));
	_name = m_plugin->name() + inst;
	_label = m_plugin->label() + inst;

	setChannels(c);
	//TODO: virtualize this method in Plugin
	
	m_plugin->updateReferences(1);

	/*controlPorts = m_plugin->controlInPorts();
	controlOutPorts = m_plugin->controlOutPorts();

	controls = new Port[controlPorts];
	controlsOut = new Port[controlOutPorts];

	// Also, pick the least number of ins or outs, and base the number of instances on that.
	unsigned long ins = m_plugin->inports();
	unsigned long outs = m_plugin->outports();
	if (outs)
	{
		instances = channel / outs;
		if (instances < 1)
			instances = 1;
	}
	else
		if (ins)
	{
		instances = channel / ins;
		if (instances < 1)
			instances = 1;
	}
	else
		instances = 1;

	m_instance.clear();// = new LilvInstance[instances];
	for (int i = 0; i < instances; ++i)
	{
		m_instance[i] = m_plugin->instantiatelv2();
		if (m_instance[i] == NULL)
			return false;
	}

	unsigned long ports = m_plugin->ports();
	const LilvPlugin *p = m_plugin->getPlugin();
	if(p)
	{
		const LilvPluginClass* pclass = lilv_plugin_get_class(p);
		const LilvNode* clabel = lilv_plugin_class_get_label(pclass);
		_label = QString(lilv_node_as_string(clabel));

		int i = 0;
		int ii = 0;
		for(unsigned long k = 0; k < ports; ++k)
		{
			//TODO: virtualize this method in Plugin
			for (int in = 0; in < instances; ++in)
			{
				lilv_instance_connect_port(m_instance[in], k, NULL);
				//_plugin->connectPort(handle[i], k, &controls[curPort].val);
			}
			const LilvPort* port = lilv_plugin_get_port_by_index(p, k);
			if(port)
			{
				if(lilv_port_is_a(p, port, lv2world->control_class))//Controll
				{
					if(lilv_port_is_a(p, port, lv2world->input_class))
					{
						//TODO: virtualize this method in Plugin
						double val = _plugin->defaultValue(k);
						controls[i].val = val;
						controls[i].tmpVal = val;
						controls[i].enCtrl = true;
						controls[i].en2Ctrl = true;
						controls[i].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controls[i].val);
						++i;
					}
					else if(lilv_port_is_a(p, port, lv2world->output_class))
					{
						controlsOut[ii].val = 0.0f;
						controlsOut[ii].tmpVal = 0.0f;
						controlsOut[ii].enCtrl = false;
						controlsOut[ii].en2Ctrl = false;
						controlsOut[ii].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controlsOut[ii].val);
						++ii;
					}
				}
			}
		}
	}*/

	activate();
	return true;
}/*}}}*/

void LV2PluginI::activate()
{
	printf("LV2PluginI::activate()\n");
	for(int j = 0; j < instances; ++j)
	{
		//if(m_instance[j])
			lilv_instance_activate(m_instance[j]);
	}
}

void LV2PluginI::deactivate()
{
	printf("LV2PluginI::deactivate()\n");
	for(int j = 0; j < instances; ++j)
	{
		lilv_instance_deactivate(m_instance[j]);
		lilv_instance_free((LilvInstance*)m_instance[j]);
	}
}

void LV2PluginI::apply(int frames)/*{{{*/
{
	//printf("LV2PluginI::apply()\n");
	unsigned long ctls = controlPorts;
	for (unsigned long k = 0; k < ctls; ++k)
	{
		if (automation && _track && _track->automationType() != AUTO_OFF && _id != -1)
		{
			if (controls[k].enCtrl && controls[k].en2Ctrl)
				controls[k].tmpVal = _track->pluginCtrlVal(genACnum(_id, k));
		}
	}
	for (int i = 0; i < instances; ++i)
	{
		lilv_instance_run(m_instance[i], (long)frames);
	}
}/*}}}*/

void LV2PluginI::showGui()
{
	if (m_plugin)/*{{{*/
	{
		if (!_gui)
		{
			makeGui();
		}
		if (_gui->isVisible())
			_gui->hide();
		else
			_gui->show();
	}/*}}}*/
}

void LV2PluginI::showGui(bool flag)
{
	if (m_plugin)/*{{{*/
	{
		if (flag)
		{
			if (!_gui)
			{
				makeGui();
			}
			if (_gui)
				_gui->show();
		}
		else
		{
			if (_gui)
				_gui->hide();
		}
	}/*}}}*/
}

static void
lv2_ui_write(SuilController /*controller*/, 
			uint32_t /*port_index*/,
			uint32_t /*buffer_size*/,
			uint32_t /*format*/,
			const void* /*buffer*/)
{
	fprintf(stderr, "UI WRITE\n");
}

void LV2PluginI::showNativeGui()
{
	if (m_plugin)/*{{{*/
	{
		if (!m_nativeui)
		{
			makeNativeGui();
		}
		if (m_nativeui->isVisible())
			m_nativeui->hide();
		else
			m_nativeui->show();
	}/*}}}*/
}

void LV2PluginI::showNativeGui(bool flag)
{
	printf("LV2PluginI::showNativeGui(%d) \n", flag);
	if (m_plugin)/*{{{*/
	{
		if (flag)
		{
			//if (!m_nativeui)
			//{
			//	printf("no gui found, ");
				makeNativeGui();
			//}
			/*if (m_nativeui)
			{
				printf("gui created, showing\n");
				m_nativeui->show();
			}*/
		}
		else
		{
			if (m_nativeui)
			{
				m_nativeui->hide();
			}
			//printf("hiding gui\n");
		}
	}/*}}}*/
}

void LV2PluginI::makeNativeGui()
{
	printf("LV2PluginI::makeNativeGui()\n");
	LilvNode* qtui = lilv_new_uri(lv2world->world, LV2_QT4_UI_URI);
	LilvNode* gtkui = lilv_new_uri(lv2world->world, LV2_GTK_UI_URI);
	LilvNode* extui = lilv_new_uri(lv2world->world, LV2_NS_UI "external");
	//const LV2_Feature instance_feature = { "http://lv2plug.in/ns/ext/instance-access", NULL};
	
	LilvNode* selectui = NULL;
	const LilvUI* ui = NULL;
	const LilvNode* ui_type = NULL;
	bool isexternal = false;
	if (qtui && gtkui) {
		printf("LV2PluginI::makeNativeGui() Found qt4ui uri\n");
		LilvUIs* uis = lilv_plugin_get_uis(m_plugin->getPlugin());
		LILV_FOREACH(uis, u, uis) {
			const LilvUI* this_ui = lilv_uis_get(uis, u);
			if (lilv_ui_is_supported(this_ui, suil_ui_supported, qtui, &ui_type)) {
				printf("LV2PluginI::makeNativeGui() qtui is supported\n");
				ui = this_ui;
				selectui = qtui;
				break;
			}
			else if(lilv_ui_is_supported(this_ui, suil_ui_supported, extui, &ui_type))
			{
				printf("LV2PluginI::makeNativeGui() extui is supported\n");
				ui = this_ui;
				isexternal = true;
				break;
			}
		}
	}

	SuilHost*     ui_host     = NULL;
	SuilInstance* ui_instance = NULL;
	if (ui) {
		printf("LV2PluginI::makeNativeGui() Found ui for plugin\n");
		ui_host = suil_host_new(lv2_ui_write, NULL, NULL, NULL);
		//const LV2_Feature* features = m_plugin->features();
		if(ui_host)
			ui_instance = suil_instance_new( 
										ui_host, 
										this, 
										lilv_node_as_uri(qtui),
										lilv_node_as_uri(lilv_plugin_get_uri(m_plugin->getPlugin())),
										lilv_node_as_uri(lilv_ui_get_uri(ui)),
										lilv_node_as_uri(ui_type),
										lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(ui))),
										lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(ui))),
										features
										);
	}
	if(ui_instance && !isexternal)
	{
		printf("LV2PluginI::makeNativeGui() Suil gui instance created\n");
		m_nativeui = (QWidget*)suil_instance_get_widget(ui_instance);
		if(m_nativeui)
		{
			printf("LV2PluginI::makeNativeGui() Suil successfully wraped gui\n");
			m_nativeui->setAttribute(Qt::WA_DeleteOnClose);
			QString title("OOMIDI: ");
			title.append(m_plugin->name());
			if(_track)
				title.append(" - ").append(_track->name());
			m_nativeui->setWindowTitle(title);
			m_nativeui->show();
		}
	}

	if(isexternal && ui)
	{
		//m_slv2_ui_instance = slv2_ui_instantiate(m_plugin->getPlugin(),
		//            ui, lv2_ui_write, this, features);
	}
}

void LV2PluginI::makeGui()
{
	_gui = new PluginGui(this);
}

bool LV2PluginI::isAudioIn(int p)/*{{{*/
{
	const LilvPort* port = lilv_plugin_get_port_by_index(m_plugin->getPlugin(), p);
	if(port)
	{
		if(lilv_port_is_a(m_plugin->getPlugin(), port, lv2world->audio_class))//Audio I/O
		{
			if(lilv_port_is_a(m_plugin->getPlugin(), port, lv2world->input_class))
			{
				return true;
			}
		}
	}
	return false;
}/*}}}*/

bool LV2PluginI::isAudioOut(int p)/*{{{*/
{
	const LilvPort* port = lilv_plugin_get_port_by_index(m_plugin->getPlugin(), p);
	if(port)
	{
		if(lilv_port_is_a(m_plugin->getPlugin(), port, lv2world->audio_class))//Audio I/O
		{
			if(lilv_port_is_a(m_plugin->getPlugin(), port, lv2world->output_class))
			{
				return true;
			}
		}
	}
	return false;
}/*}}}*/

void LV2PluginI::connect(int ports, float** src, float** dst)/*{{{*/
{
	int port = 0;
	for (int i = 0; i < instances; ++i)
	{
		for (unsigned long k = 0; k < m_plugin->ports(); ++k)
		{
			if (isAudioIn(k))
			{
				lilv_instance_connect_port(m_instance[i], k, src[port]);
				port = (port + 1) % ports;
			}
			else if(isAudioOut(k))
			{
				lilv_instance_connect_port(m_instance[i], k, dst[port]);
				port = (port + 1) % ports;
			}
		}
	}
}/*}}}*/

void LV2PluginI::setChannels(int c)/*{{{*/
{
	channel = c;

	// Also, pick the least number of ins or outs, and base the number of instances on that.

	unsigned long ins = m_plugin->inports();
	unsigned long outs = m_plugin->outports();

	int ni = 1;
	if (outs)
		ni = channel / outs;
	else
		if (ins)
		ni = channel / ins;

	if (ni < 1)
		ni = 1;

	if (ni == instances)
		return;

	// remove old instances if any:
	deactivate();
	instances = ni;

	controlPorts = m_plugin->controlInPorts();
	controlOutPorts = m_plugin->controlOutPorts();

	controls = new Port[controlPorts];
	controlsOut = new Port[controlOutPorts];
	
	//printf("111111111111111111111111111\n");
	m_instance.clear();
	for (int i = 0; i < instances; ++i)
	{
		m_instance.append(m_plugin->instantiatelv2());
		if (m_instance[i] == NULL)
		{
			printf("LV2PluginI::setChannels(%d) Instance creation failed\n", c);
			return;
		}
	}
	//printf("222222222222222222222222222\n");

	unsigned long ports = m_plugin->ports();
	const LilvPlugin *p = m_plugin->getPlugin();/*{{{*/
	if(p)
	{
		const LilvPluginClass* pclass = lilv_plugin_get_class(p);
		const LilvNode* clabel = lilv_plugin_class_get_label(pclass);
		_label = QString(lilv_node_as_string(clabel));

		int i = 0;
		int ii = 0;
		for(unsigned long k = 0; k < ports; ++k)
		{
			for (int in = 0; in < instances; ++in)
			{
				lilv_instance_connect_port(m_instance[in], k, NULL);
			}
			const LilvPort* port = lilv_plugin_get_port_by_index(p, k);
			if(port)
			{
				if(lilv_port_is_a(p, port, lv2world->control_class))//Controll
				{
					if(lilv_port_is_a(p, port, lv2world->input_class))
					{
						double val = m_plugin->defaultValue(k);
						controls[i].val = val;
						controls[i].tmpVal = val;
						controls[i].enCtrl = true;
						controls[i].en2Ctrl = true;
						controls[i].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controls[i].val);
						controls[i].logarithmic = lilv_port_has_property(p, port, lv2world->logarithmic_prop);
						controls[i].isInt = lilv_port_has_property(p, port, lv2world->integer_prop);
						controls[i].toggle = lilv_port_has_property(p, port, lv2world->toggle_prop);
						controls[i].samplerate = lilv_port_has_property(p, port, lv2world->samplerate_prop);
						m_plugin->lv2range(k, &controls[i].min, &controls[i].max);
						LilvNode* pname = lilv_port_get_name(p, port);
						if(pname)
						{
							const char* pn = lilv_node_as_string(pname);
							//printf("Found port %s\n", pn);
							controls[i].name = std::string(pn);
							lilv_node_free(pname);
						}
						//controls[i].name = m_plugin->portName(i);
						printf("Portname: %s, index:%ld\n",controls[i].name.c_str(), k);
						++i;
					}
					else if(lilv_port_is_a(p, port, lv2world->output_class))
					{
						controlsOut[ii].val = 0.0f;
						controlsOut[ii].tmpVal = 0.0f;
						controlsOut[ii].enCtrl = false;
						controlsOut[ii].en2Ctrl = false;
						controlsOut[ii].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controlsOut[ii].val);
						controlsOut[ii].logarithmic = lilv_port_has_property(p, port, lv2world->logarithmic_prop);
						controlsOut[ii].isInt = lilv_port_has_property(p, port, lv2world->integer_prop);
						controlsOut[ii].toggle = lilv_port_has_property(p, port, lv2world->toggle_prop);
						controlsOut[ii].samplerate = lilv_port_has_property(p, port, lv2world->samplerate_prop);
						m_plugin->lv2range(k, &controlsOut[ii].min, &controlsOut[ii].max);
						LilvNode* pname = lilv_port_get_name(p, port);
						if(pname)
						{
							const char* pn = lilv_node_as_string(pname);
							controlsOut[ii].name = std::string(pn);
							//controlsOut[ii].name = pn;//lilv_node_as_string(pname);
							lilv_node_free(pname);
						}
						//controlsOut[ii].name = m_plugin->portName(i);
						++ii;
					}
				}
			}
		}
	}/*}}}*/

	activate();
}/*}}}*/

LV2Plugin* LV2PluginList::find(const QString& uri, const QString& name)
{
	for (iLV2Plugin i = begin(); i != end(); ++i)
	{
		if ((uri == i->uri()) && (name == i->label()))
			return &*i;
	}
	//printf("Plugin <%s> not found\n", name.ascii());
	return 0;
}

