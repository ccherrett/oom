//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include <fastlog.h>
#include <math.h>

#include "gconfig.h"
#include "globals.h"
#include "config.h"
#include "track.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "knob.h"
#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "scrollscale.h"
#include "xml.h"
#include "midi.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
#include "midictrl.h"
#include "comment.h"
#include "header.h"
#include "node.h"
#include "instruments/minstrument.h"
#include "arranger.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "synth.h"
#include "menulist.h"
#include "midimonitor.h"
#include "pcanvas.h"
#include "trackheader.h"
#include "slider.h"
#include "mixer/meter.h"

static QString styletemplate = "QLineEdit { border-width:1px; border-radius: 0px; border-image: url(:/images/frame.png) 4; border-top-color: #1f1f22; border-bottom-color: #505050; background-color: #1a1a1a; color: #%1; font-family: fixed-width; font-weight: bold; font-size: 15px; padding-left: 15px; }";

TrackHeader::TrackHeader(Track* t, QWidget* parent)
: QFrame(parent)
{
	setupUi(this);
	m_track = t;
	m_tracktype = 0;
	m_channels = 2;
	setupStyles();
	resizeFlag = false;
	mode = NORMAL;
	inHeartBeat = true;
	m_editing = false;
	m_midiDetect = false;
	m_processEvents = true;
	m_meterVisible = true;
	m_sliderVisible = true;
	panVal = 0.0;
	volume = 0.0;
	setFrameStyle(QFrame::StyledPanel|QFrame::Raised);
	m_buttonHBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	m_panBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	initPan();
	initVolume();
	m_trackName->installEventFilter(this);

	setMouseTracking(true);
	if(m_track)/*{{{*/
	{
		Track::TrackType type = m_track->type();
		m_tracktype = (int)type;
		switch (type)
		{
			case Track::MIDI:
			case Track::DRUM:
				setObjectName("MidiTrackHeader");
			break;
			case Track::WAVE:
				setObjectName("WaveHeader");
			break;
			case Track::AUDIO_OUTPUT:
				setObjectName("AudioOutHeader");
			break;
			case Track::AUDIO_INPUT:
				setObjectName("AudioInHeader");
			break;
			case Track::AUDIO_BUSS:
				setObjectName("AudioBussHeader");
			break;
			case Track::AUDIO_AUX:
				setObjectName("AuxHeader");
			break;
			case Track::AUDIO_SOFTSYNTH:
				setObjectName("SynthTrackHeader");
			break;
		}
	}/*}}}*/
	setAcceptDrops(false);
	m_trackName->setAcceptDrops(false);
	m_btnSolo->setAcceptDrops(false);
	m_btnSolo->setIcon(*solo_trackIconSet3);
	m_btnRecord->setAcceptDrops(false);
	m_btnRecord->setIcon(*record_trackIconSet3);
	m_btnMute->setAcceptDrops(false);
	m_btnMute->setIcon(*mute_trackIconSet3);
	m_btnAutomation->setAcceptDrops(false);
	m_btnReminder1->setAcceptDrops(false);
	m_btnReminder2->setAcceptDrops(false);
	m_btnReminder3->setAcceptDrops(false);
	m_btnReminder1->setIcon(*reminder1IconSet3);
	m_btnReminder2->setIcon(*reminder2IconSet3);
	m_btnReminder3->setIcon(*reminder3IconSet3);
	m_pan->setAcceptDrops(false);
	if(m_track)
	{
		setSelected(m_track->selected());
		if(m_track->height() < MIN_TRACKHEIGHT)
		{
			setFixedHeight(MIN_TRACKHEIGHT);
			m_track->setHeight(MIN_TRACKHEIGHT);
		}
		else
		{
			setFixedHeight(m_track->height());
		}
		if(m_track->isMidiTrack())
			m_btnAutomation->setIcon(QIcon(*input_indicator_OffIcon));
		else
			m_btnAutomation->setIcon(*automation_trackIconSet3);
	}
	songChanged(-1);
	connect(m_trackName, SIGNAL(editingFinished()), this, SLOT(updateTrackName()));
	connect(m_trackName, SIGNAL(returnPressed()), this, SLOT(updateTrackName()));
	connect(m_trackName, SIGNAL(textEdited(QString)), this, SLOT(setEditing()));
	connect(m_btnRecord, SIGNAL(toggled(bool)), this, SLOT(toggleRecord(bool)));
	connect(m_btnMute, SIGNAL(toggled(bool)), this, SLOT(toggleMute(bool)));
	connect(m_btnSolo, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
	connect(m_btnReminder1, SIGNAL(toggled(bool)), this, SLOT(toggleReminder1(bool)));
	connect(m_btnReminder2, SIGNAL(toggled(bool)), this, SLOT(toggleReminder2(bool)));
	connect(m_btnReminder3, SIGNAL(toggled(bool)), this, SLOT(toggleReminder3(bool)));
	connect(m_btnAutomation, SIGNAL(clicked()), this, SLOT(generateAutomationMenu()));
	//Let header list control this for now
	//connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
	inHeartBeat = false;
}

TrackHeader::~TrackHeader()
{
	m_processEvents = false;
}

//Public member functions

void TrackHeader::stopProcessing()/*{{{*/
{
	m_processEvents = false;
}/*}}}*/

void TrackHeader::startProcessing()/*{{{*/
{
	m_processEvents = true;
}/*}}}*/

bool TrackHeader::isSelected()/*{{{*/
{
	if(!m_track)
		return false;
	return m_track->selected();
}/*}}}*/

//Public slots

void TrackHeader::setSelected(bool sel)/*{{{*/
{
	if(!m_track)
	{
		m_selected = false;
	}
	else
	{
		m_selected = sel;
		m_track->setSelected(sel);
	}
	if(!m_editing && m_processEvents)
	{
		if(m_selected)
		{
			//m_trackName->setFont();
			m_trackName->setStyleSheet(m_selectedStyle[m_tracktype]);
		}
		else
		{
			m_trackName->setStyleSheet(m_style[m_tracktype]);
			//m_strip->setStyleSheet("QFrame {background-color: blue;}");
		}
	}
}/*}}}*/

void TrackHeader::songChanged(int flags)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	
	if(!m_track->isMidiTrack())
	{
		if (flags & SC_CHANNELS)
			updateChannels();

		if (flags & SC_CONFIG)
		{
			m_slider->setRange(config.minSlider - 0.1, 10.0);

			for (int c = 0; c < ((AudioTrack*)m_track)->channels(); ++c)
				meter[c]->setRange(config.minMeter, 10.0);
		}
	}

	if (flags == -1 || (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_MIDI_TRACK_PROP | SC_SELECTION | SC_TRACK_MODIFIED)))
	{
		//printf("TrackHeader::songChanged\n");
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(m_track->recordFlag());
		m_btnRecord->blockSignals(false);

		m_btnMute->blockSignals(true);
		m_btnMute->setChecked(m_track->mute());
		m_btnMute->blockSignals(false);

		m_btnSolo->blockSignals(true);
		m_btnSolo->setChecked(m_track->solo());
		m_btnSolo->blockSignals(false);

		m_btnReminder1->blockSignals(true);
		m_btnReminder1->setChecked(m_track->getReminder1());
		m_btnReminder1->blockSignals(false);
		
		m_btnReminder2->blockSignals(true);
		m_btnReminder2->setChecked(m_track->getReminder2());
		m_btnReminder2->blockSignals(false);

		m_btnReminder3->blockSignals(true);
		m_btnReminder3->setChecked(m_track->getReminder3());
		m_btnReminder3->blockSignals(false);

		m_trackName->blockSignals(true);
		m_trackName->setText(m_track->name());
		m_trackName->blockSignals(false);
		m_trackName->setReadOnly((m_track->name() == "Master"));
		setSelected(m_track->selected());
		//setProperty("selected", m_track->selected());
		if(m_track->height() < MIN_TRACKHEIGHT)
		{
			setFixedHeight(MIN_TRACKHEIGHT);
			m_track->setHeight(MIN_TRACKHEIGHT);
		}
		else
		{
			setFixedHeight(m_track->height());
		}
		//updateBackground();
	}
}/*}}}*/

