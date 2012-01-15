//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_LADSPA_H__
#define __PLUGIN_LADSPA_H__

#include "plugin.h"
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

void LadspaPlugin::connect(int ports, float** src, float** dst)
{
    // FIXME - this is just a test and assumes we're using 2x2 fx plugin
    unsigned long pin, pout;

    for (int i=0; i < ports; i++)
    {
        pin  = m_audioInIndexes.at(i);
        pout = m_audioOutIndexes.at(i);
        descriptor->connect_port(handle, pin, src[i]);
        descriptor->connect_port(handle, pout, dst[i]);
    }
}

void LadspaPlugin::process(uint32_t frames)
{
    if (descriptor)
    {
        if (m_active)
        {
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

            if (m_activeBefore == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            descriptor->run(handle, frames);
        }
        else
        {
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
            }
        }
        m_activeBefore = m_active;
    }
}

void LadspaPlugin::bufferSizeChanged(uint32_t)
{
    // note needed
}

bool LadspaPlugin::readConfiguration(Xml& xml, bool readPreset)
{
    return false;
}

void LadspaPlugin::writeConfiguration(int level, Xml& xml)
{
}

#endif // __PLUGIN_LADSPA_H__

#if 0
//----------------------------------------------------------------------------------
//   defaultValue
//   If no default ladspa value found, still sets *def to 1.0, but returns false.
//---------------------------------------------------------------------------------

//float ladspaDefaultValue(const LADSPA_Descriptor* plugin, int k)

bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, int port, float* val)/*{{{*/
{
    LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
    //      bool isLog = LADSPA_IS_HINT_LOGARITHMIC(rh);
    //double val = 1.0;
    float m = (rh & LADSPA_HINT_SAMPLE_RATE) ? float(sampleRate) : 1.0f;
    if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh))
    {
        *val = range.LowerBound * m;
        return true;
    }
    else if (LADSPA_IS_HINT_DEFAULT_LOW(rh))
    {
        if (LADSPA_IS_HINT_LOGARITHMIC(rh))
        {
            *val = exp(fast_log10(range.LowerBound * m) * .75 +
                    log(range.UpperBound * m) * .25);
            return true;
        }
        else
        {
            *val = range.LowerBound * .75 * m + range.UpperBound * .25 * m;
            return true;
        }
    }
    else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh))
    {
        if (LADSPA_IS_HINT_LOGARITHMIC(rh))
        {
            *val = exp(log(range.LowerBound * m) * .5 +
                    log10(range.UpperBound * m) * .5);
            return true;
        }
        else
        {
            *val = range.LowerBound * .5 * m + range.UpperBound * .5 * m;
            return true;
        }
    }
    else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh))
    {
        if (LADSPA_IS_HINT_LOGARITHMIC(rh))
        {
            *val = exp(log(range.LowerBound * m) * .25 +
                    log(range.UpperBound * m) * .75);
            return true;
        }
        else
        {
            *val = range.LowerBound * .25 * m + range.UpperBound * .75 * m;
            return true;
        }
    }
    else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh))
    {
        *val = range.UpperBound*m;
        return true;
    }
    else if (LADSPA_IS_HINT_DEFAULT_0(rh))
    {
        *val = 0.0;
        return true;
    }
    else if (LADSPA_IS_HINT_DEFAULT_1(rh))
    {
        *val = 1.0;
        return true;
    }
    else if (LADSPA_IS_HINT_DEFAULT_100(rh))
    {
        *val = 100.0;
        return true;
    }
    else if (LADSPA_IS_HINT_DEFAULT_440(rh))
    {
        *val = 440.0;
        return true;
    }

    // No default found. Set return value to 1.0, but return false.
    *val = 1.0;
    return false;
}/*}}}*/

//---------------------------------------------------------
//   ladspaControlRange
//---------------------------------------------------------

void ladspaControlRange(const LADSPA_Descriptor* plugin, int i, float* min, float* max)/*{{{*/
{
    LADSPA_PortRangeHint range = plugin->PortRangeHints[i];
    LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
    if (desc & LADSPA_HINT_TOGGLED)
    {
        *min = 0.0;
        *max = 1.0;
        return;
    }
    float m = 1.0;
    if (desc & LADSPA_HINT_SAMPLE_RATE)
        m = float(sampleRate);

    if (desc & LADSPA_HINT_BOUNDED_BELOW)
        *min = range.LowerBound * m;
    else
        *min = 0.0;
    if (desc & LADSPA_HINT_BOUNDED_ABOVE)
        *max = range.UpperBound * m;
    else
        *max = 1.0;
}/*}}}*/

//---------------------------------------------------------
//   ladspa2MidiControlValues
//---------------------------------------------------------

bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, int port, int ctlnum, int* min, int* max, int* def)/*{{{*/
{
    LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

    float fmin, fmax, fdef;
    int imin, imax;
    float frng;
    //int idef;

    //ladspaControlRange(plugin, port, &fmin, &fmax);

    // TODO
    bool hasdef = false; //ladspaDefaultValue(plugin, port, &fdef);
    //bool isint = desc & LADSPA_HINT_INTEGER;
    MidiController::ControllerType t = midiControllerType(ctlnum);

#ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: ctlnum:%d ladspa port:%d has default?:%d default:%f\n", ctlnum, port, hasdef, fdef);
#endif

    if (desc & LADSPA_HINT_TOGGLED)
    {
#ifdef PLUGIN_DEBUGIN
        printf("ladspa2MidiControlValues: has LADSPA_HINT_TOGGLED\n");
#endif

        *min = 0;
        *max = 1;
        *def = (int) lrint(fdef);
        return hasdef;
    }

    float m = 1.0;
    if (desc & LADSPA_HINT_SAMPLE_RATE)
    {
#ifdef PLUGIN_DEBUGIN
        printf("ladspa2MidiControlValues: has LADSPA_HINT_SAMPLE_RATE\n");
#endif

        m = float(sampleRate);
    }

    if (desc & LADSPA_HINT_BOUNDED_BELOW)
    {
#ifdef PLUGIN_DEBUGIN
        printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_BELOW\n");
#endif

        fmin = range.LowerBound * m;
    }
    else
        fmin = 0.0;

    if (desc & LADSPA_HINT_BOUNDED_ABOVE)
    {
#ifdef PLUGIN_DEBUGIN
        printf("ladspa2MidiControlValues: has LADSPA_HINT_BOUNDED_ABOVE\n");
#endif

        fmax = range.UpperBound * m;
    }
    else
        fmax = 1.0;

    frng = fmax - fmin;
    imin = lrint(fmin);
    imax = lrint(fmax);
    //irng = imax - imin;

    int ctlmn = 0;
    int ctlmx = 127;

#ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: port min:%f max:%f \n", fmin, fmax);
#endif

    //bool isneg = (fmin < 0.0);
    bool isneg = (imin < 0);
    int bias = 0;
    switch (t)
    {
        case MidiController::RPN:
        case MidiController::NRPN:
        case MidiController::Controller7:
            if (isneg)
            {
                ctlmn = -64;
                ctlmx = 63;
                bias = -64;
            }
            else
            {
                ctlmn = 0;
                ctlmx = 127;
            }
            break;
        case MidiController::Controller14:
        case MidiController::RPN14:
        case MidiController::NRPN14:
            if (isneg)
            {
                ctlmn = -8192;
                ctlmx = 8191;
                bias = -8192;
            }
            else
            {
                ctlmn = 0;
                ctlmx = 16383;
            }
            break;
        case MidiController::Program:
            ctlmn = 0;
            //ctlmx = 0xffffff;
            ctlmx = 0x3fff; // FIXME: Really should not happen or be allowed. What to do here...
            break;
        case MidiController::Pitch:
            ctlmn = -8192;
            ctlmx = 8191;
            break;
        case MidiController::Velo: // cannot happen
        default:
            break;
    }
    //int ctlrng = ctlmx - ctlmn;
    float fctlrng = float(ctlmx - ctlmn);

    // Is it an integer control?
    if (desc & LADSPA_HINT_INTEGER)
    {
#ifdef PLUGIN_DEBUGIN
        printf("ladspa2MidiControlValues: has LADSPA_HINT_INTEGER\n");
#endif

        // If the upper or lower limit is beyond the controller limits, just scale the whole range to fit.
        // We could get fancy by scaling only the negative or positive domain, or each one separately, but no...
        //if((imin < ctlmn) || (imax > ctlmx))
        //{
        //  float scl = float(irng) / float(fctlrng);
        //  if((ctlmn - imin) > (ctlmx - imax))
        //    scl = float(ctlmn - imin);
        //  else
        //    scl = float(ctlmx - imax);
        //}
        // No, instead just clip the limits. ie fit the range into clipped space.
        if (imin < ctlmn)
            imin = ctlmn;
        if (imax > ctlmx)
            imax = ctlmx;

        *min = imin;
        *max = imax;

        //int idef = (int)lrint(fdef);
        //if(idef < ctlmn)
        //  idef = ctlmn;
        //if(idef > ctlmx)
        //  idef = ctlmx;
        //*def = idef;

        *def = (int) lrint(fdef);

        return hasdef;
    }

    // It's a floating point control, just use wide open maximum range.
    *min = ctlmn;
    *max = ctlmx;

    // Orcan: commented out next 2 lines to suppress compiler warning:
    //float fbias = (fmin + fmax) / 2.0;
    //float normbias = fbias / frng;
    float normdef = fdef / frng;
    fdef = normdef * fctlrng;

    // FIXME: TODO: Incorrect... Fix this somewhat more trivial stuff later....

    *def = (int) lrint(fdef) + bias;

#ifdef PLUGIN_DEBUGIN
    printf("ladspa2MidiControlValues: setting default:%d\n", *def);
#endif

    return hasdef;
}/*}}}*/

