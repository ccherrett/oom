//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: conf.cpp,v 1.33.2.18 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <sndfile.h>
#include <errno.h>
#include <stdio.h>
#include <QPair>
#include <QHash>

#include "app.h"
#include "transport.h"
#include "icons.h"
#include "globals.h"
#include "Performer.h"
#include "master/masteredit.h"
#include "bigtime.h"
#include "Composer.h"
#include "conf.h"
#include "gconfig.h"
#include "pitchedit.h"
#include "midiport.h"
#include "mididev.h"
#include "driver/audiodev.h"
#include "driver/jackmidi.h"
#include "xml.h"
#include "midi.h"
#include "shortcuts.h"
#include "midictrl.h"
#include "ctrlcombo.h"
#include "genset.h"
#include "midiitransform.h"
#include "audio.h"
#include "plugin.h"
#include "sync.h"
#include "wave.h"
#include "midiseq.h"
#include "AudioMixer.h"
#include "TrackManager.h"
#include "utils.h"

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);

extern void writeMidiInputTransforms(int level, Xml& xml);
extern void readMidiInputTransform(Xml&);

//---------------------------------------------------------
//   readGeometry
//---------------------------------------------------------

QRect readGeometry(Xml& xml, const QString& name)
{
	QRect r(0, 0, 50, 50);
	int val;

	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				xml.parse1();
				break;
			case Xml::Attribut:
				val = xml.s2().toInt();
				if (tag == "x")
					r.setX(val);
				else if (tag == "y")
					r.setY(val);
				else if (tag == "w")
					r.setWidth(val);
				else if (tag == "h")
					r.setHeight(val);
				break;
			case Xml::TagEnd:
				if (tag == name)
					return r;
			default:
				break;
		}
	}
	return r;
}


//---------------------------------------------------------
//   readColor
//---------------------------------------------------------

QColor readColor(Xml& xml)
{
	int val, r = 0, g = 0, b = 0;

	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token != Xml::Attribut)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::Attribut:
				val = xml.s2().toInt();
				if (tag == "r")
					r = val;
				else if (tag == "g")
					g = val;
				else if (tag == "b")
					b = val;
				break;
			default:
				break;
		}
	}

	return QColor(r, g, b);
}

//---------------------------------------------------------
//   readController
//---------------------------------------------------------

static void readController(Xml& xml, int midiPort, int channel)
{
	int id = 0;
	int val = CTRL_VAL_UNKNOWN;

	for (;;)
	{
		Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "val")
					val = xml.parseInt();
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				if (tag == "id")
					id = xml.s2().toInt();
				break;
			case Xml::TagEnd:
				if (tag == "controller")
				{
					MidiPort* port = &midiPorts[midiPort];
					//port->addManagedController(channel, id);
					val = port->limitValToInstrCtlRange(id, val);
					// The value here will actually be sent to the device LATER, in MidiPort::setMidiDevice()
					port->setHwCtrlState(channel, id, val);
					return;
				}
			default:
				return;
		}
	}
}

static PatchSequence* readMidiPortPatchSequences(Xml& xml)/*{{{*/
{
	int id = 0;
	bool checked = false;
	QString pname;
	PatchSequence* p;

	for (;;)
	{
		Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				xml.unknown("patchSequence");
				break;
			case Xml::Attribut:
				if (tag == "id")
					id = xml.s2().toInt();
				else if (tag == "name")
				{
					QStringList l = xml.s2().split(":    ");
					if (l.size() == 2)
						pname = l.at(0) + ": \n  " + l.at(1);
					else
						pname = xml.s2();
				}
				else if (tag == "checked")
				{
					int c = xml.s2().toInt();
					if (c)
						checked = true;
				}
				break;
			case Xml::TagEnd:
				p = new PatchSequence();
				p->name = pname;
				p->id = id;
				p->selected = checked;
				return p;
			default:
				break;
		}
	}
	return p;
}/*}}}*/