//Private slots
void TrackHeader::heartBeat()/*{{{*/
{
	if(!m_track || inHeartBeat || !m_processEvents)
		return;
	if(song->invalid)
		return;
	inHeartBeat = true;
	if(m_track->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)m_track;
		
		int act = track->activity();
		double dact = double(act) * (m_slider->value() / 127.0);

		if ((int) dact > track->lastActivity())
			track->setLastActivity((int) dact);

		if (meter[0] && m_meterVisible)
			meter[0]->setVal(dact, track->lastActivity(), false);

		// Gives reasonable decay with gui update set to 20/sec.
		if (act)
			track->setActivity((int) ((double) act * 0.8));
		
		//int outChannel = track->outChannel();
		//int outPort = track->outPort();

		//MidiPort* mp = &midiPorts[outPort];

		// Check for detection of midi general activity on chosen channels...
		int mpt = 0;
		RouteList* rl = track->inRoutes();

		ciRoute r = rl->begin();
		for (; r != rl->end(); ++r)
		{
			if (!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE))
				continue;

			// NOTE: TODO: Code for channelless events like sysex,
			// ** IF we end up using the 'special channel 17' method.
			if (r->channel == -1 || r->channel == 0) 
				continue;

			// No port assigned to the device?
			mpt = r->midiPort;
			if (mpt < 0 || mpt >= MIDI_PORTS)
				continue;

			if (midiPorts[mpt].syncInfo().actDetectBits() & r->channel)
			{
				if (!m_midiDetect)
				{
					m_midiDetect = true;
					m_btnAutomation->setIcon(QIcon(*input_indicator_OnIcon));
				}
				break;
			}
		}
		// No activity detected?
		if (r == rl->end())
		{
			if (m_midiDetect)
			{
				m_midiDetect = false;
				m_btnAutomation->setIcon(QIcon(*input_indicator_OffIcon));
			}
		}
	}
	else
	{
		if(m_meterVisible)
		{
			for (int ch = 0; ch < ((AudioTrack*)m_track)->channels(); ++ch)
			{
				if (meter[ch])
				{
					meter[ch]->setVal(((AudioTrack*)m_track)->meter(ch), ((AudioTrack*)m_track)->peak(ch), false);
				}
			}
		}
	}

	updateVolume();
	updatePan();
	inHeartBeat = false;
}/*}}}*/

