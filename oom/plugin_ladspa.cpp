//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_LADSPA_H__
#define __PLUGIN_LADSPA_H__

#include "plugin.h"
#include "plugingui.h"
#include "track.h"
#include "xml.h"

#include <math.h>

LadspaPlugin::LadspaPlugin()
    : BasePlugin()
{
    m_type = PLUGIN_LADSPA;
    m_paramsBuffer = 0;

    handle = 0;
    descriptor = 0;
}

LadspaPlugin::~LadspaPlugin()
{
    aboutToRemove();

    if (handle && descriptor->deactivate && m_activeBefore)
        descriptor->deactivate(handle);

    if (handle && descriptor->cleanup)
        descriptor->cleanup(handle);

    handle = 0;
    descriptor = 0;

    if (m_paramCount > 0)
    {
        delete[] m_paramsBuffer;
        m_paramsBuffer = 0;
    }

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();
}

void LadspaPlugin::initPluginI(PluginI* plugi, const QString&, const QString&, const void* nativeHandle)
{
    const LADSPA_Descriptor* descr = (LADSPA_Descriptor*)nativeHandle;
    plugi->m_hints = 0;
    plugi->m_audioInputCount  = 0;
    plugi->m_audioOutputCount = 0;

    for (unsigned long i=0; i < descr->PortCount; i++)
    {
        if (LADSPA_IS_PORT_AUDIO(descr->PortDescriptors[i]))
        {
            if (LADSPA_IS_PORT_INPUT(descr->PortDescriptors[i]))
                plugi->m_audioInputCount += 1;
            else if (LADSPA_IS_PORT_OUTPUT(descr->PortDescriptors[i]))
                plugi->m_audioOutputCount += 1;
        }
    }

    plugi->m_name  = QString(descr->Name);
    plugi->m_maker = QString(descr->Maker);

    if (LADSPA_IS_INPLACE_BROKEN(descr->Properties))
        plugi->m_hints |= PLUGIN_HAS_IN_PLACE_BROKEN;

    if (plugi->m_audioInputCount == plugi->m_audioOutputCount && plugi->m_audioOutputCount >= 1)
        // we can only process plugins that have the same number of input/output audio ports
        plugi->m_hints |= PLUGIN_IS_FX;
}

bool LadspaPlugin::init(QString filename, QString label)
{
    m_lib = lib_open(filename.toUtf8().constData());

    if (m_lib)
    {
        LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function) lib_symbol(m_lib, "ladspa_descriptor");

        if (descfn)
        {
            unsigned long i = 0;
            const char* c_label = strdup(label.toUtf8().constData());

            while ((descriptor = descfn(i++)))
            {
                if (strcmp(descriptor->Label, c_label) == 0)
                    break;
            }

            // don't need this anymore
            free((void*)c_label);

            if (descriptor)
            {
                handle = descriptor->instantiate(descriptor, sampleRate);

                if (handle)
                {
                    // store information
                    m_label = label;
                    m_maker = QString(descriptor->Maker);
                    m_copyright = QString(descriptor->Copyright);
                    m_filename = filename;
                    m_uniqueId = descriptor->UniqueID;
                    
                    if (m_name.isEmpty())
                        m_name  = QString(descriptor->Name);                    

                    // reload port information
                    reload();

                    // plugin is valid
                    return true;
                }
                else
                {
                    descriptor = 0;
                    set_last_error("Plugin failed to initialize");
                }
            }
            else
                set_last_error("Failed to find the requested plugin Label in the plugin library");
        }
        else
            set_last_error("Not a LADSPA plugin");
    }
    else
        set_last_error(lib_error());

    return false;
}

