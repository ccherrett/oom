//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.cpp,v 1.21.2.23 2009/12/15 22:07:12 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cmath>
#include <math.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalMapper>
#include <QSizePolicy>
#include <QScrollArea>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWhatsThis>

#include <QStandardItem>
#include <QStandardItemModel>

#include "globals.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "midictrl.h"
#include "plugin.h"
#include "xml.h"
#include "icons.h"
#include "song.h"
#include "doublelabel.h"
#include "fastlog.h"
#include "checkbox.h"

#include "audio.h"
#include "audiodev.h"
#include "track.h"
#include "al/dsp.h"

#include "lib_functions.h"

#include "config.h"
#include "plugingui.h"
#include "plugindialog.h"

// debug plugin scan
//#define PLUGIN_DEBUGIN

PluginList plugins;

/*
static const char* preset_file_pattern[] = {
      QT_TRANSLATE_NOOP("@default", "Presets (*.pre *.pre.gz *.pre.bz2)"),
      QT_TRANSLATE_NOOP("@default", "All Files (*)"),
      0
      };

static const char* preset_file_save_pattern[] = {
      QT_TRANSLATE_NOOP("@default", "Presets (*.pre)"),
      QT_TRANSLATE_NOOP("@default", "gzip compressed presets (*.pre.gz)"),
      QT_TRANSLATE_NOOP("@default", "bzip2 compressed presets (*.pre.bz2)"),
      QT_TRANSLATE_NOOP("@default", "All Files (*)"),
      0
      };
 */

//---------------------------------------------------------
//   Set error for last loaded plugin
//---------------------------------------------------------

static const char* last_error = 0;

const char* get_last_error()
{
    return last_error;
}

void set_last_error(const char* error)
{
    if (last_error)
        free((void*)last_error);

    last_error = strdup(error);
}

//---------------------------------------------------------
//   getAudioOutputPortName
//---------------------------------------------------------

QString BasePlugin::getAudioOutputPortName(uint32_t index)
{
    if (index < m_aoutsCount)
        return audioDevice->portName(m_portsOut[index]);
    return QString("");
}

//---------------------------------------------------------
//   process_synth
//---------------------------------------------------------

void BasePlugin::processSynth(MPEventList* eventList)
{
    if (m_enabled && m_aoutsCount > 0)
    {
        float* ains_buffer[m_ainsCount];
        float* aouts_buffer[m_aoutsCount];

        for (uint32_t i=0; i < m_ainsCount; i++)
            ains_buffer[i] = audioDevice->getBuffer(m_portsIn[i], segmentSize);

        for (uint32_t i=0; i < m_aoutsCount; i++)
            aouts_buffer[i] = audioDevice->getBuffer(m_portsOut[i], segmentSize);

        process(segmentSize, ains_buffer, aouts_buffer, eventList);
    }
    else
    {
        if (eventList)
        {
            //iMPEvent ev = eventList->begin();
            //eventList->erase(eventList->begin(), ev);
            eventList->clear();
        }
    }
}

//---------------------------------------------------------
//   makeGui
//---------------------------------------------------------

void BasePlugin::makeGui()
{
    m_gui = new PluginGui(this);
}

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void BasePlugin::deleteGui()
{
    if (m_gui)
    {
        delete m_gui;
        m_gui = 0;
    }
}

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void BasePlugin::showGui(bool yesno)
{
    if (yesno)
    {
        if (! m_gui)
            makeGui();

        m_gui->show();
    }
    else
    {
        if (m_gui)
            m_gui->hide();
    }
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool BasePlugin::guiVisible()
{
    return (m_gui && m_gui->isVisible());
}

//---------------------------------------------------------
//   SynthPluginTrack - dummy track used for automation
//---------------------------------------------------------

class SynthPluginTrack : public AudioTrack
{
public:
    SynthPluginTrack()
        : AudioTrack(AUDIO_SOFTSYNTH)
    {
    }

    ~SynthPluginTrack()
    {
    }

    SynthPluginTrack* clone(bool /*cloneParts*/) const
    {
        return new SynthPluginTrack(*this);
    }    

    virtual SynthPluginTrack* newTrack() const
    {
        return new SynthPluginTrack();
    }
    
    void setId(qint64 /*id*/)
    {
        // FIXME - is this needed?
        //m_id = id;
    }

    virtual void read(Xml& xml)
    {
        for (;;)
        {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token)
            {
                case Xml::Error:
                case Xml::End:
                    return;
                case Xml::TagStart:
                    if (tag == "SynthPluginTrack")
                    {
                        continue;
                    }
                    else if (tag == "LadspaPlugin" || tag == "Lv2Plugin" || tag == "VstPlugin")
                    {
                        // we already loaded this before
                        xml.parse1();
                        continue;
                    }
                    if (AudioTrack::readProperties(xml, tag))
                        xml.unknown("SynthPluginTrack");
                    break;
                case Xml::Attribut:
                    break;
                case Xml::TagEnd:
                    if (tag == "SynthPluginTrack")
                    {
                        mapRackPluginsToControllers();
                        return;
                    }
                default:
                    break;
            }
        }
    }

    virtual void write(int level, Xml& xml) const
    {
        xml.tag(level++, "SynthPluginTrack");
        AudioTrack::writeProperties(level, xml);
        xml.etag(--level, "SynthPluginTrack");
    }
};

