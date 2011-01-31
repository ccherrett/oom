//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: mitplugin.cpp,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "mitplugin.h"
#include "app.h"
#include "event.h"
#include "xml.h"

#include "midiitransform.h"
#include "mittranspose.h"
#include "midifilterimpl.h"
#include "mrconfig.h"

#ifdef BUILD_EXPERIMENTAL
#include "rhythm.h"
#endif

MITPluginList mitPlugins;

//---------------------------------------------------------
//   startMidiInputPlugin
//---------------------------------------------------------

void OOMidi::startMidiInputPlugin(int id)
{
	bool flag = false;
	QWidget* w = 0;
	QAction* act;
	if (id == 0)
	{
		if (!mitPluginTranspose)
		{
			mitPluginTranspose = new MITPluginTranspose();
			mitPluginTranspose->setObjectName("mitPluginTranspose");
			mitPluginTranspose->setStyleSheet("QDialog{background-color: #595966;}");
			mitPlugins.push_back(mitPluginTranspose);
			connect(mitPluginTranspose, SIGNAL(hideWindow()),
					SLOT(hideMitPluginTranspose()));
		}
		w = mitPluginTranspose;
		act = midiTrpAction;
	}
	else if (id == 1)
	{
		if (!midiInputTransform)
		{
			midiInputTransform = new MidiInputTransformDialog();
			midiInputTransform->setObjectName("midiInputTransform");
			connect(midiInputTransform, SIGNAL(hideWindow()),
					SLOT(hideMidiInputTransform()));
		}
		w = midiInputTransform;
		act = midiInputTrfAction;
	}
	else if (id == 2)
	{
		if (!midiFilterConfig)
		{
			midiFilterConfig = new MidiFilterConfig();
			midiFilterConfig->setObjectName("midiFilterConfig");
			connect(midiFilterConfig, SIGNAL(hideWindow()),
					SLOT(hideMidiFilterConfig()));
		}
		w = midiFilterConfig;
		act = midiInputFilterAction;
	}
	else if (id == 3)
	{
		if (!midiRemoteConfig)
		{
			midiRemoteConfig = new MRConfig();
			midiRemoteConfig->setObjectName("midiRemoteConfig");
			connect(midiRemoteConfig, SIGNAL(hideWindow()),
					SLOT(hideMidiRemoteConfig()));
		}
		w = midiRemoteConfig;
		act = midiRemoteAction;
	}
#ifdef BUILD_EXPERIMENTAL
	else if (id == 4)
	{
		if (!midiRhythmGenerator)
		{
			midiRhythmGenerator = new RhythmGen();
			midiRhythmGenerator->setObjectName("midiRhythmGenerator");
			connect(midiRhythmGenerator, SIGNAL(hideWindow()),
					SLOT(hideMidiRhythmGenerator()));
		}
		w = midiRhythmGenerator;
		act = midiRhythmAction;
	}
#endif
	if (w)
	{
		flag = !w->isVisible();
		if (flag)
			w->show();
		else
			w->hide();
	}
	act->setChecked(flag);
}

void OOMidi::hideMitPluginTranspose()
{
	midiTrpAction->setChecked(false);
}

void OOMidi::hideMidiInputTransform()
{
	midiInputTrfAction->setChecked(false);
}

void OOMidi::hideMidiFilterConfig()
{
	midiInputFilterAction->setChecked(false);
}

void OOMidi::hideMidiRemoteConfig()
{
	midiRemoteAction->setChecked(false);
}
#ifdef BUILD_EXPERIMENTAL

void OOMidi::hideMidiRhythmGenerator()
{
	midiRhythmAction->setChecked(false);
}
#endif

//---------------------------------------------------------
//   processMidiInputTransformPlugins
//---------------------------------------------------------

void processMidiInputTransformPlugins(MEvent& event)
{
	for (iMITPlugin i = mitPlugins.begin(); i != mitPlugins.end(); ++i)
		(*i)->process(event);
}

//---------------------------------------------------------
//   startMidiTransformer
//---------------------------------------------------------

void OOMidi::startMidiTransformer()
{
	if (midiTransformerDialog == 0)
		midiTransformerDialog = new MidiTransformerDialog;
	midiTransformerDialog->show();
}

//---------------------------------------------------------
//   writeStatusMidiInputTransformPlugins
//---------------------------------------------------------

void writeStatusMidiInputTransformPlugins(int level, Xml& xml)
{
	for (iMITPlugin i = mitPlugins.begin(); i != mitPlugins.end(); ++i)
	{
		xml.tag(level++, "mplugin name=\"%d\"");
		(*i)->writeStatus(level, xml);
		xml.etag(level, "mplugin");
	}
}

//---------------------------------------------------------
//   readStatusMidiInputTransformPlugin
//---------------------------------------------------------

void readStatusMidiInputTransformPlugin(Xml&)
{
}

