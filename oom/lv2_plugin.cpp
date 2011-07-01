//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ladspa_plugin.cpp $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett 
//  	(info@openoctave.org)
//=========================================================

#include "lv2_plugin.h"
#ifdef LV2_SUPPORT
#include <QDialog>
#include <QMessageBox>
#include <QEvent>
#include <QX11EmbedContainer>
#include <string.h>

#include "track.h"
#include "plugingui.h"
#include "song.h"
#include "audio.h"

#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
//#include "lv2/lv2plug.in/ns/ext/files/files.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/uri-unmap/uri-unmap.h"
#include "lv2/lv2plug.in/ns/ext/instance-access/instance-access.h"

#define LV2_GTK_UI_URI "http://lv2plug.in/ns/extensions/ui#GtkUI"
#define LV2_QT4_UI_URI "http://lv2plug.in/ns/extensions/ui#Qt4UI"
#define LV2_NS_UI   "http://lv2plug.in/ns/extensions/ui#"

LV2EventFilter::LV2EventFilter(LV2PluginI *p, QWidget *w)
	: QObject(), m_plugin(p), m_widget(w)
{
	m_widget->installEventFilter(this); 
	//connect(m_widget, SIGNAL(clientClosed()), this, SLOT(closeWidget()));
}

void LV2EventFilter::closeWidget()
{
	if(m_plugin)
		m_plugin->closeNativeGui();
}

bool LV2EventFilter::eventFilter(QObject *o, QEvent *event)/*{{{*/
{
	if(m_widget)
	{
		if (event->type() == QEvent::Close) {
			m_widget = NULL;
			printf("LV2EventFilter::eventFilter() Plugin window closed\n");
			if(m_plugin)
				m_plugin->closeNativeGui();
		}
	}
	return QObject::eventFilter(o, event);
}/*}}}*/

//Temporary id to uri maps
//These are used to convert uri to id and back during the lifecycle of the plugin instance
static QHash<QString, uint32_t>    uri_map;
static QHash<uint32_t, QByteArray> ids_map;

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

//static LV2_Feature persist_feature = {"http://lv2plug.in/ns/ext/persist", NULL};
		
static const LV2_Feature* features[] = { 
	&lv2_uri_map_feature,
	&lv2_uri_unmap_feature,
    NULL
};

static void external_ui_closed(LV2UI_Controller ui_controller)/*{{{*/
{
	printf("external_ui_closed()\n");
	LV2PluginI* plugin = static_cast<LV2PluginI*>(ui_controller);
	if(plugin)
	{
		printf("external_ui_closed: calling closeNativeGui\n");
		plugin->closeNativeGui();
	}
}/*}}}*/
#ifdef GTK2UI_SUPPORT

#undef signals 

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
static void lv2_gtk_window_destroy (GtkWidget * /*gtkWindow*/, gpointer plug )/*{{{*/
{
	LV2PluginI* plugin = static_cast<LV2PluginI*> (plug);
	if(plugin)
	{
		printf("lv2_gtk_window_destroy: calling closeNativeGui\n");
		plugin->closeNativeGui();
	}
}/*}}}*/
#endif

static LV2World* lv2world;

void initLV2()/*{{{*/
{
	lv2world = new LV2World;
#ifdef SLV2_SUPPORT
	lv2world->world = slv2_world_new();/*{{{*/
	slv2_world_load_all(lv2world->world);

	lv2world->plugins = slv2_world_get_all_plugins(lv2world->world);

	lv2world->input_class = slv2_value_new_uri(lv2world->world, LILV_URI_INPUT_PORT);
	lv2world->output_class = slv2_value_new_uri(lv2world->world, LILV_URI_OUTPUT_PORT);
	lv2world->control_class = slv2_value_new_uri(lv2world->world, LILV_URI_CONTROL_PORT);
	lv2world->event_class = slv2_value_new_uri(lv2world->world, LILV_URI_EVENT_PORT);
	lv2world->audio_class = slv2_value_new_uri(lv2world->world, LILV_URI_AUDIO_PORT);
	lv2world->midi_class = slv2_value_new_uri(lv2world->world, LILV_URI_MIDI_EVENT);

	lv2world->opt_class = slv2_value_new_uri(lv2world->world, LILV_NS_LV2 "connectionOptional");
	lv2world->qtui_class = slv2_value_new_uri(lv2world->world, LV2_QT4_UI_URI);
	lv2world->gtkui_class = slv2_value_new_uri(lv2world->world, LV2_GTK_UI_URI);
	lv2world->in_place_broken = slv2_value_new_uri(lv2world->world, LILV_NS_LV2 "inPlaceBroken");

	lv2world->toggle_prop = slv2_value_new_uri(lv2world->world, LILV_NS_LV2 "toggled");    
	lv2world->integer_prop = slv2_value_new_uri(lv2world->world, LILV_NS_LV2 "integer");
	lv2world->samplerate_prop = slv2_value_new_uri(lv2world->world, LILV_NS_LV2 "sampleRate");
	lv2world->logarithmic_prop = slv2_value_new_uri(lv2world->world, "http://lv2plug.in/ns/dev/extportinfo#logarithmic");

	//printf("initLV2()\n");
	printf("Found %d LV2 Plugins\n", slv2_plugins_size(lv2world->plugins));
	for(uint32_t i = 0; i < slv2_plugins_size(lv2world->plugins); ++i) 
	{
		SLV2Plugin p = slv2_plugins_get_at(lv2world->plugins, i);
		//lv2world->plugin_list.insert(QString(lilv_node_as_string(lilv_plugin_get_uri(p))), p);
		const char* curi = slv2_value_as_string(slv2_plugin_get_uri(p));
		//printf(" uri: %s\n", curi);
        plugins.push_back(LV2Plugin(curi));
		//lv2plugins.add(curi);
	}/*}}}*/
#else
	lv2world->world = lilv_world_new();/*{{{*/
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
	}/*}}}*/