//---------------------------------------------------------
//   SynthPluginDevice
//---------------------------------------------------------

SynthPluginDevice::SynthPluginDevice(PluginType type, QString filename, QString name, QString label, bool duplicated)
    : MidiDevice(),
      MidiInstrument()
{
    _rwFlags = 1;
    _openFlags = 1;
    _readEnable = false;
    _writeEnable = false;
    m_cachenrpn = false;

    m_type = type;
    m_filename = filename;
    m_label = label;
    m_duplicated = duplicated;
    m_plugin = 0;
    m_audioTrack = 0;

    m_name = name;

    // handle duplicate names properly
    if (m_name.contains(" [LV2]_") || m_name.contains(" [VST]_"))
    {
        m_name = m_name.split(" [LV2]_").at(0);
        m_name = m_name.split(" [VST]_").at(0);
    }

    if (m_name.endsWith(" [LV2]") == false && m_name.endsWith(" [VST]") == false)
    {
        switch (type)
        {
        case PLUGIN_LV2:
            m_name += " [LV2]";
            break;
        case PLUGIN_VST:
            m_name += " [VST]";
            break;
        default:
            break;
        }
    }

    MidiDevice::setName(m_name);
    MidiInstrument::setIName(m_name);
}

SynthPluginDevice::~SynthPluginDevice()
{
    close();
}

//---------------------------------------------------------
//   setTrackId
//---------------------------------------------------------

void SynthPluginDevice::setTrackId(qint64 /*id*/)
{
    //if (m_audioTrack)
    //    ((SynthPluginTrack*)m_audioTrack)->setId(id);
}

//---------------------------------------------------------
//   open, init plugin
//---------------------------------------------------------

QString SynthPluginDevice::open()
{
    if (m_plugin)
        return QString("OK");

    // Make it behave like a regular midi device.
    _readEnable = false;
    _writeEnable = (_openFlags & 0x01);

    if (m_type == PLUGIN_LV2)
        m_plugin = new Lv2Plugin();
    else if (m_type == PLUGIN_VST)
        m_plugin = new VstPlugin();
    
    if (m_plugin)
    {
        m_plugin->setName(m_name);
        if (m_plugin->init(m_filename, m_label))
        {
            // disable plugin if jack is not running
            if (!audioDevice || audioDevice->isJackAudio() == false)
                m_plugin->aboutToRemove();
            
            m_audioTrack = new SynthPluginTrack();
            m_plugin->setTrack(m_audioTrack);
            audio->msgAddPlugin(m_audioTrack, 0, m_plugin);

            m_audioTrack->mapRackPluginsToControllers();
            m_audioTrack->setName(m_name);
            m_audioTrack->setWantsAutomation(true);

            m_plugin->setActive(true);
            return QString("OK");
        }
        else
        {
            m_audioTrack = 0;
            m_plugin->setTrack(0);
        }
    }

    return QString("Fail");
}

//---------------------------------------------------------
//   close, delete plugin
//---------------------------------------------------------

