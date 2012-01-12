//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_LV2_H__
#define __PLUGIN_LV2_H__

#include "plugin.h"
#include "lib_functions.h"

#include "lv2_event.h"
#include "lv2_event_helpers.h"
#include "lv2_uri_map.h"
#include "lv2_urid.h"

#define NS_ATOM  "http://lv2plug.in/ns/ext/atom#"
#define NS_LV2UI "http://lv2plug.in/ns/extensions/ui#"

// define extra URIs not yet in lilv
#define LILV_URI_TIME_EVENT   "http://lv2plug.in/ns/ext/time#TimeEvent"

// static max values
const unsigned int MAX_EVENT_BUFFER = 0x7FFF; // 32767

// feature ids
const uint16_t lv2_feature_id_uri_map         = 0;
const uint16_t lv2_feature_id_urid_map        = 1;
const uint16_t lv2_feature_id_urid_unmap      = 2;
const uint16_t lv2_feature_id_event           = 3;
const uint16_t lv2_feature_id_data_access     = 4;
const uint16_t lv2_feature_id_instance_access = 5;
const uint16_t lv2_feature_id_ui_resize       = 6;
const uint16_t lv2_feature_id_ui_parent       = 7;
const uint16_t lv2_feature_id_external_ui     = 8;
const uint16_t lv2_feature_id_external_ui_old = 9;
const uint16_t lv2_feature_count              = 10;

// uri[d] map ids
const uint16_t OOM_URI_MAP_ID_EVENT_MIDI      = 1; // 0x1
const uint16_t OOM_URI_MAP_ID_EVENT_TIME      = 2; // 0x2
const uint16_t OOM_URI_MAP_ID_ATOM_STRING     = 3;
const uint16_t OOM_URI_MAP_ID_COUNT           = 4;

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE = 0x100;

//static LV2_Time_Position oom_lv2_time_pos = { 0, 0, LV2_TIME_STOPPED, 0, 0, 0, 0, 0, 0, 0.0 };

struct LV2World {
    LilvWorld* world;

    LilvNode* connectionOptional;
    LilvNode* inPlaceBroken;

    LilvNode* portInput;
    LilvNode* portOutput;
    LilvNode* portAudio;
    LilvNode* portControl;
    LilvNode* portEvent;
    LilvNode* portEventMidi;
    LilvNode* portEventTime;

//    LilvNode* opt_class;
    //LilvNode* uiGtk2;
    //LilvNode* uiQt4;
    //LilvNode* uiX11;
    //LilvNode* uiExternal;
//    LilvNode* gtkui_class;
//    LilvNode* toggle_prop;
//    LilvNode* integer_prop;
//    LilvNode* logarithmic_prop;
//    LilvNode* samplerate_prop;
//    LilvNode* units_prop;
//    LilvNode* persist_prop;

    //LilvInstance* instance;
    const LilvPlugins* plugins;
};

static LV2World* lv2world = 0;

