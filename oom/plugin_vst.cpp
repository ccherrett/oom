//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_VST_H__
#define __PLUGIN_VST_H__

#include "plugin.h"
#include "plugingui.h"
#include "jackaudio.h"
#include "song.h"

#include <math.h>

#ifndef kVstVersion
#define kVstVersion 2400
#endif

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    VstPlugin* plugin = (effect && effect->user) ? (VstPlugin*)effect->user : 0;

    switch (opcode)
    {
    case audioMasterAutomate:
        if (plugin)
        {
            plugin->setParameterValue(index, opt);
            if (plugin->gui())
                plugin->gui()->setParameterValue(index, opt);
        }
        return 1; // FIXME?

    case audioMasterVersion:
        return kVstVersion;

    case audioMasterIdle:
        if (effect)
            effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0.0f);
        return 1; // FIXME?

    case audioMasterGetTime:
        if (audioDevice && audioDevice->deviceType() == AudioDevice::JACK_AUDIO)
        {
            jack_client_t* client = ((JackAudioDevice*)audioDevice)->getJackClient();
            if (client)
            {
                static VstTimeInfo timeInfo;
                memset(&timeInfo, 0, sizeof(VstTimeInfo));

                static jack_transport_state_t jack_state;
                static jack_position_t jack_pos;
                jack_state = jack_transport_query(client, &jack_pos);

                if (jack_state == JackTransportRolling)
                    timeInfo.flags |= kVstTransportChanged|kVstTransportPlaying;
                else
                    timeInfo.flags |= kVstTransportChanged;

                if (jack_pos.unique_1 == jack_pos.unique_2)
                {
                    timeInfo.samplePos  = jack_pos.frame;
                    timeInfo.sampleRate = jack_pos.frame_rate;

                    if (jack_pos.valid & JackPositionBBT)
                    {
                        // Tempo
                        timeInfo.tempo = jack_pos.beats_per_minute;
                        timeInfo.timeSigNumerator = jack_pos.beats_per_bar;
                        timeInfo.timeSigDenominator = jack_pos.beat_type;
                        timeInfo.flags |= kVstTempoValid|kVstTimeSigValid;

                        // Position
                        double dPos = timeInfo.samplePos / timeInfo.sampleRate;
                        timeInfo.nanoSeconds = dPos * 1000.0;
                        timeInfo.flags |= kVstNanosValid;

                        // Position
                        timeInfo.barStartPos = 0;
                        timeInfo.ppqPos = dPos * timeInfo.tempo / 60.0;
                        timeInfo.flags |= kVstBarsValid|kVstPpqPosValid;
                    }
                }
                return (intptr_t)&timeInfo;
            }
        }
        return 0;

    case audioMasterTempoAt:
        // Deprecated in VST SDK 2.4
        // TODO - return BPM at a specific song pos
        return 0;

    case audioMasterGetNumAutomatableParameters:
        // Deprecated in VST SDK 2.4
        // TODO
        return 0;

    case audioMasterIOChanged:
        if (plugin)
        {
            if (plugin->active())
            {
                plugin->dispatcher(effStopProcess, 0, 0, 0, 0.0f);
                plugin->dispatcher(effMainsChanged, 0, 0, 0, 0.0f);
            }

            plugin->reload();

            if (plugin->active())
            {
                plugin->dispatcher(effMainsChanged, 0, 1, 0, 0.0f);
                plugin->dispatcher(effStartProcess, 0, 0, 0, 0.0f);
            }
        }
        return 1;

    case audioMasterNeedIdle:
        // Deprecated in VST SDK 2.4
        effect->dispatcher(effect, effIdle, 0, 0, 0, 0.0f);
        return 1;

    case audioMasterSizeWindow:
        if (plugin)
            plugin->resizeNativeGui(index, value);
        return 1;

    case audioMasterGetSampleRate:
        return sampleRate;

    case audioMasterGetBlockSize:
        return segmentSize;

    case audioMasterGetVendorString:
        if (ptr) strcpy((char*)ptr, "OpenOctave");
        return 1;

    case audioMasterGetProductString:
        if (ptr) strcpy((char*)ptr, "OpenOctave MIDI");
        return 1;

    case audioMasterGetVendorVersion:
        return 2012;

    case audioMasterCanDo:
        if (strcmp((char*)ptr, "sendVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
            return -1;
        else if (strcmp((char*)ptr, "sizeWindow") == 0)
            return 1;
        else if (strcmp((char*)ptr, "acceptIOChanges") == 0)
            return 1;
        else
            return 0;

    case audioMasterGetLanguage:
        return kVstLangEnglish;

    default:
        return 0;
    }
}