void SynthPluginDevice::close()
{
	if(debugMsg)
		qDebug("SynthPluginDevice::close");
    _readEnable = false;
    _writeEnable = false;

    if (m_audioTrack)
    {
		if(debugMsg)
			qDebug("SynthPluginDevice::close Deleting internal SynthAudioTrack");
        audio->msgAddPlugin(m_audioTrack, 0, 0);
        delete (SynthPluginTrack*)m_audioTrack;
        m_audioTrack = 0;
		if(debugMsg)
			qDebug("SynthPluginDevice::close Done");
    }

    if (m_plugin)
    {
		if(debugMsg)
			qDebug("SynthPluginDevice::close calling delete on plugin");
        m_plugin->setTrack(0);
        m_plugin->aboutToRemove();

        // Delete the appropriate class
        switch (m_plugin->type())
        {
        case PLUGIN_LADSPA:
            delete (LadspaPlugin*)m_plugin;
            break;
        case PLUGIN_LV2:
            delete (Lv2Plugin*)m_plugin;
            break;
        case PLUGIN_VST:
            delete (VstPlugin*)m_plugin;
            break;
        default:
            break;
        }
        m_plugin = 0;
    }
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void SynthPluginDevice::setName(const QString& s)
{
    m_name = s;
    MidiDevice::setName(s);
}

void SynthPluginDevice::setPluginName(const QString& s)
{
    m_name = s;
}

//---------------------------------------------------------
//   getAudioOutputPortName
//---------------------------------------------------------

QString SynthPluginDevice::getAudioOutputPortName(uint32_t index)
{
    if (m_plugin)
        return m_plugin->getAudioOutputPortName(index);
    return QString("");
}

//---------------------------------------------------------
//   getCurrentProgram
//---------------------------------------------------------

int32_t SynthPluginDevice::getCurrentProgram()
{
    if (m_plugin)
        return m_plugin->getCurrentProgram();
    return -1;
}

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void SynthPluginDevice::writeRouting(int, Xml&) const
{
    if (m_plugin)
    {
        //qWarning("SynthPluginDevice::writeRouting()");
    }
}

//---------------------------------------------------------
//   Midi processing (inside jack process callback)
//---------------------------------------------------------

void SynthPluginDevice::collectMidiEvents()
{
    if (m_plugin)
    {
        //qWarning("SynthPluginDevice::collectMidiEvents()");
    }
}

void SynthPluginDevice::processMidi()
{
    if (_writeEnable && m_plugin)
    {
        MPEventList* eventList = playEvents();
        if (m_plugin)
            m_plugin->processSynth(eventList);
    }
}

//---------------------------------------------------------
//   GUI Stuff
//---------------------------------------------------------

bool SynthPluginDevice::guiVisible() const
{
    if (m_plugin)
        return m_plugin->guiVisible();
    return false;
}

void SynthPluginDevice::showGui(bool yesno)
{
    if (m_plugin)
        m_plugin->showGui(yesno);
}

bool SynthPluginDevice::hasNativeGui() const
{
    if (m_plugin)
        return m_plugin->hasNativeGui();
    return false;
}

bool SynthPluginDevice::nativeGuiVisible()
{
    if (m_plugin)
        return m_plugin->nativeGuiVisible();
    return false;
}

void SynthPluginDevice::showNativeGui(bool yesno)
{
    if (m_plugin && m_plugin->hasNativeGui())
        m_plugin->showNativeGui(yesno);
}

void SynthPluginDevice::updateNativeGui()
{
    if (m_plugin && m_plugin->active() && m_plugin->hasNativeGui())
    //if (m_plugin && m_plugin->hasNativeGui())
        return m_plugin->updateNativeGui();
}
 
//---------------------------------------------------------
//   Patch/Programs Stuff
//---------------------------------------------------------

void SynthPluginDevice::reset(int, MType)
{
	if(debugMsg)
    	qWarning("SynthPluginDevice::reset");
}

QString SynthPluginDevice::getPatchName(int, int prog, MType, bool)
{
    if (m_plugin)
    {
        if (prog < (int32_t)m_plugin->getProgramCount())
            return m_plugin->getProgramName(prog);
    }
    return QString("");
}

Patch* SynthPluginDevice::getPatch(int, int prog, MType, bool)
{
    //qWarning("SynthPluginDevice::getPatch(%i)", prog);
#if 1
    if (m_plugin && prog < (int32_t)m_plugin->getProgramCount())
    {
        static Patch patch;
        patch.typ   = 1; // 1 - GM  2 - GS  4 - XG
        patch.hbank = 0;
        patch.lbank = 0;
        patch.prog  = prog;
        patch.name = m_plugin->getProgramName(prog);
        patch.loadmode = 0; // ?
        //patch.engine;
        //patch.filename;
        patch.index = prog;
        patch.volume = 1.0f; // ?
        patch.drum = false;
        
        if (prog != m_plugin->getCurrentProgram())
            m_plugin->setProgram(prog);

        return &patch;
    }
#endif
    return 0;
}

void SynthPluginDevice::populatePatchPopup(QMenu*, int, MType, bool)
{
	if(debugMsg)
    	qWarning("SynthPluginDevice::populatePatchPopup();");
}

void SynthPluginDevice::populatePatchModel(QStandardItemModel* model, int, MType, bool)
{
	if(debugMsg)
    	qWarning("SynthPluginDevice::populatePatchModel();");
    model->clear();

    if (m_plugin)
    {
        QStandardItem* root = model->invisibleRootItem();

        for (uint32_t i=0; i < m_plugin->getProgramCount(); i++)
        {
            QList<QStandardItem*> row;
            QString strId = QString::number(i);
            QStandardItem* idItem = new QStandardItem(strId);
            QStandardItem* nItem = new QStandardItem(m_plugin->getProgramName(i));
            nItem->setToolTip(m_plugin->getProgramName(i));
            row.append(nItem);
            row.append(idItem);
            root->appendRow(row);
            //QAction *act = menu->addAction(QString(mp->name));
            //act->setData(id);
            //mp = _mess->getPatchInfo(ch, mp);
        }
    }
}

//---------------------------------------------------------
//   XML Stuff
//---------------------------------------------------------

void SynthPluginDevice::read(Xml& xml)
{
    if (m_plugin)
    {
        m_plugin->readConfiguration(xml, false);
        if (m_audioTrack)
            ((SynthPluginTrack*)m_audioTrack)->read(xml);
    }
}

void SynthPluginDevice::write(int level, Xml& xml)
{
    if (m_plugin)
    {
        m_plugin->writeConfiguration(level, xml);
        if (m_audioTrack)
            m_audioTrack->write(level, xml);
    }
}


//---------------------------------------------------------
//   receiveEvent
//---------------------------------------------------------

MidiPlayEvent SynthPluginDevice::receiveEvent()
{
	if(debugMsg)
    	qWarning("SynthPluginDevice::receiveEvent()");
    return MidiPlayEvent() ;//_sif->receiveEvent();
}

//---------------------------------------------------------
//   eventsPending
//---------------------------------------------------------

int SynthPluginDevice::eventsPending() const
{
    //qWarning("SynthPluginDevice::eventsPending()");
    return 0; //_sif->eventsPending();
}

//---------------------------------------------------------
//   putMidiEvent
//---------------------------------------------------------

void SynthPluginDevice::segmentSizeChanged(unsigned newSegmentSize)
{
    if (m_plugin && m_plugin->active())
    {
        m_plugin->bufferSizeChanged(newSegmentSize);
    }
}

//---------------------------------------------------------
//   putMidiEvent
//---------------------------------------------------------

bool SynthPluginDevice::putMidiEvent(const MidiPlayEvent& ev)
{
    if (_writeEnable)
    {
        MPEventList* pe = playEvents();
        pe->add(ev);
    }
    return false;
}

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
: std::vector<BasePlugin*>()
{
    for (int i = 0; i < MAX_CHANNELS; ++i)
        posix_memalign((void**) (buffer + i), 16, sizeof (float) * segmentSize);
}

//---------------------------------------------------------
//   ~Pipeline
//---------------------------------------------------------

Pipeline::~Pipeline()
{
    removeAll();
    for (int i = 0; i < MAX_CHANNELS; ++i)
        ::free(buffer[i]);
}

//---------------------------------------------------------
//   insert
//    give ownership of object plugin to Pipeline
//---------------------------------------------------------

int Pipeline::addPlugin(BasePlugin* plugin, int index)
{
	int s = size();
	if(index >= s || index == -1)
	{
		push_back(plugin);
		return s;
	}
	else
	{
    	//remove(index);
    	//(*this)[index] = plugin;
    	insert(begin()+index, plugin);
		return index;
	}
}

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Pipeline::remove(int index)
{
	int s = size();
	if(index >= s)
		return;
	if(debugMsg)
		qDebug(" Pipeline::remove(%d)", index);
    BasePlugin* plugin = (*this)[index];

    if (plugin && !(plugin->hints() & PLUGIN_IS_SYNTH))
    {//Synth type plugins are deleted elsewhere in SynthPluginDevice::close(), DO NOT delete here
        plugin->aboutToRemove();

        // Delete the appropriate class
        switch(plugin->type())
        {
        case PLUGIN_LADSPA:
            delete (LadspaPlugin*)plugin;
            break;
        case PLUGIN_LV2:
            delete (Lv2Plugin*)plugin;
            break;
        case PLUGIN_VST:
            delete (VstPlugin*)plugin;
            break;
        default:
            break;
        }
    }

	erase(begin()+index);
    //(*this)[index] = 0;
}

//---------------------------------------------------------
//   removeAll
//---------------------------------------------------------

void Pipeline::removeAll()
{
	int s = size();
    for (int i = 0; i < s; ++i)
        remove(i);
}

//---------------------------------------------------------
//   isActive
//---------------------------------------------------------

bool Pipeline::isActive(int idx) const
{
	int s = size();
	if(idx >= s)
		return false;

    BasePlugin* p = (*this)[idx];
    if (p)
        return p->active();
    return false;
}

//---------------------------------------------------------
//   setActive
//---------------------------------------------------------

void Pipeline::setActive(int idx, bool flag)
{
	int s = size();
	if(idx >= s)
		return;
    BasePlugin* p = (*this)[idx];
    if (p)
    {
        p->setActive(flag);

        if (p->gui())
            p->gui()->setActive(flag);
    }
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void Pipeline::setChannels(int n)
{
	int s = size();
    for (int i = 0; i < s; ++i)
        if ((*this)[i])
            (*this)[i]->setChannels(n);
}

//---------------------------------------------------------
//   label
//---------------------------------------------------------

QString Pipeline::label(int idx) const
{
	int s = size();
	if(idx >= s)
    	return QString("");
		
    BasePlugin* p = (*this)[idx];
    if (p)
        return p->label();
    return QString("");
}

//---------------------------------------------------------
//   name
//---------------------------------------------------------

QString Pipeline::name(int idx) const
{
	int s = (*this).size();
	if(idx >= s)
		return QString("empty");
    BasePlugin* p = (*this)[idx];
    if (p)
        return p->name();
    return QString("empty");
}

//---------------------------------------------------------
//   empty
//---------------------------------------------------------

bool Pipeline::empty(int idx) const
{
	int s = size();
	return idx >= s;
    //BasePlugin* p = (*this)[idx];
    //return p == 0;
}

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void Pipeline::move(int idx, bool up)
{
    BasePlugin* p1 = (*this)[idx];
    if (up)
    {
        (*this)[idx] = (*this)[idx - 1];

        if ((*this)[idx])
            (*this)[idx]->setId(idx);

        (*this)[idx - 1] = p1;

        if (p1)
        {
            p1->setId(idx - 1);
            if (p1->track())
                audio->msgSwapControllerIDX(p1->track(), idx, idx - 1);
        }
    }
    else
    {
        (*this)[idx] = (*this)[idx + 1];

        if ((*this)[idx])
            (*this)[idx]->setId(idx);

        (*this)[idx + 1] = p1;

        if (p1)
        {
            p1->setId(idx + 1);
            if (p1->track())
                audio->msgSwapControllerIDX(p1->track(), idx, idx + 1);
        }
    }
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void Pipeline::apply(int ports, uint32_t nframes, float** buffer1)
{
    //fprintf(stderr, "Pipeline::apply data: nframes:%ld %e %e %e %e\n", nframes, buffer1[0][0], buffer1[0][1], buffer1[0][2], buffer1[0][3]);

    bool swap = false;

    for (iPluginI ip = begin(); ip != end(); ++ip)
    {
        BasePlugin* p = *ip;
        if (p && p->enabled())
        {
            p->setChannels(ports);

            if (p->hints() & PLUGIN_HAS_IN_PLACE_BROKEN)
            {
                if (swap)
                    p->process(nframes, buffer, buffer1, 0);
                else
                    p->process(nframes, buffer1, buffer, 0);
                swap = !swap;
            }
            else
            {
                if (swap)
                    p->process(nframes, buffer, buffer, 0);
                else
                    p->process(nframes, buffer1, buffer1, 0);
            }
        }
    }

    if (swap)
    {
        for (int i = 0; i < ports; ++i)
            AL::dsp->cpy(buffer1[i], buffer[i], nframes);
    }

    // p3.3.41
    //fprintf(stderr, "Pipeline::apply after data: nframes:%ld %e %e %e %e\n", nframes, buffer1[0][0], buffer1[0][1], buffer1[0][2], buffer1[0][3]);
}

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void Pipeline::showGui(int idx, bool flag)
{
	int s = size();
	if(idx >= s)
		return;

    BasePlugin* p = (*this)[idx];

    if (p)
        p->showGui(flag);
}

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void Pipeline::deleteGui(int idx)
{
	int s = size();
    if (idx >= s)
        return;

    BasePlugin* p = (*this)[idx];

    if (p)
        p->deleteGui();
}

//---------------------------------------------------------
//   deleteAllGuis
//---------------------------------------------------------

void Pipeline::deleteAllGuis()
{
	int s = size();
    for (int i = 0; i < s; i++)
        deleteGui(i);
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool Pipeline::guiVisible(int idx)
{
	int s = size();
	if(idx >= s)
		return false;
    BasePlugin* p = (*this)[idx];

    if (p)
        return p->guiVisible();
    else
        return false;
}

//---------------------------------------------------------
//   hasNativeGui
//---------------------------------------------------------

bool Pipeline::hasNativeGui(int idx)
{
	int s = size();
	if(idx >= s)
		return false;
    BasePlugin* p = (*this)[idx];
    if (p)
        return p->hasNativeGui();
    return false;
}

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void Pipeline::showNativeGui(int idx, bool flag)
{
	int s = size();
	if(idx >= s)
		return;
    BasePlugin* p = (*this)[idx];
    if(p)
        p->showNativeGui(flag);
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool Pipeline::nativeGuiVisible(int idx)
{
	int s = size();
	if(idx >= s)
		return false;
    BasePlugin* p = (*this)[idx];
    if (p)
        return p->nativeGuiVisible();
    return false;
}

void Pipeline::updateGuis()
{
    for (iPluginI i = begin(); i != end(); ++i)
    {
        BasePlugin* p = (BasePlugin*)*i;
        if(p)
        {
            p->updateNativeGui();
            //if (p->gui())
            //    p->gui()->updateValues();
        }
    }
}

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi, const PluginType t)
{
    if (debugMsg)
        qWarning("looking up %s", fi->filePath().toAscii().constData());

    void* handle = lib_open(fi->filePath().toAscii().constData());
	if (handle == 0)
	{
		fprintf(stderr, "dlopen(%s) failed: %s\n",
				fi->filePath().toAscii().constData(), dlerror());
		return;
	}

    if (t == PLUGIN_LADSPA)
	{
        LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function) lib_symbol(handle, "ladspa_descriptor");
		if (!ladspa)
		{
			const char *txt = dlerror();
			if (txt)
			{
				fprintf(stderr,
						"Unable to find ladspa_descriptor() function in plugin "
						"library file \"%s\": %s.\n"
						"Are you sure this is a LADSPA plugin file?\n",
						fi->filePath().toAscii().constData(),
						txt);
			}
            lib_close(handle);
			return;
		}

		const LADSPA_Descriptor* descr;
		for (int i = 0;; ++i)
		{
			descr = ladspa(i);
			if (descr == NULL)
				break;

			// Make sure it doesn't already exist.
			if (plugins.find(fi->completeBaseName(), QString(descr->Label)) != 0)
				continue;

#ifdef PLUGIN_DEBUGIN
			fprintf(stderr, "loadPluginLib: ladspa effect name:%s inPlaceBroken:%d\n", descr->Name, LADSPA_IS_INPLACE_BROKEN(descr->Properties));
#endif
            plugins.add(PLUGIN_LADSPA, fi->absoluteFilePath(), QString(descr->Label), descr);
		}
	}
    else if (t == PLUGIN_VST)
    {
        VST_Function vstfn = (VST_Function) lib_symbol(handle, "VSTPluginMain");

        if (! vstfn)
        {
            vstfn = (VST_Function) lib_symbol(handle, "main");

    #ifdef TARGET_API_MAC_CARBON
            if (! vstfn)
                vstfn = (VST_Function)lib_symbol(lib_handle, "main_macho");
    #endif
        }

        if (! vstfn)
        {
            const char *txt = dlerror();
            if (txt)
            {
                fprintf(stderr,
                        "Unable to find vst entry function in plugin "
                        "library file \"%s\": %s.\n"
                        "Are you sure this is a VST plugin file?\n",
                        fi->filePath().toAscii().constData(),
                        txt);
            }
            lib_close(handle);
            return;
        }

        AEffect* effect = vstfn(VstHostCallback);

        if (effect && (effect->flags & effFlagsCanReplacing) > 0)
        {
            QString PluginLabel = fi->baseName();

            char buf_str[255] = { 0 };
            effect->dispatcher(effect, effOpen, 0, 0, 0, 0.0f);
            effect->dispatcher(effect, effGetProductString, 0, 0, buf_str, 0.0f);

            if (buf_str[0] != 0)
                PluginLabel = QString(buf_str);

            // Make sure it doesn't already exist.
            if (plugins.find(fi->completeBaseName(), QString(PluginLabel)) == 0)
            {
                plugins.add(PLUGIN_VST, fi->absoluteFilePath(), PluginLabel, effect);
            }

            effect->dispatcher(effect, effClose, 0, 0, 0, 0.0f);
        }
    }

    lib_close(handle);
}

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s, const PluginType t)
{
    if (debugMsg)
        qWarning("scan plugin dir <%s>\n", s.toLatin1().constData());

#ifdef __WINDOWS__
    QDir pluginDir(s, QString("*.dll"));
#else
    QDir pluginDir(s, QString("*.so"));
#endif

    if (pluginDir.exists())
    {
        QFileInfoList list = pluginDir.entryInfoList();
        QFileInfoList::iterator it = list.begin();
        while (it != list.end())
        {
            // Disable known broken plugins that may crash oom on startup
            QStringList blacklist;
            blacklist.append("dssi-vst.so");
            blacklist.append("liteon_biquad-vst.so");
            blacklist.append("liteon_biquad-vst_64bit.so");
            blacklist.append("fx_blur-vst.so");
            blacklist.append("fx_blur-vst_64bit.so");
            blacklist.append("Scrubby_64bit.so");
            blacklist.append("Skidder_64bit.so");
            blacklist.append("libwormhole2_64bit.so");
            blacklist.append("vexvst.so");

            if (blacklist.contains( ((QFileInfo*)&*it)->fileName()) )
            {
                ++it;
                continue;
            }

            loadPluginLib(&*it, t);
            ++it;
        }
    }
}

//---------------------------------------------------------
//   initPlugins
//---------------------------------------------------------

void initLV2();

void initPlugins(bool ladspa, bool lv2, bool vst)
{
    //ladspa = vst = false;

    // LADSPA
    if (ladspa)
    {
        fprintf(stderr, "Looking up LADSPA plugins...\n");

		QStringList ladspaPathList;
#ifdef __WINDOWS__
        // TODO - look for ladspa in known locations
#else
        const char* ladspaPath = getenv("LADSPA_PATH");
        if (ladspaPath == 0)
		{
			ladspaPathList = config.ladspaPaths.split(":");//"/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";
        }
		else
			ladspaPathList = QString(ladspaPath).split(":");
#endif

        foreach(QString s, ladspaPathList)
            loadPluginDir(s, PLUGIN_LADSPA);
    }

    if (lv2)
    {
        fprintf(stderr, "Looking up LV2 plugins...\n");
        initLV2();
    }

    if (vst)
    {
        // TODO
        fprintf(stderr, "Looking up VST plugins...\n");

        QStringList vstPathList;
#ifdef __WINDOWS__
        // TODO - look for vst in known locations
#else
        const char* vstPath = getenv("VST_PATH");
        if (vstPath == 0)
		{
			if(debugMsg)
				qDebug("from config VST_PATH: %s", config.vstPaths.toUtf8().constData());
			vstPathList = config.vstPaths.split(":");
		}
		else
		{
			 vstPathList = QString(vstPath).split(":");
			if(debugMsg)
				qDebug("from env VST_PATH: %s", vstPath);
		}
			//"/usr/local/lib64/vst:/usr/lib64/vst:/usr/local/lib/vst:/usr/lib/vst";
#endif

        foreach (QString s, vstPathList)
            loadPluginDir(s, PLUGIN_VST);
    }

    // Add the synth plugins to the midi-device list
    for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (i->hints() & PLUGIN_IS_SYNTH)
        {
            SynthPluginDevice* dev = new SynthPluginDevice(i->type(), i->filename(true), i->name(), i->label());
            midiDevices.add(dev);
        }
    }
}