void TrackHeader::generatePopupMenu()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	TrackList selectedTracksList = song->getSelectedTracks();
	bool multipleSelectedTracks = false;
	if (selectedTracksList.size() > 1)
	{
		multipleSelectedTracks = true;
	}

	QMenu* p = new QMenu;
	//Part Color menu
	QMenu* colorPopup = p->addMenu(tr("Default Part Color"));/*{{{*/

	QMenu* colorSub;
	for (int i = 0; i < NUM_PARTCOLORS; ++i)
	{
		QString colorname(config.partColorNames[i]);
		if(colorname.contains("menu:", Qt::CaseSensitive))
		{
			colorSub = colorPopup->addMenu(colorname.replace("menu:", ""));
		}
		else
		{
			if(m_track->getDefaultPartColor() == i)
			{
				colorname = QString(config.partColorNames[i]);
				colorPopup->setIcon(PartCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true));
				colorPopup->setTitle(colorSub->title()+": "+colorname);
	
				colorname = QString("* "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(PartCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true), colorname);
				act_color->setData(20 + i);
			}
			else
			{
				colorname = QString("     "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(PartCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80), colorname);
				act_color->setData(20 + i);
			}
		}
	}/*}}}*/
	if(m_track && m_track->type() == Track::WAVE)
		p->addAction(tr("Import Audio File"))->setData(1);
	/*if (!multipleSelectedTracks)
	{
		p->addAction(QIcon(*midi_edit_instrumentIcon), tr("Rename Track"))->setData(15);
	}*/

	//Add Track menu
	QMenu* trackMenu = p->addMenu(tr("Add Track"));
	QAction* midi = trackMenu->addAction(*addtrack_addmiditrackIcon, tr("Midi Track"));
	midi->setData(Track::MIDI+10000);
	QAction* wave = trackMenu->addAction(*addtrack_wavetrackIcon, tr("Audio Track"));
	wave->setData(Track::WAVE+10000);
	QAction* aoutput = trackMenu->addAction(*addtrack_audiooutputIcon, tr("Output"));
	aoutput->setData(Track::AUDIO_OUTPUT+10000);
	QAction* ainput = trackMenu->addAction(*addtrack_audioinputIcon, tr("Input"));
	ainput->setData(Track::AUDIO_INPUT+10000);
	QAction* agroup = trackMenu->addAction(*addtrack_audiogroupIcon, tr("Buss"));
	agroup->setData(Track::AUDIO_BUSS+10000);
	QAction* aaux = trackMenu->addAction(*addtrack_auxsendIcon, tr("Aux Send"));
	aaux->setData(Track::AUDIO_AUX+10000);


	p->addAction(QIcon(*automation_clear_dataIcon), tr("Delete Track"))->setData(0);
	

	QMenu* trackHeightsMenu = p->addMenu("Track Height");
	trackHeightsMenu->addAction("Default")->setData(6);
	trackHeightsMenu->addAction("2")->setData(7);
	trackHeightsMenu->addAction("3")->setData(8);
	trackHeightsMenu->addAction("4")->setData(9);
	trackHeightsMenu->addAction("5")->setData(10);
	trackHeightsMenu->addAction("6")->setData(11);
	trackHeightsMenu->addAction("Full Screen")->setData(12);
	if (selectedTracksList.size() > 1)
	{
		trackHeightsMenu->addAction("Fit Selection in View")->setData(13);
	}

	if (m_track->type() == Track::AUDIO_SOFTSYNTH && !multipleSelectedTracks)
	{
		SynthI* synth = (SynthI*) m_track;

		QAction* sga = p->addAction(tr("Show Gui"));
		sga->setData(2);
		sga->setCheckable(true);
		//printf("synth hasgui %d, gui visible %d\n",synth->hasGui(), synth->guiVisible());
		sga->setEnabled(synth->hasGui());
		sga->setChecked(synth->guiVisible());

		// If it has a gui but we don't have OSC, disable the action.
#ifndef OSC_SUPPORT
#ifdef DSSI_SUPPORT
		if (dynamic_cast<DssiSynthIF*> (synth->sif()))
		{
			sga->setChecked(false);
			sga->setEnabled(false);
		}
#endif
#endif
	}
	else if(m_track->isMidiTrack() && !multipleSelectedTracks)
	{
		int oPort = ((MidiTrack*) m_track)->outPort();
		MidiPort* port = &midiPorts[oPort];

		QAction* mact = p->addAction(tr("Show Gui"));
		mact->setCheckable(true);
		mact->setEnabled(port->hasGui());
		mact->setChecked(port->guiVisible());
		mact->setData(3);

		// If it has a gui but we don't have OSC, disable the action.
#ifndef OSC_SUPPORT
#ifdef DSSI_SUPPORT
		MidiDevice* dev = port->device();
		if (dev && dev->isSynti() && (dynamic_cast<DssiSynthIF*> (((SynthI*) dev)->sif())))
		{
			mact->setChecked(false);
			mact->setEnabled(false);
		}
#endif
#endif
		//p->addAction(QIcon(*addtrack_addmiditrackIcon), tr("Midi"))->setData(4);
		//p->addAction(QIcon(*addtrack_drumtrackIcon), tr("Drum"))->setData(5);
	}

	QAction* act = p->exec(QCursor::pos());
	if (act)
	{
		int n = act->data().toInt();
		switch (n)
		{
			case 0: // delete track
				if (multipleSelectedTracks)
				{
					song->startUndo();
					audio->msgRemoveTracks();
					song->endUndo(SC_TRACK_REMOVED);
					song->updateSoloStates();
				}
				else
				{
					if(m_track->name() == "Master")
						break;
					song->removeTrack0(m_track);
					audio->msgUpdateSoloStates();
				}

			break;

			case 1: // Import audio file
			{
				oom->importWave(m_track);
			}
			break;
			case 2:
			{
				SynthI* synth = (SynthI*) m_track;
				bool show = !synth->guiVisible();
				audio->msgShowInstrumentGui(synth, show);
			}
			break;
			case 3:
			{
				int oPort = ((MidiTrack*) m_track)->outPort();
				MidiPort* port = &midiPorts[oPort];
				bool show = !port->guiVisible();
				audio->msgShowInstrumentGui(port->instrument(), show);
			}
			break;
			case 4:
			{
				if (m_track->type() == Track::DRUM)
				{
					//
					//    Drum -> Midi
					//
					audio->msgIdle(true);
					PartList* pl = m_track->parts();
					MidiTrack* m = (MidiTrack*) m_track;
					for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
					{
						EventList* el = ip->second->events();
						for (iEvent ie = el->begin(); ie != el->end(); ++ie)
						{
							Event ev = ie->second;
							if (ev.type() == Note)
							{
								int pitch = ev.pitch();
								// Changed by T356.
								// Tested: Notes were being mixed up switching back and forth between midi and drum.
								//pitch = drumMap[pitch].anote;
								pitch = drumMap[pitch].enote;

								ev.setPitch(pitch);
							}
							else
								if (ev.type() == Controller)
							{
								int ctl = ev.dataA();
								// Is it a drum controller event, according to the track port's instrument?
								MidiController *mc = midiPorts[m->outPort()].drumController(ctl);
								if (mc)
									// Change the controller event's index into the drum map to an instrument note.
									ev.setA((ctl & ~0xff) | drumMap[ctl & 0x7f].enote);
							}

						}
					}
					m_track->setType(Track::MIDI);
					audio->msgIdle(false);
				}
			}
			break;
			case 5:
			{
				if (m_track->type() == Track::MIDI)
				{
					//
					//    Midi -> Drum
					//
					bool change = QMessageBox::question(this, tr("Update drummap?"),
							tr("Do you want to use same port and channel for all instruments in the drummap?"),
							tr("&Yes"), tr("&No"), QString::null, 0, 1);

					audio->msgIdle(true);
					// Delete all port controller events.
					//audio->msgChangeAllPortDrumCtrlEvents(false);
					song->changeAllPortDrumCtrlEvents(false);

					if (!change)
					{
						MidiTrack* m = (MidiTrack*) m_track;
						for (int i = 0; i < DRUM_MAPSIZE; i++)
						{
							drumMap[i].channel = m->outChannel();
							drumMap[i].port = m->outPort();
						}
					}

					//audio->msgIdle(true);
					PartList* pl = m_track->parts();
					MidiTrack* m = (MidiTrack*) m_track;
					for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
					{
						EventList* el = ip->second->events();
						for (iEvent ie = el->begin(); ie != el->end(); ++ie)
						{
							Event ev = ie->second;
							if (ev.type() == Note)
							{
								int pitch = ev.pitch();
								pitch = drumInmap[pitch];
								ev.setPitch(pitch);
							}
							else
							{
								if (ev.type() == Controller)
								{
									int ctl = ev.dataA();
									// Is it a drum controller event, according to the track port's instrument?
									MidiController *mc = midiPorts[m->outPort()].drumController(ctl);
									if (mc)
										// Change the controller event's instrument note to an index into the drum map.
										ev.setA((ctl & ~0xff) | drumInmap[ctl & 0x7f]);
								}

							}

						}
					}
					m_track->setType(Track::DRUM);

					// Add all port controller events.
					//audio->msgChangeAllPortDrumCtrlEvents(true);
					song->changeAllPortDrumCtrlEvents(true);

					audio->msgIdle(false);
				}
			}
			case 6:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 40);
				}
				else
				{
					m_track->setHeight(40);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 7:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 60);
				}
				else
				{
					m_track->setHeight(60);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 8:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 100);
				}
				else
				{
					m_track->setHeight(100);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 9:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 180);
				}
				else
				{
					m_track->setHeight(180);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 10:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 320);
				}
				else
				{
					m_track->setHeight(320);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 11:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, 640);
				}
				else
				{
					m_track->setHeight(640);
					song->update(SC_TRACK_MODIFIED);
				}
				break;
			}
			case 12:
			{
				int canvasHeight = oom->arranger->getCanvas()->height();

				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, canvasHeight);
					Track* firstSelectedTrack = *selectedTracksList.begin();
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(firstSelectedTrack));

				}
				else
				{
					m_track->setHeight(canvasHeight);
					song->update(SC_TRACK_MODIFIED);
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(m_track));
				}
				break;
			}
			case 13:
			{
				int canvasHeight = oom->arranger->getCanvas()->height();
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, canvasHeight / selectedTracksList.size());
					Track* firstSelectedTrack = *selectedTracksList.begin();
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(firstSelectedTrack));

				}
				else
				{
					m_track->setHeight(canvasHeight);
					song->update(SC_TRACK_MODIFIED);
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(m_track));
				}
				break;
			}
			case 15:
			{
				//if(!multipleSelectedTracks)
				//{
				//	renameTrack(m_track);
				//}
				break;
			}
			case 20 ... NUM_PARTCOLORS + 20:
			{
				int curColorIndex = n - 20;
				m_track->setDefaultPartColor(curColorIndex);
				break;
			}
			case Track::MIDI+10000 ... Track::AUDIO_AUX+10000:
			{
				Track* t = song->addTrack((Track::TrackType)n-10000);

				if (t)
				{
					midiMonitor->msgAddMonitoredTrack(t);
					song->deselectTracks();
					t->setSelected(true);

					emit selectionChanged(t);
					emit trackInserted(n-10000);

					song->updateTrackViews1();
				}
				break;
			}
			default:
				printf("action %d\n", n);
			break;
		}
	}
	delete trackHeightsMenu;
	delete p;
}/*}}}*/

