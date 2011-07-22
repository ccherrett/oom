//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "gconfig.h"
#include "globals.h"
#include "config.h"
#include "track.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "scrollscale.h"
#include "xml.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
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

TrackHeader::TrackHeader(Track* t, QWidget* parent)
: QFrame(parent)
{
	m_track = t;
	connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
}

QSize TrackHeader::sizeHint()
{
	if(m_track)
		return QSize(200, m_track->height());
	else
		return QSize(200, 40);
}

void TrackHeader::songChanged(int flags)
{
	if(!m_track)
		return;
	if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_MIDI_TRACK_PROP | SC_SELECTION | SC_TRACK_MODIFIED))
	{
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
	}
}

void TrackHeader::generateAutomationMenu()/*{{{*/
{
	if(!m_track || m_track->isMidiTrack())
		return;
	PopupMenu* p = new PopupMenu();
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
	connect(p, SIGNAL(triggered(QAction*)), SLOT(changeAutomation(QAction*)));
	p->exec(QCursor::pos());

	delete p;
}/*}}}*/

void TrackHeader::changeAutomation(QAction* act)/*{{{*/
{
	if (m_track->isMidiTrack())
	{
		//printf("this is wrong, we can't edit automation for midi tracks from arranger yet!\n");
		return;
	}

	CtrlListList* cll = ((AudioTrack*)m_track)->controller();
	for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
	{
		CtrlList *cl = icll->second;
		if (act->data() == cl->id()) 
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
}/*}}}*/

void TrackHeader::toggleRecord(bool state)/*{{{*/
{
	if(!m_track)
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

void TrackHeader::toggleMute(bool state)
{
	m_track->setMute(state);
	m_btnMute->blockSignals(true);
	m_btnMute->setChecked(state);
	m_btnMute->blockSignals(false);
	song->update(SC_MUTE);
}

void TrackHeader::toggleSolo(bool state)
{
	audio->msgSetSolo(m_track, state);
	m_btnSolo->blockSignals(true);
	m_btnSolo->setChecked(state);
	m_btnSolo->blockSignals(false);
	song->update(SC_SOLO);
}

void TrackHeader::toggleOffState(bool state)
{
	m_track->setOff(state);
}

void TrackHeader::updateTrackName(QString name)
{
	m_track->setName(name);
}

void TrackHeader::generatePopupMenu()/*{{{*/
{
	TrackList selectedTracksList = song->getSelectedTracks();
	bool multipleSelectedTracks = false;
	if (selectedTracksList.size() > 1)
	{
		multipleSelectedTracks = true;
	}

	QMenu* p = new QMenu;
	QMenu* colorPopup = p->addMenu(tr("Default Part Color"));

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
	}
	if(m_track && m_track->type() == Track::WAVE)
		p->addAction(tr("Import Audio File"))->setData(1);
	if (!multipleSelectedTracks)
	{
		p->addAction(QIcon(*midi_edit_instrumentIcon), tr("Rename Track"))->setData(15);
	}
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
		//printf("synth hasgui %d, gui visible %d\n",port->hasGui(), port->guiVisible());
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
			default:
				printf("action %d\n", n);
			break;
		}
	}
	delete trackHeightsMenu;
	delete p;
}/*}}}*/