VstPlugin::VstPlugin()
    : BasePlugin()
{
    m_type = PLUGIN_VST;

    isOldSdk = false;

    ui.width = 0;
    ui.height = 0;
    ui.widget = 0;

    effect = 0;
    events.numEvents = 0;
    events.reserved  = 0;

    for (int i=0; i<MAX_VST_EVENTS; i++)
    {
        memset(&midiEvents[i], 0, sizeof(VstMidiEvent));
        events.data[i] = (VstEvent*)&midiEvents[i];
    }
}

VstPlugin::~VstPlugin()
{
    if (effect)
    {
        if (ui.widget)
        {
            effect->dispatcher(effect, effEditClose, 0, 0, 0, 0.0f);
            delete ui.widget;
        }

        if (m_activeBefore)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, 0, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0.0f);
        }

        effect->dispatcher(effect, effClose, 0, 0, 0, 0.0f);
    }

    // use global client to delete synth audio ports
    if (m_hints & PLUGIN_IS_SYNTH)
    {
        jack_client_t* jclient = 0;
        if (audioDevice && audioDevice->deviceType() == AudioDevice::JACK_AUDIO)
            jclient = ((JackAudioDevice*)audioDevice)->getJackClient();

        if (m_ainsCount > 0)
        {
            for (uint32_t i=0; i < m_ainsCount; i++)
                jack_port_unregister(jclient, m_ports_in[i]);
        }

        if (m_aoutsCount > 0)
        {
            for (uint32_t i=0; i < m_ainsCount; i++)
                jack_port_unregister(jclient, m_ports_out[i]);
        }
    }
}

void VstPlugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    AEffect* effect = (AEffect*)nativeHandle;

    plugi->m_audioInputCount = effect->numInputs;
    plugi->m_audioOutputCount = effect->numOutputs;

    char buf_str[255] = { 0 };
    effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);

    if (buf_str[0] != 0)
        plugi->m_name = QString(buf_str);
    else
        plugi->m_name = QString(label);

    buf_str[0] = 0;
    effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    if (buf_str[0] != 0)
        plugi->m_maker = QString(buf_str);

    if ((effect->flags & effFlagsIsSynth) > 0 && plugi->m_audioOutputCount > 0)
        plugi->m_hints |= PLUGIN_IS_SYNTH;
    else if (plugi->m_audioInputCount >= 1 && plugi->m_audioOutputCount >= 1)
        plugi->m_hints |= PLUGIN_IS_FX;

    if (effect->flags & effFlagsHasEditor)
        plugi->m_hints |= PLUGIN_HAS_NATIVE_GUI;

    Q_UNUSED(filename);
}