void TrackHeader::generateAutomationMenu()/*{{{*/
{
	if(!m_track || m_track->isMidiTrack() || !m_processEvents)
		return;
	QMenu* p = new QMenu(this);
	p->disconnect();
	p->clear();
	p->setTitle(tr("Viewable automation"));
	CtrlListList* cll = ((AudioTrack*) m_track)->controller();
	QAction* act = 0;
	for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
	{
		CtrlList *cl = icll->second;
		if (cl->dontShow())
			continue;
		QString name(cl->pluginName().isEmpty() ? cl->name() : cl->pluginName() + " : " + cl->name()); 
		act = p->addAction(name);
		act->setCheckable(true);
		act->setChecked(cl->isVisible());
		act->setData(cl->id());
	}
	//connect(p, SIGNAL(triggered(QAction*)), SLOT(changeAutomation(QAction*)));
	
	QAction* act1 = p->exec(QCursor::pos());

	if(act1)
	{
		int id = act1->data().toInt();

		CtrlListList* cll = ((AudioTrack*)m_track)->controller();
		for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
		{
			CtrlList *cl = icll->second;
			if (id == cl->id()) 
			{
				cl->setVisible(!cl->isVisible());
				if(cl->id() == AC_PAN)
				{
					AutomationType at = ((AudioTrack*) m_track)->automationType();
					if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
						((AudioTrack*) m_track)->enablePanController(false);
				
					double panVal = ((AudioTrack*) m_track)->pan();
					audio->msgSetPan(((AudioTrack*) m_track), panVal);
					((AudioTrack*) m_track)->startAutoRecord(AC_PAN, panVal);

					if (((AudioTrack*) m_track)->automationType() != AUTO_WRITE)
						((AudioTrack*) m_track)->enablePanController(true);
					((AudioTrack*) m_track)->stopAutoRecord(AC_PAN, panVal);
				}
			}
		}
		song->update(SC_TRACK_MODIFIED);
	}

	delete p;
}/*}}}*/