#endif
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
#ifdef SLV2_SUPPORT
		SLV2Value plugin_uri = slv2_value_new_uri(lv2world->world, uri);/*{{{*/
		//                    slv2_world_get_plugin_by_uri_string
		SLV2Plugin p = slv2_plugins_get_by_uri(lv2world->plugins, plugin_uri);
		slv2_value_free(plugin_uri);
		if(p)
		{
			m_plugin = p;//const_cast<SLV2Plugin*>(p);
			if(m_plugin && debugMsg)
				printf("LV2Plugin::init Found LilvPlugin, setting m_plugin\n");
			SLV2Value name = slv2_plugin_get_name(m_plugin);
			if(name)
			{
				_name = QString(slv2_value_as_string(name));
				slv2_value_free(name);
				//printf("Found plugin: %s\n", _name.toLatin1().constData());
			}
			SLV2Value lvmaker = slv2_plugin_get_author_name(m_plugin);
			_maker = QString(lvmaker ? slv2_value_as_string(lvmaker) : "Unknown");
			slv2_value_free(lvmaker);
			//QString uri(lilv_node_as_string(lilv_plugin_get_uri(p)));
			SLV2PluginClass pclass = slv2_plugin_get_class(m_plugin);
			SLV2Value clabel = slv2_plugin_class_get_label(pclass);
			_label = QString(slv2_value_as_string(clabel));
			//printf("Found label: %s\n", _label.toLatin1().constData());
			//lilv_node_free(clabel);
			_uniqueID = (unsigned long)lv2_uri_to_id(uri);

			_portCount = slv2_plugin_get_num_ports(m_plugin);
			
			for(int i = 0; i < (int)_portCount; ++i)
			{
				SLV2Port port = slv2_plugin_get_port_by_index(m_plugin, i);
				if(port)
				{
					if(slv2_port_is_a(m_plugin, port, lv2world->control_class))//Controll
					{
						if(slv2_port_is_a(m_plugin, port, lv2world->input_class))
						{
							++_controlInPorts;;
						}
						else if(slv2_port_is_a(m_plugin, port, lv2world->output_class))
						{
							++_controlOutPorts;
						}
					}
					else if(slv2_port_is_a(m_plugin, port, lv2world->audio_class))//Audio I/O
					{
						if(slv2_port_is_a(m_plugin, port, lv2world->input_class))
						{
							++_inports;;
						}
						else if(slv2_port_is_a(m_plugin, port, lv2world->output_class))
						{
							++_outports;
						}
					}
				}
			}
		}/*}}}*/
#else
		LilvNode* plugin_uri = lilv_new_uri(lv2world->world, uri);/*{{{*/
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
					/*else if(lilv_port_is_a(m_plugin, port, lv2world->event_class)) //Midi I/O
					{
						if(lilv_port_is_a(m_plugin, port, lv2world->input_class))
						{
							//++_inports;;
						}
						else if(lilv_port_is_a(m_plugin, port, lv2world->output_class) || lilv_port_is_a(m_plugin, port, lv2world->midi_class))
						{
							//++_outports;
						}
					}*/
				}
			}
		}/*}}}*/
#endif
	}
}/*}}}*/
#ifdef SLV2_SUPPORT
SLV2Plugin LV2Plugin::getPlugin()/*{{{*/
{
	SLV2Value plugin_uri = slv2_value_new_uri(lv2world->world, m_uri.toUtf8().constData());
	SLV2Plugin p = slv2_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	slv2_value_free(plugin_uri);
	if(!p)
		return NULL;
	return p;
#else
const LilvPlugin* LV2Plugin::getPlugin()
{
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());
	const LilvPlugin *p = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	if(p == NULL)
	{
		return NULL;
	}
	return p;
#endif
}/*}}}*/

void LV2Plugin::lv2range(unsigned long i, float* def, float* min, float* max) const/*{{{*/
{
#ifdef SLV2_SUPPORT
	SLV2Value plugin_uri = slv2_value_new_uri(lv2world->world, m_uri.toUtf8().constData());/*{{{*/
	SLV2Plugin plug = slv2_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	slv2_value_free(plugin_uri);
	SLV2Port port = slv2_plugin_get_port_by_index(plug, i);
	if(port)
	{
		SLV2Value lmin;
		SLV2Value lmax;
		SLV2Value ldef;
		slv2_port_get_range(plug, port, &ldef, &lmin, &lmax);

		if(slv2_port_has_property(plug, port, lv2world->toggle_prop))
		{
			*min = 0.0f;
			*max = 1.0f;
			if(ldef)
				*def = slv2_value_as_float(ldef);
			else
				*def = 0.0f;
			if(lmin) slv2_value_free(lmin);
			if(lmax) slv2_value_free(lmax);
			if(ldef) slv2_value_free(ldef);
			return;
		}
		//if(lilv_port_has_property(m_plugin, port, lv2world->samplerate_prop))
		if(lmin)
			*min = slv2_value_as_float(lmin);
		else
			*min = 0.0f;
		if(lmax)
			*max = slv2_value_as_float(lmax);
		else
			*max = 1.0f;
		if(ldef)
			*def = slv2_value_as_float(ldef);
		else
			*def = 0.0f;

		if(lmin) slv2_value_free(lmin);
		if(lmax) slv2_value_free(lmax);
		if(ldef) slv2_value_free(ldef);
	}/*}}}*/
#else
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());/*{{{*/
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
			if(ldef)
				*def = lilv_node_as_float(ldef);
			else
				*def = 0.0f;
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
		if(ldef)
			*def = lilv_node_as_float(ldef);
		else
			*def = 0.0f;

		if(lmin) lilv_node_free(lmin);
		if(lmax) lilv_node_free(lmax);
		if(ldef) lilv_node_free(ldef);
	}/*}}}*/
#endif
}/*}}}*/

double LV2Plugin::defaultValue(unsigned int p) const/*{{{*/
{
#ifdef SLV2_SUPPORT
	SLV2Value plugin_uri = slv2_value_new_uri(lv2world->world, m_uri.toUtf8().constData());/*{{{*/
	SLV2Plugin plug = slv2_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	slv2_value_free(plugin_uri);
	if (p >= _portCount)//lilv_plugin_get_num_ports(plug))
		return 0.0;

	double val = 1.0;
	SLV2Port port = slv2_plugin_get_port_by_index(plug, p);
	if(port)
	{
		SLV2Value lmin;
		SLV2Value lmax;
		SLV2Value ldef;
		slv2_port_get_range(plug, port, &ldef, &lmin, &lmax);

		//TODO: implement proper logarithmic support
		/*if(lilv_port_has_property(m_plugin, port, lv2world->logarithmic_prop))
		{
		}
		else
		{
		}*/
		if(ldef)
			val = (double)slv2_value_as_float(ldef);
		if(lmin) slv2_value_free(lmin);
		if(lmax) slv2_value_free(lmax);
		if(ldef) slv2_value_free(ldef);
	}/*}}}*/
	return val;
#else
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_uri.toUtf8().constData());/*{{{*/
	const LilvPlugin *plug = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	if (p >= _portCount)//lilv_plugin_get_num_ports(plug))
		return 0.0;

	double val = 1.0;
	const LilvPort* port = lilv_plugin_get_port_by_index(plug, p);
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
#endif
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
#ifdef SLV2_SUPPORT
			SLV2Port port = slv2_plugin_get_port_by_index(getPlugin(), i);/*{{{*/
			if(port)
			{
				if(slv2_port_is_a(getPlugin(), port, lv2world->control_class))//Controll
				{
					if(slv2_port_is_a(getPlugin(), port, lv2world->input_class))
					{
						rpIdx.push_back(cin);
						++cin;;
					}
					else if(slv2_port_is_a(getPlugin(), port, lv2world->output_class))
					{
						rpIdx.push_back((unsigned long) - 1);
					}
				}
				else if(slv2_port_is_a(getPlugin(), port, lv2world->audio_class))//Audio I/O
				{
					rpIdx.push_back((unsigned long) - 1);
				}
				_inPlaceCapable = !slv2_port_has_property(getPlugin(), port, lv2world->in_place_broken);
			}/*}}}*/