void initLV2()
{
    lv2world = new LV2World;

    lv2world->world = lilv_world_new();
    lilv_world_load_all(lv2world->world);

    lv2world->connectionOptional = lilv_new_uri(lv2world->world, LILV_NS_LV2 "connectionOptional");
    lv2world->inPlaceBroken = lilv_new_uri(lv2world->world, LILV_NS_LV2 "inPlaceBroken");

    lv2world->portInput     = lilv_new_uri(lv2world->world, LILV_URI_INPUT_PORT);
    lv2world->portOutput    = lilv_new_uri(lv2world->world, LILV_URI_OUTPUT_PORT);
    lv2world->portAudio     = lilv_new_uri(lv2world->world, LILV_URI_AUDIO_PORT);
    lv2world->portControl   = lilv_new_uri(lv2world->world, LILV_URI_CONTROL_PORT);
    lv2world->portEvent     = lilv_new_uri(lv2world->world, LILV_URI_EVENT_PORT);
    lv2world->portEventMidi = lilv_new_uri(lv2world->world, LILV_URI_MIDI_EVENT);
    lv2world->portEventTime = lilv_new_uri(lv2world->world, LILV_URI_TIME_EVENT);

//    lv2world->opt_class = lilv_new_uri(lv2world->world, LILV_NS_LV2 "connectionOptional");
//    lv2world->qtui_class = lilv_new_uri(lv2world->world, LV2_QT4_UI_URI);
//    lv2world->gtkui_class = lilv_new_uri(lv2world->world, LV2_GTK_UI_URI);
//    lv2world->in_place_broken =

//    lv2world->toggle_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "toggled");
//    lv2world->integer_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "integer");
//    lv2world->samplerate_prop = lilv_new_uri(lv2world->world, LILV_NS_LV2 "sampleRate");
//    lv2world->logarithmic_prop = lilv_new_uri(lv2world->world, "http://lv2plug.in/ns/dev/extportinfo#logarithmic");
//    lv2world->persist_prop = lilv_new_uri(lv2world->world,  LV2_PERSIST_URI);

    lv2world->plugins = lilv_world_get_all_plugins(lv2world->world);

    printf("Found %d LV2 Plugins\n", lilv_plugins_size(lv2world->plugins));
    LILV_FOREACH(plugins, i, lv2world->plugins)
    {
        const LilvPlugin* p = lilv_plugins_get(lv2world->plugins, i);
        LilvNode* name = lilv_plugin_get_name(p);

        QString p_uri = QString(lilv_node_as_string(lilv_plugin_get_uri(p)));
        QString p_name = QString(name ? lilv_node_as_string(name) : "");

        if (name)
            lilv_node_free(name);

        // Make sure it doesn't already exist.
        if (plugins.find(p_uri, p_name) == 0)
        {
            plugins.add(PLUGIN_LV2, p_uri, p_name, p);
        }
    }
}

// ----------------- URI-Map Feature -------------------------------------------------
static uint32_t oom_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
{
    qDebug("oom_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

    if (map && strcmp(map, LV2_EVENT_URI) == 0)
    {
        // Event types
        if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
            return OOM_URI_MAP_ID_EVENT_MIDI;
        else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
            return OOM_URI_MAP_ID_EVENT_TIME;
    }
    else if (strcmp(uri, NS_ATOM "String") == 0)
    {
        return OOM_URI_MAP_ID_ATOM_STRING;
    }

    // Custom types
    if (data)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)data;
        return plugin->getCustomURIId(uri);
    }

    return 0;
}

// ----------------- URID Feature ----------------------------------------------------
static LV2_URID oom_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
    qDebug("oom_lv2_urid_map(%p, %s)", handle, uri);

    if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
        return OOM_URI_MAP_ID_EVENT_MIDI;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
        return OOM_URI_MAP_ID_EVENT_TIME;
    else if (strcmp(uri, NS_ATOM "String") == 0)
        return OOM_URI_MAP_ID_ATOM_STRING;

    // Custom types
    if (handle)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIId(uri);
    }

    return 0;
}

static const char* oom_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
    qDebug("oom_lv2_urid_unmap(%p, %i)", handle, urid);

    if (urid == OOM_URI_MAP_ID_EVENT_MIDI)
        return "http://lv2plug.in/ns/ext/midi#MidiEvent";
    else if (urid == OOM_URI_MAP_ID_EVENT_TIME)
        return "http://lv2plug.in/ns/ext/time#Position";
    else if (urid == OOM_URI_MAP_ID_ATOM_STRING)
        return NS_ATOM "String";

    // Custom types
    if (handle)
    {
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIString(urid);
    }

    return 0;
}

Lv2Plugin::Lv2Plugin()
{
    m_type = PLUGIN_LV2;
    m_paramsBuffer = 0;

    m_customURIs.clear();

    // Fill pre-set URI keys
    for (uint16_t i=0; i < OOM_URI_MAP_ID_COUNT; i++)
        m_customURIs.append(0);

    handle = 0;
    descriptor = 0;

    ui.handle = 0;
    ui.widget = 0;
    ui.descriptor = 0;
    ui._lib = 0;

    for (uint16_t i=0; i < lv2_feature_count+1; i++)
        features[i] = 0;

    lplug = 0;
}