static QPair<int, QString> readMidiPortPreset(Xml& xml)/*{{{*/
{
	int id = 0;
	QString sysex;

	for (;;)
	{
		Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				xml.unknown("midiPreset");
				break;
			case Xml::Attribut:
				if (tag == "id")
					id = xml.s2().toInt();
				else if (tag == "sysex")
				{
					sysex = xml.s2();
				}
				break;
			case Xml::TagEnd:
				return qMakePair(id, sysex);
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   readPortChannel
//---------------------------------------------------------

static void readPortChannel(Xml& xml, int midiPort)
{
	int idx = 0; //torbenh
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "controller")
				{
					readController(xml, midiPort, idx);
				}
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				if (tag == "idx")
					idx = xml.s2().toInt();
				break;
			case Xml::TagEnd:
				if (tag == "channel")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   readConfigMidiPort
//---------------------------------------------------------

static void readConfigMidiPort(Xml& xml)/*{{{*/
{
	int idx = 0;
	qint64 id = -1;
	QString device;
	bool isGlobal = false;

	QString instrument("GM");

	QList<PatchSequence*> patchSequences;
	QList<QPair<int, QString> > presets;

	int openFlags = 1;
	int dic = 0;
	int doc = 0;
	MidiSyncInfo tmpSi;
	int type = MidiDevice::ALSA_MIDI;
	bool cachenrpn = false;

    MidiDevice* dev = 0;

	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "name")
                {
					device = xml.parse1();
                    if (!dev)//Look for it as an alsa or already created device
                        dev = midiDevices.find(device);
				}
				else if (tag == "type")
                {
					type = xml.parseInt();
                }
				else if (tag == "record")
				{   // old
					bool f = xml.parseInt();
					if (f)
						openFlags |= 2;
				}
				else if (tag == "openFlags")
					openFlags = xml.parseInt();
				else if (tag == "defaultInChans")
					dic = xml.parseInt();
				else if (tag == "defaultOutChans")
					doc = xml.parseInt();
				else if (tag == "midiSyncInfo")
					tmpSi.read(xml);
				else if (tag == "instrument")
				{
					instrument = xml.parse1();

                    if (instrument.endsWith(" [LV2]") || instrument.endsWith(" [VST]"))
                        dev = midiDevices.find(instrument);
				}
				else if (tag == "channel")
				{
					readPortChannel(xml, idx);
				}
				else if (tag == "preset" || tag == "patchSequence")
				{
					PatchSequence* p = readMidiPortPatchSequences(xml);
					if (p)
						patchSequences.append(p);
				}
				else if(tag == "midiPreset")
				{
					presets.append(readMidiPortPreset(xml));
				}
				else if(tag == "cacheNRPN")
				{
					cachenrpn = xml.parseInt();
				}
                else if (tag == "plugin")
                {
                    if (dev && type == MidiDevice::SYNTH_MIDI)
                    {
                        SynthPluginDevice* oldSynth = (SynthPluginDevice*)dev;
                        SynthPluginDevice* synth = oldSynth->clone(device);
                        synth->open();

                        // get into the plugin type
                        xml.parse();
                        // now load state
                        synth->read(xml);

                        dev = synth;
                    }
                    else
                        xml.parse1();
                }
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				if (tag == "idx")
				{//Check to see if this port is already used, and bump if so
					idx = xml.s2().toInt();
					int freePort = getFreeMidiPort();
					if(freePort != idx)
					{//Set a flag here so we know when loading tracks later that we are dealing with an old file or global inputs changed
						idx = freePort;
					}
				}
				else if(tag == "portId")
				{//New style
					id = xml.s2().toLongLong();
					idx = getFreeMidiPort();
				}
				else if(tag == "isGlobalInput")
				{//Find the matching input if posible and set our index to it
					isGlobal = xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if (tag == "midiport")
				{
					if(isGlobal)
					{//Find the global input that matches
						//
						if(gInputListPorts.size())
						{
							int myport = -1;
							for(int i = 0; i < gInputListPorts.size(); ++i)
							{
								myport =  gInputListPorts.at(i);

								MidiPort* inport = &midiPorts[i];
								if(inport && inport->device() && inport->device()->name() == device)
								{
									idx = myport;
									break;
								}
							}
						}
					}

					if (idx < 0 || idx >= MIDI_PORTS)
					{
						fprintf(stderr, "bad midi port %d (>%d)\n",
								idx, MIDI_PORTS);
						idx = 0;
					}

                    if (!dev)
                    {
                        if (type == MidiDevice::JACK_MIDI)
                        {
                            dev = MidiJackDevice::createJackMidiDevice(device); // p3.3.55

                            if (debugMsg)
                                fprintf(stderr, "readConfigMidiPort: creating jack midi device %s\n", device.toLatin1().constData());
                        }
                        else
                            dev = midiDevices.find(device);
                    }

					if (debugMsg && !dev)
						fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.toLatin1().constData());

					MidiPort* mp = &midiPorts[idx];
					if(id)
						mp->setPortId(id);

					mp->setInstrument(registerMidiInstrument(instrument));
					mp->setDefaultInChannels(dic);
					mp->setDefaultOutChannels(doc);

					mp->syncInfo().copyParams(tmpSi);
					//Indicate the port was found in the song file, even if no device is assigned to it.
					mp->setFoundInSongFile(true);

					if (!patchSequences.isEmpty())
					{
						for (int i = 0; i < patchSequences.size(); ++i)
						{
							mp->appendPatchSequence(patchSequences.at(i));
						}
					}
					if(!presets.isEmpty())
					{
						for(int i = 0; i < presets.size(); ++i)
						{
							QPair<int, QString> pair = presets.at(i);
							mp->addPreset(pair.first, pair.second);
						}
					}

					if (dev)
					{
						dev->setOpenFlags(openFlags);
						midiSeq->msgSetMidiDevice(mp, dev);
						dev->setCacheNRPN(cachenrpn);
					}
					oomMidiPorts.insert(mp->id(), mp);
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   loadConfigMetronom
//---------------------------------------------------------

static void loadConfigMetronom(Xml& xml)/*{{{*/
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "premeasures")
					preMeasures = xml.parseInt();
				else if (tag == "measurepitch")
					measureClickNote = xml.parseInt();
				else if (tag == "measurevelo")
					measureClickVelo = xml.parseInt();
				else if (tag == "beatpitch")
					beatClickNote = xml.parseInt();
				else if (tag == "beatvelo")
					beatClickVelo = xml.parseInt();
				else if (tag == "channel")
					clickChan = xml.parseInt();
				else if (tag == "port")
					clickPort = xml.parseInt();
				else if (tag == "precountEnable")
					precountEnableFlag = xml.parseInt();
				else if (tag == "fromMastertrack")
					precountFromMastertrackFlag = xml.parseInt();
				else if (tag == "signatureZ")
					precountSigZ = xml.parseInt();
				else if (tag == "signatureN")
					precountSigN = xml.parseInt();
				else if (tag == "prerecord")
					precountPrerecord = xml.parseInt();
				else if (tag == "preroll")
					precountPreroll = xml.parseInt();
				else if (tag == "midiClickEnable")
					midiClickFlag = xml.parseInt();
				else if (tag == "audioClickEnable")
					audioClickFlag = xml.parseInt();
				else if (tag == "audioClickVolume")
					audioClickVolume = xml.parseFloat();
				else
					xml.skip(tag);
				break;
			case Xml::TagEnd:
				if (tag == "metronom")
					return;
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   readSeqConfiguration
//---------------------------------------------------------

static void readSeqConfiguration(Xml& xml)
{
	//bool updatePorts = false;
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "metronom")
					loadConfigMetronom(xml);
				else if (tag == "midiport")
				{
					//updatePorts = true;
					readConfigMidiPort(xml);
				}
				else if (tag == "rcStop")
					rcStopNote = xml.parseInt();
				else if (tag == "rcEnable")
					rcEnable = xml.parseInt();
				else if (tag == "rcRecord")
					rcRecordNote = xml.parseInt();
				else if (tag == "rcGotoLeft")
					rcGotoLeftMarkNote = xml.parseInt();
				else if (tag == "rcPlay")
					rcPlayNote = xml.parseInt();
				else
					xml.skip(tag);
				break;
			case Xml::TagEnd:
				if (tag == "sequencer")
				{
					//All Midiports have been read so put all the unconfigured ports in the id list
					//if(updatePorts)
					//{
						for(int i = 0; i < MIDI_PORTS; ++i)
						{
							MidiPort *mp = &midiPorts[i];
							if(!oomMidiPorts.contains(mp->id()))
								oomMidiPorts.insert(mp->id(), mp);
						}
					//}
					return;
				}
			default:
				break;
		}
	}
}