#else
			const LilvPort* port = lilv_plugin_get_port_by_index(getPlugin(), i);/*{{{*/
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
			}/*}}}*/
#endif
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
#ifdef SLV2_SUPPORT
	SLV2Port port = slv2_plugin_get_port_by_index(getPlugin(), i);
	if(port)
	{
		SLV2Value pname = slv2_port_get_name(getPlugin(), port);
		if(pname)
		{
			rv = slv2_value_as_string(pname);
			slv2_value_free(pname);
		}
	}
#else
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
#endif
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
	m_controlFifo = 0;
	m_eventFilter = 0;
	m_guiVisible = false;
	m_ui_type = 0;
	m_update_gui = false;
	m_lv2_ui_widget = NULL;
#ifdef SLV2_SUPPORT
	m_slv2_ui_instance = NULL;
	m_slv2_uis = NULL;
	m_slv2_ui = NULL;
#endif
#ifdef GTK2UI_SUPPORT
	m_gtkWindow = 0;
#endif
	int fcount = 0;
	while (features[fcount]) { ++fcount; }

	m_features = new LV2_Feature * [fcount + 1];
	for (int i = 0; i < fcount; ++i)
		m_features[i] = (LV2_Feature *) features[i];
	m_features[fcount] = NULL;
}

LV2PluginI::~LV2PluginI()
{
	//We need to free the resources from lilv here
	if(m_plugin)
	{
		closeNativeGui();
		deactivate();
		int ref = m_plugin->updateReferences(-1);
		if(debugMsg)
			printf("Reference count:%d\n", ref);
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
	
	int ref = m_plugin->updateReferences(1);
	if(debugMsg)
		printf("LV2PluginI: Reference count: %d\n", ref);
	return true;
}/*}}}*/

#ifdef SLV2_SUPPORT
SLV2Instance LV2PluginI::instantiatelv2()/*{{{*/
{
	printf("LV2PluginI::instantiatelv2()\n");
	SLV2Value plugin_uri = slv2_value_new_uri(lv2world->world, m_plugin->uri().toUtf8().constData());
	SLV2Plugin p = slv2_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	slv2_value_free(plugin_uri);
	if(p)
	{
		SLV2Instance lvinstance = slv2_plugin_instantiate(p, sampleRate, m_features);
		printf("After instantiate\n");
		if(lvinstance == NULL)
		{
			fprintf(stderr, "LV2PluginI::instantiatelv2() Error: plugin:%s instantiate failed!\n", m_plugin->label().toUtf8().constData());
			return NULL;
		}
		return lvinstance;
	}
	else
		printf("LV2PluginI::instantiatelv2() Plugin is null\n");
	return NULL;
#else
LilvInstance* LV2PluginI::instantiatelv2()
{
	printf("LV2PluginI::instantiatelv2()\n");
	LilvNode* plugin_uri = lilv_new_uri(lv2world->world, m_plugin->uri().toUtf8().constData());
	const LilvPlugin *p = lilv_plugins_get_by_uri(lv2world->plugins, plugin_uri);
	lilv_node_free(plugin_uri);
	LilvInstance* lvinstance = lilv_plugin_instantiate(p, sampleRate, m_features);//NULL);
	printf("After instantiate\n");
	if(lvinstance == NULL)
	{
		fprintf(stderr, "LV2PluginI::instantiate() Error: plugin:%s instantiate failed!\n", m_plugin->label().toUtf8().constData());
		return NULL;
	}
	return lvinstance;
#endif
}/*}}}*/

void LV2PluginI::heartBeat()/*{{{*/
{
	if(m_lv2_ui_widget == NULL)
		return;
#ifdef SLV2_SUPPORT
		//printf("LV2PluginI::heartBeat() SLV2\n");
	const LV2UI_Descriptor *ui_descriptor = lv2_ui_descriptor();/*{{{*/
	if (ui_descriptor == NULL)
		return;
	if (ui_descriptor->port_event == NULL)
		return;

	LV2UI_Handle ui_handle = lv2_ui_handle();
	if (ui_handle == NULL)
		return;/*}}}*/

	if (m_ui_type == UITYPE_EXT)
		LV2_EXTERNAL_UI_RUN((lv2_external_ui *) m_lv2_ui_widget);

	for(unsigned long j = 0; j < (unsigned)controlOutPorts; ++j)
	{
		if(controlsOut[j].tmpVal != controlsOut[j].val)
		{
	//		printf("LV2PluginI::heartBeat() updating value\n");
			(*ui_descriptor->port_event)(ui_handle,	controlsOut[j].idx, sizeof(float), 0, &controlsOut[j].val);
			controlsOut[j].tmpVal = controlsOut[j].val;
		}
	}
#else
	if(m_uinstance.isEmpty())
		return;
	//printf("LV2PluginI::heartBeat() LILV %d\n", controlOutPorts);
	if (m_ui_type == UITYPE_EXT && m_lv2_ui_widget)
		LV2_EXTERNAL_UI_RUN((lv2_external_ui *) m_lv2_ui_widget);
	for(unsigned long j = 0; j < (unsigned)controlOutPorts; ++j)
	{
		if(controlsOut[j].tmpVal != controlsOut[j].val)
		{
			//printf("LV2PluginI::heartBeat() updating value\n");
			if(!m_uinstance.isEmpty())
			{
				suil_instance_port_event(m_uinstance.at(0), controlsOut[j].idx, sizeof(float), 0, &controlsOut[j].val);
				controlsOut[j].tmpVal = controlsOut[j].val;
			}
			else
				return;
		}
	}
#endif
	unsigned long ctls = controlPorts;
	for (unsigned long k = 0; k < ctls; ++k)
	{
		if(m_uinstance.isEmpty())
			return;
		if(controls[k].update || controls[k].lastGuiVal != controls[k].val)/*{{{*/
		{
	#ifdef SLV2_SUPPORT
			const LV2UI_Descriptor *ui_descriptor = lv2_ui_descriptor();
			if (ui_descriptor != NULL)
			{
				if(ui_descriptor->port_event != NULL)
				{
					LV2UI_Handle ui_handle = lv2_ui_handle();
					if(ui_handle != NULL)
					{
						(*ui_descriptor->port_event)(ui_handle,	controls[k].idx, sizeof(float), 0, &controls[k].val);
						controls[k].lastGuiVal = controls[k].val;
						controls[k].update = false;
					}
				}
			}
	#else
			if(!m_uinstance.isEmpty())
			{
				suil_instance_port_event(m_uinstance.at(0), controls[k].idx, sizeof(float), 0, &controls[k].val);
				controls[k].lastGuiVal = controls[k].val;
				controls[k].update = false;
			}
	#endif
		}/*}}}*/
	}
}/*}}}*/

void LV2PluginI::activate()/*{{{*/
{
	printf("LV2PluginI::activate()\n");
	for(int j = 0; j < instances; ++j)
	{
#ifdef SLV2_SUPPORT
		slv2_instance_activate(m_instance[j]);
#else
		lilv_instance_activate(m_instance[j]);
#endif
		if (initControlValues)/*{{{*/
		{
			for (int i = 0; i < controlPorts; ++i)
			{
				controls[i].val = controls[i].tmpVal;
			}
		}
		else
		{
			//
			// get initial control values from plugin
			//
			for (int i = 0; i < controlPorts; ++i)
			{
				controls[i].tmpVal = controls[i].val;
			}
		}/*}}}*/
	}
}/*}}}*/

void LV2PluginI::deactivate()/*{{{*/
{
	printf("LV2PluginI::deactivate()\n");
	for(int j = 0; j < instances; ++j)
	{
#ifdef SLV2_SUPPORT
		slv2_instance_deactivate((SLV2Instance)m_instance[j]);
		slv2_instance_free((SLV2Instance)m_instance[j]);
#else
		lilv_instance_deactivate((LilvInstance*)m_instance[j]);
		lilv_instance_free((LilvInstance*)m_instance[j]);
#endif
	}
}/*}}}*/

void LV2PluginI::apply(int frames)/*{{{*/
{
	//printf("LV2PluginI::apply()\n");
	unsigned long ctls = controlPorts;
	for (unsigned long k = 0; k < ctls; ++k)
	{
		//TODO: Read from lv2 gui fifo here and apply to controls
		LV2ControlFifo* cfifo = getControlFifo(k);
		if(cfifo && !cfifo->isEmpty())
		{
			LV2Data v = cfifo->get();
			controls[k].tmpVal = v.value;
			if (_track && _id != -1)
			{
				if(debugMsg)
					printf("Applying values from fifo %f\n", v.value);
				_track->setPluginCtrlVal(genACnum(_id, k), v.value);
			}
			//controls[k].update = false;
			controls[k].lastGuiVal = v.value;
		}
		else
		{
			if (automation && _track && _track->automationType() != AUTO_OFF && _id != -1)
			{
				if (controls[k].enCtrl && controls[k].en2Ctrl)
					controls[k].tmpVal = _track->pluginCtrlVal(genACnum(_id, k));
			}
			if(controls[k].val != controls[k].tmpVal)
			{
				if(debugMsg)
					printf("Applying values from automation tmpVal: %f val:%f\n", controls[k].tmpVal, controls[k].val);
				controls[k].val = controls[k].tmpVal;
			//	controls[k].update = true;
				//FIXME: Not sure if this is being called in the right thread
				//I suspect this should happen in the gui thread
			}
			//else
			//	controls[k].update = false;
		}
	}
	for (int i = 0; i < instances; ++i)
	{
#ifdef SLV2_SUPPORT
		slv2_instance_run(m_instance[i], (long)frames);
#else
		lilv_instance_run(m_instance[i], (long)frames);
#endif
	}
}/*}}}*/

void LV2PluginI::showGui()/*{{{*/
{
	if (m_plugin)
	{
		if (!_gui)
		{
			showGui(true);
		}
		if (_gui->isVisible())
			showGui(false);
		else
			showGui(true);
	}
}/*}}}*/

void LV2PluginI::showGui(bool flag)/*{{{*/
{
	if (m_plugin)
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
	}
}/*}}}*/

static void lv2_ui_write(/*{{{*/
#ifdef SLV2_SUPPORT
			LV2UI_Controller controller,
#else
			SuilController controller, 
#endif
			uint32_t port_index,
			uint32_t buffer_size,
			uint32_t format,
			const void* buffer)
{
	LV2PluginI* p = static_cast<LV2PluginI*>(controller);
	if(p)
	{
		if(buffer_size != sizeof(float) || format != 0)
			return;
		float value = *(float*)buffer;
		LV2Plugin* lp = (LV2Plugin*)p->plugin();

		unsigned long index = lp->port2InCtrl(port_index);
		Port* cport = p->getControlPort(index);

		if ((int) index == -1)
		{
			fprintf(stderr, "lv2_ui_write: port number:%d is not a control input\n", port_index);
			return;
		}

		if(cport && cport->tmpVal != value)
		{
			//printf("lv2_ui_write: gui changed param %d\n", port_index);
			LV2ControlFifo* cfifo = p->getControlFifo(index);
			if (cfifo)
			{
				LV2Data cv;
				cv.value = value;
				cv.frame = audio->timestamp();
				if (!cfifo->put(cv) && debugMsg)
				{
					fprintf(stderr, "lv2_ui_write: fifo overflow: in control number:%ld\n", index);
				}
			}
		}

		//FIXME:Should this only happen during playback since that's the only time in matters
		if (p->track() && p->id() != -1)
		{
			int id = genACnum(p->id(), index);
			AutomationType at = p->track()->automationType();

			if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
				p->enableController(index, false);

			p->track()->recordAutomation(id, value);
		}
	}
}/*}}}*/

void LV2PluginI::showNativeGui()/*{{{*/
{
	showNativeGui(!m_guiVisible);
}/*}}}*/

void LV2PluginI::showNativeGui(bool flag)/*{{{*/
{
	printf("LV2PluginI::showNativeGui(%d) \n", flag);
	if (m_plugin)
	{
		if (flag)
		{
			if(m_lv2_ui_widget == NULL)
				makeNativeGui();
			switch(m_ui_type)
			{
				case UITYPE_EXT:
					if(m_lv2_ui_widget)
						LV2_EXTERNAL_UI_SHOW((lv2_external_ui *) m_lv2_ui_widget);
				break;
			#ifdef GTK2UI_SUPPORT
				case UITYPE_GTK2:
					if(m_gtkWindow)
					{
						gtk_widget_show_all(m_gtkWindow);
					}
				break;
			#endif
				case UITYPE_QT4:
					if(m_nativeui)
						m_nativeui->show();
				break;
			}
			m_guiVisible = true;
		}
		else
		{
			m_guiVisible = false;
			switch(m_ui_type)
			{
				case UITYPE_EXT:
					if(m_lv2_ui_widget)
						LV2_EXTERNAL_UI_HIDE((lv2_external_ui *) m_lv2_ui_widget);
				break;
			#ifdef GTK2UI_SUPPORT
				case UITYPE_GTK2:
					if(m_gtkWindow)
						gtk_widget_hide_all(m_gtkWindow);
				break;
			#endif
				case UITYPE_QT4:
				{
					if(m_nativeui)
						m_nativeui->hide();
				}
				break;
			}
		}
	}
}/*}}}*/

void LV2PluginI::closeNativeGui()/*{{{*/
{
	printf("LV2PluginI::closeNativeGui()\n");
	showNativeGui(false);
	#ifdef GTK2UI_SUPPORT
	if(m_gtkWindow)
	{
		m_gtkWindow = NULL;
	}
	#endif
	if(m_eventFilter)
	{
		delete m_eventFilter;
		m_eventFilter = 0;
	}
	if(m_nativeui)
	{
		delete m_nativeui;
		m_nativeui = 0;
	}
#ifdef SLV2_SUPPORT
		//FIXME:Both of these causes crash of oom we need to find a proper way to free
		//the ui_handle for now we'll just null the internal copy of the instance
		//and clear the internal list for lilv mode.
	if(m_ui_type == UITYPE_EXT)
	{
		//	slv2_ui_instance_free(m_slv2_ui_instance);
		const LV2UI_Descriptor *ui_descriptor = lv2_ui_descriptor();
		if (ui_descriptor && ui_descriptor->cleanup) 
		{
			LV2UI_Handle ui_handle = lv2_ui_handle();
			if (ui_handle)
				(*ui_descriptor->cleanup)(ui_handle);
		}
	}
	if(m_slv2_ui_instance)
		m_slv2_ui_instance = NULL;
#else
	if(m_ui_type == UITYPE_EXT)
	{
		if(!m_uinstance.isEmpty())
		{
			SuilInstance* inst = m_uinstance.takeAt(0);
			if(inst)
			{
				suil_instance_free(inst);
			}
			if(m_uihost)
			{
				suil_host_free(m_uihost);
			}
		}
	}
	m_uinstance.clear();
#endif
	m_lv2_ui_widget = NULL;
}/*}}}*/

void LV2PluginI::makeNativeGui()/*{{{*/
{
	std::string title("OOMIDI: ");
	title.append(m_plugin->name().toLatin1().constData());
	if(_track)
		title.append(" - ").append(_track->name().toLatin1().constData());
#ifdef SLV2_SUPPORT
	SLV2Value qtui = slv2_value_new_uri(lv2world->world, LV2_QT4_UI_URI);/*{{{*/
	SLV2Value gtkui = slv2_value_new_uri(lv2world->world, LV2_GTK_UI_URI);
	SLV2Value extui = slv2_value_new_uri(lv2world->world, LV2_EXTERNAL_UI_URI);
	
	m_slv2_uis = slv2_plugin_get_uis(m_plugin->getPlugin());
	if(m_slv2_uis == NULL)
		return;
	int uinum = slv2_uis_size(m_slv2_uis);
	for(int i = 0; i < uinum; ++i)
	{
		SLV2UI ui = const_cast<SLV2UI> (slv2_uis_get_at(m_slv2_uis, i));
		if(slv2_ui_is_a(ui, extui))
		{
			m_ui_type = UITYPE_EXT;
			m_slv2_ui = const_cast<SLV2UI> (ui);
			break;
		}
		if(slv2_ui_is_a(ui, gtkui))
		{
			m_ui_type = UITYPE_GTK2;
			m_slv2_ui = const_cast<SLV2UI> (ui);
			break;
		}
		if(slv2_ui_is_a(ui, qtui))
		{
			m_ui_type = UITYPE_QT4;
			m_slv2_ui = const_cast<SLV2UI> (ui);
			break;
		}
	}

	if (m_slv2_ui == NULL)
		return;
	
	const SLV2Instance instance = m_instance.at(0);
	if (instance == NULL)
		return;
	
	const LV2_Descriptor *descriptor = slv2_instance_get_descriptor(instance);
	if (descriptor == NULL)
		return;

	int fcount = 0;
	while (m_features[fcount]) { ++fcount; }

	m_ui_features = new LV2_Feature * [fcount + 4];
	for (int i = 0; i < fcount; ++i)
		m_ui_features[i] = (LV2_Feature *) m_features[i];

	m_instance_access_feature.URI = LV2_INSTANCE_ACCESS_URI;
	m_instance_access_feature.data = slv2_instance_get_handle(instance);
	m_ui_features[fcount++] = &m_instance_access_feature;

	m_data_access.data_access = descriptor->extension_data;
	m_data_access_feature.URI = LV2_DATA_ACCESS_URI;
	m_data_access_feature.data = &m_data_access;
	m_ui_features[fcount++] = &m_data_access_feature;

	if(m_ui_type == UITYPE_EXT)
	{
		m_lv2_ui_external.ui_closed = external_ui_closed;
		m_lv2_ui_external.plugin_human_id = title.c_str();
		m_ui_feature.URI = LV2_EXTERNAL_UI_URI;
		m_ui_feature.data = &m_lv2_ui_external;
		m_ui_features[fcount++] = &m_ui_feature;
	}
	m_ui_features[fcount] = NULL;

	m_slv2_ui_instance = slv2_ui_instantiate(m_plugin->getPlugin(),
											m_slv2_ui, 
											lv2_ui_write, 
											this, 
											m_ui_features);
	if(m_slv2_ui_instance == NULL)
		return;
	const LV2UI_Descriptor *ui_descriptor = lv2_ui_descriptor();
	if (ui_descriptor && ui_descriptor->port_event)
	{
		LV2UI_Handle ui_handle = lv2_ui_handle();
		if (ui_handle)
		{
			for(unsigned long j = 0; j < (unsigned)controlOutPorts; ++j)
			{
				//printf("LV2PluginI::makeNativeGui(): controlOut val: %4.2f \n",controlsOut[j].val);
				(*ui_descriptor->port_event)(ui_handle, controlsOut[j].idx, sizeof(float), 0, &controlsOut[j].val);
			}
		}
	}

	if (m_slv2_ui_instance) 
	{
		m_lv2_ui_widget = slv2_ui_instance_get_widget(m_slv2_ui_instance);
		if (m_lv2_ui_widget)
		{
			if (m_ui_type == UITYPE_EXT)
				LV2_EXTERNAL_UI_RUN((lv2_external_ui *) m_lv2_ui_widget);
			if(m_ui_type == UITYPE_GTK2)
			{
	#ifdef GTK2UI_SUPPORT
				m_gtkWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
				gtk_window_set_resizable(GTK_WINDOW(m_gtkWindow), 1);
				gtk_window_set_role(GTK_WINDOW(m_gtkWindow), title.c_str());
				gtk_window_set_title(GTK_WINDOW(m_gtkWindow), title.c_str());
				gtk_container_add(GTK_CONTAINER(m_gtkWindow), static_cast<GtkWidget *> (m_lv2_ui_widget));
				g_signal_connect(G_OBJECT(m_gtkWindow), "destroy", G_CALLBACK(lv2_gtk_window_destroy), this);
	#endif
			}
			if(m_ui_type == UITYPE_QT4)
			{
				m_nativeui = (QWidget*)m_lv2_ui_widget;
				if(m_nativeui)
				{
					m_eventFilter = new LV2EventFilter(this, m_nativeui);
					printf("LV2PluginI::makeNativeGui() Suil successfully wraped gui\n");
					m_nativeui->setWindowRole(QString(title.c_str()));
					m_nativeui->setAttribute(Qt::WA_DeleteOnClose);
					m_nativeui->setWindowTitle(QString(title.c_str()));
					//m_nativeui->show();
					//m_guiVisible = true;
				}
			}
		}
	}
/*}}}*/
#else
	printf("LV2PluginI::makeNativeGui()\n");
	LilvNode* qtui = lilv_new_uri(lv2world->world, LV2_QT4_UI_URI);/*{{{*/
	LilvNode* gtkui = lilv_new_uri(lv2world->world, LV2_GTK_UI_URI);
	LilvNode* extui = lilv_new_uri(lv2world->world, LV2_EXTERNAL_UI_URI);
	
	const LilvUI* ui = NULL;
	const LilvNode* ui_type = NULL;
	LilvUIs* uis = lilv_plugin_get_uis(m_plugin->getPlugin());
	LILV_FOREACH(uis, u, uis) {
		const LilvUI* this_ui = lilv_uis_get(uis, u);
		if(lilv_ui_is_a(this_ui, extui))
		{
			printf("LV2PluginI::makeNativeGui() GUI type is extui\n");
			ui = this_ui;
			m_ui_type = UITYPE_EXT;
			ui_type = extui;
			break;
		}
		if(lilv_ui_is_a(this_ui, gtkui))
		{
			printf("LV2PluginI::makeNativeGui() GUI type is gtk\n");
			ui = this_ui;
			m_ui_type = UITYPE_GTK2;
			ui_type = gtkui;
			break;
		}
		if (lilv_ui_is_a(this_ui, qtui)) {
			printf("LV2PluginI::makeNativeGui() GUI type is qtui\n");
			ui = this_ui;
			m_ui_type = UITYPE_QT4;
			ui_type = qtui;
			break;
		}
	}

	const LilvInstance *instance = m_instance.at(0);
	if (instance == NULL)
		return;
	
	const LV2_Descriptor *descriptor = lilv_instance_get_descriptor(instance);
	if (descriptor == NULL)
		return;

	int fcount = 0;
	while (m_features[fcount]) { ++fcount; }

	m_ui_features = new LV2_Feature * [fcount + 4];
	for (int i = 0; i < fcount; ++i)
		m_ui_features[i] = (LV2_Feature *) m_features[i];

	m_instance_access_feature.URI = LV2_INSTANCE_ACCESS_URI;
	m_instance_access_feature.data = lilv_instance_get_handle(instance);
	m_ui_features[fcount++] = &m_instance_access_feature;

	m_data_access.data_access = descriptor->extension_data;
	m_data_access_feature.URI = LV2_DATA_ACCESS_URI;
	m_data_access_feature.data = &m_data_access;
	m_ui_features[fcount++] = &m_data_access_feature;

	if(m_ui_type == UITYPE_EXT)
	{
		m_lv2_ui_external.ui_closed = external_ui_closed;
		m_lv2_ui_external.plugin_human_id = title.c_str();
		m_ui_feature.URI = LV2_EXTERNAL_UI_URI;
		m_ui_feature.data = &m_lv2_ui_external;
		m_ui_features[fcount++] = &m_ui_feature;
	}
	m_ui_features[fcount] = NULL;

	SuilHost*     ui_host     = NULL;
	SuilInstance* ui_instance = NULL;
	m_uinstance.clear();
	if (ui) {
		printf("LV2PluginI::makeNativeGui() Found ui for plugin\n");
		ui_host = suil_host_new(lv2_ui_write, NULL, NULL, NULL);
		//const LV2_Feature* features = m_plugin->features();
		if(ui_host)
		{
			m_uihost = ui_host;
			ui_instance = suil_instance_new( 
										ui_host, 
										this, 
										lilv_node_as_uri(ui_type),
										m_plugin->uri().toUtf8().constData(),
										lilv_node_as_uri(lilv_ui_get_uri(ui)),
										lilv_node_as_uri(ui_type),
										lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(ui))),
										lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(ui))),
										m_ui_features
										);
			if(ui_instance)
			{	
				m_lv2_ui_widget = suil_instance_get_widget(ui_instance);
				m_uinstance.append(ui_instance);
				printf("LV2PluginI::makeNativeGui() Suil gui instance created controlOutPorts:%d\n", controlOutPorts);
				for(unsigned long j = 0; j < (unsigned)controlOutPorts; ++j)
				{
					//printf("LV2PluginI::makeNativeGui(): controlOut val: %4.2f \n",controlsOut[j].val);
					suil_instance_port_event(ui_instance, controlsOut[j].idx, sizeof(float), 0, &controlsOut[j].val);
				}
				if (m_ui_type == UITYPE_EXT && m_lv2_ui_widget)
					LV2_EXTERNAL_UI_RUN((lv2_external_ui *) m_lv2_ui_widget);
				if(m_ui_type == UITYPE_GTK2)
				{
				#ifdef GTK2UI_SUPPORT
					m_gtkWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
					gtk_window_set_resizable(GTK_WINDOW(m_gtkWindow), 1);
					gtk_window_set_role(GTK_WINDOW(m_gtkWindow), title.c_str());
					gtk_window_set_title(GTK_WINDOW(m_gtkWindow), title.c_str());
					gtk_container_add(GTK_CONTAINER(m_gtkWindow), static_cast<GtkWidget *> (m_lv2_ui_widget));
					g_signal_connect(G_OBJECT(m_gtkWindow), "destroy", G_CALLBACK(lv2_gtk_window_destroy), this);
				#endif
				}
				if(m_ui_type == UITYPE_QT4)
				{
					m_nativeui = (QWidget*)m_lv2_ui_widget;
					if(m_nativeui)
					{
						m_eventFilter = new LV2EventFilter(this, (QX11EmbedContainer*)m_nativeui);
						m_nativeui->setWindowRole(QString(title.c_str()));
						m_nativeui->setAttribute(Qt::WA_DeleteOnClose);
						m_nativeui->setWindowTitle(QString(title.c_str()));
					}
				}
			}
		}
	}/*}}}*/
