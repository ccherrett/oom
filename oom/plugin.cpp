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
#include "al/dsp.h"

#include "lib_functions.h"

#include "config.h"
#include "plugingui.h"
#include "plugindialog.h"

// debug plugin scan
#define PLUGIN_DEBUGIN

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
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
: std::vector<BasePlugin*>()
{
    // Added by Tim. p3.3.15
    for (int i = 0; i < MAX_CHANNELS; ++i)
        posix_memalign((void**) (buffer + i), 16, sizeof (float) * segmentSize);

    for (int i = 0; i < PipelineDepth; ++i)
        push_back(0);
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

void Pipeline::insert(BasePlugin* plugin, int index)
{
    remove(index);
    (*this)[index] = plugin;
}

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Pipeline::remove(int index)
{
    BasePlugin* plugin = (*this)[index];

    if (plugin)
    {
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

    (*this)[index] = 0;
}

//---------------------------------------------------------
//   removeAll
//---------------------------------------------------------

void Pipeline::removeAll()
{
    for (int i = 0; i < PipelineDepth; ++i)
        remove(i);
}

//---------------------------------------------------------
//   isActive
//---------------------------------------------------------

bool Pipeline::isActive(int idx) const
{
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
    for (int i = 0; i < PipelineDepth; ++i)
        if ((*this)[i])
            (*this)[i]->setChannels(n);
}

//---------------------------------------------------------
//   label
//---------------------------------------------------------

QString Pipeline::label(int idx) const
{
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
    BasePlugin* p = (*this)[idx];
    return p == 0;
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
        if (p)
        {
            if (p->type() == PLUGIN_LADSPA)
            {
                LadspaPlugin* ladp = (LadspaPlugin*)p;

                if (ladp->hints() & PLUGIN_HAS_IN_PLACE_BROKEN)
                {
                    if (swap)
                        ladp->connect(ports, buffer, buffer1);
                    else
                        ladp->connect(ports, buffer1, buffer);
                    swap = !swap;
                }
                else
                {
                    if (swap)
                        ladp->connect(ports, buffer, buffer);
                    else
                        ladp->connect(ports, buffer1, buffer1);
                }
                ladp->process(nframes);
            }
            else if (p->type() == PLUGIN_LV2)
            {
                Lv2Plugin* lv2p = (Lv2Plugin*)p;

                if (lv2p->hints() & PLUGIN_HAS_IN_PLACE_BROKEN)
                {
                    if (swap)
                        lv2p->connect(ports, buffer, buffer1);
                    else
                        lv2p->connect(ports, buffer1, buffer);
                    swap = !swap;
                }
                else
                {
                    if (swap)
                        lv2p->connect(ports, buffer, buffer);
                    else
                        lv2p->connect(ports, buffer1, buffer1);
                }
                lv2p->process(nframes);
            }
            else if (p->type() == PLUGIN_VST)
            {
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
    BasePlugin* p = (*this)[idx];

    if (p)
        p->showGui(flag);
}

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void Pipeline::deleteGui(int idx)
{
    if (idx >= PipelineDepth)
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
    for (int i = 0; i < PipelineDepth; i++)
        deleteGui(i);
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool Pipeline::guiVisible(int idx)
{
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
    BasePlugin* p = (*this)[idx];
    if(p)
        p->showNativeGui(flag);
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool Pipeline::nativeGuiVisible(int idx)
{
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

#if 0
//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

Plugin::Plugin(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi)
{
	initLadspa(f, d, isDssi);
}

Plugin::Plugin(PluginType t, const char*)
{
	m_type = t;
	switch(m_type)
	{
        case PLUGIN_LV2:
			plugin = NULL;
			ladspa = NULL;
			_handle = 0;
			_references = 0;
			_instNo = 0;
		break;
		default:
			printf("Invalid PluginType specified\n");
			return;
	}
}

void Plugin::initLadspa(QFileInfo* f, const LADSPA_Descriptor* d, bool isDssi)/*{{{*/
{
    m_type = PLUGIN_LADSPA;

	fi = *f;
	plugin = NULL;
	ladspa = NULL;
	_handle = 0;
	_references = 0;
	_instNo = 0;
    m_label = QString(d->Label);
    m_name = QString(d->Name);
    m_uniqueId = d->UniqueID;
    m_maker = QString(d->Maker);
    m_copyright = QString(d->Copyright);

	_portCount = d->PortCount;
	//_portDescriptors = 0;
	//if(_portCount)
	//  _portDescriptors = new LADSPA_PortDescriptor[_portCount];


	_inports = 0;
	_outports = 0;
	_controlInPorts = 0;
	_controlOutPorts = 0;
	for (unsigned long k = 0; k < _portCount; ++k)
	{
		LADSPA_PortDescriptor pd = d->PortDescriptors[k];
		//_portDescriptors[k] = pd;
		if (pd & LADSPA_PORT_AUDIO)
		{
			if (pd & LADSPA_PORT_INPUT)
				++_inports;
			else
				if (pd & LADSPA_PORT_OUTPUT)
				++_outports;
		}
		else
			if (pd & LADSPA_PORT_CONTROL)
		{
			if (pd & LADSPA_PORT_INPUT)
				++_controlInPorts;
			else
				if (pd & LADSPA_PORT_OUTPUT)
				++_controlOutPorts;
		}
	}

    //_inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(d->Properties);

	// By T356. Blacklist vst plugins in-place configurable for now. At one point they
	//   were working with in-place here, but not now, and RJ also reported they weren't working.
	// Fixes problem with vst plugins not working or feeding back loudly.
	// I can only think of two things that made them stop working:
	// 1): I switched back from Jack-2 to Jack-1
	// 2): I changed winecfg audio to use Jack instead of ALSA.
	// Will test later...
	// Possibly the first one because under Mandriva2007.1 (Jack-1), no matter how hard I tried,
	//  the same problem existed. It may have been when using Jack-2 with Mandriva2009 that they worked.
	// Apparently the plugins are lying about their in-place capability.
	// Quote:
	/* Property LADSPA_PROPERTY_INPLACE_BROKEN indicates that the plugin
	  may cease to work correctly if the host elects to use the same data
	  location for both input and output (see connect_port()). This
	  should be avoided as enabling this flag makes it impossible for
	  hosts to use the plugin to process audio `in-place.' */
	// Examination of all my ladspa and vst synths and effects plugins showed only one -
	//  EnsembleLite (EnsLite VST) has the flag set, but it is a vst synth and is not involved here!
	// Yet many (all?) ladspa vst effect plugins exhibit this problem.
	// Changed by Tim. p3.3.14
    //if ((_inports != _outports) || (fi.completeBaseName() == QString("dssi-vst") && !config.vstInPlace))
    //	_inPlaceCapable = false;
}/*}}}*/

//---------------------------------------------------------
//   incReferences
//   called but currently the results are not unused anywhere
//---------------------------------------------------------

int Plugin::incReferences(int val)/*{{{*/
{
#ifdef PLUGIN_DEBUGIN 
	fprintf(stderr, "Plugin::incReferences _references:%d val:%d\n", _references, val);
#endif

	int newref = _references + val;

	if (newref == 0)
	{
		_references = 0;
		if (_handle)
		{
#ifdef PLUGIN_DEBUGIN 
			fprintf(stderr, "Plugin::incReferences no more instances, closing library\n");
#endif

			dlclose(_handle);
		}

		_handle = 0;
		ladspa = NULL;
		plugin = NULL;
		rpIdx.clear();

#ifdef DSSI_SUPPORT
		dssi_descr = NULL;
#endif

		return 0;
	}

	//if(_references == 0)
	if (_handle == 0)
	{
		//_references = 0;
		_handle = dlopen(fi.filePath().toLatin1().constData(), RTLD_NOW);
		//handle = dlopen(fi.absFilePath().toLatin1().constData(), RTLD_NOW);

		if (_handle == 0)
		{
			fprintf(stderr, "Plugin::incReferences dlopen(%s) failed: %s\n",
					fi.filePath().toLatin1().constData(), dlerror());
			//fi.absFilePath().toLatin1().constData(), dlerror());
			return 0;
		}

#ifdef DSSI_SUPPORT
		DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function) dlsym(_handle, "dssi_descriptor");
		if (dssi)
		{
			const DSSI_Descriptor* descr;
			for (int i = 0;; ++i)
			{
				descr = dssi(i);
				if (descr == NULL)
					break;

				QString label(descr->LADSPA_Plugin->Label);
				// Listing effect plugins only while excluding synths:
				// Do exactly what dssi-vst.cpp does for listing ladspa plugins.
				//if(label == _name &&
				if (label == _label &&
						!descr->run_synth &&
						!descr->run_synth_adding &&
						!descr->run_multiple_synths &&
						!descr->run_multiple_synths_adding)
				{
					_isDssi = true;
					ladspa = NULL;
					dssi_descr = descr;
					plugin = descr->LADSPA_Plugin;
					break;
				}
			}
		}
		else
#endif // DSSI_SUPPORT   
		{
			LADSPA_Descriptor_Function ladspadf = (LADSPA_Descriptor_Function) dlsym(_handle, "ladspa_descriptor");
			if (ladspadf)
			{
				const LADSPA_Descriptor* descr;
				for (int i = 0;; ++i)
				{
					descr = ladspadf(i);
					if (descr == NULL)
						break;

					QString label(descr->Label);
					//if(label == _name)
                    if (label == m_label)
					{
						ladspa = ladspadf;
						plugin = descr;
						break;
					}
				}
			}
		}

		if (plugin != NULL)
		{
			//_instNo     = 0;
            m_name = QString(plugin->Name);
            m_uniqueId = plugin->UniqueID;
            m_maker = QString(plugin->Maker);
            m_copyright = QString(plugin->Copyright);

			//if(_portDescriptors)
			//  delete[] _portDescriptors;
			//_portDescriptors = 0;
			_portCount = plugin->PortCount;
			//if(_portCount)
			//  _portDescriptors = new LADSPA_PortDescriptor[_portCount];

			_inports = 0;
			_outports = 0;
			_controlInPorts = 0;
			_controlOutPorts = 0;
			for (unsigned long k = 0; k < _portCount; ++k)
			{
				LADSPA_PortDescriptor pd = plugin->PortDescriptors[k];
				//_portDescriptors[k] = pd;
				if (pd & LADSPA_PORT_AUDIO)
				{
					if (pd & LADSPA_PORT_INPUT)
						++_inports;
					else
						if (pd & LADSPA_PORT_OUTPUT)
						++_outports;

					rpIdx.push_back((unsigned long) - 1);
				}
				else
					if (pd & LADSPA_PORT_CONTROL)
				{
					if (pd & LADSPA_PORT_INPUT)
					{
						rpIdx.push_back(_controlInPorts);
						++_controlInPorts;
					}
					else
						if (pd & LADSPA_PORT_OUTPUT)
					{
						rpIdx.push_back((unsigned long) - 1);
						++_controlOutPorts;
					}
				}
			}

            //_inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(plugin->Properties);

			// Blacklist vst plugins in-place configurable for now.
            //if ((_inports != _outports) || (fi.completeBaseName() == QString("dssi-vst") && !config.vstInPlace))
            //	_inPlaceCapable = false;
		}
	}

	if (plugin == NULL)
	{
		dlclose(_handle);
		_handle = 0;
		_references = 0;
		fprintf(stderr, "Plugin::incReferences Error: %s no plugin!\n", fi.filePath().toLatin1().constData());
		return 0;
	}

	_references = newref;

	//QString guiPath(info.dirPath() + "/" + info.baseName());
	//QDir guiDir(guiPath, "*", QDir::Unsorted, QDir::Files);
	//_hasGui = guiDir.exists();

	return _references;
}/*}}}*/

//---------------------------------------------------------
//   range
//---------------------------------------------------------

void Plugin::range(unsigned long i, float* min, float* max) const/*{{{*/
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
//   defaultValue
//---------------------------------------------------------

double Plugin::defaultValue(unsigned long port) const/*{{{*/
{
	if (port >= plugin->PortCount)
		return 0.0;

	LADSPA_PortRangeHint range = plugin->PortRangeHints[port];
	LADSPA_PortRangeHintDescriptor rh = range.HintDescriptor;
	double val = 1.0;
	if (LADSPA_IS_HINT_DEFAULT_MINIMUM(rh))
		val = range.LowerBound;
	else if (LADSPA_IS_HINT_DEFAULT_LOW(rh))
		if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
			val = exp(fast_log10(range.LowerBound) * .75 +
				log(range.UpperBound) * .25);
		else
			val = range.LowerBound * .75 + range.UpperBound * .25;
	else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(rh))
		if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
			val = exp(log(range.LowerBound) * .5 +
				log(range.UpperBound) * .5);
		else
			val = range.LowerBound * .5 + range.UpperBound * .5;
	else if (LADSPA_IS_HINT_DEFAULT_HIGH(rh))
		if (LADSPA_IS_HINT_LOGARITHMIC(range.HintDescriptor))
			val = exp(log(range.LowerBound) * .25 +
				log(range.UpperBound) * .75);
		else
			val = range.LowerBound * .25 + range.UpperBound * .75;
	else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(rh))
		val = range.UpperBound;
	else if (LADSPA_IS_HINT_DEFAULT_0(rh))
		val = 0.0;
	else if (LADSPA_IS_HINT_DEFAULT_1(rh))
		val = 1.0;
	else if (LADSPA_IS_HINT_DEFAULT_100(rh))
		val = 100.0;
	else if (LADSPA_IS_HINT_DEFAULT_440(rh))
		val = 440.0;

	return val;
}/*}}}*/

const char* Plugin::portName(unsigned long i)
{
    return plugin ? plugin->PortNames[i] : 0;
}
#endif

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

        if (effect)
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

            if (blacklist.contains( ((QFileInfo*)&*it)->baseName()) )
                continue;

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
    //loadPluginDir(oomGlobalLib + QString("/plugins"), PLUGIN_LADSPA); // FIXME?

    //lv2 = vst = false;

    // LADSPA
    if (ladspa)
    {
        fprintf(stderr, "Looking up LADSPA plugins...\n");

#ifdef __WINDOWS__
        // TODO - look for ladspa in known locations
#else
        const char* ladspaPath = getenv("LADSPA_PATH");
        if (ladspaPath == 0)
            ladspaPath = "/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";
        QStringList ladspaPathList = QString(ladspaPath).split(":");
#endif

        for (int i=0; i < ladspaPathList.count(); i++)
            loadPluginDir(ladspaPathList[i], PLUGIN_LADSPA);
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

#ifdef __WINDOWS__
        // TODO - look for vst in known locations
#else
        const char* vstPath = getenv("VST_PATH");
        if (vstPath == 0)
            vstPath = "/usr/local/lib64/vst:/usr/lib64/vst:/usr/local/lib/vst:/usr/lib/vst";
        QStringList vstPathList = QString(vstPath).split(":");
#endif

        for (int i=0; i < vstPathList.count(); i++)
            loadPluginDir(vstPathList[i], PLUGIN_VST);
    }
}

#if 0
//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

void PluginI::init()
{
	_plugin = 0;
	instances = 0;
	handle = 0;
	controls = 0;
	controlsOut = 0;
	controlPorts = 0;
	controlOutPorts = 0;
	_gui = 0;
	_on = true;
	initControlValues = false;
	_showNativeGuiPending = false;
}

PluginI::PluginI()
{
    m_type = PLUGIN_LADSPA;
	_id = -1;
	_track = 0;

	init();
}

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::~PluginI()
{
    if (_plugin && m_type != PLUGIN_LV2)
	{
		deactivate();
		_plugin->incReferences(-1);
	}
	if (_gui)
		delete _gui;
	if (controlsOut)
		delete[] controlsOut;
	if (controls)
		delete[] controls;
	if (handle)
		delete[] handle;
}

//---------------------------------------------------------
//   setID
//---------------------------------------------------------

void PluginI::setID(int i)
{
	_id = i;
}

//---------------------------------------------------------
//   updateControllers
//---------------------------------------------------------

void PluginI::updateControllers()
{
	if (!_track)
		return;

	for (int i = 0; i < controlPorts; ++i)
		audio->msgSetPluginCtrlVal(_track, genACnum(_id, i), controls[i].val);
}

bool PluginI::isAudioIn(int k)
{
    return (_plugin->portd(k) & AUDIO_IN) == AUDIO_IN;
}

bool PluginI::isAudioOut(int k)
{
    return (_plugin->portd(k) & AUDIO_OUT) == AUDIO_OUT;
}

//---------------------------------------------------------
//   valueType
//---------------------------------------------------------

CtrlValueType PluginI::valueType() const
{
	return VAL_LINEAR;
}

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void PluginI::setChannels(int c)/*{{{*/
{
	channel = c;

	// Also, pick the least number of ins or outs, and base the number of instances on that.
	unsigned long ins = _plugin->inports();
	unsigned long outs = _plugin->outports();
	int ni = 1;
	if (outs)
		ni = c / outs;
	else
		if (ins)
		ni = c / ins;

	if (ni < 1)
		ni = 1;

	if (ni == instances)
		return;

	// remove old instances:
	deactivate();
	delete[] handle;
	instances = ni;
	handle = new LADSPA_Handle[instances];
	for (int i = 0; i < instances; ++i)
	{
		handle[i] = _plugin->instantiate();
		if (handle[i] == NULL)
		{
			printf("cannot instantiate instance %d\n", i);
			return;
		}
	}

	int curPort = 0;
	int curOutPort = 0;
	unsigned long ports = _plugin->ports();
	for (unsigned long k = 0; k < ports; ++k)
	{
		LADSPA_PortDescriptor pd = _plugin->portd(k);
		if (pd & LADSPA_PORT_CONTROL)
		{
			if (pd & LADSPA_PORT_INPUT)
			{
				for (int i = 0; i < instances; ++i)
					_plugin->connectPort(handle[i], k, &controls[curPort].val);
				controls[curPort].idx = k;
				++curPort;
			}
			else
				if (pd & LADSPA_PORT_OUTPUT)
			{
				for (int i = 0; i < instances; ++i)
					_plugin->connectPort(handle[i], k, &controlsOut[curOutPort].val);
				controlsOut[curOutPort].idx = k;
				++curOutPort;
			}
		}
	}

	activate();
}/*}}}*/

//---------------------------------------------------------
//   defaultValue
//---------------------------------------------------------

double PluginI::defaultValue(unsigned int param) const
{
	//#warning controlPorts should really be unsigned
	if (param >= (unsigned) controlPorts)
		return 0.0;

	return _plugin->defaultValue(controls[param].idx);
}

LADSPA_Handle Plugin::instantiate()/*{{{*/
{
	LADSPA_Handle h = plugin->instantiate(plugin, sampleRate);
	if (h == NULL)
	{
		fprintf(stderr, "Plugin::instantiate() Error: plugin:%s instantiate failed!\n", plugin->Label);
		return NULL;
	}

	return h;
}/*}}}*/

//---------------------------------------------------------
//   initPluginInstance
//---------------------------------------------------------

bool PluginI::initPluginInstance(Plugin* plug, int c)/*{{{*/
{
	channel = c;
	if (plug == 0)
	{
		printf("initPluginInstance: zero plugin\n");
		return false;
	}
	_plugin = plug;
	m_type = _plugin->type();

	_plugin->incReferences(1);

#ifdef OSC_SUPPORT
	_oscif.oscSetPluginI(this);
#endif

	QString inst("-" + QString::number(_plugin->instNo()));
	_name = _plugin->name() + inst;
	_label = _plugin->label() + inst;

	// Also, pick the least number of ins or outs, and base the number of instances on that.
	unsigned long ins = plug->inports();
	unsigned long outs = plug->outports();
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

	handle = new LADSPA_Handle[instances];
	for (int i = 0; i < instances; ++i)
	{
#ifdef PLUGIN_DEBUGIN 
		fprintf(stderr, "PluginI::initPluginInstance instance:%d\n", i);
#endif

		handle[i] = _plugin->instantiate();
		if (handle[i] == NULL)
			return false;
	}

	unsigned long ports = _plugin->ports();

	controlPorts = 0;
	controlOutPorts = 0;

	for (unsigned long k = 0; k < ports; ++k)
	{
		LADSPA_PortDescriptor pd = _plugin->portd(k);
		if (pd & LADSPA_PORT_CONTROL)
		{
			if (pd & LADSPA_PORT_INPUT)
				++controlPorts;
			else
				if (pd & LADSPA_PORT_OUTPUT)
				++controlOutPorts;
		}
	}

	controls = new Port[controlPorts];
	controlsOut = new Port[controlOutPorts];

	int i = 0;
	int ii = 0;
	for (unsigned long k = 0; k < ports; ++k)
	{
		LADSPA_PortDescriptor pd = _plugin->portd(k);
		if (pd & LADSPA_PORT_CONTROL)
		{
			if (pd & LADSPA_PORT_INPUT)
			{
				double val = _plugin->defaultValue(k);
				controls[i].val = val;
				controls[i].tmpVal = val;
				controls[i].enCtrl = true;
				controls[i].en2Ctrl = true;
				++i;
			}
			else
				if (pd & LADSPA_PORT_OUTPUT)
			{
				//double val = _plugin->defaultValue(k);
				controlsOut[ii].val = 0.0;
				controlsOut[ii].tmpVal = 0.0;
				controlsOut[ii].enCtrl = false;
				controlsOut[ii].en2Ctrl = false;
				++ii;
			}
		}
	}
	unsigned long curPort = 0;
	unsigned long curOutPort = 0;
	for (unsigned long k = 0; k < ports; ++k)
	{
		LADSPA_PortDescriptor pd = _plugin->portd(k);
		if (pd & LADSPA_PORT_CONTROL)
		{
			if (pd & LADSPA_PORT_INPUT)
			{
				for (int i = 0; i < instances; ++i)
					_plugin->connectPort(handle[i], k, &controls[curPort].val);
				controls[curPort].idx = k;
				LADSPA_PortRangeHint range = _plugin->range(k);
				LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
				controls[curPort].logarithmic = false;
				controls[curPort].isInt = false;
				controls[curPort].toggle = false;
				controls[curPort].samplerate = false;
				if(LADSPA_IS_HINT_TOGGLED(desc))
					controls[curPort].toggle = true;
				if(LADSPA_IS_HINT_LOGARITHMIC(desc))
					controls[curPort].logarithmic = true;
				if(LADSPA_IS_HINT_INTEGER(desc))
					controls[curPort].isInt = true;
				if(LADSPA_IS_HINT_SAMPLE_RATE(desc))
					controls[curPort].samplerate = true;
				_plugin->range(k, &controls[curPort].min, &controls[curPort].max);
				controls[curPort].name = std::string(_plugin->portName(curPort));
				++curPort;
			}
			else
				if (pd & LADSPA_PORT_OUTPUT)
			{
				for (int i = 0; i < instances; ++i)
					_plugin->connectPort(handle[i], k, &controlsOut[curOutPort].val);
				controlsOut[curOutPort].idx = k;
				LADSPA_PortRangeHint range = _plugin->range(k);
				LADSPA_PortRangeHintDescriptor desc = range.HintDescriptor;
				controlsOut[curOutPort].logarithmic = false;
				controlsOut[curOutPort].isInt = false;
				controlsOut[curOutPort].toggle = false;
				controlsOut[curOutPort].samplerate = false;
				if(LADSPA_IS_HINT_TOGGLED(desc))
					controlsOut[curOutPort].toggle = true;
				if(LADSPA_IS_HINT_LOGARITHMIC(desc))
					controlsOut[curOutPort].logarithmic = true;
				if(LADSPA_IS_HINT_INTEGER(desc))
					controlsOut[curOutPort].isInt = true;
				if(LADSPA_IS_HINT_SAMPLE_RATE(desc))
					controlsOut[curOutPort].samplerate = true;
				_plugin->range(k, &controlsOut[curOutPort].min, &controlsOut[curOutPort].max);
				controlsOut[curOutPort].name = std::string(_plugin->portName(curOutPort));
				++curOutPort;
			}
		}
	}
	activate();
	return true;
}/*}}}*/

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void PluginI::connect(int ports, float** src, float** dst)/*{{{*/
{
	int port = 0;
	for (int i = 0; i < instances; ++i)
	{
		for (unsigned long k = 0; k < _plugin->ports(); ++k)
		{
			if (isAudioIn(k))
			{
				//printf("PluginI::connect Audio input port: %d value: %4.2f\n", port, *src[port]);
				_plugin->connectPort(handle[i], k, src[port]);
				port = (port + 1) % ports;
			}
		}
	}
	port = 0;
	for (int i = 0; i < instances; ++i)
	{
		for (unsigned long k = 0; k < _plugin->ports(); ++k)
		{
			if (isAudioOut(k))
			{
				//printf("PluginI::connect Audio output port: %d value: %4.2f\n", port, *dst[port]);
				_plugin->connectPort(handle[i], k, dst[port]);
				port = (port + 1) % ports; // overwrite output?
			}
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   deactivate
//---------------------------------------------------------

void PluginI::deactivate()
{
    if(m_type == PLUGIN_LV2)
		return;
	for (int i = 0; i < instances; ++i)
	{
		_plugin->deactivate(handle[i]);
		_plugin->cleanup(handle[i]);
	}
}

//---------------------------------------------------------
//   activate
//---------------------------------------------------------

void PluginI::activate()
{
	for (int i = 0; i < instances; ++i)
		_plugin->activate(handle[i]);
	if (initControlValues)
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
	}
}

//---------------------------------------------------------
//   setControl
//    set plugin instance controller value by name
//---------------------------------------------------------

bool PluginI::setControl(const QString& s, double val)/*{{{*/
{
	for (int i = 0; i < controlPorts; ++i)
	{
		if (_plugin->portName(controls[i].idx) == s)
		{
			controls[i].val = controls[i].tmpVal = val;
			return false;
		}
	}
	printf("PluginI:setControl(%s, %f) controller not found\n",
			s.toLatin1().constData(), val);
	return true;
}/*}}}*/

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void PluginI::writeConfiguration(int level, Xml& xml)/*{{{*/
{
	xml.tag(level++, "plugin file=\"%s\" label=\"%s\" channel=\"%d\"",
			Xml::xmlString(_plugin->lib()).toLatin1().constData(), Xml::xmlString(_plugin->label()).toLatin1().constData(), channel);

	for (int i = 0; i < controlPorts; ++i)
	{
		int idx = controls[i].idx;
		QString s("control name=\"%1\" val=\"%2\" /");
		xml.tag(level, s.arg(Xml::xmlString(_plugin->portName(idx)).toLatin1().constData()).arg(controls[i].tmpVal).toLatin1().constData());
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
	xml.tag(level--, "/plugin");
}/*}}}*/

//---------------------------------------------------------
//   loadControl
//---------------------------------------------------------

bool PluginI::loadControl(Xml& xml)/*{{{*/
{
	QString file;
	QString label;
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

//---------------------------------------------------------
//   readConfiguration
//    return true on error
//---------------------------------------------------------

bool PluginI::readConfiguration(Xml& xml, bool readPreset)/*{{{*/
{
	QString file;
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
					_plugin = plugins.find(file, label);

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
					xml.unknown("PluginI");
				break;
			case Xml::Attribut:
				if (tag == "file")
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
						file = s;
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
				if (tag == "plugin")
				{
					if (!readPreset && _plugin == 0)
					{
						_plugin = plugins.find(file, label);
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

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void PluginI::showNativeGui()
{
#ifdef OSC_SUPPORT
	if (_plugin)
	{
		if (_oscif.oscGuiVisible())
			_oscif.oscShowGui(false);
		else
			_oscif.oscShowGui(true);
	}
#endif
	_showNativeGuiPending = false;
}

#ifdef OSC_SUPPORT
void PluginI::showNativeGui(bool flag)
{
	if (_plugin)
	{
		_oscif.oscShowGui(flag);
	}
#else
void PluginI::showNativeGui(bool)
{
#endif
	_showNativeGuiPending = false;
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool PluginI::nativeGuiVisible()
{
#ifdef OSC_SUPPORT
	return _oscif.oscGuiVisible();
#endif    

	return false;
}

//---------------------------------------------------------
//   enableAllControllers
//---------------------------------------------------------

void PluginI::enableAllControllers(bool v)
{
	for (int i = 0; i < controlPorts; ++i)
		controls[i].enCtrl = v;
}

//---------------------------------------------------------
//   enable2AllControllers
//---------------------------------------------------------

void PluginI::enable2AllControllers(bool v)
{
	for (int i = 0; i < controlPorts; ++i)
		controls[i].en2Ctrl = v;
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void PluginI::apply(int n)/*{{{*/
{
	unsigned long ctls = controlPorts;
	for (unsigned long k = 0; k < ctls; ++k)
	{
		// First, update the temporary value if needed...

#ifdef OSC_SUPPORT
		// Process OSC gui input control fifo events.
		// It is probably more important that these are processed so that they take precedence over all other
		//  events because OSC + DSSI/DSSI-VST are fussy about receiving feedback via these control ports, from GUI changes.

		OscControlFifo* cfifo = _oscif.oscFifo(k);

		// If there are 'events' in the fifo, get exactly one 'event' per control per process cycle...
		if (cfifo && !cfifo->isEmpty())
		{
			OscControlValue v = cfifo->get();

#ifdef PLUGIN_DEBUGIN
			fprintf(stderr, "PluginI::apply OscControlFifo event input control number:%ld value:%f\n", k, v.value);
#endif

			// Set the ladspa control port value.
			controls[k].tmpVal = v.value;

			// Need to update the automation value, otherwise it overwrites later with the last automation value.
			if (_track && _id != -1)
			{
				// Since we are now in the audio thread context, there's no need to send a message,
				//  just modify directly.
				_track->setPluginCtrlVal(genACnum(_id, k), v.value);

				// Record automation.
				// NO! Take care of this immediately in the OSC control handler, because we don't want
				//  the silly delay associated with processing the fifo one-at-a-time here.

				//AutomationType at = _track->automationType();
				// TODO: Taken from our native gui control handlers.
				// This may need modification or may cause problems -
				//  we don't have the luxury of access to the dssi gui controls !
				//if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
				//  enableController(k, false);
				//_track->recordAutomation(id, v.value);
			}
		}
		else
#endif // OSC_SUPPORT
		{
			// Process automation control value.
			if (automation && _track && _track->automationType() != AUTO_OFF && _id != -1)
			{
				if (controls[k].enCtrl && controls[k].en2Ctrl)
					controls[k].tmpVal = _track->pluginCtrlVal(genACnum(_id, k));
			}
		}

		// Now update the actual value from the temporary value...
		controls[k].val = controls[k].tmpVal;
	}

	for (int i = 0; i < instances; ++i)
	{
		_plugin->apply(handle[i], n);
	}
}/*}}}*/
#endif