void readGlobalInputList(Xml& xml)/*{{{*/
{
	gInputList.clear();
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
				if(tag == "globalInput")
				{
					readGlobalInput(xml);
				}
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if(tag == "globalInputList")
				{//Just return
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

void readGlobalInput(Xml& xml)/*{{{*/
{
	QString name;
	int type;
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
				break;
			case Xml::Attribut:
				if(tag == "deviceType")
				{
					type = xml.s2().toInt();
				}
				else if(tag == "deviceName")
				{
					name = xml.s2();
				}
				break;
			case Xml::TagEnd:
				if(tag == "globalInput")
				{
					if(debugMsg)
						qDebug("readGlobalInput: Adding My Input to list: Type: %d, Name: %s", type,  name.toUtf8().constData());
					gInputList.append(qMakePair(type, name));
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/


//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void readConfiguration(Xml& xml, bool readOnlySequencer)/*{{{*/
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				/* the reading of configuration is split in two; read
				   "sequencer" and read ALL. The reason is that it is
				   possible to load a song without configuration. In
				   this case the <configuration> chapter in the song
				   file should be skipped. However the sub part
				   <sequencer> contains elements that are necessary
				   to preserve composition consistency. Mainly
				   midiport configuration and VOLUME.
				 */
				if (tag == "sequencer")
				{
					readSeqConfiguration(xml);
					break;
				}
				else if (readOnlySequencer)
				{
					xml.skip(tag);
					break;
				}

				if (tag == "theme")
					config.style = xml.parse1();
				else if (tag == "styleSheetFile")
					config.styleSheetFile = xml.parse1();
				else if (tag == "useOldStyleStopShortCut")
					config.useOldStyleStopShortCut = xml.parseInt();
				else if (tag == "moveArmedCheckBox")
					config.moveArmedCheckBox = xml.parseInt();
				else if (tag == "externalWavEditor")
					config.externalWavEditor = xml.parse1();
				else if (tag == "font0")
					config.fonts[0].fromString(xml.parse1());
				else if (tag == "font1")
					config.fonts[1].fromString(xml.parse1());
				else if (tag == "font2")
					config.fonts[2].fromString(xml.parse1());
				else if (tag == "font3")
					config.fonts[3].fromString(xml.parse1());
				else if (tag == "font4")
					config.fonts[4].fromString(xml.parse1());
				else if (tag == "font5")
					config.fonts[5].fromString(xml.parse1());
				else if (tag == "font6")
					config.fonts[6].fromString(xml.parse1());
				else if (tag == "globalAlphaBlend")
					config.globalAlphaBlend = xml.parseInt();
				else if (tag == "palette0")
					config.palette[0] = readColor(xml);
				else if (tag == "palette1")
					config.palette[1] = readColor(xml);
				else if (tag == "palette2")
					config.palette[2] = readColor(xml);
				else if (tag == "palette3")
					config.palette[3] = readColor(xml);
				else if (tag == "palette4")
					config.palette[4] = readColor(xml);
				else if (tag == "palette5")
					config.palette[5] = readColor(xml);
				else if (tag == "palette6")
					config.palette[6] = readColor(xml);
				else if (tag == "palette7")
					config.palette[7] = readColor(xml);
				else if (tag == "palette8")
					config.palette[8] = readColor(xml);
				else if (tag == "palette9")
					config.palette[9] = readColor(xml);
				else if (tag == "palette10")
					config.palette[10] = readColor(xml);
				else if (tag == "palette11")
					config.palette[11] = readColor(xml);
				else if (tag == "palette12")
					config.palette[12] = readColor(xml);
				else if (tag == "palette13")
					config.palette[13] = readColor(xml);
				else if (tag == "palette14")
					config.palette[14] = readColor(xml);
				else if (tag == "palette15")
					config.palette[15] = readColor(xml);
				else if (tag == "palette16")
					config.palette[16] = readColor(xml);
				else if (tag == "ctrlGraphFg")
					config.ctrlGraphFg = readColor(xml);
				else if (tag == "extendedMidi")
					config.extendedMidi = xml.parseInt();
				else if (tag == "midiExportDivision")
					config.midiDivision = xml.parseInt();
				else if (tag == "smfFormat")
					config.smfFormat = xml.parseInt();
				else if (tag == "exp2ByteTimeSigs")
					config.exp2ByteTimeSigs = xml.parseInt();
				else if (tag == "expOptimNoteOffs")
					config.expOptimNoteOffs = xml.parseInt();
				else if (tag == "importMidiSplitParts")
					config.importMidiSplitParts = xml.parseInt();
				else if (tag == "midiInputDevice")
					midiInputPorts = xml.parseInt();
				else if (tag == "midiInputChannel")
					midiInputChannel = xml.parseInt();
				else if (tag == "midiRecordType")
					midiRecordType = xml.parseInt();
				else if (tag == "midiThruType")
					midiThruType = xml.parseInt();
				else if (tag == "midiFilterCtrl1")
					midiFilterCtrl1 = xml.parseInt();
				else if (tag == "midiFilterCtrl2")
					midiFilterCtrl2 = xml.parseInt();
				else if (tag == "midiFilterCtrl3")
					midiFilterCtrl3 = xml.parseInt();
				else if (tag == "midiFilterCtrl4")
					midiFilterCtrl4 = xml.parseInt();
				else if (tag == "showSplashScreen")
					config.showSplashScreen = xml.parseInt();
				else if (tag == "canvasShowPartType")
					config.canvasShowPartType = xml.parseInt();
				else if (tag == "canvasShowPartEvent")
					config.canvasShowPartEvent = xml.parseInt();
				else if (tag == "canvasShowGrid")
					config.canvasShowGrid = xml.parseInt();
				else if (tag == "canvasBgPixmap")
					config.canvasBgPixmap = xml.parse1();
				else if (tag == "canvasCustomBgList")
					config.canvasCustomBgList = xml.parse1().split(";", QString::SkipEmptyParts);
				else if(tag == "vuColorStrip")
				{
					vuColorStrip = xml.parseInt();	
				}	
				else if(tag == "nextTrackPartColor")
				{
					lastTrackPartColorIndex = xml.parseInt(); 
				}
				else if (tag == "mtctype")
					mtcType = xml.parseInt();
				else if (tag == "sendClockDelay")
					syncSendFirstClockDelay = xml.parseUInt();
				else if (tag == "extSync")
					extSyncFlag.setValue(xml.parseInt());
				else if (tag == "useJackTransport")
				{
					useJackTransport.setValue(xml.parseInt());
				}
				else if (tag == "jackTransportMaster")
				{
					jackTransportMaster = xml.parseInt();
					if (audioDevice)
						audioDevice->setMaster(jackTransportMaster);
				}
				else if (tag == "mtcoffset")
				{
					QString qs(xml.parse1());
					QByteArray ba = qs.toLatin1();
					const char* str = ba.constData();
					int h, m, s, f, sf;
					sscanf(str, "%d:%d:%d:%d:%d", &h, &m, &s, &f, &sf);
					mtcOffset = MTC(h, m, s, f, sf);
				}
				else if (tag == "shortcuts")
					readShortCuts(xml);
				else if (tag == "division")
					config.division = xml.parseInt();
				else if (tag == "guiDivision")
					config.guiDivision = xml.parseInt();
				else if (tag == "rtcTicks")
					config.rtcTicks = xml.parseInt();
				else if (tag == "minMeter")
					config.minMeter = xml.parseInt();
				else if (tag == "minSlider")
					config.minSlider = xml.parseDouble();
				else if (tag == "freewheelMode")
					config.freewheelMode = xml.parseInt();
				else if (tag == "denormalProtection")
					config.useDenormalBias = xml.parseInt();
				else if (tag == "didYouKnow")
					config.showDidYouKnow = xml.parseInt();
				else if (tag == "outputLimiter")
					config.useOutputLimiter = xml.parseInt();
				else if (tag == "vstInPlace")
					config.vstInPlace = xml.parseInt();
				else if (tag == "dummyAudioSampleRate")
					config.dummyAudioSampleRate = xml.parseInt();
				else if (tag == "dummyAudioBufSize")
					config.dummyAudioBufSize = xml.parseInt();
				else if (tag == "guiRefresh")
					config.guiRefresh = xml.parseInt();
				else if (tag == "userInstrumentsDir")
					config.userInstrumentsDir = xml.parse1();
				else if (tag == "midiTransform")
					readMidiTransform(xml);
				else if (tag == "midiInputTransform")
					readMidiInputTransform(xml);
				else if (tag == "startMode")
					config.startMode = xml.parseInt();
				else if (tag == "startSong")
					config.startSong = xml.parse1();
				else if (tag == "projectBaseFolder")
					config.projectBaseFolder = xml.parse1();
				else if (tag == "projectStoreInFolder")
					config.projectStoreInFolder = xml.parseInt();
				else if (tag == "useProjectSaveDialog")
					config.useProjectSaveDialog = xml.parseInt();
				else if (tag == "useAutoCrossFades")
					config.useAutoCrossFades = xml.parseInt();
				else if(tag == "lsClientHost")
				{
					config.lsClientHost = xml.parse1();
				}
				else if(tag == "lsClientPort")
				{
					config.lsClientPort = xml.parseInt();;
				}
				else if(tag == "lsClientTimeout")
				{
					config.lsClientTimeout = xml.parseInt();
				}
				else if(tag == "lsClientRetry")
				{
					config.lsClientRetry = xml.parseInt();
				}
				else if(tag == "lsClientBankAsNumber")
				{
					config.lsClientBankAsNumber = xml.parseInt();
				}
				else if(tag == "lsClientAutoStart")
				{
					config.lsClientAutoStart = xml.parseInt();
				}
				else if(tag == "lsClientResetOnStart")
				{
					config.lsClientResetOnStart = xml.parseInt();
				}
				else if(tag == "lsClientResetOnSongStart")
				{
					config.lsClientResetOnSongStart = xml.parseInt();
				}
				else if(tag == "lsClientStartLS")
				{
					config.lsClientStartLS = xml.parseInt();
				}
				else if(tag == "lsClientLSPath")
				{
					config.lsClientLSPath = xml.parse1();
				}
				else if(tag == "globalInputList")
				{
					readGlobalInputList(xml);
				}
				else if(tag == "loadLV2")
				{
					config.loadLV2 = xml.parseInt();
				}
				else if(tag == "loadLADSPA")
				{
					config.loadLADSPA = xml.parseInt();
				}
				else if(tag == "loadVST")
				{
					config.loadVST = xml.parseInt();
				}
				else if(tag == "vstPaths")
				{
					config.vstPaths = xml.parse1();
				}
				else if(tag == "ladspaPaths")
				{
					config.ladspaPaths = xml.parse1();
				}
				else
					xml.skip(tag);
				break;
			case Xml::Text:
				printf("text <%s>\n", xml.s1().toLatin1().constData());
				break;
			case Xml::Attribut:
				if (readOnlySequencer)
					break;
				if (tag == "version")
				{
					int major = xml.s2().section('.', 0, 0).toInt();
					int minor = xml.s2().section('.', 1, 1).toInt();
					xml.setVersion(major, minor);
				}
				break;
			case Xml::TagEnd:
				if (tag == "configuration")
				{
					return;
				}
				break;
			case Xml::Proc:
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

bool readConfiguration()/*{{{*/
{
	FILE* f = fopen(configName.toLatin1().constData(), "r");
	if (f == 0)
	{
		if (debugMsg || debugMode)
			fprintf(stderr, "NO Config File <%s> found\n", configName.toLatin1().constData());

		if (config.userInstrumentsDir.isEmpty())
			config.userInstrumentsDir = configPath + "/instruments";
		return true;
	}
	Xml xml(f);
	bool skipmode = true;
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				fclose(f);
				return true;
			case Xml::TagStart:
				if (skipmode && (tag == "oom" || tag == "muse"))
					skipmode = false;
				else if (skipmode)
					break;
				else if (tag == "configuration")
					readConfiguration(xml, false);
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				if (tag == "version")
				{
					int major = xml.s2().section('.', 0, 0).toInt();
					int minor = xml.s2().section('.', 1, 1).toInt();
					xml.setVersion(major, minor);
				}
				break;
			case Xml::TagEnd:
				if (!skipmode && (tag == "oom" || tag == "muse"))
				{
					fclose(f);
					return false;
				}
			default:
				break;
		}
	}
	fclose(f);
	return true;
}/*}}}*/

//---------------------------------------------------------
//   writeSeqConfiguration
//---------------------------------------------------------

static void writeSeqConfiguration(int level, Xml& xml, bool writePortInfo)/*{{{*/
{
	xml.tag(level++, "sequencer");

	xml.tag(level++, "metronom");
	xml.intTag(level, "premeasures", preMeasures);
	xml.intTag(level, "measurepitch", measureClickNote);
	xml.intTag(level, "measurevelo", measureClickVelo);
	xml.intTag(level, "beatpitch", beatClickNote);
	xml.intTag(level, "beatvelo", beatClickVelo);
	xml.intTag(level, "channel", clickChan);
	xml.intTag(level, "port", clickPort);

	xml.intTag(level, "precountEnable", precountEnableFlag);
	xml.intTag(level, "fromMastertrack", precountFromMastertrackFlag);
	xml.intTag(level, "signatureZ", precountSigZ);
	xml.intTag(level, "signatureN", precountSigN);
	xml.intTag(level, "prerecord", precountPrerecord);
	xml.intTag(level, "preroll", precountPreroll);
	xml.intTag(level, "midiClickEnable", midiClickFlag);
	xml.intTag(level, "audioClickEnable", audioClickFlag);
	xml.floatTag(level, "audioClickVolume", audioClickVolume);
    xml.tag(--level, "/metronom");

	if (writePortInfo)
	{
		//
		// write information about all midi ports, their assigned
		// instruments and all managed midi controllers
		//
		for (int i = 0; i < MIDI_PORTS; ++i)
		{
            bool used = false;
			MidiPort* mport = &midiPorts[i];

			// Route check by Tim. Port can now be used for routing even if no device.
			// Also, check for other non-defaults and save port, to preserve settings even if no device.
			// Dont write the config for the global inputs list they will be auto created with each startup
            if (mport->defaultInChannels() || mport->defaultOutChannels() ||
                    (mport->instrument() && !mport->instrument()->iname().isEmpty() && mport->instrument()->iname() != "GM") || !mport->syncInfo().isDefault() )
            {
				used = true;
            }
			else
			{//Put the ID of this track into a list 
				MidiTrackList* tl = song->midis();
				for (iMidiTrack it = tl->begin(); it != tl->end(); ++it)
				{
					MidiTrack* t = *it;
					if (t->outPort() == i)
					{
						used = true;
						break;
					}
				}
			}

			MidiDevice* dev = mport->device();
			if (!used && !dev)
				continue;
			bool isGlobal = gInputListPorts.contains(mport->portno());
			xml.tag(level++, "midiport portId=\"%lld\" isGlobalInput=\"%d\"", mport->id(), isGlobal);

			if (mport->defaultInChannels())
				xml.intTag(level, "defaultInChans", mport->defaultInChannels());
			if (mport->defaultOutChannels())
				xml.intTag(level, "defaultOutChans", mport->defaultOutChannels());

			if (mport->instrument() && !mport->instrument()->iname().isEmpty() &&
					(mport->instrument()->iname() != "GM")) // FIXME: TODO: Make this user configurable.
            {
				xml.strTag(level, "instrument", mport->instrument()->iname());
            }

			if (dev)
			{
				xml.strTag(level, "name", dev->name());
				xml.intTag(level, "cacheNRPN", (int)dev->cacheNRPN());

				if (dev->deviceType() != MidiDevice::ALSA_MIDI)
					xml.intTag(level, "type", dev->deviceType());

				xml.intTag(level, "openFlags", dev->openFlags());

                // save state of synth plugin
                if (dev->isSynthPlugin())
                {
                    SynthPluginDevice* synth = (SynthPluginDevice*)dev;
                    xml.tag(level++, "plugin");
                    synth->write(level++, xml);
                    level -= 2; // adjust indentation
                    xml.tag(level, "/plugin");
                }
			}
			mport->syncInfo().write(level, xml);
			// write out registered controller for all channels
			MidiCtrlValListList* vll = mport->controller();
			for (int k = 0; k < MIDI_CHANNELS; ++k)
			{
				int min = k << 24;
				int max = min + 0x100000;
				xml.tag(level++, "channel idx=\"%d\"", k);
				iMidiCtrlValList s = vll->lower_bound(min);
				iMidiCtrlValList e = vll->lower_bound(max);
				if (s != e)
				{
					for (iMidiCtrlValList i = s; i != e; ++i)
					{
						if(i->second->num() != 262145)
						{
							xml.tag(level++, "controller id=\"%d\"", i->second->num());
							if (i->second->hwVal() != CTRL_VAL_UNKNOWN)
                                xml.intTag(level, "val", i->second->hwVal());
                            xml.etag(--level, "controller");
						}
					}
				}
                xml.etag(--level, "channel");
			}
			QList<PatchSequence*> *patchSequences = mport->patchSequences();
			if (patchSequences && !patchSequences->isEmpty())
			{
				for (int p = 0; p < patchSequences->size(); ++p)
				{
					PatchSequence* ps = patchSequences->at(p);
					QString pm = ps->name.replace('\n', " ");
					xml.put(level, "<patchSequence id=\"%d\" name=\"%s\" checked=\"%d\" />", ps->id, pm.toStdString().c_str(), ps->selected);
				}
			}
			if(!mport->presets()->isEmpty())
			{
				QHashIterator<int, QString> iter(*mport->presets());
				while(iter.hasNext())
				{
					iter.next();
					xml.put(level, "<midiPreset id=\"%d\" sysex=\"%s\"/>", iter.key(), iter.value().toLatin1().constData());
				}
			}
            xml.etag(--level, "midiport");
		}
	}
    xml.tag(--level, "/sequencer");
}/*}}}*/

//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void OOMidi::writeGlobalConfiguration() const
{
	FILE* f = fopen(configName.toLatin1().constData(), "w");
	if (f == 0)
	{
		printf("save configuration to <%s> failed: %s\n",
				configName.toLatin1().constData(), strerror(errno));
		return;
	}
	Xml xml(f);
	xml.header();
	xml.tag(0, "oom version=\"2.0\"");
	writeGlobalConfiguration(1, xml);
	xml.tag(0, "/oom");
	fclose(f);
}

void OOMidi::writeGlobalConfiguration(int level, Xml& xml) const
{
	xml.tag(level++, "configuration");

	xml.intTag(level, "division", config.division);
	xml.intTag(level, "rtcTicks", config.rtcTicks);
	xml.intTag(level, "minMeter", config.minMeter);
	xml.doubleTag(level, "minSlider", config.minSlider);
	xml.intTag(level, "freewheelMode", config.freewheelMode);
	xml.intTag(level, "denormalProtection", config.useDenormalBias);
	xml.intTag(level, "didYouKnow", config.showDidYouKnow);
	xml.intTag(level, "outputLimiter", config.useOutputLimiter);
	xml.intTag(level, "vstInPlace", config.vstInPlace);
	xml.intTag(level, "dummyAudioBufSize", config.dummyAudioBufSize);
	xml.intTag(level, "dummyAudioSampleRate", config.dummyAudioSampleRate);

	xml.intTag(level, "guiRefresh", config.guiRefresh);
	xml.strTag(level, "userInstrumentsDir", config.userInstrumentsDir);
	xml.intTag(level, "extendedMidi", config.extendedMidi);
	xml.intTag(level, "midiExportDivision", config.midiDivision);
	xml.intTag(level, "smfFormat", config.smfFormat);
	xml.intTag(level, "exp2ByteTimeSigs", config.exp2ByteTimeSigs);
	xml.intTag(level, "expOptimNoteOffs", config.expOptimNoteOffs);
	xml.intTag(level, "importMidiSplitParts", config.importMidiSplitParts);
	xml.intTag(level, "startMode", config.startMode);
	xml.strTag(level, "startSong", config.startSong);
	xml.strTag(level, "projectBaseFolder", config.projectBaseFolder);
	xml.intTag(level, "projectStoreInFolder", config.projectStoreInFolder);
	xml.intTag(level, "useProjectSaveDialog", config.useProjectSaveDialog);
	xml.intTag(level, "useAutoCrossFades", config.useAutoCrossFades);
	xml.intTag(level, "midiInputDevice", midiInputPorts);
	xml.intTag(level, "midiInputChannel", midiInputChannel);
	xml.intTag(level, "midiRecordType", midiRecordType);
	xml.intTag(level, "midiThruType", midiThruType);
	xml.intTag(level, "midiFilterCtrl1", midiFilterCtrl1);
	xml.intTag(level, "midiFilterCtrl2", midiFilterCtrl2);
	xml.intTag(level, "midiFilterCtrl3", midiFilterCtrl3);
	xml.intTag(level, "midiFilterCtrl4", midiFilterCtrl4);

	xml.strTag(level, "externalWavEditor", config.externalWavEditor);
	xml.intTag(level, "useOldStyleStopShortCut", config.useOldStyleStopShortCut);
	xml.intTag(level, "moveArmedCheckBox", config.moveArmedCheckBox);
	
	xml.strTag(level, "lsClientHost", config.lsClientHost);
	xml.intTag(level, "lsClientPort", config.lsClientPort);
	xml.intTag(level, "lsClientTimeout", config.lsClientTimeout);
	xml.intTag(level, "lsClientRetry", config.lsClientRetry);
	xml.intTag(level, "lsClientBankAsNumber", config.lsClientBankAsNumber);
	xml.intTag(level, "lsClientAutoStart", config.lsClientAutoStart);
	xml.intTag(level, "lsClientResetOnStart", config.lsClientResetOnStart);
	xml.intTag(level, "lsClientResetOnSongStart", config.lsClientResetOnSongStart);
	xml.strTag(level, "lsClientLSPath", config.lsClientLSPath);
	xml.intTag(level, "lsClientStartLS", config.lsClientStartLS);
	
	xml.intTag(level, "loadLV2", config.loadLV2);
	xml.intTag(level, "loadLADSPA", config.loadLADSPA);
	xml.intTag(level, "loadVST", config.loadVST);
	xml.strTag(level, "ladspaPaths", config.ladspaPaths);
	xml.strTag(level, "vstPaths", config.vstPaths);

	xml.intTag(level, "vuColorStrip", vuColorStrip);
	if(gInputList.size())
	{
		std::string tag = "globalInputList";
		xml.put(level, "<%s count=\"%d\">", tag.c_str(), gInputList.size());
		level++;
		for(int i = 0; i < gInputList.size(); ++i)
		{
			QPair<int, QString> in = gInputList.at(i);
			xml.put(level, "<globalInput deviceType=\"%d\" deviceName=\"%s\" />", in.first, in.second.toUtf8().constData());
		}
		level--;
		xml.put(level--, "</%s>", tag.c_str());
		level++;
	}

	xml.intTag(level, "mtctype", mtcType);
	xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
			mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
			mtcOffset.f(), mtcOffset.sf());
	extSyncFlag.save(level, xml);

	writeSeqConfiguration(level, xml, false);

	writeShortCuts(level, xml);
    xml.etag(--level, "configuration");
	writeInstrumentTemplates(level, xml);
}

void OOMidi::writeInstrumentTemplates(int level, Xml& xml) const 
{
	xml.tag(level++, "instrumentTemplateList");
	trackManager->write(level, xml);
	foreach(TrackView* tv, m_instrumentTemplates)
	{
		tv->write(level, xml);
	}
	level--;
	xml.etag(level--, "instrumentTemplateList");
}


void OOMidi::readInstrumentTemplates()
{
	FILE* f = fopen(configName.toLatin1().constData(), "r");
	if (f == 0)
	{
		return;
	}
	bool skipmode = true;
	Xml xml(f);
	for(;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				fclose(f);
				return;
			case Xml::TagStart:
				if (skipmode && tag == "oom")
					skipmode = false;
				else if (skipmode)
					break;
				else if (tag == "instrumentTemplateList")
					readInstrumentTemplates(xml);
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				if (tag == "version")
				{
					int major = xml.s2().section('.', 0, 0).toInt();
					int minor = xml.s2().section('.', 1, 1).toInt();
					xml.setVersion(major, minor);
				}
				break;
			case Xml::TagEnd:
				if(tag == "oom")
				{
					fclose(f);
					return;
				}
			default:
				break;
		}
	}
	fclose(f);
}