bool VstPlugin::init(QString filename, QString label)
{
    m_lib = lib_open(filename.toAscii().constData());

    if (m_lib)
    {
        VST_Function vstfn = (VST_Function) lib_symbol(m_lib, "VSTPluginMain");

        if (! vstfn)
        {
            vstfn = (VST_Function) lib_symbol(m_lib, "main");

#ifdef TARGET_API_MAC_CARBON
            if (! vstfn)
                vstfn = (VST_Function)lib_symbol(m_lib, "main_macho");
#endif
        }

        if (vstfn)
        {
            effect = vstfn(VstHostCallback);

            if (effect && (effect->flags & effFlagsCanReplacing) > 0)
            {
                effect->dispatcher(effect, effOpen, 0, 0, 0, 0.0f);
                effect->dispatcher(effect, effSetSampleRate, 0, 0, 0, sampleRate);
                effect->dispatcher(effect, effSetBlockSize, 0, segmentSize, 0, 0.0f);
                effect->dispatcher(effect, effSetProcessPrecision, 0, 0 /* float 32bit */, 0, 0.0f);
                effect->user = this;

                if (effect->dispatcher(effect, effGetVstVersion, 0, 0, 0, 0.0f) <= 2)
                    isOldSdk = true;

                // store information
                char buf_str[255] = { 0 };
                effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
                if (buf_str[0] != 0)
                    m_name = QString(buf_str);
                else
                    m_name = label;

                buf_str[0] = 0;
                effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
                if (buf_str[0] != 0)
                    m_maker = QString(buf_str);

                m_label = label;
                m_copyright = m_maker;
                m_filename = filename;
                m_uniqueId = effect->uniqueID;

                // reload port information
                reload();

                // GUI Stuff
                if (effect->flags & effFlagsHasEditor)
                {
                    m_hints |= PLUGIN_HAS_NATIVE_GUI;
                    // create UI only when requested
                }

                // plugin is valid
                return true;
            }
            else
                set_last_error("Plugin failed to initialize");
        }
        else
            set_last_error("Not a VST plugin");
    }
    else
        set_last_error(lib_error());

    return false;
}