#endif
	//Update all the gui ports to the plugin value
	unsigned long ctls = controlPorts;
	for (unsigned long k = 0; k < ctls; ++k)
	{
		controls[k].update = true;
	}
}/*}}}*/

void LV2PluginI::makeGui()/*{{{*/
{
	_gui = new PluginGui(this);
}/*}}}*/

const LV2UI_Descriptor *LV2PluginI::lv2_ui_descriptor(void) const/*{{{*/
{
#ifdef SLV2_SUPPORT
	if (m_slv2_ui_instance)
		return slv2_ui_instance_get_descriptor(m_slv2_ui_instance);
	else
#endif
	return NULL;
}/*}}}*/

LV2UI_Handle LV2PluginI::lv2_ui_handle(void) const/*{{{*/
{
#ifdef SLV2_SUPPORT
	if (m_slv2_ui_instance)
		return slv2_ui_instance_get_handle(m_slv2_ui_instance);
	else
#endif
	return NULL;
}/*}}}*/

bool LV2PluginI::isAudioIn(int p)/*{{{*/
{
#ifdef SLV2_SUPPORT
	SLV2Port port = slv2_plugin_get_port_by_index(m_plugin->getPlugin(), p);
	if(port)
	{
		if(slv2_port_is_a(m_plugin->getPlugin(), port, lv2world->audio_class))//Audio I/O
		{
			if(slv2_port_is_a(m_plugin->getPlugin(), port, lv2world->input_class))
			{
				return true;
			}
		}
	}
#else
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
#endif
	return false;
}/*}}}*/