void LadspaPlugin::reload()
{
    // safely disable plugin during reload
    m_proc_lock.lock();
    m_enabled = false;
    m_proc_lock.unlock();

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
    m_paramCount   = 0;
    m_paramsBuffer = 0;

    // query new data
    uint32_t ains, aouts, params, j;
    ains = aouts = params = 0;

    const unsigned long PortCount = descriptor->PortCount;

    for (unsigned long i=0; i<PortCount; i++)
    {
        LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[i];
        if (LADSPA_IS_PORT_AUDIO(PortType))
        {
            if (LADSPA_IS_PORT_INPUT(PortType))
                ains += 1;
            else if (LADSPA_IS_PORT_OUTPUT(PortType))
                aouts += 1;
        }
        else if (LADSPA_IS_PORT_CONTROL(PortType))
            params += 1;
    }

    // plugin hints
    if (LADSPA_IS_INPLACE_BROKEN(descriptor->Properties))
        m_hints |= PLUGIN_HAS_IN_PLACE_BROKEN;

    if (ains == aouts && aouts >= 1)
        m_hints |= PLUGIN_IS_FX;

    // allocate data
    if (params > 0)
    {
        m_params = new ParameterPort [params];
        m_paramsBuffer = new float [params];
    }

    // fill in all the data
    for (unsigned long i=0; i<PortCount; i++)
    {
        LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[i];
        LADSPA_PortRangeHint PortHint  = descriptor->PortRangeHints[i];

        if (LADSPA_IS_PORT_AUDIO(PortType))
        {
            if (LADSPA_IS_PORT_INPUT(PortType))
            {
                m_audioInIndexes.push_back(i);
            }
            else if (LADSPA_IS_PORT_OUTPUT(PortType))
            {
                m_audioOutIndexes.push_back(i);
            }
            else
                qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
        }
        else if (LADSPA_IS_PORT_CONTROL(PortType))
        {
            j = m_paramCount++;
            m_params[j].rindex = i;

            double min, max, def, step, step_small, step_large;

            // min value
            if (LADSPA_IS_HINT_BOUNDED_BELOW(PortHint.HintDescriptor))
                min = PortHint.LowerBound;
            else
                min = 0.0;

            // max value
            if (LADSPA_IS_HINT_BOUNDED_ABOVE(PortHint.HintDescriptor))
                max = PortHint.UpperBound;
            else
                max = 1.0;

            if (min > max)
                max = min;
            else if (max < min)
                min = max;

            // default value
            if (LADSPA_IS_HINT_HAS_DEFAULT(PortHint.HintDescriptor))
            {
                switch (PortHint.HintDescriptor & LADSPA_HINT_DEFAULT_MASK)
                {
                case LADSPA_HINT_DEFAULT_MINIMUM:
                    def = min;
                    break;
                case LADSPA_HINT_DEFAULT_MAXIMUM:
                    def = max;
                    break;
                case LADSPA_HINT_DEFAULT_0:
                    def = 0.0;
                    break;
                case LADSPA_HINT_DEFAULT_1:
                    def = 1.0;
                    break;
                case LADSPA_HINT_DEFAULT_100:
                    def = 100.0;
                    break;
                case LADSPA_HINT_DEFAULT_440:
                    def = 440.0;
                    break;
                case LADSPA_HINT_DEFAULT_LOW:
                    if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                        def = exp((log(min)*0.75) + (log(max)*0.25));
                    else
                        def = (min*0.75) + (max*0.25);
                    break;
                case LADSPA_HINT_DEFAULT_MIDDLE:
                    if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                        def = sqrt(min*max);
                    else
                        def = (min+max)/2;
                    break;
                case LADSPA_HINT_DEFAULT_HIGH:
                    if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                        def = exp((log(min)*0.25) + (log(max)*0.75));
                    else
                        def = (min*0.25) + (max*0.75);
                    break;
                default:
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
                    else
                        def = min;
                    break;
                }
            }
            else
            {
                // no default value
                if (min < 0.0 && max > 0.0)
                    def = 0.0;
                else
                    def = min;
            }

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (max - min == 0.0)
            {
                qWarning("Broken plugin parameter -> max - min == 0");
                max = min + 0.1;
            }

            // parameter hints
            if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
            {
                min *= sampleRate;
                max *= sampleRate;
                def *= sampleRate;
                m_params[j].hints |= PARAMETER_USES_SAMPLERATE;
            }

            if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
            {
                m_params[j].hints |= PARAMETER_IS_LOGARITHMIC;
            }

            if (LADSPA_IS_HINT_INTEGER(PortHint.HintDescriptor))
            {
                step = 1.0;
                step_small = 1.0;
                step_large = (max - min > 10.0) ? 10.0 : 1.0;
                m_params[j].hints |= PARAMETER_IS_INTEGER;
            }
            else if (LADSPA_IS_HINT_TOGGLED(PortHint.HintDescriptor))
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

            if (LADSPA_IS_PORT_INPUT(PortType))
            {
                m_params[j].type = PARAMETER_INPUT;
                m_params[j].hints |= PARAMETER_IS_ENABLED;
                m_params[j].hints |= PARAMETER_IS_AUTOMABLE;
            }
            else if (LADSPA_IS_PORT_OUTPUT(PortType))
            {
                m_params[j].type = PARAMETER_OUTPUT;
                m_params[j].hints |= PARAMETER_IS_ENABLED;

                if (strcmp(descriptor->PortNames[i], "latency") != 0 && strcmp(descriptor->PortNames[i], "_latency") != 0)
                {
                    m_params[j].hints |= PARAMETER_IS_AUTOMABLE;
                }
                else
                {
                    // latency parameter
                    min = 0;
                    max = sampleRate;
                    def = 0;
                    step = 1;
                    step_small = 1;
                    step_large = 1;
                }
            }
            else
            {
                qWarning("WARNING - Got a broken Port (Control, but not input or output)");
            }

            m_params[j].ranges.def = def;
            m_params[j].ranges.min = min;
            m_params[j].ranges.max = max;
            m_params[j].ranges.step = step;
            m_params[j].ranges.step_small = step_small;
            m_params[j].ranges.step_large = step_large;

            // Start parameters in their default values
            m_params[j].value = m_params[j].tmpValue = m_paramsBuffer[j] = def;

            descriptor->connect_port(handle, i, &m_paramsBuffer[j]);
        }
        else
        {
            // Not Audio or Control
            qCritical("ERROR - Got a broken Port (neither Audio or Control)");
            descriptor->connect_port(handle, i, 0);
        }
    }

    // enable it again
    m_proc_lock.lock();
    m_enabled = true;
    m_proc_lock.unlock();
}

