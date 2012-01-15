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
#include "song.h"

#include <math.h>

LadspaPlugin::LadspaPlugin()
{
    m_type = PLUGIN_LADSPA;
    m_paramsBuffer = 0;

    handle = 0;
    descriptor = 0;
}

LadspaPlugin::~LadspaPlugin()
{
    if (handle && descriptor->deactivate && m_activeBefore)
        descriptor->deactivate(handle);

    if (handle && descriptor->cleanup)
        descriptor->cleanup(handle);

    if (m_paramCount > 0)
        delete[] m_paramsBuffer;

    m_audioInIndexes.clear();
    m_audioOutIndexes.clear();
}

void LadspaPlugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    const LADSPA_Descriptor* descr = (LADSPA_Descriptor*)nativeHandle;

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

    if (plugi->m_audioInputCount > 1 && plugi->m_audioOutputCount > 1)
        plugi->m_hints |= PLUGIN_IS_FX;

    Q_UNUSED(filename);
    Q_UNUSED(label);
}

bool LadspaPlugin::init(QString filename, QString label)
{
    _lib = lib_open(filename.toAscii().constData());

    if (_lib)
    {
        LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function) lib_symbol(_lib, "ladspa_descriptor");

        if (descfn)
        {
            unsigned long i = 0;
            const char* c_label = strdup(label.toStdString().data());

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
                    m_name  = QString(descriptor->Name);
                    m_label = label;
                    m_maker = QString(descriptor->Maker);
                    m_copyright = QString(descriptor->Copyright);
                    m_filename = filename;
                    m_uniqueId = descriptor->UniqueID;

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

    if (ains > 1 && aouts > 1)
        m_hints |= PLUGIN_IS_FX;

    // allocate data
    if (params > 0)
    {
        m_params = new ParameterPort[params];
        m_paramsBuffer = new float[params];
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

            // hints
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
                step_large = 10.0;
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

            m_params[j].ranges.min = min;
            m_params[j].ranges.max = max;
            m_params[j].ranges.def = def;
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
}

void LadspaPlugin::reloadPrograms(bool)
{
    // LADSPA has no program support
}

QString LadspaPlugin::getParameterName(uint32_t index)
{
    if (descriptor && index < m_paramCount)
        return QString(descriptor->PortNames[m_params[index].rindex]);
    else
        return QString("");
}

void LadspaPlugin::setNativeParameterValue(uint32_t index, double value)
{
    if (descriptor && index < m_paramCount)
        m_paramsBuffer[index] = value;
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

void LadspaPlugin::process(uint32_t frames, float** src, float** dst)
{
    if (descriptor && _enabled)
    {
        _proc_lock.lock();
        if (m_active)
        {
            // connect ports
            int ains  = m_audioInIndexes.size();
            int aouts = m_audioOutIndexes.size();
            bool need_buffer_copy  = false;
            bool need_extra_buffer = false;
            uint32_t pin, pout;

            if (ains == aouts)
            {
                if (aouts <= m_channels)
                {
                    for (int i=0; i < ains; i++)
                    {
                        pin  = m_audioInIndexes.at(i);
                        pout = m_audioOutIndexes.at(i);
                        descriptor->connect_port(handle, pin, src[i]);
                        descriptor->connect_port(handle, pout, dst[i]);
                        need_buffer_copy = true;
                    }
                }
                else
                {
                    for (int i=0; i < m_channels ; i++)
                    {
                        pin  = m_audioInIndexes.at(i);
                        pout = m_audioOutIndexes.at(i);
                        descriptor->connect_port(handle, pin, src[i]);
                        descriptor->connect_port(handle, pout, dst[i]);
                        need_extra_buffer = true;
                    }
                }
            }
            else
            {
                // cannot proccess
                _proc_lock.unlock();
                return;
            }

            // Process automation
            qWarning("Automation TEST %p %i %i", m_track, m_track->automationType(), m_id);
            if (/*automation &&*/ m_track /*&& m_track->automationType() != AUTO_OFF*/ && m_id != -1)
            {
                for (uint32_t i = 0; i < m_paramCount; i++)
                {
                    if (m_params[i].enCtrl && m_params[i].en2Ctrl)
                    {
                        m_params[i].tmpValue = m_track->pluginCtrlVal(genACnum(m_id, i));
                    }

                    if (m_params[i].value != m_params[i].tmpValue)
                    {
                        qWarning("Automation Success 444 --------------------------------------------------------------------------------");
                        m_params[i].value = m_paramsBuffer[i] = m_params[i].tmpValue;
                        m_params[i].update = true;
                    }
                }
            }
            else
                qWarning("Automation Fail");

            // activate if needed
            if (m_activeBefore == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            // process
            if (need_extra_buffer)
            {
                if (m_hints & PLUGIN_HAS_IN_PLACE_BROKEN)
                {
                    // cannot proccess
                    _proc_lock.unlock();
                    return;
                }

                float extra_buffer[frames];
                memset(extra_buffer, 0, sizeof(float)*frames);

                for (int i=m_channels; i < ains ; i++)
                {
                    descriptor->connect_port(handle, m_audioInIndexes.at(i), extra_buffer);
                    descriptor->connect_port(handle, m_audioOutIndexes.at(i), extra_buffer);
                }
            }
            else
            {
                descriptor->run(handle, frames);

                if (need_buffer_copy)
                {
                    for (int i=ains; i < m_channels ; i++)
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
        _proc_lock.unlock();
    }
}

void LadspaPlugin::bufferSizeChanged(uint32_t)
{
    // note needed
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
                if (readPreset == false && ! _lib)
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
                    xml.unknown("LadspaPlugin");

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
                if (tag == "LadspaPlugin")
                {
                    if (! _lib)
                        return true;

                    if (m_gui)
                        m_gui->updateValues();

                    return false;
                }
                return true;

            default:
                break;
        }
    }
    return true;
}

void LadspaPlugin::writeConfiguration(int level, Xml& xml)
{
    xml.tag(level++, "LadspaPlugin filename=\"%s\" label=\"%s\" channels=\"%d\"",
            Xml::xmlString(m_filename).toLatin1().constData(), Xml::xmlString(m_label).toLatin1().constData(), m_channels);

    for (uint32_t i = 0; i < m_paramCount; i++)
    {
        double value = m_params[i].value;
        if (m_params[i].hints & PARAMETER_USES_SAMPLERATE)
            value /= sampleRate;

        QString s("control rindex=\"%1\" val=\"%2\" /");
        xml.tag(level, s.arg(m_params[i].rindex).arg(value, 0, 'f', 6).toLatin1().constData());
    }

    xml.intTag(level, "active", m_active);

    if (m_gui)
    {
        xml.intTag(level, "gui", 1);
        xml.geometryTag(level, "geometry", m_gui);
    }

    xml.tag(level--, "/LadspaPlugin");
}

bool LadspaPlugin::loadControl(Xml& xml)
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
            xml.unknown("LadspaPlugin::control");
            break;

        case Xml::Attribut:
            if (tag == "rindex")
                idx = xml.s2().toInt();
            else if (tag == "val")
                val = xml.s2().toDouble();
            break;

        case Xml::TagEnd:
            if (tag == "control" && idx >= 0)
                return setControl(idx, val);
            return true;

        default:
            break;
        }
    }
    return true;
}

bool LadspaPlugin::setControl(int32_t idx, double value)
{
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