void VstPlugin::reload()
{
    // safely disable plugin during reload
    m_proc_lock.lock();
    m_enabled = false;
    m_proc_lock.unlock();

    // use global client to create/delete synth audio ports
    jack_client_t* jclient = 0;
    if (audioDevice && audioDevice->deviceType() == AudioDevice::JACK_AUDIO)
        jclient = ((JackAudioDevice*)audioDevice)->getJackClient();

    // delete old data
    if (m_paramCount > 0)
        delete[] m_params;

    if (m_ainsCount > 0)
    {
        for (uint32_t i=0; i < m_ainsCount; i++)
            jack_port_unregister(jclient, m_ports_in[i]);

        delete[] m_ports_in;
    }

    if (m_aoutsCount > 0)
    {
        for (uint32_t i=0; i < m_ainsCount; i++)
            jack_port_unregister(jclient, m_ports_out[i]);

        delete[] m_ports_out;
    }

    // reset
    m_hints  = 0;
    m_params = 0;
    m_ainsCount  = 0;
    m_aoutsCount = 0;
    m_paramCount = 0;

    // query new data
    m_ainsCount  = effect->numInputs;
    m_aoutsCount = effect->numOutputs;
    m_paramCount = effect->numParams;

    if (effect->flags & effFlagsIsSynth && effect->numOutputs > 0)
        m_hints |= PLUGIN_IS_SYNTH;
    else if (effect->numInputs > 1 && effect->numOutputs > 1)
        m_hints |= PLUGIN_IS_FX;

    // allocate data
    if (m_paramCount > 0)
        m_params = new ParameterPort[m_paramCount];

    if (m_hints & PLUGIN_IS_SYNTH)
    {
        // synths output directly to jack
        if (m_ainsCount > 0)
        {
            m_ports_in = new jack_port_t* [m_ainsCount];

            for (uint32_t j=0; j<m_ainsCount; j++)
            {
                QString port_name = m_name + ":input_" + QString::number(j+1);
                m_ports_in[j] = jack_port_register(jclient, port_name.toUtf8().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            }
        }

        if (m_aoutsCount > 0)
        {
            m_ports_out = new jack_port_t* [m_aoutsCount];

            for (uint32_t j=0; j<m_aoutsCount; j++)
            {
                QString port_name = m_name + ":output_" + QString::number(j+1);
                m_ports_out[j] = jack_port_register(jclient, port_name.toUtf8().constData(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            }
        }
    }

    for (uint32_t j=0; j<m_paramCount; j++)
    {
        m_params[j].type = PARAMETER_INPUT;
        m_params[j].hints |= PARAMETER_IS_ENABLED;
        m_params[j].rindex = j;

        double min, max, def, step, step_small, step_large;

        VstParameterProperties prop;
        prop.flags = 0;

        if (effect->dispatcher(effect, effGetParameterProperties, j, 0, &prop, 0))
        {
            if (prop.flags & kVstParameterUsesIntegerMinMax)
            {
                min = prop.minInteger;
                max = prop.maxInteger;
            }
            else
            {
                min = 0.0;
                max = 1.0;
            }

            if (prop.flags & kVstParameterUsesIntStep)
            {
                step = prop.stepInteger;
                step_small = prop.stepInteger;
                step_large = prop.largeStepInteger;
                m_params[j].hints |= PARAMETER_IS_INTEGER;
            }
            else if (prop.flags & kVstParameterUsesFloatStep)
            {
                step = prop.stepFloat;
                step_small = prop.smallStepFloat;
                step_large = prop.largeStepFloat;
            }
            else if (prop.flags & kVstParameterIsSwitch)
            {
                step = max - min;
                step_small = step;
                step_large = step;
                m_params[j].hints |= PARAMETER_IS_TOGGLED;
            }
            else
            {
                double range = max - min;
                step = range/100.0;
                step_small = range/1000.0;
                step_large = range/10.0;
            }
        }
        else
        {
            min = 0.0;
            max = 1.0;
            step = 0.001;
            step_small = 0.0001;
            step_large = 0.1;
        }

        if (min > max)
            max = min;
        else if (max < min)
            min = max;

        // no such thing as VST default parameters
        def = effect->getParameter(effect, j);

        if (def < min)
            def = min;
        else if (def > max)
            def = max;

        if (max - min == 0.0)
        {
            qWarning("Broken plugin parameter -> max - min == 0");
            max = min + 0.1;
        }

        m_params[j].ranges.min = min;
        m_params[j].ranges.max = max;
        m_params[j].ranges.def = def;
        m_params[j].ranges.step = step;
        m_params[j].ranges.step_small = step_small;
        m_params[j].ranges.step_large = step_large;
        m_params[j].value = def;

        if (isOldSdk || effect->dispatcher(effect, effCanBeAutomated, j, 0, 0, 0.0f) == 1)
            m_params[j].hints |= PARAMETER_IS_AUTOMABLE;
    }

    reloadPrograms(true);

    // enable it again
    m_proc_lock.lock();
    m_enabled = true;
    m_proc_lock.unlock();
}

void VstPlugin::reloadPrograms(bool)
{
    // TODO
}

QString VstPlugin::getParameterName(uint32_t index)
{
    char buf_str[255] = { 0 };
    effect->dispatcher(effect, effGetParamName, index, 0, buf_str, 0.0f);
    if (buf_str[0] != 0)
        return QString(buf_str);
    return QString("");
}

void VstPlugin::setNativeParameterValue(uint32_t index, double value)
{
    effect->setParameter(effect, index, value);
}

bool VstPlugin::hasNativeGui()
{
    return (m_hints & PLUGIN_HAS_NATIVE_GUI);
}

void VstPlugin::showNativeGui(bool yesno)
{
    // Initialize UI if needed
    if (! ui.widget)
    {
        ui.widget = new QWidget();
        // TODO - set X11 Display as 'value'
        if (effect->dispatcher(effect, effEditOpen, 0, 0, (void*)ui.widget->winId(), 0.0f) == 1)
        {
#ifndef ERect
            struct ERect {
                short top;
                short left;
                short bottom;
                short right;
            };
#endif
            ERect* vst_rect;


            if (effect->dispatcher(effect, effEditGetRect, 0, 0, &vst_rect, 0.0f))
            {
                int width  = vst_rect->right  - vst_rect->left;
                int height = vst_rect->bottom - vst_rect->top;
                ui.widget->setFixedSize(width, height);
            }
        }
        else
        {
            // failed to open UI
            m_hints -= PLUGIN_HAS_NATIVE_GUI;
            effect->dispatcher(effect, effEditClose, 0, 0, 0, 0.0f);
        }
    }

    ui.widget->setVisible(yesno);
}

bool VstPlugin::nativeGuiVisible()
{
    if (ui.widget)
        return ui.widget->isVisible();
    return false;
}

void VstPlugin::updateNativeGui()
{
    if (ui.widget)
    {
        if (ui.width > 0 && ui.height > 0)
            ui.widget->setFixedSize(ui.width, ui.height);
        ui.width = 0;
        ui.height = 0;
    }

    effect->dispatcher(effect, effIdle, 0, 0, 0, 0.0f);
}

void VstPlugin::resizeNativeGui(int width, int height)
{
    // We need to postpone this to the main event thread
    ui.width = width;
    ui.height = height;
}

intptr_t VstPlugin::dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    return effect->dispatcher(effect, opcode, index, value, ptr, opt);
}

void VstPlugin::process(uint32_t frames, float** src, float** dst, MPEventList* eventList)
{
    if (effect && m_enabled)
    {
        m_proc_lock.lock();
        // --------------------------

        if (m_active)
        {
            if ((m_hints & PLUGIN_IS_SYNTH) == 0 && (effect->numInputs != effect->numOutputs || effect->numOutputs != m_channels))
            {
                // cannot proccess
                m_proc_lock.unlock();
                return;
            }

            // activate if needed
            if (m_activeBefore == false)
            {
                effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, 0, 0.0f);
            }

            // Process MIDI events
            if (eventList)
            {
                uint32_t midiEventCount = 0;

                iMPEvent ev = eventList->begin();
                for (; ev != eventList->end(); ++ev)
                {
                    qWarning("Has event -> %i | %i | %i : 0x%02X 0x%02X 0x%02X", ev->channel(), ev->len(), ev->time(), ev->type(), ev->dataA(), ev->dataB());
                    VstMidiEvent* midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->midiData[0] = ev->type() + ev->channel();
                    midiEvent->midiData[1] = ev->dataA();
                    midiEvent->midiData[2] = ev->dataB();

                    midiEventCount += 1;
                }
                eventList->erase(eventList->begin(), ev);

                // VST Events
                if (midiEventCount > 0)
                {
                    events.numEvents = midiEventCount;
                    events.reserved = 0;
                    effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
                }
            }

            // Process automation
            if (m_track && m_track->automationType() != AUTO_OFF && m_id != -1)
            {
                for (uint32_t i = 0; i < m_paramCount; i++)
                {
                    if (m_params[i].enCtrl && m_params[i].en2Ctrl)
                    {
                        m_params[i].tmpValue = m_track->pluginCtrlVal(genACnum(m_id, i));
                    }

                    if (m_params[i].value != m_params[i].tmpValue)
                    {
                        m_params[i].value = m_params[i].tmpValue;
                        effect->setParameter(effect, i, m_params[i].value);
                    }
                }
            }

            effect->processReplacing(effect, src, dst, frames);
        }
        else
        {
            if (m_activeBefore)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, 0, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0.0f);
            }
        }

        m_activeBefore = m_active;

        // --------------------------
        m_proc_lock.unlock();
    }
}

