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

#include "config.h"
#include "plugingui.h"
#include "plugindialog.h"

#ifdef LV2_SUPPORT
#include "lv2_plugin.h"
#endif

// Turn on debugging messages.
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

	bool hasdef = ladspaDefaultValue(plugin, port, &fdef);
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
		case LV2:
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
	_isDssi = isDssi;
	if(isDssi)
	{
		m_type = DSSI;
	}
	else
		m_type = LADSPA;
#ifdef DSSI_SUPPORT
	dssi_descr = NULL;
#endif

	fi = *f;
	plugin = NULL;
	ladspa = NULL;
	_handle = 0;
	_references = 0;
	_instNo = 0;
	_label = QString(d->Label);
	_name = QString(d->Name);
	_uniqueID = d->UniqueID;
	_maker = QString(d->Maker);
	_copyright = QString(d->Copyright);

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

	_inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(d->Properties);

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
	if ((_inports != _outports) || (fi.completeBaseName() == QString("dssi-vst") && !config.vstInPlace))
		_inPlaceCapable = false;
}/*}}}*/

Plugin::~Plugin()
{
	//if(_portDescriptors)
	//  delete[] _portDescriptors;
}

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
					if (label == _label)
					{
						_isDssi = false;
						ladspa = ladspadf;
						plugin = descr;

#ifdef DSSI_SUPPORT
						dssi_descr = NULL;
#endif

						break;
					}
				}
			}
		}

		if (plugin != NULL)
		{
			//_instNo     = 0;
			_name = QString(plugin->Name);
			_uniqueID = plugin->UniqueID;
			_maker = QString(plugin->Maker);
			_copyright = QString(plugin->Copyright);

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

			_inPlaceCapable = !LADSPA_IS_INPLACE_BROKEN(plugin->Properties);

			// Blacklist vst plugins in-place configurable for now.
			if ((_inports != _outports) || (fi.completeBaseName() == QString("dssi-vst") && !config.vstInPlace))
				_inPlaceCapable = false;
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

//---------------------------------------------------------
//   loadPluginLib
//---------------------------------------------------------

static void loadPluginLib(QFileInfo* fi)
{
	void* handle = dlopen(fi->filePath().toAscii().constData(), RTLD_NOW);
	if (handle == 0)
	{
		fprintf(stderr, "dlopen(%s) failed: %s\n",
				fi->filePath().toAscii().constData(), dlerror());
		return;
	}

#ifdef DSSI_SUPPORT
	DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function) dlsym(handle, "dssi_descriptor");
	if (dssi)
	{
		const DSSI_Descriptor* descr;
		for (int i = 0;; ++i)
		{
			descr = dssi(i);
			if (descr == 0)
				break;

			// Listing effect plugins only while excluding synths:
			// Do exactly what dssi-vst.cpp does for listing ladspa plugins.
			if (!descr->run_synth &&
					!descr->run_synth_adding &&
					!descr->run_multiple_synths &&
					!descr->run_multiple_synths_adding)
			{
				// Make sure it doesn't already exist.
				if (plugins.find(fi->completeBaseName(), QString(descr->LADSPA_Plugin->Label)) != 0)
					continue;

#ifdef PLUGIN_DEBUGIN 
				fprintf(stderr, "loadPluginLib: dssi effect name:%s inPlaceBroken:%d\n", descr->LADSPA_Plugin->Name, LADSPA_IS_INPLACE_BROKEN(descr->LADSPA_Plugin->Properties));
#endif

				//LADSPA_Properties properties = descr->LADSPA_Plugin->Properties;
				//bool inPlaceBroken = LADSPA_IS_INPLACE_BROKEN(properties);
				//plugins.add(fi, descr, !inPlaceBroken);
				if (debugMsg)
					fprintf(stderr, "loadPluginLib: adding dssi effect plugin:%s name:%s label:%s\n", fi->filePath().toLatin1().constData(), descr->LADSPA_Plugin->Name, descr->LADSPA_Plugin->Label);

				plugins.add(fi, descr->LADSPA_Plugin, true);
			}
		}
	}
	else
#endif
	{
		LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function) dlsym(handle, "ladspa_descriptor");
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
			dlclose(handle);
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

			//LADSPA_Properties properties = descr->Properties;
			//bool inPlaceBroken = LADSPA_IS_INPLACE_BROKEN(properties);
			//plugins.add(fi, ladspa, descr, !inPlaceBroken);
			if (debugMsg)
				fprintf(stderr, "loadPluginLib: adding ladspa plugin:%s name:%s label:%s\n", fi->filePath().toLatin1().constData(), descr->Name, descr->Label);
			plugins.add(fi, descr);
		}
	}

	dlclose(handle);
}