bool LV2PluginI::isAudioOut(int p)/*{{{*/
{
#ifdef SLV2_SUPPORT
	SLV2Port port = slv2_plugin_get_port_by_index(m_plugin->getPlugin(), p);
	if(port)
	{
		if(slv2_port_is_a(m_plugin->getPlugin(), port, lv2world->audio_class))//Audio I/O
		{
			if(slv2_port_is_a(m_plugin->getPlugin(), port, lv2world->output_class))
			{
				return true;
			}
		}
	}
#else
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
#endif
	return false;
}/*}}}*/

void LV2PluginI::connect(int ports, float** src, float** dst)/*{{{*/
{
	//printf("LV2PluginI::connect\n");
	int port = 0;
	for (int i = 0; i < instances; ++i)
	{
		for (unsigned long k = 0; k < m_plugin->ports(); ++k)
		{
			if (isAudioIn(k))
			{
				//printf("LV2PluginI::connect Audio input port: %d value: %4.2f\n", port, *src[port]);
#ifdef SLV2_SUPPORT
				slv2_instance_connect_port(m_instance[i], k, src[port]);
#else
				lilv_instance_connect_port(m_instance[i], k, src[port]);
#endif
				port = (port + 1) % ports;
			}
			else if(isAudioOut(k))
			{
				//printf("LV2PluginI::connect Audio output port: %d value: %4.2f\n", port, *dst[port]);
#ifdef SLV2_SUPPORT
				slv2_instance_connect_port(m_instance[i], k, dst[port]);
#else
				lilv_instance_connect_port(m_instance[i], k, dst[port]);
#endif
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

	printf("LV2PluginI::setChannels(%d): controls:%d, controlsOut:%d\n", c, controlPorts, controlOutPorts);
	controls = new Port[controlPorts];
	controlsOut = new Port[controlOutPorts];
	m_controlFifo = new LV2ControlFifo[controlPorts];
	
	//printf("111111111111111111111111111\n");
	m_instance.clear();
	for (int i = 0; i < instances; ++i)
	{
		m_instance.append(instantiatelv2());
		if (m_instance[i] == NULL)
		{
			printf("LV2PluginI::setChannels(%d) Instance creation failed\n", c);
			return;
		}
	}
	//printf("222222222222222222222222222\n");

	unsigned long ports = m_plugin->ports();

#ifdef SLV2_SUPPORT
	SLV2Plugin p = m_plugin->getPlugin();/*{{{*/
	if(p)
	{
		SLV2PluginClass pclass = slv2_plugin_get_class(p);
		SLV2Value clabel = slv2_plugin_class_get_label(pclass);
		_label = QString(slv2_value_as_string(clabel));

		int i = 0;
		int ii = 0;
		for(unsigned long k = 0; k < ports; ++k)
		{
			for (int in = 0; in < instances; ++in)
			{
				slv2_instance_connect_port(m_instance[in], k, NULL);
			}
			SLV2Port port = slv2_plugin_get_port_by_index(p, k);
			if(port)
			{
				if(slv2_port_is_a(p, port, lv2world->control_class))//Controll
				{
					if(slv2_port_is_a(p, port, lv2world->input_class))
					{
						//double val = m_plugin->defaultValue(k);
						float min;
						float max;
						float def;
						m_plugin->lv2range(k, &def, &min, &max);
						controls[i].val = def;
						controls[i].tmpVal = def;
						controls[i].enCtrl = true;
						controls[i].en2Ctrl = true;
						controls[i].update = true;
						controls[i].lastGuiVal = def;
						controls[i].idx = k;
						for(int j = 0; j < instances; ++j)
							slv2_instance_connect_port(m_instance[j], k, &controls[i].val);
						controls[i].logarithmic = slv2_port_has_property(p, port, lv2world->logarithmic_prop);
						controls[i].isInt = slv2_port_has_property(p, port, lv2world->integer_prop);
						controls[i].toggle = slv2_port_has_property(p, port, lv2world->toggle_prop);
						controls[i].samplerate = slv2_port_has_property(p, port, lv2world->samplerate_prop);
						controls[i].min = min;
						controls[i].max = max;
						SLV2Value pname = slv2_port_get_name(p, port);
						if(pname)
						{
							const char* pn = slv2_value_as_string(pname);
							//printf("Found port %s\n", pn);
							controls[i].name = std::string(pn);
							slv2_value_free(pname);
						}
						//controls[i].name = m_plugin->portName(i);
						//printf("Portname: %s, index:%ld\n",controls[i].name.c_str(), k);
						++i;
					}
					else if(slv2_port_is_a(p, port, lv2world->output_class))
					{
						float min;
						float max;
						float def;
						m_plugin->lv2range(k, &def, &min, &max);
						//printf("LV2PluginI::setChannels Port range for index: %d default:%f min:%f max:%f\n", k, def, min, max);
						controlsOut[ii].val = def;
						controlsOut[ii].tmpVal = 1.0f;
						controlsOut[ii].enCtrl = false;
						controlsOut[ii].en2Ctrl = false;
						controlsOut[ii].idx = k;
						for(int j = 0; j < instances; ++j)
							slv2_instance_connect_port(m_instance[j], k, &controlsOut[ii].val);
						controlsOut[ii].logarithmic = slv2_port_has_property(p, port, lv2world->logarithmic_prop);
						controlsOut[ii].isInt = slv2_port_has_property(p, port, lv2world->integer_prop);
						controlsOut[ii].toggle = slv2_port_has_property(p, port, lv2world->toggle_prop);
						controlsOut[ii].samplerate = slv2_port_has_property(p, port, lv2world->samplerate_prop);
						controlsOut[ii].min = min;
						controlsOut[ii].max = max;
						SLV2Value pname = slv2_port_get_name(p, port);
						if(pname)
						{
							const char* pn = slv2_value_as_string(pname);
							controlsOut[ii].name = std::string(pn);
							//controlsOut[ii].name = pn;//lilv_node_as_string(pname);
							slv2_value_free(pname);
						}
						//controlsOut[ii].name = m_plugin->portName(i);
						++ii;
					}
				}
			}
		}
		activate();
		//audio->msgSetPluginCtrlVal(_track, genACnum(_id, controls[i].idx), init_val);
	}/*}}}*/
#else
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
						//double val = m_plugin->defaultValue(k);
						float min;
						float max;
						float def;
						m_plugin->lv2range(k, &def, &min, &max);
						controls[i].val = def;
						controls[i].tmpVal = def;
						controls[i].enCtrl = true;
						controls[i].en2Ctrl = true;
						controls[i].update = true;
						controls[i].lastGuiVal = def;
						controls[i].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controls[i].val);
						controls[i].logarithmic = lilv_port_has_property(p, port, lv2world->logarithmic_prop);
						controls[i].isInt = lilv_port_has_property(p, port, lv2world->integer_prop);
						controls[i].toggle = lilv_port_has_property(p, port, lv2world->toggle_prop);
						controls[i].samplerate = lilv_port_has_property(p, port, lv2world->samplerate_prop);
						controls[i].min = min;
						controls[i].max = max;
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
						float min;
						float max;
						float def;
						m_plugin->lv2range(k, &def, &min, &max);
						//printf("LV2PluginI::setChannels Port range for index: %d default:%f min:%f max:%f\n", k, def, min, max);
						controlsOut[ii].val = def;
						controlsOut[ii].tmpVal = def;
						controlsOut[ii].enCtrl = false;
						controlsOut[ii].en2Ctrl = false;
						controlsOut[ii].idx = k;
						for(int j = 0; j < instances; ++j)
							lilv_instance_connect_port(m_instance[j], k, &controlsOut[ii].val);
						controlsOut[ii].logarithmic = lilv_port_has_property(p, port, lv2world->logarithmic_prop);
						controlsOut[ii].isInt = lilv_port_has_property(p, port, lv2world->integer_prop);
						controlsOut[ii].toggle = lilv_port_has_property(p, port, lv2world->toggle_prop);
						controlsOut[ii].samplerate = lilv_port_has_property(p, port, lv2world->samplerate_prop);
						controlsOut[ii].min = min;
						controlsOut[ii].max = max;
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
		activate();
	}/*}}}*/
#endif
	//apply(1);
}/*}}}*/

bool LV2PluginI::setControl(const QString& s, double val)/*{{{*/
{
	for (int i = 0; i < controlPorts; ++i)
	{
		if (m_plugin->portName(controls[i].idx) == s)
		{
			controls[i].val = controls[i].tmpVal = val;
			return false;
		}
	}
	printf("LV2PluginI:setControl(%s, %f) controller not found\n",
			s.toLatin1().constData(), val);
	return true;
}/*}}}*/

void LV2PluginI::writeConfiguration(int level, Xml& xml)/*{{{*/
{
	xml.tag(level++, "lv2plugin uri=\"%s\" label=\"%s\" channel=\"%d\"",
			Xml::xmlString(m_plugin->uri()).toLatin1().constData(), Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);

	for (int i = 0; i < controlPorts; ++i)
	{
		int idx = controls[i].idx;
		QString s("control name=\"%1\" val=\"%2\" /");
		xml.tag(level, s.arg(Xml::xmlString(m_plugin->portName(idx)).toLatin1().constData()).arg(controls[i].tmpVal).toLatin1().constData());
	}
	if (_on == false)
		xml.intTag(level, "on", _on);
	if (guiVisible())
	{
		xml.intTag(level, "gui", 1);
		xml.geometryTag(level, "geometry", _gui);
	}
	if (nativeGuiVisible())
	{
		xml.intTag(level, "nativegui", 1);
	}
	xml.tag(level--, "/lv2plugin");
}/*}}}*/

bool LV2PluginI::loadControl(Xml& xml)/*{{{*/
{
	QString name("mops");
	double val = 0.0;

	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();

		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return true;
			case Xml::TagStart:
				xml.unknown("PluginI-Control");
				break;
			case Xml::Attribut:
				if (tag == "name")
					name = xml.s2();
				else if (tag == "val")
					val = xml.s2().toDouble();
				break;
			case Xml::TagEnd:
				if (tag == "control")
				{
					if (setControl(name, val))
					{
						return false;
					}
					initControlValues = true;
				}
				return true;
			default:
				break;
		}
	}
	return true;
}/*}}}*/