void OOMidi::readInstrumentTemplates(Xml& xml)
{
	for (;;)/*{{{*/
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "instrumentTemplate")
				{
					TrackView* tv = new TrackView();
					tv->read(xml);
					insertInstrumentTemplate(tv, -1);
				}
				else if(tag == "trackManager")
				{
					trackManager->read(xml);
				}
				else
					xml.skip(tag);
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if (tag == "instrumentTemplateList")
					return;
			default:
				break;
		}
	}/*}}}*/
}

//---------------------------------------------------------
//   writeConfiguration
//    write song specific configuration
//---------------------------------------------------------

void OOMidi::writeConfiguration(int level, Xml& xml) const
{
	xml.tag(level++, "configuration");
	
	xml.intTag(level, "nextTrackPartColor", lastTrackPartColorIndex);

	xml.intTag(level, "midiInputDevice", midiInputPorts);
	xml.intTag(level, "midiInputChannel", midiInputChannel);
	xml.intTag(level, "midiRecordType", midiRecordType);
	xml.intTag(level, "midiThruType", midiThruType);
	xml.intTag(level, "midiFilterCtrl1", midiFilterCtrl1);
	xml.intTag(level, "midiFilterCtrl2", midiFilterCtrl2);
	xml.intTag(level, "midiFilterCtrl3", midiFilterCtrl3);
	xml.intTag(level, "midiFilterCtrl4", midiFilterCtrl4);

	xml.intTag(level, "mtctype", mtcType);
	xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
			mtcOffset.h(), mtcOffset.m(), mtcOffset.s(),
			mtcOffset.f(), mtcOffset.sf());
	xml.uintTag(level, "sendClockDelay", syncSendFirstClockDelay);
	xml.intTag(level, "useJackTransport", useJackTransport.value());
	xml.intTag(level, "jackTransportMaster", jackTransportMaster);
	extSyncFlag.save(level, xml);

	composer->writeStatus(level, xml);
	writeSeqConfiguration(level, xml, true);

	writeMidiTransforms(level, xml);
	writeMidiInputTransforms(level, xml);
    xml.etag(--level, "configuration");
}