//---------------------------------------------------------
//   loadPluginDir
//---------------------------------------------------------

static void loadPluginDir(const QString& s)
{
	if (debugMsg)
		printf("scan ladspa plugin dir <%s>\n", s.toLatin1().constData());
	QDir pluginDir(s, QString("*.so")); // ddskrjo
	if (pluginDir.exists())
	{
		QFileInfoList list = pluginDir.entryInfoList();
		QFileInfoList::iterator it = list.begin();
		while (it != list.end())
		{
			loadPluginLib(&*it);
			++it;
		}
	}
}

//---------------------------------------------------------
//   initPlugins
//---------------------------------------------------------

void initPlugins()
{
	loadPluginDir(oomGlobalLib + QString("/plugins"));

	const char* p = 0;

	// Take care of DSSI plugins first...
#ifdef DSSI_SUPPORT
	const char* dssiPath = getenv("DSSI_PATH");
	if (dssiPath == 0)
		dssiPath = "/usr/local/lib64/dssi:/usr/lib64/dssi:/usr/local/lib/dssi:/usr/lib/dssi";
	p = dssiPath;
	while (*p != '\0')
	{
		const char* pe = p;
		while (*pe != ':' && *pe != '\0')
			pe++;

		int n = pe - p;
		if (n)
		{
			char* buffer = new char[n + 1];
			strncpy(buffer, p, n);
			buffer[n] = '\0';
			loadPluginDir(QString(buffer));
			delete[] buffer;
		}
		p = pe;
		if (*p == ':')
			p++;
	}
#endif

	// Now do LADSPA plugins...
	const char* ladspaPath = getenv("LADSPA_PATH");
	if (ladspaPath == 0)
		ladspaPath = "/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";
	p = ladspaPath;

	if (debugMsg)
		fprintf(stderr, "loadPluginDir: ladspa path:%s\n", ladspaPath);

	while (*p != '\0')
	{
		const char* pe = p;
		while (*pe != ':' && *pe != '\0')
			pe++;

		int n = pe - p;
		if (n)
		{
			char* buffer = new char[n + 1];
			strncpy(buffer, p, n);
			buffer[n] = '\0';
			if (debugMsg)
				fprintf(stderr, "loadPluginDir: loading ladspa dir:%s\n", buffer);

			loadPluginDir(QString(buffer));
			delete[] buffer;
		}
		p = pe;
		if (*p == ':')
			p++;
	}
	//Now we do the LV2
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Plugin* PluginList::find(const QString& file, const QString& name)
{
	for (iPlugin i = begin(); i != end(); ++i)
	{
		if ((file == i->lib()) && (name == i->label()))
			return &*i;
	}
	//printf("Plugin <%s> not found\n", name.ascii());
	return 0;
}

//---------------------------------------------------------
//   Pipeline
//---------------------------------------------------------

Pipeline::Pipeline()
: std::vector<PluginI*>()
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
//   setChannels
//---------------------------------------------------------

void Pipeline::setChannels(int n)
{
	for (int i = 0; i < PipelineDepth; ++i)
		if ((*this)[i])
			(*this)[i]->setChannels(n);
}

//---------------------------------------------------------
//   insert
//    give ownership of object plugin to Pipeline
//---------------------------------------------------------

void Pipeline::insert(PluginI* plugin, int index)
{
	remove(index);
	(*this)[index] = plugin;
}

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Pipeline::remove(int index)
{
	PluginI* plugin = (*this)[index];
	if (plugin)
		delete plugin;
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
//   isOn
//---------------------------------------------------------

bool Pipeline::isOn(int idx) const
{
	PluginI* p = (*this)[idx];
	if (p)
		return p->on();
	return false;
}

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void Pipeline::setOn(int idx, bool flag)
{
	PluginI* p = (*this)[idx];
	if (p)
	{
		p->setOn(flag);
		if (p->gui())
			p->gui()->setOn(flag);
	}
}

//---------------------------------------------------------
//   label
//---------------------------------------------------------

QString Pipeline::label(int idx) const
{
	PluginI* p = (*this)[idx];
	if (p)
		return p->label();
	return QString("");
}

//---------------------------------------------------------
//   name
//---------------------------------------------------------

QString Pipeline::name(int idx) const
{
	PluginI* p = (*this)[idx];
	if (p)
		return p->name();
	return QString("empty");
}

//---------------------------------------------------------
//   empty
//---------------------------------------------------------

bool Pipeline::empty(int idx) const
{
	PluginI* p = (*this)[idx];
	return p == 0;
}

//---------------------------------------------------------
//   move
//---------------------------------------------------------

void Pipeline::move(int idx, bool up)
{
	PluginI* p1 = (*this)[idx];
	if (up)
	{
		(*this)[idx] = (*this)[idx - 1];

		if ((*this)[idx])
			(*this)[idx]->setID(idx);

		(*this)[idx - 1] = p1;

		if (p1)
		{
			p1->setID(idx - 1);
			if (p1->track())
				audio->msgSwapControllerIDX(p1->track(), idx, idx - 1);
		}
	}
	else
	{
		(*this)[idx] = (*this)[idx + 1];

		if ((*this)[idx])
			(*this)[idx]->setID(idx);

		(*this)[idx + 1] = p1;

		if (p1)
		{
			p1->setID(idx + 1);
			if (p1->track())
				audio->msgSwapControllerIDX(p1->track(), idx, idx + 1);
		}
	}
}

//---------------------------------------------------------
//   isDssiPlugin
//---------------------------------------------------------

bool Pipeline::isDssiPlugin(int idx) const
{
	PluginI* p = (*this)[idx];
	if (p)
	{
#ifdef LV2_SUPPORT
		if(p->type() == 2)
		{
			//LV2PluginI* lp = (LV2PluginI*)p;
			//FIXME: For now this just check to enable native gui support, so we return true for lv2
			return true;
		}
		else
#endif
			return p->isDssiPlugin();
	}

	return false;
}

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void Pipeline::showGui(int idx, bool flag)
{
	PluginI* p = (*this)[idx];
	if (p)
	{
#ifdef LV2_SUPPORT
		if(p->type() == 2)
		{
			LV2PluginI* lp = (LV2PluginI*)p;
			lp->showGui(flag);
		}
		else
#endif
		{
			p->showGui(flag);
		}
	}
}

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

#ifdef OSC_SUPPORT
void Pipeline::showNativeGui(int idx, bool flag)
{
	bool islv2 = false;
	PluginI* p = (*this)[idx];
#ifdef LV2_SUPPORT
	islv2 = (p->type() == 2);
	if(islv2)
	{
		LV2PluginI* lvp = (LV2PluginI*)p;
		if(lvp)
			lvp->showNativeGui(flag);
	}
#endif
	if (p && !islv2)
		p->oscIF().oscShowGui(flag);
}
#else
void Pipeline::showNativeGui(int idx, bool flag)
{
#ifdef LV2_SUPPORT
	PluginI* p = (*this)[idx];
	bool islv2 = (p->type() == 2);
	if(islv2)
	{
		LV2PluginI* lvp = (LV2Plugin*)p;
		if(lvp)
			lvp->showNativeGui(flag);
	}
#endif
}
#endif      

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void Pipeline::deleteGui(int idx)
{
	if (idx >= PipelineDepth)
		return;
	PluginI* p = (*this)[idx];
#ifdef LV2_SUPPORT
	if(p && p->type() == 2)
	{
		LV2PluginI* lp = (LV2PluginI*)p;
		if(lp)
			lp->deleteGui();
	}
	else
#endif
	{
		if (p)
			p->deleteGui();
	}
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
	PluginI* p = (*this)[idx];
	if (p)
		return p->guiVisible();
	return false;
}

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool Pipeline::nativeGuiVisible(int idx)
{
	PluginI* p = (*this)[idx];
	if (p)
		return p->nativeGuiVisible();
	return false;
}

void Pipeline::updateNativeGui()
{
#ifdef LV2_SUPPORT/*{{{*/
	for (iPluginI i = begin(); i != end(); ++i)
	{
		PluginI* p = (PluginI*)*i;
		if(p && p->type() == 2)
		{
			LV2PluginI* lp = (LV2PluginI*)p;
			if(lp)
				lp->heartBeat();
		}
	}
#endif/*}}}*/
}
//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void Pipeline::apply(int ports, unsigned long nframes, float** buffer1)/*{{{*/
{
	//fprintf(stderr, "Pipeline::apply data: nframes:%ld %e %e %e %e\n", nframes, buffer1[0][0], buffer1[0][1], buffer1[0][2], buffer1[0][3]);

	bool swap = false;

	for (iPluginI ip = begin(); ip != end(); ++ip)
	{
		PluginI* p = *ip;
#ifdef LV2_SUPPORT
		if(p && p->type() == 2)
		{
			LV2PluginI* lp = (LV2PluginI*)p;/*{{{*/
			if (lp && lp->on())
			{
				if (lp->inPlaceCapable())
				{
					if (swap)
						lp->connect(ports, buffer, buffer);
					else
						lp->connect(ports, buffer1, buffer1);
				}
				else
				{
					if (swap)
						lp->connect(ports, buffer, buffer1);
					else
						lp->connect(ports, buffer1, buffer);
					swap = !swap;
				}
				lp->apply(nframes);
			}/*}}}*/
		}
		else
#endif
		{
			if (p && p->on())
			{
				if (p->inPlaceCapable())
				{
					if (swap)
						p->connect(ports, buffer, buffer);
					else
						p->connect(ports, buffer1, buffer1);
				}
				else
				{
					if (swap)
						p->connect(ports, buffer, buffer1);
					else
						p->connect(ports, buffer1, buffer);
					swap = !swap;
				}
				p->apply(nframes);
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

}/*}}}*/

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
	m_type = LADSPA;
	_id = -1;
	_track = 0;

	init();
}

//---------------------------------------------------------
//   PluginI
//---------------------------------------------------------

PluginI::~PluginI()
{
	if (_plugin && m_type != 2)
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
//   showGui
//---------------------------------------------------------

void PluginI::showGui()
{
	if (_plugin)
	{
		if (!_gui)
		{
			printf("PluginI::showGui() before makeGui\n");
			makeGui();
			printf("PluginI::showGui() after makeGui\n");
		}
		if (_gui->isVisible())
			_gui->hide();
		else
			_gui->show();
	}
}

void PluginI::showGui(bool flag)
{
	if (_plugin)
	{
		if (flag)
		{
			if (!_gui)
				makeGui();
			if (_gui)
				_gui->show();
		}
		else
		{
			if (_gui)
				_gui->hide();
		}
	}
}

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool PluginI::guiVisible()
{
	return _gui && _gui->isVisible();
}

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
//   makeGui
//---------------------------------------------------------

void PluginI::makeGui()
{
	_gui = new PluginGui(this);
}

//---------------------------------------------------------
//   deleteGui
//---------------------------------------------------------

void PluginI::deleteGui()
{
	if (_gui)
	{
		delete _gui;
		_gui = 0;
	}
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

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

#ifdef OSC_SUPPORT

int Plugin::oscConfigure(LADSPA_Handle handle, const char* key, const char* value)/*{{{*/
{
#ifdef PLUGIN_DEBUGIN 
	printf("Plugin::oscConfigure effect plugin label:%s key:%s value:%s\n", plugin->Label, key, value);
#endif

#ifdef DSSI_SUPPORT
	if (!dssi_descr || !dssi_descr->configure)
		return 0;

	if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
			strlen(DSSI_RESERVED_CONFIGURE_PREFIX)))
	{
		fprintf(stderr, "Plugin::oscConfigure OSC: UI for plugin '%s' attempted to use reserved configure key \"%s\", ignoring\n",
				plugin->Label, key);

		return 0;
	}

	char* message = dssi_descr->configure(handle, key, value);
	if (message)
	{
		printf("Plugin::oscConfigure on configure '%s' '%s', plugin '%s' returned error '%s'\n",
				key, value, plugin->Label, message);

		free(message);
	}

#endif // DSSI_SUPPORT

	return 0;
}/*}}}*/

//---------------------------------------------------------
//   oscConfigure
//---------------------------------------------------------

int PluginI::oscConfigure(const char *key, const char *value)/*{{{*/
{
	if (!_plugin)
		return 0;

#ifdef PLUGIN_DEBUGIN 
	printf("PluginI::oscConfigure effect plugin name:%s label:%s key:%s value:%s\n", _name.toLatin1().constData(), _label.toLatin1().constData(), key, value);
#endif

#ifdef DSSI_SUPPORT
	// FIXME: Don't think this is right, should probably do as example shows below.
	for (int i = 0; i < instances; ++i)
		_plugin->oscConfigure(handle[i], key, value);

	// also call back on UIs for plugins other than the one
	// that requested this:
	// if (n != instance->number && instances[n].uiTarget) {
	//      lo_send(instances[n].uiTarget,
	//      instances[n].ui_osc_configure_path, "ss", key, value);
	//      }

	// configure invalidates bank and program information, so
	//  we should do this again now:
	//queryPrograms();
#endif // DSSI_SUPPORT

	return 0;
}/*}}}*/

//---------------------------------------------------------
//   oscUpdate
//---------------------------------------------------------

int PluginI::oscUpdate()
{
#ifdef DSSI_SUPPORT
	// Send project directory.
	_oscif.oscSendConfigure(DSSI_PROJECT_DIRECTORY_KEY, oomProject.toLatin1().constData()); // song->projectPath()
#endif
	return 0;
}

//---------------------------------------------------------
//   oscControl
//---------------------------------------------------------

int PluginI::oscControl(unsigned long port, float value)/*{{{*/
{
#ifdef PLUGIN_DEBUGIN  
	printf("PluginI::oscControl received oscControl port:%ld val:%f\n", port, value);
#endif

	// Convert from DSSI port number to control input port index.
	unsigned long cport = _plugin->port2InCtrl(port);

	if ((int) cport == -1)
	{
		fprintf(stderr, "PluginI::oscControl: port number:%ld is not a control input\n", port);
		return 0;
	}

	// (From DSSI module).
	// p3.3.39 Set the DSSI control input port's value.
	// Observations: With a native DSSI synth like LessTrivialSynth, the native GUI's controls do not change the sound at all
	//  ie. they don't update the DSSI control port values themselves.
	// Hence in response to the call to this oscControl, sent by the native GUI, it is required to that here.
	///  controls[cport].val = value;
	// DSSI-VST synths however, unlike DSSI synths, DO change their OWN sound in response to their gui controls.
	// AND this function is called !
	// Despite the descrepency we are STILL required to update the DSSI control port values here
	//  because dssi-vst is WAITING FOR A RESPONSE! (A CHANGE in the control port value).
	// It will output something like "...4 events expected..." and count that number down as 4 actual control port value CHANGES
	//  are done here in response. Normally it says "...0 events expected..." when OOMidi is the one doing the DSSI control changes.
	// TODO: May need FIFOs on each control(!) so that the control changes get sent one per process cycle!
	// Observed countdown not actually going to zero upon string of changes.
	// Try this ...
	OscControlFifo* cfifo = _oscif.oscFifo(cport);
	if (cfifo)
	{
		OscControlValue cv;
		//cv.idx = cport;
		cv.value = value;
		cv.frame = audio->timestamp();
		if (cfifo->put(cv))
		{
			fprintf(stderr, "PluginI::oscControl: fifo overflow: in control number:%ld\n", cport);
		}
	}

	// Record automation:
	// Take care of this immediately, because we don't want the silly delay associated with
	//  processing the fifo one-at-a-time in the apply().
	// NOTE: Ahh crap! We don't receive control events until the user RELEASES a control !
	// So the events all arrive at once when the user releases a control.
	// That makes this pretty useless... But what the heck...
	if (_track && _id != -1)
	{
		int id = genACnum(_id, cport);
		AutomationType at = _track->automationType();

		// TODO: Taken from our native gui control handlers.
		// This may need modification or may cause problems -
		//  we don't have the luxury of access to the dssi gui controls !
		if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
			enableController(cport, false);

		_track->recordAutomation(id, value);
	}

#if 0
	int port = argv[0]->i;
	LADSPA_Data value = argv[1]->f;

	if (port < 0 || port > instance->plugin->descriptor->LADSPA_Plugin->PortCount)
	{
		fprintf(stderr, "OOMidi: OSC: %s port number (%d) is out of range\n",
				instance->friendly_name, port);
		return 0;
	}
	if (instance->pluginPortControlInNumbers[port] == -1)
	{
		fprintf(stderr, "OOMidi: OSC: %s port %d is not a control in\n",
				instance->friendly_name, port);
		return 0;
	}
	pluginControlIns[instance->pluginPortControlInNumbers[port]] = value;
	if (verbose)
	{
		printf("OOMidi: OSC: %s port %d = %f\n",
				instance->friendly_name, port, value);
	}
#endif
	return 0;
}/*}}}*/

#endif // OSC_SUPPORT

//---------------------------------------------------------
//   PluginLoader
//---------------------------------------------------------

QWidget* PluginLoader::createWidget(const QString & className, QWidget * parent, const QString & name)
{
	if (className == QString("DoubleLabel"))
		return new DoubleLabel(parent, name.toLatin1().constData());
	if (className == QString("Slider"))
		return new Slider(parent, name.toLatin1().constData(), Qt::Horizontal);

	return QUiLoader::createWidget(className, parent, name);
};