bool LV2PluginI::readConfiguration(Xml& xml, bool readPreset)/*{{{*/
{
	QString uri;
	QString label;
	if (!readPreset)
		channel = 1;

	for (;;)
	{
		Xml::Token token(xml.parse());
		const QString & tag(xml.s1());
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return true;
			case Xml::TagStart:
				if (!readPreset && _plugin == 0)
				{
					//printf("LV2PluginI::readConfiguration 1111111111111\n");
					_plugin = plugins.find(uri, label);

					if (_plugin && !initPluginInstance(_plugin, channel))
					{
						_plugin = 0;
						xml.parse1();
						break;
					}
				}
				if (tag == "control")
					loadControl(xml);
				else if (tag == "on")
				{
					bool flag = xml.parseInt();
					if (!readPreset)
						_on = flag;
				}
				else if (tag == "gui")
				{
					bool flag = xml.parseInt();
					showGui(flag);
				}
				else if (tag == "nativegui")
				{
					// We can't tell OSC to show the native plugin gui
					//  until the parent track is added to the lists.
					// OSC needs to find the plugin in the track lists.
					// Use this 'pending' flag so it gets done later.
					_showNativeGuiPending = xml.parseInt();
				}
				else if (tag == "geometry")
				{
					QRect r(readGeometry(xml, tag));
					if (_gui)
					{
						_gui->resize(r.size());
						_gui->move(r.topLeft());
					}
				}
				else
					xml.unknown("LV2PluginI");
				break;
			case Xml::Attribut:
				if (tag == "uri")
				{
					QString s = xml.s2();
					if (readPreset)
					{
						if (s != plugin()->lib())
						{
							printf("Error: Wrong preset type %s. Type must be a %s\n",
									s.toLatin1().constData(), plugin()->lib().toLatin1().constData());
							return true;
						}
					}
					else
					{
						uri = s;
					}
				}
				else if (tag == "label")
				{
					if (!readPreset)
						label = xml.s2();
				}
				else if (tag == "channel")
				{
					if (!readPreset)
						channel = xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if (tag == "lv2plugin")
				{
					if (!readPreset && _plugin == 0)
					{
						_plugin = plugins.find(uri, label);
						if (_plugin == 0)
							return true;

						if (!initPluginInstance(_plugin, channel))
							return true;
					}
					if (_gui)
						_gui->updateValues();
					return false;
				}
				return true;
			default:
				break;
		}
	}
	return true;
}/*}}}*/

bool LV2ControlFifo::put(const LV2Data& event)/*{{{*/
{
	if (size < LV2_FIFO_SIZE)
	{
		fifo[wIndex] = event;
		wIndex = (wIndex + 1) % LV2_FIFO_SIZE;
		++size;
		return true;
	}
	return false;
}/*}}}*/

LV2Data LV2ControlFifo::get()/*{{{*/
{
	LV2Data event(fifo[rIndex]);
	rIndex = (rIndex + 1) % LV2_FIFO_SIZE;
	--size;
	return event;
}/*}}}*/

const LV2Data& LV2ControlFifo::peek(int n)/*{{{*/
{
	int idx = (rIndex + n) % LV2_FIFO_SIZE;
	return fifo[idx];

}/*}}}*/

void LV2ControlFifo::remove()/*{{{*/
{
	rIndex = (rIndex + 1) % LV2_FIFO_SIZE;
	--size;
}/*}}}*/
#endif