//---------------------------------------------------------
//   configMidiFile
//---------------------------------------------------------

void OOMidi::configMidiFile()
{
	if (!midiFileConfig)
		midiFileConfig = new MidiFileConfig();
	midiFileConfig->updateValues();

	if (midiFileConfig->isVisible())
	{
		midiFileConfig->raise();
		midiFileConfig->activateWindow();
	}
	else
		midiFileConfig->show();
}

//---------------------------------------------------------
//   MidiFileConfig
//    config properties of exported midi files
//---------------------------------------------------------

MidiFileConfig::MidiFileConfig(QWidget* parent)
: QDialog(parent), ConfigMidiFileBase()
{
	setupUi(this);
	connect(buttonOk, SIGNAL(clicked()), SLOT(okClicked()));
	connect(buttonCancel, SIGNAL(clicked()), SLOT(cancelClicked()));
}

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void MidiFileConfig::updateValues()
{
	int divisionIdx = 2;
	switch (config.midiDivision)
	{
		case 96: divisionIdx = 0;
			break;
		case 192: divisionIdx = 1;
			break;
		case 384: divisionIdx = 2;
			break;
	}
	divisionCombo->setCurrentIndex(divisionIdx);
	formatCombo->setCurrentIndex(config.smfFormat);
	extendedFormat->setChecked(config.extendedMidi);
	copyrightEdit->setText(config.copyright);
	optNoteOffs->setChecked(config.expOptimNoteOffs);
	twoByteTimeSigs->setChecked(config.exp2ByteTimeSigs);
	splitPartsCheckBox->setChecked(config.importMidiSplitParts);
}

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
{
	int divisionIdx = divisionCombo->currentIndex();

	int divisions[3] = {96, 192, 384};
	if (divisionIdx >= 0 && divisionIdx < 3)
		config.midiDivision = divisions[divisionIdx];
	config.extendedMidi = extendedFormat->isChecked();
	config.smfFormat = formatCombo->currentIndex();
	config.copyright = copyrightEdit->text();
	config.expOptimNoteOffs = optNoteOffs->isChecked();
	config.exp2ByteTimeSigs = twoByteTimeSigs->isChecked();
	config.importMidiSplitParts = splitPartsCheckBox->isChecked();

	oom->changeConfig(true); // write config file
	close();
}