Lv2Plugin::~Lv2Plugin()
{
    // close UI
    if (m_hints & PLUGIN_HAS_NATIVE_GUI)
    {
//        switch(ui.type)
//        {
//        case GUI_EXTERNAL_LV2:
//            if (gui.visible)
//                LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);

//            break;

//        default:
//            break;
//        }

        //if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
        //    ui.descriptor->cleanup(ui.handle);

//        if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
//            delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

//        if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
//            delete (LV2_UI_Resize_Feature*)features[lv2_feature_id_ui_resize]->data;

//        if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
//            delete (lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data;

        if (ui._lib)
            lib_close(ui._lib);
    }

    for (int i=0; i < m_customURIs.count(); i++)
    {
        if (m_customURIs[i])
            free((void*)m_customURIs[i]);
    }

    m_customURIs.clear();

    if (handle && descriptor->deactivate && m_activeBefore)
        descriptor->deactivate(handle);

    if (handle && descriptor->cleanup)
        descriptor->cleanup(handle);

//    if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
//        delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

//    if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
//        delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

//    if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
//        delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

//    if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
//        delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

    for (uint16_t i=0; i<lv2_feature_count && features[i]; i++)
        delete features[i];

    if (m_paramCount > 0)
        delete[] m_paramsBuffer;

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();
}

void Lv2Plugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    LilvPlugin* lv2plug = (LilvPlugin*)nativeHandle;

    uint32_t portCount = lilv_plugin_get_num_ports(lv2plug);
    for (uint32_t i = 0; i < portCount; i++)
    {
        const LilvPort* port = lilv_plugin_get_port_by_index(lv2plug, i);

        if (port)
        {
            if (lilv_port_is_a(lv2plug, port, lv2world->portAudio))
            {
                if(lilv_port_is_a(lv2plug, port, lv2world->portInput))
                    plugi->m_audioInputCount += 1;
                else if(lilv_port_is_a(lv2plug, port, lv2world->portOutput))
                    plugi->m_audioOutputCount += 1;
            }
        }
    }

    plugi->m_name = QString(label);

    LilvNode* lv2maker = lilv_plugin_get_author_name(lv2plug);
    if (lv2maker)
    {
        plugi->m_maker = QString(lilv_node_as_string(lv2maker));
        lilv_node_free(lv2maker);
    }

    const LilvPluginClass* lilvClass = lilv_plugin_get_class(lv2plug);
    const LilvNode* lilvClassLabel = lilv_plugin_class_get_label(lilvClass);
    const char* lv2Class = lilv_node_as_string(lilvClassLabel);

    if (strcmp(lv2Class, "Instrument") == 0)
        plugi->m_hints |= PLUGIN_IS_SYNTH;
    else if (plugi->m_audioInputCount > 1 && plugi->m_audioOutputCount > 1)
        plugi->m_hints |= PLUGIN_IS_FX;

    if (lilv_plugin_has_feature(lv2plug, lv2world->inPlaceBroken))
        plugi->m_hints |= PLUGIN_HAS_IN_PLACE_BROKEN;

    // TODO - test if this works or not
    if (lilv_plugin_get_uis(lv2plug))
        plugi->m_hints |= PLUGIN_HAS_NATIVE_GUI;

    Q_UNUSED(filename);
}