void TrackHeader::toggleRecord(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if (!m_track->isMidiTrack())
	{
		if (m_track->type() == Track::AUDIO_OUTPUT)
		{
			if (state && !m_track->recordFlag())
			{
				oom->bounceToFile((AudioOutput*) m_track);
			}
			audio->msgSetRecord((AudioOutput*) m_track, state);
			if (!((AudioOutput*) m_track)->recFile())
			{
				state = false;
			}
			else
			{
				m_btnRecord->blockSignals(true);
				m_btnRecord->setChecked(state);
				m_btnRecord->blockSignals(false);
				return;
			}
		}
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(state);
		m_btnRecord->blockSignals(false);
		song->setRecordFlag(m_track, state);
	}
	else
	{
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(state);
		m_btnRecord->blockSignals(false);
		song->setRecordFlag(m_track, state);
	}
	/*
	else if (button == Qt::RightButton)
	{
		// enable or disable ALL tracks of this type
		if (!m_track->isMidiTrack() && valid)
		{
			if (m_track->type() == Track::AUDIO_OUTPUT)
			{
				return;
			}
			WaveTrackList* wtl = song->waves();

			foreach(WaveTrack *wt, *wtl)
			{
				song->setRecordFlag(wt, val);
			}
		}
		else if(valid)
		{
			MidiTrackList* mtl = song->midis();

			foreach(MidiTrack *mt, *mtl)
			{
				song->setRecordFlag(mt, val);
			}
		}
		else
		{
			updateSelection(m_track, shift);
		}
	}
	*/
}/*}}}*/

void TrackHeader::toggleMute(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setMute(state);
	m_btnMute->blockSignals(true);
	m_btnMute->setChecked(state);
	m_btnMute->blockSignals(false);
	song->update(SC_MUTE);
}/*}}}*/

void TrackHeader::toggleSolo(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	audio->msgSetSolo(m_track, state);
	m_btnSolo->blockSignals(true);
	m_btnSolo->setChecked(state);
	m_btnSolo->blockSignals(false);
	song->update(SC_SOLO);
}/*}}}*/

void TrackHeader::toggleOffState(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setOff(state);
}/*}}}*/

void TrackHeader::toggleReminder1(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder1(state);
}/*}}}*/

void TrackHeader::toggleReminder2(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder2(state);
}/*}}}*/

void TrackHeader::toggleReminder3(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder3(state);
}/*}}}*/

void TrackHeader::updateTrackName()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	QString name = m_trackName->text();
	if(name.isEmpty())
	{
		m_trackName->blockSignals(true);
		m_trackName->setText(m_track->name());
		m_trackName->blockSignals(false);
		setEditing(false);
		return;
	}
	if (name != m_track->name())
	{
		TrackList* tl = song->tracks();
		for (iTrack i = tl->begin(); i != tl->end(); ++i)
		{
			if ((*i)->name() == name)
			{
				QMessageBox::critical(this,
						tr("OOMidi: bad trackname"),
						tr("please choose a unique track name"),
						QMessageBox::Ok,
						Qt::NoButton,
						Qt::NoButton);
				m_trackName->blockSignals(true);
				m_trackName->setText(m_track->name());
				m_trackName->blockSignals(false);
				setEditing(false);
				return;
			}
		}
		Track* track = m_track->clone(false);
		m_track->setName(name);
		audio->msgChangeTrack(track, m_track);
	}
	setEditing(false);
}/*}}}*/

void TrackHeader::updateVolume()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		int channel = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		ciMidiCtrlValList icl;

		MidiController* ctrl = mp->midiController(CTRL_VOLUME);
		int nvolume = mp->hwCtrlState(channel, CTRL_VOLUME);
		if (nvolume == CTRL_VAL_UNKNOWN)
		{
			volume = CTRL_VAL_UNKNOWN;
			nvolume = mp->lastValidHWCtrlState(channel, CTRL_VOLUME);
			if (nvolume != CTRL_VAL_UNKNOWN)
			{
				nvolume -= ctrl->bias();
				if (double(nvolume) != m_slider->value())
				{
					m_slider->setValue(double(nvolume));
				}
			}
		}
		else
		{
			nvolume -= ctrl->bias();
			if (nvolume != volume)
			{
				m_slider->setValue(double(nvolume));
				volume = nvolume;
			}
		}
	}
	else
	{
		double vol = ((AudioTrack*) m_track)->volume();
		if (vol != volume)
		{
			m_slider->blockSignals(true);
			double val = fast_log10(vol) * 20.0;
			m_slider->setValue(val);
			m_slider->blockSignals(false);
			volume = vol;
			if(((AudioTrack*) m_track)->volFromAutomation())
			{
				midiMonitor->msgSendAudioOutputEvent((Track*)m_track, CTRL_VOLUME, vol);
			}
		}
	}
}/*}}}*/

void TrackHeader::volumeChanged(double val)/*{{{*/
{
	if(!m_track || inHeartBeat)
		return;
	if(m_track->isMidiTrack())
	{
		int num = CTRL_VOLUME;
		int  mval = lrint(val);

		MidiTrack* t = (MidiTrack*) m_track;
		int port = t->outPort();

		int chan = t->outChannel();
		MidiPort* mp = &midiPorts[port];
		MidiController* mctl = mp->midiController(num);
		if ((mval < mctl->minVal()) || (mval > mctl->maxVal()))
		{
			if (mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
				audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
		}
		else
		{
			mval += mctl->bias();

			int tick = song->cpos();

			MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, num, mval);

			audio->msgPlayMidiEvent(&ev);
			midiMonitor->msgSendMidiOutputEvent(m_track, num, mval);
		}
		song->update(SC_MIDI_CONTROLLER);
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
			((AudioTrack*)m_track)->enableVolumeController(false);
	
		double vol;
		if (val <= config.minSlider)
		{
			vol = 0.0;
			val -= 1.0; 
		}
		else
			vol = pow(10.0, val / 20.0);
		volume = vol;
		audio->msgSetVolume((AudioTrack*) m_track, vol);
		((AudioTrack*) m_track)->recordAutomation(AC_VOLUME, vol);
		song->update(SC_TRACK_MODIFIED);
	}
}/*}}}*/