void LadspaPlugin::reloadPrograms(bool)
{
    // LADSPA has no programs support
}

QString LadspaPlugin::getParameterName(uint32_t index)
{
    if (index < m_paramCount && descriptor)
        return QString(descriptor->PortNames[m_params[index].rindex]);
    return QString("");
}

QString LadspaPlugin::getParameterUnit(uint32_t)
{
    return QString("");
}

void LadspaPlugin::setNativeParameterValue(uint32_t index, double value)
{
    if (index < m_paramCount)
        m_paramsBuffer[index] = value;
}

uint32_t LadspaPlugin::getProgramCount()
{
    return 0;
}

QString LadspaPlugin::getProgramName(uint32_t)
{
    return QString("");
}

void LadspaPlugin::setProgram(uint32_t)
{
}

bool LadspaPlugin::hasNativeGui()
{
    return false;
}

void LadspaPlugin::showNativeGui(bool)
{
}

bool LadspaPlugin::nativeGuiVisible()
{
    return false;
}

void LadspaPlugin::updateNativeGui()
{
}

void LadspaPlugin::process(uint32_t frames, float** src, float** dst, MPEventList*)
{
    if (descriptor && m_enabled)
    {
        m_proc_lock.lock();
        // --------------------------

        if (m_active)
        {
            // connect ports
            int ains  = m_audioInIndexes.size();
            int aouts = m_audioOutIndexes.size();
            bool need_buffer_copy  = false;
            bool need_extra_buffer = false;

            if (ains == aouts)
            {
                uint32_t pin, pout;
                int max = m_channels;

                if (aouts < m_channels)
                {
                    max = aouts;
                    need_buffer_copy = true;
                }
                else if (aouts > m_channels)
                {
                    need_extra_buffer = true;
                }

                for (int i=0; i < max; i++)
                {
                    pin  = m_audioInIndexes.at(i);
                    pout = m_audioOutIndexes.at(i);
                    descriptor->connect_port(handle, pin, src[i]);
                    descriptor->connect_port(handle, pout, dst[i]);
                }
            }
            else
            {
                // cannot proccess (this should not happen)
                m_proc_lock.unlock();
                return;
            }

            // activate if needed
            if (m_activeBefore == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            // Process automation
            if (automation && m_track && m_track->automationType() != AUTO_OFF && m_id != -1)
            {
                for (uint32_t i = 0; i < m_paramCount; i++)
                {
                    if (m_params[i].enCtrl && m_params[i].en2Ctrl)
                    {
                        m_params[i].tmpValue = m_track->pluginCtrlVal(genACnum(m_id, i));
                    }

                    if (m_params[i].value != m_params[i].tmpValue)
                    {
                        m_params[i].value = m_paramsBuffer[i] = m_params[i].tmpValue;
                        m_params[i].update = true;
                    }
                }
            }

            // process
            if (need_extra_buffer)
            {
                if (m_hints & PLUGIN_HAS_IN_PLACE_BROKEN)
                {
                    // cannot proccess
                    m_proc_lock.unlock();
                    return;
                }

                float extra_buffer[frames];
                memset(extra_buffer, 0, sizeof(float)*frames);

                for (int i=m_channels; i < aouts ; i++)
                {
                    descriptor->connect_port(handle, m_audioInIndexes.at(i), extra_buffer);
                    descriptor->connect_port(handle, m_audioOutIndexes.at(i), extra_buffer);
                }

                descriptor->run(handle, frames);
            }
            else
            {
                descriptor->run(handle, frames);

                if (need_buffer_copy)
                {
                    for (int i=aouts; i < m_channels ; i++)
                        memcpy(dst[i], dst[i-1], sizeof(float)*frames);
                }
            }
        }
        else
        {   // not active
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
            }
        }

        m_activeBefore = m_active;

        // --------------------------
        m_proc_lock.unlock();
    }
}