bool Lv2Plugin::init(QString filename, QString /*label*/)
{
    LilvNode* pluginURI = lilv_new_uri(lv2world->world, filename.toAscii().constData());
    lplug = lilv_plugins_get_by_uri(lv2world->plugins, pluginURI);
    lilv_node_free(pluginURI);

    if (lplug)
    {
        _lib = lib_open(lilv_node_as_uri(lilv_plugin_get_library_uri(lplug)));

        if (_lib)
        {
            LV2_Descriptor_Function descfn = (LV2_Descriptor_Function) lib_symbol(_lib, "lv2_descriptor");

            if (descfn)
            {
                uint32_t i = 0;
                const char* c_uri = strdup(filename.toAscii().constData());

                while ((descriptor = descfn(i++)))
                {
                    if (strcmp(descriptor->URI, c_uri) == 0)
                        break;
                }

                if (descriptor)
                {
                    bool can_continue = true;

                    // Check supported ports
                    uint32_t PortCount = lilv_plugin_get_num_ports(lplug);
                    for (i=0; i < PortCount; i++)
                    {
                        const LilvPort* port = lilv_plugin_get_port_by_index(lplug, i);

                        if (port)
                        {
                            if (lilv_port_is_a(lplug, port, lv2world->portAudio) == false && lilv_port_is_a(lplug, port, lv2world->portControl) == false && lilv_port_is_a(lplug, port, lv2world->portEvent) == false)
                            {
                                // TODO - check if port is optional
                                if (false)
                                {
                                    set_last_error("Plugin requires a port that is not currently supported");
                                    can_continue = false;
                                    break;
                                }
                            }
                        }
                    }

                    // Check supported features
//                    for (i=0; i < rdf_descriptor->FeatureCount; i++)
//                    {
//                        if (is_lv2_feature_supported(rdf_descriptor->Features[i].URI) == false)
//                        {
//                            if (LV2_IS_FEATURE_REQUIRED(rdf_descriptor->Features[i].Type))
//                            {
//                                QString msg = QString("Plugin requires a feature that is not supported:\n%1").arg(rdf_descriptor->Features[i].URI);
//                                set_last_error(msg.toStdString().data());
//                                can_continue = false;
//                                break;
//                            }
//                            else
//                                qDebug("Plugin wants a feature that is not supported:\n%s", rdf_descriptor->Features[i].URI);
//                        }
//                    }

                    if (can_continue)
                    {
                        // Initialize features
                        LV2_URI_Map_Feature* URI_Map_Feature = new LV2_URI_Map_Feature;
                        URI_Map_Feature->callback_data       = this;
                        URI_Map_Feature->uri_to_id           = oom_lv2_uri_to_id;

                        LV2_URID_Map* URID_Map_Feature       = new LV2_URID_Map;
                        URID_Map_Feature->handle             = this;
                        URID_Map_Feature->map                = oom_lv2_urid_map;

                        LV2_URID_Unmap* URID_Unmap_Feature   = new LV2_URID_Unmap;
                        URID_Unmap_Feature->handle           = this;
                        URID_Unmap_Feature->unmap            = oom_lv2_urid_unmap;


                        features[lv2_feature_id_uri_map]          = new LV2_Feature;
                        features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
                        features[lv2_feature_id_uri_map]->data    = URI_Map_Feature;

                        features[lv2_feature_id_urid_map]         = new LV2_Feature;
                        features[lv2_feature_id_urid_map]->URI    = LV2_URID_MAP_URI;
                        features[lv2_feature_id_urid_map]->data   = URID_Map_Feature;

                        features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
                        features[lv2_feature_id_urid_unmap]->URI  = LV2_URID_UNMAP_URI;
                        features[lv2_feature_id_urid_unmap]->data = URID_Unmap_Feature;
                    }
                    else
                    {
                        descriptor = 0;
                    }
                }
                else
                    set_last_error("Failed to find the requested plugin URI in the plugin library");
            }
            else
                set_last_error("Not a LV2 plugin");
        }
        else
            set_last_error(lib_error());
    }
    else
        set_last_error("Failed to find plugin in the LV2 collection");

    return false;
}

void Lv2Plugin::reload()
{
    // delete old data
    if (m_paramCount > 0)
    {
        delete[] m_params;
        delete[] m_paramsBuffer;
    }

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();

    // reset
    m_hints  = 0;
    m_params = 0;
    m_paramsBuffer = 0;
    m_paramCount   = 0;

    // query new data
    uint32_t ains, aouts, params, j;
    ains = aouts = params = 0;

}

void Lv2Plugin::reloadPrograms(bool)
{
}

QString Lv2Plugin::getParameterName(uint32_t index)
{
    return QString("");
}

void Lv2Plugin::setNativeParameterValue(uint32_t index, double value)
{
}

void Lv2Plugin::process(uint32_t frames)
{
}

void Lv2Plugin::bufferSizeChanged(uint32_t)
{
}

bool Lv2Plugin::readConfiguration(Xml& xml, bool readPreset)
{
    return false;
}

void Lv2Plugin::writeConfiguration(int level, Xml& xml)
{
}

uint32_t Lv2Plugin::getCustomURIId(const char *uri)
{
    qDebug("Lv2Plugin::getCustomURIId(%s)", uri);

    for (int i=0; i < m_customURIs.count(); i++)
    {
        if (m_customURIs[i] && strcmp(m_customURIs[i], uri) == 0)
            return i;
    }

    m_customURIs.append(strdup(uri));
    return m_customURIs.count()-1;
}

const char* Lv2Plugin::getCustomURIString(int uri_id)
{
    qDebug("Lv2Plugin::getCustomURIString(%i)", uri_id);

    if (uri_id < m_customURIs.count())
        return m_customURIs.at(uri_id);
    else
        return 0;
}

#endif // __PLUGIN_LV2_H__