void TrackHeader::volumePressed()/*{{{*/
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
			m_track->enableVolumeController(false);

		double val = m_slider->value();
		double vol;
		if (val <= config.minSlider)
		{
			vol = 0.0;
		}
		else
			vol = pow(10.0, val / 20.0);
		volume = vol;
		audio->msgSetVolume((AudioTrack*) m_track, volume);
		((AudioTrack*) m_track)->startAutoRecord(AC_VOLUME, volume);
	}

}/*}}}*/

void TrackHeader::volumeReleased()/*{{{*/
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
	}
	else
	{
		if (((AudioTrack*)m_track)->automationType() != AUTO_WRITE)
			((AudioTrack*)m_track)->enableVolumeController(true);
		((AudioTrack*) m_track)->stopAutoRecord(AC_VOLUME, volume);
	}

}/*}}}*/

void TrackHeader::volumeRightClicked(const QPoint &p, int ctrl)/*{{{*/
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		song->execMidiAutomationCtlPopup((MidiTrack*) m_track, 0, p, ctrl);
	}
	else
	{
		song->execAutomationCtlPopup((AudioTrack*) m_track, p, AC_VOLUME);
	}

}/*}}}*/

void TrackHeader::updatePan()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		int channel = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		//MidiCtrlValListList* mc = mp->controller();
		ciMidiCtrlValList icl;

		MidiController* ctrl = mp->midiController(CTRL_PANPOT);
		int npan = mp->hwCtrlState(channel, CTRL_PANPOT);
		if (npan == CTRL_VAL_UNKNOWN)
		{
			panVal = CTRL_VAL_UNKNOWN;
			npan = mp->lastValidHWCtrlState(channel, CTRL_PANPOT);
			if (npan != CTRL_VAL_UNKNOWN)
			{
				npan -= ctrl->bias();
				if (double(npan) != m_pan->value())
				{
					m_pan->setValue(double(npan));
				}
			}
		}
		else
		{
			npan -= ctrl->bias();
			if (npan != panVal)
			{
				m_pan->setValue(double(npan));
				panVal = npan;
			}
		}
	}
	else
	{
		double v = ((AudioTrack*) m_track)->pan();
		if (v != panVal)
		{
			m_pan->blockSignals(true);
			m_pan->setValue(v);
			m_pan->blockSignals(false);
			panVal = v;
			if(((AudioTrack*) m_track)->panFromAutomation())
			{
				midiMonitor->msgSendAudioOutputEvent((Track*)m_track, CTRL_PANPOT, v);
			}
		}
	}
	if(m_pan && panVal != CTRL_VAL_UNKNOWN)
	{
		QString val = QString::number(panVal);
		m_pan->setToolTip(val+" Panorama");
	}
	else if(m_pan)
		m_pan->setToolTip("Panorama");
}/*}}}*/

void TrackHeader::panChanged(double val)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		int ctrl = CTRL_PANPOT;
		int ival =  lrint(val);
		MidiTrack* t = (MidiTrack*) m_track;
		int port = t->outPort();

		int chan = t->outChannel();
		MidiPort* mp = &midiPorts[port];
		MidiController* mctl = mp->midiController(ctrl);
		if ((ival < mctl->minVal()) || (ival > mctl->maxVal()))
		{
			if (mp->hwCtrlState(chan, ctrl) != CTRL_VAL_UNKNOWN)
				audio->msgSetHwCtrlState(mp, chan, ctrl, CTRL_VAL_UNKNOWN);
			panVal = 0.0;
		}
		else
		{
			val += mctl->bias();

			int tick = song->cpos();

			MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, ctrl, val);

			audio->msgPlayMidiEvent(&ev);
			midiMonitor->msgSendMidiOutputEvent(m_track, ctrl, val);
			panVal = ival;
		}
		song->update(SC_MIDI_CONTROLLER);
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
			m_track->enablePanController(false);

		panVal = val;
		audio->msgSetPan(((AudioTrack*) m_track), val);
		((AudioTrack*) m_track)->recordAutomation(AC_PAN, val);
	}
	QString span = QString::number(panVal);
	span.append(tr(" Panorama"));

	QPoint cursorPos = QCursor::pos();
	QToolTip::showText(cursorPos, span, this, QRect(cursorPos.x(), cursorPos.y(), 2, 2));
}/*}}}*/

void TrackHeader::panPressed()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
			m_track->enablePanController(false);

		panVal = m_pan->value();
		audio->msgSetPan(((AudioTrack*) m_track), panVal);
		((AudioTrack*) m_track)->startAutoRecord(AC_PAN, panVal);
	}
}/*}}}*/

void TrackHeader::panReleased()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
	}
	else
	{
		if (m_track->automationType() != AUTO_WRITE)
			m_track->enablePanController(true);
		((AudioTrack*) m_track)->stopAutoRecord(AC_PAN, panVal);
	}
}/*}}}*/

void TrackHeader::panRightClicked(const QPoint &p)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
	}
	else
		song->execAutomationCtlPopup((AudioTrack*) m_track, p, AC_PAN);
}/*}}}*/

//Private member functions

bool TrackHeader::eventFilter(QObject *obj, QEvent *event)/*{{{*/
{
	if(!m_processEvents)
		return true;
	if (event->type() == QEvent::MouseButtonPress) {
		QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
		if(mEvent && mEvent->button() == Qt::LeftButton)
		{
			mousePressEvent(mEvent);
			mode = NORMAL;
		}
	}

	// standard event processing
	return QObject::eventFilter(obj, event);

}/*}}}*/