void VstPlugin::bufferSizeChanged(uint32_t bsize)
{
    if (m_active)
    {
        effect->dispatcher(effect, effStopProcess, 0, 0, 0, 0.0f);
        effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0.0f);
    }

    effect->dispatcher(effect, effSetBlockSize, 0, bsize, 0, 0.0f);

    if (m_active)
    {
        effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0.0f);
        effect->dispatcher(effect, effStartProcess, 0, 0, 0, 0.0f);
    }
}

bool VstPlugin::readConfiguration(Xml& xml, bool readPreset)
{
    QString new_filename;
    QString new_label;

    if (readPreset == false)
        m_channels = 1;

    for (;;)
    {
        Xml::Token token(xml.parse());
        const QString& tag(xml.s1());

        switch (token)
        {
            case Xml::Error:
            case Xml::End:
                return true;

            case Xml::TagStart:
                if (readPreset == false && !m_lib)
                {
                    QFileInfo fi(new_filename);

                    if (fi.exists() == false)
                    {
                        PluginI* plugi = plugins.find(fi.completeBaseName(), new_label);
                        if (plugi)
                            new_filename = plugi->filename();
                    }

                    if (init(new_filename, new_label))
                    {
                        xml.parse1();
                        break;
                    }
                    else
                        return true;
                }

                if (tag == "parameter")
                {
                    loadParameter(xml);
                }
                else if (tag == "chunk")
                {
                    QByteArray chunk = QByteArray::fromBase64(xml.parse1().toUtf8().constData());
                    effect->dispatcher(effect, effSetChunk, 1 /* Preset */, chunk.size(), chunk.data(), 0.0f);
                }
                else if (tag == "active")
                {
                    if (readPreset == false)
                        m_active = xml.parseInt();
                }
                else if (tag == "gui")
                {
                    bool yesno = xml.parseInt();

                    if (yesno)
                    {
                        if (! m_gui)
                            m_gui = new PluginGui(this);
                        m_gui->show();
                    }
                }
                else if (tag == "geometry")
                {
                    QRect r(readGeometry(xml, tag));
                    if (m_gui)
                    {
                        m_gui->resize(r.size());
                        m_gui->move(r.topLeft());
                    }
                }
                else
                    xml.unknown("VstPlugin");

                break;

            case Xml::Attribut:
                if (tag == "filename")
                {
                    if (readPreset == false)
                        new_filename = xml.s2();
                }
                else if (tag == "label")
                {
                    QString s = xml.s2();

                    if (readPreset)
                    {
                        if (s != m_label)
                        {
                            printf("Error: Wrong preset label %s. Label must be %s\n",
                                    s.toUtf8().constData(), m_label.toUtf8().constData());
                            return true;
                        }
                    }
                    else
                        new_label = s;
                }
                else if (tag == "channels")
                {
                    if (readPreset == false)
                        m_channels = xml.s2().toInt();
                }
                break;

            case Xml::TagEnd:
                if (tag == "VstPlugin")
                {
                    if (m_lib)
                    {
                        if (m_gui)
                            m_gui->updateValues();
                        return false;
                    }
                }
                return true;

            default:
                break;
        }
    }
    return true;
}