//---------------------------------------------------------
//   midi2LadspaValue
//---------------------------------------------------------

float midi2LadspaValue(const LADSPA_Descriptor* plugin, int port, int ctlnum, int val)/*{{{*/
{
    LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;

    float fmin, fmax;
    int imin;
    //int imax;
    float frng;
    //int idef;

    //ladspaControlRange(plugin, port, &fmin, &fmax);

    //bool hasdef = ladspaDefaultValue(plugin, port, &fdef);
    //bool isint = desc & LADSPA_HINT_INTEGER;
    MidiController::ControllerType t = midiControllerType(ctlnum);

#ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: ctlnum:%d ladspa port:%d val:%d\n", ctlnum, port, val);
#endif

    float m = 1.0;
    if (desc & LADSPA_HINT_SAMPLE_RATE)
    {
#ifdef PLUGIN_DEBUGIN
        printf("midi2LadspaValue: has LADSPA_HINT_SAMPLE_RATE\n");
#endif

        m = float(sampleRate);
    }

    if (desc & LADSPA_HINT_BOUNDED_BELOW)
    {
#ifdef PLUGIN_DEBUGIN
        printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_BELOW\n");
#endif

        fmin = range.LowerBound * m;
    }
    else
        fmin = 0.0;

    if (desc & LADSPA_HINT_BOUNDED_ABOVE)
    {
#ifdef PLUGIN_DEBUGIN
        printf("midi2LadspaValue: has LADSPA_HINT_BOUNDED_ABOVE\n");
#endif

        fmax = range.UpperBound * m;
    }
    else
        fmax = 1.0;

    frng = fmax - fmin;
    imin = lrint(fmin);
    //imax = lrint(fmax);
    //irng = imax - imin;

    if (desc & LADSPA_HINT_TOGGLED)
    {
#ifdef PLUGIN_DEBUGIN
        printf("midi2LadspaValue: has LADSPA_HINT_TOGGLED\n");
#endif

        if (val > 0)
            return fmax;
        else
            return fmin;
    }

    int ctlmn = 0;
    int ctlmx = 127;

#ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: port min:%f max:%f \n", fmin, fmax);
#endif

    //bool isneg = (fmin < 0.0);
    bool isneg = (imin < 0);
    int bval = val;
    int cval = val;
    switch (t)
    {
        case MidiController::RPN:
        case MidiController::NRPN:
        case MidiController::Controller7:
            if (isneg)
            {
                ctlmn = -64;
                ctlmx = 63;
                bval -= 64;
                cval -= 64;
            }
            else
            {
                ctlmn = 0;
                ctlmx = 127;
                cval -= 64;
            }
            break;
        case MidiController::Controller14:
        case MidiController::RPN14:
        case MidiController::NRPN14:
            if (isneg)
            {
                ctlmn = -8192;
                ctlmx = 8191;
                bval -= 8192;
                cval -= 8192;
            }
            else
            {
                ctlmn = 0;
                ctlmx = 16383;
                cval -= 8192;
            }
            break;
        case MidiController::Program:
            ctlmn = 0;
            ctlmx = 0xffffff;
            break;
        case MidiController::Pitch:
            ctlmn = -8192;
            ctlmx = 8191;
            break;
        case MidiController::Velo: // cannot happen
        default:
            break;
    }
    int ctlrng = ctlmx - ctlmn;
    float fctlrng = float(ctlmx - ctlmn);

    // Is it an integer control?
    if (desc & LADSPA_HINT_INTEGER)
    {
        float ret = float(cval);
        if (ret < fmin)
            ret = fmin;
        if (ret > fmax)
            ret = fmax;
#ifdef PLUGIN_DEBUGIN
        printf("midi2LadspaValue: has LADSPA_HINT_INTEGER returning:%f\n", ret);
#endif

        return ret;
    }

    // Avoid divide-by-zero error below.
    if (ctlrng == 0)
        return 0.0;

    // It's a floating point control, just use wide open maximum range.
    float normval = float(bval) / fctlrng;
    //float fbias = (fmin + fmax) / 2.0;
    //float normfbias = fbias / frng;
    //float ret = (normdef + normbias) * fctlrng;
    //float normdef = fdef / frng;

    float ret = normval * frng + fmin;

#ifdef PLUGIN_DEBUGIN
    printf("midi2LadspaValue: float returning:%f\n", ret);
#endif

    return ret;
}/*}}}*/


// Works but not needed.
/*
//---------------------------------------------------------
//   ladspa2MidiController
//---------------------------------------------------------

MidiController* ladspa2MidiController(const LADSPA_Descriptor* plugin, int port, int ctlnum)
{
  int min, max, def;

  if(!ladspa2MidiControlValues(plugin, port, ctlnum, &min, &max, &def))
    return 0;

  MidiController* mc = new MidiController(QString(plugin->PortNames[port]), ctlnum, min, max, def);

  return mc;
}
 */
#endif