void TrackHeader::updateChannels()/*{{{*/
{
	if(m_track && !m_track->isMidiTrack())
	{
		AudioTrack* t = (AudioTrack*) m_track;
		int c = t->channels();
		//printf("TrackHeader::updateChannels(%d) chanels: %d\n", c, m_channels);

		if (c > m_channels)
		{
			//printf("Going stereo\n");
			int size = 3+m_channels;
			for (int cc = m_channels; cc < c; ++size, ++cc)
			{
				//meter[cc] = new Meter(this);
				meter[cc] = new Meter(this, Meter::DBMeter, Qt::Horizontal);
				meter[cc]->setRange(config.minMeter, 10.0);
				meter[cc]->setFixedHeight(15);
				meter[cc]->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
				//connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
				m_mainVBox->insertWidget(size, meter[cc]);
				meter[cc]->show();
			}
		}
		else if (c < m_channels)
		{
			//printf("Going mono\n");
			for (int cc = m_channels - 1; cc >= c; --cc)
			{
				delete meter[cc];
				meter[cc] = 0;
			}
		}
		m_channels = c;
	}
}/*}}}*/

void TrackHeader::initPan()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	QString img(":images/knob_midi_new.png");
	Track::TrackType type = m_track->type();
	switch (type)
	{
		case Track::MIDI:
		case Track::DRUM:
			img = QString(":images/knob_audio_new.png");
		break;
		case Track::WAVE:
			img = QString(":images/knob_input_new.png");
		break;
		case Track::AUDIO_OUTPUT:
			img = QString(":images/knob_output_new.png");
		break;
		case Track::AUDIO_INPUT:
			img = QString(":images/knob_midi_new.png");
		break;
		case Track::AUDIO_BUSS:
			img = QString(":images/knob_buss_new.png");
		break;
		case Track::AUDIO_AUX:
			img = QString(":images/knob_aux_new.png");
		break;
		case Track::AUDIO_SOFTSYNTH:
			img = QString(":images/knob_audio_new.png");
		break;
	}
	if(m_track->isMidiTrack())
	{
		int ctl = CTRL_PANPOT, mn, mx, v;
		int chan = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		MidiController* mc = mp->midiController(ctl);
		mn = mc->minVal();
		mx = mc->maxVal();

		m_pan = new Knob(this);
		m_pan->setRange(double(mn), double(mx), 1.0);
		m_pan->setId(ctl);
		m_pan->setKnobImage(img);

		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		m_pan->setBackgroundRole(QPalette::Mid);
		m_pan->setToolTip("Panorama");
		m_pan->setEnabled(true);
		//m_pan->setIgnoreWheel(true);

		v = mp->hwCtrlState(chan, ctl);
		if (v == CTRL_VAL_UNKNOWN)
		{
			int lastv = mp->lastValidHWCtrlState(chan, ctl);
			if (lastv == CTRL_VAL_UNKNOWN)
			{
				if (mc->initVal() == CTRL_VAL_UNKNOWN)
					v = 0;
				else
					v = mc->initVal();
			}
			else
				v = lastv - mc->bias();
		}
		else
		{
			// Auto bias...
			v -= mc->bias();
		}

		m_pan->setValue(double(v));

		m_panLayout->insertWidget(0, m_pan);

		connect(m_pan, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
		//connect(m_pan, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
	}
	else
	{
		m_pan = new Knob(this);
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
		m_pan->setRange(-1.0, +1.0);
		m_pan->setToolTip(tr("Panorama"));
		m_pan->setKnobImage(img);
		//m_pan->setIgnoreWheel(true);
		m_pan->setBackgroundRole(QPalette::Mid);
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		m_panLayout->insertWidget(0, m_pan);
		connect(m_pan, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
		connect(m_pan, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
		connect(m_pan, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
	}
}/*}}}*/

void TrackHeader::initVolume()
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)m_track;
		MidiPort* mp = &midiPorts[track->outPort()];
		MidiController* mc = mp->midiController(CTRL_VOLUME);
		//int chan = track->outChannel();
		int mn = mc->minVal();
		int mx = mc->maxVal();

		m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, g_trackColorList.value(m_track->type()));
		m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m_slider->setCursorHoming(true);
		m_slider->setRange(double(mn), double(mx), 1.0);
		m_slider->setFixedHeight(20);
		m_slider->setFont(config.fonts[1]);
		m_slider->setId(CTRL_VOLUME);
		m_mainVBox->insertWidget(2, m_slider);
		connect(m_slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
		connect(m_slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &, int)));

		meter[0] = new Meter(this, Meter::LinMeter, Qt::Horizontal);
		meter[0]->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		meter[0]->setRange(0, 127.0);
		meter[0]->setFixedHeight(15);
		//meter[0]->setFixedWidth(150);
		m_mainVBox->insertWidget(3, meter[0]);
		//connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
	}
	else
	{
		int channels = ((AudioTrack*)m_track)->channels();
		m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, g_trackColorList.value(m_track->type()));
		m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m_slider->setCursorHoming(true);
		m_slider->setRange(config.minSlider - 0.1, 10.0);
		m_slider->setFixedHeight(20);
		m_slider->setFont(config.fonts[1]);
		m_slider->setValue(fast_log10(((AudioTrack*)m_track)->volume())*20.0);
		m_mainVBox->insertWidget(2, m_slider);

		connect(m_slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
		connect(m_slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
		connect(m_slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));

		for (int i = 0; i < channels; ++i)/*{{{*/
		{
			meter[i] = new Meter(this, Meter::DBMeter, Qt::Horizontal);
			meter[i]->setRange(config.minMeter, 10.0);
			meter[i]->setFixedHeight(15);
			//meter[i]->setFixedWidth(150);
			meter[i]->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
			//connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
			//connect(meter[i], SIGNAL(meterClipped()), this, SLOT(playbackClipped()));
			m_mainVBox->insertWidget(3+i, meter[i]);
		}/*}}}*/
		m_channels = channels;
	}
}