void VstPlugin::writeConfiguration(int level, Xml& xml)
{
    xml.tag(level++, "VstPlugin filename=\"%s\" label=\"%s\" channels=\"%d\"",
            Xml::xmlString(m_filename).toLatin1().constData(), Xml::xmlString(m_label).toLatin1().constData(), m_channels);

    for (uint32_t i = 0; i < m_paramCount; i++)
    {
        double value = m_params[i].value;
        if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
            value /= sampleRate;

        QString s("parameter index=\"%1\" val=\"%2\" /");
        xml.tag(level, s.arg(m_params[i].rindex).arg(value, 0, 'f', 6).toLatin1().constData());
    }

    if (effect->flags & effFlagsProgramChunks)
    {
        void* data = 0;
        intptr_t data_size = effect->dispatcher(effect, effGetChunk, 1 /* Preset */, 0, &data, 0.0f);

        if (data && data_size >= 4)
        {
            QByteArray qchunk((const char*)data, data_size);
            const char* chunk = strdup(qchunk.toBase64().data());
            xml.strTag(level, "chunk", chunk);
            free((void*)chunk);
        }
    }

    xml.intTag(level, "active", m_active);

    if (m_gui)
    {
        xml.intTag(level, "gui", 1);
        xml.geometryTag(level, "geometry", m_gui);
    }

    xml.tag(level--, "/VstPlugin");
}

bool VstPlugin::loadParameter(Xml& xml)
{
    int32_t idx = -1;
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
            xml.unknown("VstPlugin::control");
            break;

        case Xml::Attribut:
            if (tag == "index")
                idx = xml.s2().toInt();
            else if (tag == "val")
                val = xml.s2().toDouble();
            break;

        case Xml::TagEnd:
            if (tag == "parameter" && idx >= 0 && idx < (int32_t)m_paramCount)
            {
                setParameterValue(idx, val);
                return false;
            }
            return true;

        default:
            break;
        }
    }
    return true;
}

#endif // __PLUGIN_VST_H__
