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
#include "audiodev.h"
#include "jackaudio.h"
#include "track.h"
#include "xml.h"

#include "icons.h"
#include "midi.h"
#include "midictrl.h"

#include <math.h>

// FIXME - check return values

#ifdef USE_OFFICIAL_VSTSDK
typedef VstTimeInfo VstTimeInfo_R;
#else
struct VstTimeInfo_R
{
	double samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos;
	int32_t timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags;
};
#endif

VstPlugin* VstHostUserCheck(AEffect* effect)
{
    if (effect && effect->user)
    {
        VstPlugin* plugin = (VstPlugin*)effect->user;
         if (plugin->sanityCheck == VST_SANITY_CHECK)
             return plugin;
    }
    return 0;
}

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    VstPlugin* plugin = VstHostUserCheck(effect);

    switch (opcode)
    {
    case audioMasterAutomate:
        if (plugin)
            plugin->setAutomatedParameterValue(index, opt);
        else if (effect)
            effect->setParameter(effect, index, opt);
        break;

    case audioMasterVersion:
        return kVstVersion;
        
    case audioMasterIdle:
        if (effect)
            effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0.0f);
        break;

    case audioMasterGetTime:
        static VstTimeInfo_R timeInfo;
        memset(&timeInfo, 0, sizeof(VstTimeInfo_R));
        timeInfo.sampleRate = sampleRate;

        if (audioDevice->isJackAudio())
        {
            JackAudioDevice* jackAudioDevice = (JackAudioDevice*)audioDevice;
            
            jack_position_t jack_pos;
            jack_transport_state_t jack_state = jackAudioDevice->transportQuery(&jack_pos);
            
            if (jack_state == JackTransportRolling)
                timeInfo.flags |= kVstTransportChanged|kVstTransportPlaying;
            else
                timeInfo.flags |= kVstTransportChanged;
            
            if (jack_pos.unique_1 == jack_pos.unique_2)
            {
                timeInfo.samplePos  = jack_pos.frame;
                
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
        }

        return (intptr_t)&timeInfo;

    case audioMasterTempoAt:
        // Deprecated in VST SDK 2.4
        // TODO - return BPM at a specific song pos
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

    case audioMasterWillReplaceOrAccumulate:
        // Deprecated in VST SDK 2.4
        return 1; // replace, FIXME

    case audioMasterGetVendorString:
        if (ptr)
            strcpy((char*)ptr, "OpenOctave");
        break;

    case audioMasterGetProductString:
        if (ptr)
            strcpy((char*)ptr, "OpenOctave MIDI");
        break;

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
#if !VST_FORCE_DEPRECATED
        else if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
            return -1;
#endif
        else if (strcmp((char*)ptr, "reportConnectionChanges") == 0)
            return 1;
        else if (strcmp((char*)ptr, "acceptIOChanges") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sizeWindow") == 0)
            return 1;
        else if (strcmp((char*)ptr, "offline") == 0)
            return -1;
        else if (strcmp((char*)ptr, "openFileSelector") == 0)
            return -1;
        else if (strcmp((char*)ptr, "closeFileSelector") == 0)
            return -1;
        else if (strcmp((char*)ptr, "startStopProcess") == 0)
            return 1;
        else if (strcmp((char*)ptr, "shellCategory") == 0)
            return -1;
        else if (strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime") == 0)
            return -1;
        else
            return 0;

    case audioMasterGetLanguage:
        return kVstLangEnglish;
        
    case audioMasterUpdateDisplay:
        if (plugin)
        {
            // Update current program name
            plugin->updateCurrentProgram();
        }
        return 1;
    }
    return 0;
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

    sanityCheck = 0;
}

VstPlugin::~VstPlugin()
{
	if(debugMsg)
		qDebug("VstPlugin::~VstPlugin");
    sanityCheck = 0;
    aboutToRemove();

    if (effect)
    {
        if (ui.widget)
        {
            effect->dispatcher(effect, effEditClose, 0, 0, 0, 0.0f);
            delete ui.widget;
        }
        
        ui.widget = 0;
        ui.width  = 0;
        ui.height = 0;

        if (m_activeBefore)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, 0, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0.0f);
        }

        effect->dispatcher(effect, effClose, 0, 0, 0, 0.0f);

        effect = 0;
    }

    // delete synth audio ports
    if (m_hints & PLUGIN_IS_SYNTH)
    {
        if (m_ainsCount > 0 &&  audioDevice)
        {
            for (uint32_t i=0; i < m_ainsCount; i++)
			{
				if (m_portsIn[i])
                	audioDevice->unregisterPort(m_portsIn[i]);
			}
        }
        
        if (m_aoutsCount > 0 &&  audioDevice)
        {
            for (uint32_t i=0; i < m_aoutsCount; i++)
			{
				if (m_portsOut[i])
                	audioDevice->unregisterPort(m_portsOut[i]);
			}
        }
    }
}

void VstPlugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    AEffect* effect = (AEffect*)nativeHandle;
    plugi->m_hints = 0;
    plugi->m_audioInputCount  = effect->numInputs;
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
                sanityCheck = VST_SANITY_CHECK;
                effect->dispatcher(effect, effOpen, 0, 0, 0, 0.0f);
                effect->dispatcher(effect, effSetSampleRate, 0, 0, 0, sampleRate);
                effect->dispatcher(effect, effSetBlockSize, 0, segmentSize, 0, 0.0f);
                effect->dispatcher(effect, effSetProcessPrecision, 0, 0 /* float 32bit */, 0, 0.0f);
                effect->user = this;

                if (effect->dispatcher(effect, effGetVstVersion, 0, 0, 0, 0.0f) <= 2)
                    isOldSdk = true;

                // store information
                char buf_str[255] = { 0 };
                
                if (m_name.isEmpty())
                {
                    effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
                    if (buf_str[0] != 0)
                        m_name = QString(buf_str);
                    else
                        m_name = label;
                }

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

    // delete old data
    if (m_paramCount > 0)
        delete[] m_params;

    if (m_ainsCount > 0)
    {
        for (uint32_t i=0; i < m_ainsCount; i++)
            audioDevice->unregisterPort(m_portsIn[i]);

        delete[] m_portsIn;
    }

    if (m_aoutsCount > 0)
    {
        for (uint32_t i=0; i < m_aoutsCount; i++)
            audioDevice->unregisterPort(m_portsOut[i]);

        delete[] m_portsOut;
    }

    // reset
    m_hints  = 0;
    m_params = 0;

    // query new data
    m_ainsCount  = effect->numInputs;
    m_aoutsCount = effect->numOutputs;
    m_paramCount = effect->numParams;

    if (effect->flags & effFlagsIsSynth && effect->numOutputs >= 1)
        m_hints |= PLUGIN_IS_SYNTH;
    else if (effect->numInputs > 1 && effect->numOutputs >= 1)
        m_hints |= PLUGIN_IS_FX;

    // allocate data
    if (m_paramCount > 0)
        m_params = new ParameterPort[m_paramCount];

    if (m_hints & PLUGIN_IS_SYNTH)
    {
        // synths output directly to jack
        if (m_ainsCount > 0)
        {
            m_portsIn = new void* [m_ainsCount];

            for (uint32_t j=0; j<m_ainsCount; j++)
            {
                QString port_name = m_name + ":input_" + QString::number(j+1);
                m_portsIn[j] = audioDevice->registerInPort(port_name.toUtf8().constData(), false);
            }
        }

        if (m_aoutsCount > 0)
        {
            m_portsOut = new void* [m_aoutsCount];

            for (uint32_t j=0; j<m_aoutsCount; j++)
            {
                QString port_name = m_name + ":output_" + QString::number(j+1);
                m_portsOut[j] = audioDevice->registerOutPort(port_name.toUtf8().constData(), false);
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

    // enable it again (only if jack is active, otherwise non-needed)
    if (audioDevice && audioDevice->isJackAudio())
    {
        m_proc_lock.lock();
        m_enabled = true;
        m_proc_lock.unlock();
    }
}

void VstPlugin::reloadPrograms(bool)
{
    int32_t old_count = m_programNames.count();
    m_programNames.clear();

    // Update names
    for (int32_t i=0; i < effect->numPrograms; i++)
    {
        char buf_str[255] = { 0 };
        if (effect->dispatcher(effect, effGetProgramNameIndexed, i, 0, buf_str, 0.0f) != 1)
        {
            // program will be [re-]changed later
            effect->dispatcher(effect, effSetProgram, 0, i, 0, 0.0f);
            effect->dispatcher(effect, effGetProgramName, 0, 0, buf_str, 0.0f);
        }
        m_programNames.append(QString(buf_str));
    }

    // Check if current program is invalid
    bool program_changed = false;
    int32_t new_count = effect->numPrograms;

    if (new_count == old_count+1)
    {
        // one program added, probably created by user
        m_currentProgram = old_count;
        program_changed = true;
    }
    else if (m_currentProgram >= (int32_t)new_count)
    {
        // current program > count
        m_currentProgram = 0;
        program_changed = true;
    }
    else if (m_currentProgram < 0 && new_count > 0)
    {
        // programs exist now, but not before
        m_currentProgram = 0;
        program_changed = true;
    }
    else if (m_currentProgram >= 0 && new_count == 0)
    {
        // programs existed before, but not anymore
        m_currentProgram = -1;
        program_changed = true;
    }

    if (program_changed)
    {
        setProgram(m_currentProgram);
    }
    else
    {
        // Program was changed during update, re-set it
        if (m_currentProgram >= 0)
            effect->dispatcher(effect, effSetProgram, 0, m_currentProgram, 0, 0.0f);
    }
}

QString VstPlugin::getParameterName(uint32_t index)
{
    char buf_str[255] = { 0 };
    effect->dispatcher(effect, effGetParamName, index, 0, buf_str, 0.0f);
    if (buf_str[0] != 0)
        return QString(buf_str);
    return QString("");
}

QString VstPlugin::getParameterUnit(uint32_t index)
{
    char buf_str[255] = { 0 };
    effect->dispatcher(effect, effGetParamLabel, index, 0, buf_str, 0.0f);
    if (buf_str[0] != 0)
        return QString(buf_str);
    return QString("");
}

void VstPlugin::setNativeParameterValue(uint32_t index, double value)
{
    effect->setParameter(effect, index, value);
}

void VstPlugin::setAutomatedParameterValue(uint32_t index, double value)
{
    if (index < m_paramCount)
    {
        if (m_params[index].hints & PARAMETER_IS_INTEGER)
            value = rint(value);

        m_params[index].value = m_params[index].tmpValue = value;
        m_params[index].update = true;

        effect->setParameter(effect, index, value);        

        // Record automation from plugin's native UI
        if (m_track && m_id != -1)
        {
            AutomationType at = m_track->automationType();

            if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
                enableController(index, false);

            int id = genACnum(m_id, index);

            audio->msgSetPluginCtrlVal(m_track, id, value, false);
            m_track->recordAutomation(id, value);
        }
    }
}

uint32_t VstPlugin::getProgramCount()
{
    return m_programNames.count();
}

QString VstPlugin::getProgramName(uint32_t index)
{
    if (index < (uint32_t)m_programNames.count())
        return m_programNames.at(index);
    return QString("");
}

void VstPlugin::setProgram(uint32_t index)
{
    if (index < (uint32_t)m_programNames.count())
    {
        effect->dispatcher(effect, effSetProgram, 0, index, 0, 0.0f);

        for (uint32_t i=0; i < m_paramCount; i++)
            m_params[i].value = m_params[i].tmpValue = effect->getParameter(effect, i);

        m_currentProgram = index;
    }
}

void VstPlugin::updateCurrentProgram()
{
    if (m_currentProgram >= 0 && m_programNames.size() > 0)
    {
        char buf_str[255] = { 0 };

        if (effect->dispatcher(effect, effGetProgramNameIndexed, m_currentProgram, 0, buf_str, 0.0f) != 1)
            effect->dispatcher(effect, effGetProgramName, 0, 0, buf_str, 0.0f);

        if (buf_str[0] != 0)
        {
            m_programNames[m_currentProgram] = QString(buf_str);
            // TODO - tell oom to update preset names
        }
    }
}

bool VstPlugin::hasNativeGui()
{
    return (m_hints & PLUGIN_HAS_NATIVE_GUI);
}

void VstPlugin::showNativeGui(bool yesno)
{
    // Initialize UI if needed
    if (! ui.widget && yesno)
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

        QString title;
        title += "OOStudio: ";
        title += m_name;
        title += " (GUI)";
        if (m_track && m_track->name().isEmpty() == false)
        {
            title += " - ";
            title += m_track->name();
        }

        ui.widget->setWindowTitle(title);
        ui.widget->setWindowIcon(*oomIcon);
    }

    if (ui.widget)
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

    if (m_gui && m_gui->isVisible())
    {
        for (uint32_t i=0; i < m_paramCount; i++)
        {
            if (m_params[i].update)
            {
                m_gui->setParameterValue(i, m_params[i].value);
                m_params[i].update = false;
            }
        }
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
                    //qWarning("VST Event: 0x%02X %02i %02i", ev->type()+ev->channel(), ev->dataA(), ev->dataB());

                    switch (ev->type())
                    {
                    case ME_NOTEOFF:
                        if (ev->dataA() < 0 || ev->dataA() > 127)
                            continue;
                        break;
                    case ME_NOTEON:
                        if (ev->dataA() < 0 || ev->dataA() > 127)
                            continue;
                        break;
                    case ME_CONTROLLER:
                        if (ev->dataA() == CTRL_PROGRAM)
                        {
                            setProgram(ev->dataB());
                            continue;
                        }
                        break;
                    }

                    VstMidiEvent* midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->midiData[0] = ev->type() + ev->channel();
                    midiEvent->midiData[1] = ev->dataA();
                    midiEvent->midiData[2] = ev->dataB();

                    // Fix note-off
                    if (ev->type() == ME_NOTEON && ev->dataB() == 0)
                        midiEvent->midiData[0] = ME_NOTEOFF + ev->channel();

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
                        m_params[i].value = m_params[i].tmpValue;
                        m_params[i].update = true;
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

                    if (init(new_filename, new_label) == false)
                        // plugin failed to initialize
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
                else if (tag == "chunkFull")
                {
                    QByteArray chunk = QByteArray::fromBase64(xml.parse1().toUtf8().constData());
                    effect->dispatcher(effect, effSetChunk, 0 /* Bank */, chunk.size(), chunk.data(), 0.0f);
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
                else if (tag == "nativegui")
                {
                    bool yesno = xml.parseInt();
                    showNativeGui(yesno);
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
                	return true;
                }

            default:
                break;
        }
    }
    return true;
}

void VstPlugin::writeConfiguration(int level, Xml& xml)
{
    xml.tag(level++, "VstPlugin filename=\"%s\" label=\"%s\" uniqueId=\"%i\" channels=\"%d\"",
            Xml::xmlString(m_filename).toLatin1().constData(), Xml::xmlString(m_label).toLatin1().constData(), effect->uniqueID, m_channels);

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
        intptr_t data_size = effect->dispatcher(effect, effGetChunk, 0 /* Bank */, 0, &data, 0.0f);

        if (data && data_size >= 4)
        {
            QByteArray qchunk((const char*)data, data_size);
            const char* chunk = strdup(qchunk.toBase64().data());
            xml.strTag(level, "chunkFull", chunk);
            free((void*)chunk);
        }
    }

    xml.intTag(level, "active", m_active);

    if (guiVisible())
    {
        xml.intTag(level, "gui", 1);
        xml.geometryTag(level, "geometry", m_gui);
    }

    if (hasNativeGui() && nativeGuiVisible())
    {
        xml.intTag(level, "nativegui", 1);
    }

    xml.tag(--level, "/VstPlugin");
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
			else if(tag == "parameter")
	            return true;

        default:
            break;
        }
    }
    return true;
}

#endif // __PLUGIN_VST_H__