//---------------------------------------------------------
//   cancelClicked
//---------------------------------------------------------

void MidiFileConfig::cancelClicked()
{
	close();
}

//---------------------------------------------------------
//   configGlobalSettings
//---------------------------------------------------------

void OOMidi::configGlobalSettings(int tab)
{
	if (!globalSettingsConfig)
		globalSettingsConfig = new GlobalSettingsConfig(this);
	else
		globalSettingsConfig->populateInputs(); //Make sure the input list is refreshed on show
	globalSettingsConfig->setCurrentTab(tab); //Set tab to current
	
	if (globalSettingsConfig->isVisible())
	{
		globalSettingsConfig->raise();
		globalSettingsConfig->activateWindow();
	}
	else
		globalSettingsConfig->show();
}


//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MixerConfig::write(int level, Xml& xml)
{
	xml.tag(level++, "Mixer");

	xml.strTag(level, "name", name);

	xml.qrectTag(level, "geometry", geometry);

	xml.intTag(level, "showMidiTracks", showMidiTracks);
	xml.intTag(level, "showDrumTracks", showDrumTracks);
	xml.intTag(level, "showInputTracks", showInputTracks);
	xml.intTag(level, "showOutputTracks", showOutputTracks);
	xml.intTag(level, "showWaveTracks", showWaveTracks);
	xml.intTag(level, "showGroupTracks", showGroupTracks);
	xml.intTag(level, "showAuxTracks", showAuxTracks);
	xml.intTag(level, "showSyntiTracks", showSyntiTracks);

	xml.etag(level, "Mixer");
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MixerConfig::read(Xml& xml)
{
	for (;;)
	{
		Xml::Token token(xml.parse());
		const QString & tag(xml.s1());
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "name")
					name = xml.parse1();
				else if (tag == "geometry")
					geometry = readGeometry(xml, tag);
				else if (tag == "showMidiTracks")
					showMidiTracks = xml.parseInt();
				else if (tag == "showInputTracks")
					showInputTracks = xml.parseInt();
				else if (tag == "showOutputTracks")
					showOutputTracks = xml.parseInt();
				else if (tag == "showWaveTracks")
					showWaveTracks = xml.parseInt();
				else if (tag == "showGroupTracks")
					showGroupTracks = xml.parseInt();
				else if (tag == "showAuxTracks")
					showAuxTracks = xml.parseInt();
				else
					xml.skip(tag);
				break;
			case Xml::TagEnd:
				if (tag == "Mixer")
					return;
			default:
				break;
		}
	}

}