void TrackHeader::setupStyles()/*{{{*/
{
	m_style.insert(Track::MIDI, styletemplate.arg(QString("939393")));
	m_style.insert(Track::WAVE, styletemplate.arg(QString("939393")));
	m_style.insert(Track::AUDIO_OUTPUT, styletemplate.arg(QString("939393")));
	m_style.insert(Track::AUDIO_INPUT, styletemplate.arg(QString("939393")));
	m_style.insert(Track::AUDIO_BUSS, styletemplate.arg(QString("939393")));
	m_style.insert(Track::AUDIO_AUX, styletemplate.arg(QString("939393")));
	m_style.insert(Track::AUDIO_SOFTSYNTH, styletemplate.arg(QString("939393")));
	
	m_selectedStyle.insert(Track::AUDIO_INPUT, styletemplate.arg(QString("e18fff")));
	m_selectedStyle.insert(Track::MIDI, styletemplate.arg(QString("01e6ee")));
	m_selectedStyle.insert(Track::AUDIO_OUTPUT, styletemplate.arg(QString("fc7676")));
	m_selectedStyle.insert(Track::WAVE, styletemplate.arg(QString("81f476")));
	m_selectedStyle.insert(Track::AUDIO_BUSS, styletemplate.arg(QString("fca424")));
	m_selectedStyle.insert(Track::AUDIO_AUX, styletemplate.arg(QString("ecf276")));
	m_selectedStyle.insert(Track::AUDIO_SOFTSYNTH, styletemplate.arg(QString("01e6ee")));
}/*}}}*/

//Protected events
//We overwrite these from QWidget to implement our own functionality

void TrackHeader::mousePressEvent(QMouseEvent* ev) //{{{
{
	if(!m_track || !m_processEvents)
		return;
	int button = ev->button();
	bool shift = ((QInputEvent*) ev)->modifiers() & Qt::ShiftModifier;

	if (button == Qt::LeftButton)
	{
		if (resizeFlag)
		{
			startY = ev->y();
			mode = RESIZE;
			return;
		}
		else
		{
			m_startPos = ev->pos();
			if (!shift)
			{
				song->deselectTracks();
				if(song->hasSelectedParts)
					song->deselectAllParts();
				setSelected(true);

				//record enable track if expected
				int recd = 0;
				TrackList* tracks = song->visibletracks();
				Track* recTrack = 0;
				for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
				{
					if ((*t)->recordFlag())
					{
						if(!recTrack)
							recTrack = *t;
						recd++;
					}
				}
				if (recd == 1 && config.moveArmedCheckBox)
				{ 
					//one rec enabled track, move rec enabled with selection
					song->setRecordFlag(recTrack, false);
					song->setRecordFlag(m_track, true);
				}
			}
			else
			{
				song->deselectAllParts();
				setSelected(true);
			}
			song->update(SC_SELECTION | SC_RECFLAG);
			mode = START_DRAG;
		}
	}
	else if(button == Qt::RightButton)
	{
		generatePopupMenu();
	}
}/*}}}*/

void TrackHeader::mouseMoveEvent(QMouseEvent* ev)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	bool shift = ((QInputEvent*) ev)->modifiers() & Qt::ShiftModifier;
	if(shift)
	{
		resizeFlag = false;
		setCursor(QCursor(Qt::ArrowCursor));
		return;
	}
	if ((((QInputEvent*) ev)->modifiers() | ev->buttons()) == 0)
	{
		QRect geo = geometry();
		QRect hotBox(0, m_track->height() - 2, width(), 2);
		//printf("HotBox: x: %d, y: %d, event pos x: %d, y: %d, geo bottom: %d\n", hotBox.x(), hotBox.y(), ev->x(), ev->y(), geo.bottom());
		if (hotBox.contains(ev->pos()))
		{
			//printf("Hit hotspot\n");
			if (!resizeFlag)
			{
				resizeFlag = true;
				setCursor(QCursor(Qt::SplitVCursor));
			}
		}
		else
		{
			resizeFlag = false;
			setCursor(QCursor(Qt::ArrowCursor));
		}
		return;
	}
	curY = ev->y();
	int delta = curY - startY;
	switch (mode)
	{
		case START_DRAG:
		{
			if ((ev->pos() - m_startPos).manhattanLength() < QApplication::startDragDistance())
				return;

			m_editing = true;
			mode = DRAG;
			QPoint hotSpot = ev->pos();
			int index = song->visibletracks()->index(m_track);
			
			QByteArray itemData;
			QDataStream dataStream(&itemData, QIODevice::WriteOnly);
			dataStream << m_track->name() << index << QPoint(hotSpot);
			
			QMimeData *mimeData = new QMimeData;
			mimeData->setData("oomidi/x-trackinfo", itemData);
			mimeData->setText(m_track->name());
			
			QDrag *drag = new QDrag(this);
			drag->setMimeData(mimeData);
			//drag->setPixmap();
			drag->setHotSpot(hotSpot);
			drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction);
		}
		break;
		case NORMAL:
		case DRAG:
		break;
		case RESIZE:
		{
			if (m_track)
			{
				int h = m_track->height() + delta;
				startY = curY;
				if (h < MIN_TRACKHEIGHT)
					h = MIN_TRACKHEIGHT;
				m_track->setHeight(h);
				song->update(SC_TRACK_MODIFIED);
			}
		}
		break;
	}
}/*}}}*/

void TrackHeader::mouseReleaseEvent(QMouseEvent*)/*{{{*/
{
	mode = NORMAL;
	setCursor(QCursor(Qt::ArrowCursor));
	m_editing = false;
	resizeFlag = false;
}/*}}}*/

void TrackHeader::resizeEvent(QResizeEvent* event)/*{{{*/
{
	//We will trap this to disappear widgets like vu's and volume slider
	//on the track header. For now we'll just pass it up the chain
	QSize size = event->size();
	if(m_track)
	{
		m_meterVisible = size.height() > MIN_TRACKHEIGHT_VU;
		m_sliderVisible = size.height() > MIN_TRACKHEIGHT_SLIDER;
		if(m_track->isMidiTrack())
		{
			if(meter[0])
				meter[0]->setVisible(m_meterVisible);
		}
		else
		{
			for (int ch = 0; ch < ((AudioTrack*)m_track)->channels(); ++ch)
			{
				if (meter[ch])
				{
					meter[ch]->setVisible(m_meterVisible);
				}
			}
		}
		if(m_slider)
			m_slider->setVisible(m_sliderVisible);
	}
	QFrame::resizeEvent(event);
}/*}}}*/