void LadspaPlugin::bufferSizeChanged(uint32_t)
{
    // not needed
}

bool LadspaPlugin::readConfiguration(Xml& xml, bool readPreset)
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

                if (init(new_filename, new_label) == false)
                    // plugin failed to initialize
                    return true;
            }

            if (tag == "control")
            {
                loadControl(xml);
            }
            else if (tag == "active")
            {
                if (readPreset == false)
                    m_active = xml.parseInt();
            }
            else if (tag == "gui")
            {
                bool yesno = xml.parseInt();
                showGui(yesno);
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
                xml.unknown("LadspaPlugin");

            break;

        case Xml::Attribut:
            if (tag == "filename" || tag == "file")
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
            else if (tag == "channels" || tag == "channel")
            {
                if (readPreset == false)
                    m_channels = xml.s2().toInt();
            }
            break;

        case Xml::TagEnd:
            if (tag == "LadspaPlugin" || tag == "plugin")
            {
                if (m_lib)
                {
                    if (m_gui)
                        m_gui->updateValues();
                    return false;
                }
            	return true;
            }

        default:
            break;
        }
    }
    return true;
}

void LadspaPlugin::writeConfiguration(int level, Xml& xml)
{
    xml.tag(level++, "LadspaPlugin filename=\"%s\" label=\"%s\" channels=\"%d\"",
            Xml::xmlString(m_filename).toUtf8().constData(), Xml::xmlString(m_label).toUtf8().constData(), m_channels);

    for (uint32_t i = 0; i < m_paramCount; i++)
    {
        double value = m_params[i].value;
        if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
            value /= sampleRate;

        QString s("control rindex=\"%1\" val=\"%2\" /");
        xml.tag(level, s.arg(m_params[i].rindex).arg(value, 0, 'f', 6).toLatin1().constData());
    }

    xml.intTag(level, "active", m_active);

    if (guiVisible())
    {
        xml.intTag(level, "gui", 1);
        xml.geometryTag(level, "geometry", m_gui);
    }

    xml.tag(--level, "/LadspaPlugin");
}

bool LadspaPlugin::loadControl(Xml& xml)
{
    int32_t idx = -1;
    double val = 0.0;
    QString oldName;

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
            xml.unknown("LadspaPlugin::control");
            break;

        case Xml::Attribut:
            if (tag == "rindex")
                idx = xml.s2().toInt();
            else if (tag == "name")
                oldName = xml.s2();
            else if (tag == "val")
                val = xml.s2().toDouble();
            break;

        case Xml::TagEnd:
            if (tag == "control")
                return setControl(idx, oldName, val);
            return true;

        default:
            break;
        }
    }
    return true;
}

bool LadspaPlugin::setControl(int32_t idx, QString oldName, double value)
{
    if (idx == -1 && oldName.isEmpty() == false)
    {
        for (unsigned long i; i < descriptor->PortCount; i++)
        {
            if (QString(descriptor->PortNames[i]) == oldName)
            {
                idx = i;
                break;
            }
        }
    }

    if (idx == -1)
        return true;

    if (idx < (int32_t)descriptor->PortCount)
    {
        for (uint32_t i = 0; i < m_paramCount; i++)
        {
            if (m_params[i].rindex == idx)
            {
                if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
                    value *= sampleRate;
                m_params[i].value = m_params[i].tmpValue = m_paramsBuffer[i] = value;
                return false;
            }
        }
    }
    return true;
}

#endif // __PLUGIN_LADSPA_H__
