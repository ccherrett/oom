//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//=========================================================

#include <QTimer>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndexList>
#include <QHeaderView>
#include <QToolButton>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <values.h>

#include "Conductor.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "icons.h"
#include "app.h"
#include "route.h"
#include "popupmenu.h"
#include "pctable.h"
#include "plugin.h"
#include "shortcuts.h"
#include "TrackManager.h"
#include "TrackInstrumentMenu.h"

#include "traverso_shared/TConfig.h"

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void Conductor::setTrack(Track* t)
{
	if (!t)
	{
		selected = 0;
		return;
	}

	if (!t->isMidiTrack())
		return;
	selected = t;

	populatePatches();
	updateConductor(-1);
	//printf("Conductor::setTrack()\n");
	populateMatrix();
	rebuildMatrix();
	if(_resetProgram)
	{
		MidiTrack* midiTrack = static_cast<MidiTrack*>(t);/*{{{*/
		if(midiTrack)
		{
			int outChannel = midiTrack->outChannel();
			int outPort = midiTrack->outPort();
			MidiPort* mp = &midiPorts[outPort];
			if (mp->hwCtrlState(outChannel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
				audio->msgSetHwCtrlState(mp, outChannel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
			int hbank = iHBank->value();
			int lbank = iLBank->value();
			int prog = iProgram->value();

			if (hbank > 0 && hbank < 129)
				hbank -= 1;
			else
				hbank = 0xff;
			if (lbank > 0 && lbank < 129)
				lbank -= 1;
			else
				lbank = 0xff;
			if (prog > 0 && prog < 129)
				prog -= 1;
			else
				prog = 0xff;
			program = (hbank << 16) + (lbank << 8) + prog;
			MidiPlayEvent ev(0, outPort, outChannel, ME_CONTROLLER, CTRL_PROGRAM, program, t);
			audio->msgPlayMidiEvent(&ev);
		}/*}}}*/
	}
	_resetProgram = false;
}

//---------------------------------------------------------
//   midiConductor
//---------------------------------------------------------

Conductor::Conductor(QWidget* parent, Track* sel_track) : QFrame(parent)//QWidget(parent)
{
	setupUi(this);
	_midiDetect = false;
	_progRowNum = 0;
	_selectedIndex = 0;
	editing = false;
	_useMatrix = false;
	_autoExapand = true;
	_resetProgram = false;
	m_globalState = false;
	m_eventPart = 0;
	
	//Hide nonsense
	frame->hide();
	textLabel1->hide();
	recordButton->hide();
	iHBank->hide();
	iLBank->hide();
	TextLabel4->hide();
	TextLabel5->hide();
	iLautst->hide();
	iPan->hide();
	iVerz->hide();
	iLen->hide();
	iKompr->hide();
	iAnschl->hide();
	iProgram->hide();
	progRecButton->hide();
	volRecButton->hide();
	panRecButton->hide();
	TextLabel10->hide();
	TextLabel11->hide();
	TextLabel12->hide();
	TextLabel13->hide();

	//iOutputChannel->setPrefix("Chan: ");	
	_matrix = new QList<int>;
	_tableModel = new ProgramChangeTableModel(this);
	tableView = new ProgramChangeTable(this);
	tableView->installEventFilter(oom);
	tableView->setMinimumHeight(150);
	tableBox->addWidget(tableView);
	_selModel = new QItemSelectionModel(_tableModel);//tableView->selectionModel();
	_patchModel = new QStandardItemModel(0, 2, this);
	_patchSelModel = new QItemSelectionModel(_patchModel);
	patchList->installEventFilter(oom);

	selected = sel_track;

	// Since program covers 3 controls at once, it is in 'midi controller' units rather than 'gui control' units.
	//program  = -1;
	program = CTRL_VAL_UNKNOWN;
	pan = -65;
	volume = -1;

	setFont(config.fonts[2]);

	//iChanDetectLabel->setPixmap(*darkgreendotIcon);
	iChanDetectLabel->setPixmap(*darkRedLedIcon);

	QIcon recEchoIconSet;
	//recEchoIconSet.addPixmap(*midiThruOnIcon, QIcon::Normal, QIcon::On);
	//recEchoIconSet.addPixmap(*midiThruOffIcon, QIcon::Normal, QIcon::Off);
	recEchoButton->setIcon(*midiInIconSet3);
	recEchoButton->setIconSize(QSize(25,25));

	tableView->setModel(_tableModel);
	tableView->setShowGrid(true);
	tableView->setSelectionModel(_selModel);
	tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
	updateTableHeader();

	patchList->setModel(_patchModel);
	patchList->setSelectionModel(_patchSelModel);

	btnUp->setIcon(*up_arrowIconSet3);
	btnDown->setIcon(*down_arrowIconSet3);
	btnDelete->setIcon(*garbageIconSet3);
	btnCopy->setIcon(*duplicateIconSet3);
	btnShowGui->setIcon(*pluginGUIIconSet3);
	//btnCopy->setIconSize(duplicatePCIcon->size());


	connect(tableView, SIGNAL(rowOrderChanged()), SLOT(rebuildMatrix()));
	connect(tableView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(patchSequenceClicked(const QModelIndex&)));
	connect(_selModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SLOT(matrixSelectionChanged(QItemSelection, QItemSelection)));
	connect(_tableModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(matrixItemChanged(QStandardItem*)));
	connect(_tableModel, SIGNAL(rowsInserted(QModelIndex, int, int)), SLOT(patchSequenceInserted(QModelIndex, int, int)));
	connect(_tableModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), SLOT(patchSequenceRemoved(QModelIndex, int, int)));
	connect(patchList, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT(patchDoubleClicked(const QModelIndex&) ) );
	connect(patchList, SIGNAL( clicked(const QModelIndex&) ), this, SLOT(patchClicked(const QModelIndex&) ) );
	//connect(_patchSelModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SLOT(patchSelectionChanged(QItemSelection, QItemSelection)));
	//connect(chkAdvanced, SIGNAL(stateChanged(int)), SLOT(toggleAdvanced(int)));
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(deleteSelectedPatches(bool)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(movePatchUp(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(movePatchDown(bool)));
	connect(btnCopy, SIGNAL(clicked(bool)), SLOT(clonePatchSequence()));

	//setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));

	//connect(iPatch, SIGNAL(released()), SLOT(instrPopup()));

	connect(iOutputChannel, SIGNAL(valueChanged(int)), SLOT(iOutputChannelChanged(int)));
	///connect(iInputChannel, SIGNAL(textChanged(const QString&)), SLOT(iInputChannelChanged(const QString&)));
	connect(iHBank, SIGNAL(valueChanged(int)), SLOT(iProgHBankChanged()));
	connect(iLBank, SIGNAL(valueChanged(int)), SLOT(iProgLBankChanged()));
	connect(iProgram, SIGNAL(valueChanged(int)), SLOT(iProgramChanged()));
	connect(iHBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
	connect(iLBank, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
	connect(iProgram, SIGNAL(doubleClicked()), SLOT(iProgramDoubleClicked()));
	connect(iLautst, SIGNAL(valueChanged(int)), SLOT(iLautstChanged(int)));
	connect(iLautst, SIGNAL(doubleClicked()), SLOT(iLautstDoubleClicked()));
	connect(iTransp, SIGNAL(valueChanged(int)), SLOT(iTranspChanged(int)));
	connect(chkTranspose, SIGNAL(toggled(bool)), SLOT(transposeStateChanged(bool)));
	connect(iAnschl, SIGNAL(valueChanged(int)), SLOT(iAnschlChanged(int)));
	connect(iVerz, SIGNAL(valueChanged(int)), SLOT(iVerzChanged(int)));
	connect(iLen, SIGNAL(valueChanged(int)), SLOT(iLenChanged(int)));
	connect(iKompr, SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
	connect(iPan, SIGNAL(valueChanged(int)), SLOT(iPanChanged(int)));
	connect(iPan, SIGNAL(doubleClicked()), SLOT(iPanDoubleClicked()));
	connect(iOutput, SIGNAL(activated(int)), SLOT(iOutputPortChanged(int)));
	connect(recordButton, SIGNAL(clicked()), SLOT(recordClicked()));
	connect(progRecButton, SIGNAL(clicked()), SLOT(progRecClicked()));
	connect(volRecButton, SIGNAL(clicked()), SLOT(volRecClicked()));
	connect(panRecButton, SIGNAL(clicked()), SLOT(panRecClicked()));
	connect(recEchoButton, SIGNAL(toggled(bool)), SLOT(recEchoToggled(bool)));
	connect(iRButton, SIGNAL(pressed()), SLOT(inRoutesPressed()));
	connect(btnTranspose, SIGNAL(toggled(bool)), SIGNAL(globalTransposeClicked(bool)));
	connect(btnComments, SIGNAL(toggled(bool)), SIGNAL(toggleComments(bool)));

	btnInstrument->setIcon(*instrumentIconSet3);
	connect(btnInstrument, SIGNAL(clicked()), this, SLOT(generateInstrumentMenu()));

	btnShowGui->setShortcut(shortcuts[SHRT_SHOW_PLUGIN_GUI].key);
	connect(btnShowGui, SIGNAL(toggled(bool)), this, SLOT(toggleSynthGui(bool)));

	btnComments->setIcon(QIcon(*commentIconSet3));
	btnTranspose->setIcon(QIcon(*transposeIconSet3));

	// TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
	//oRButton->setEnabled(false);
	//oRButton->setVisible(false);
	//connect(oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));

	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
	//bool adv = tconfig().get_property("Conductor", "advanced", false).toBool();
	//chkAdvanced->setChecked(adv);
	iRButton->setIcon(*inputIconSet3);
}

Conductor::~Conductor()
{
	//tconfig().set_property("Conductor", "advanced", chkAdvanced->isChecked());
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Conductor::heartBeat()/*{{{*/
{
	if (!isVisible() || !isEnabled() || !selected)
		return;
	switch (selected->type())
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			MidiTrack* track = (MidiTrack*) selected;

			int outChannel = track->outChannel();
			int outPort = track->outPort();

			MidiPort* mp = &midiPorts[outPort];

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
					if (!_midiDetect)
					{
						_midiDetect = true;
						iChanDetectLabel->setPixmap(*redLedIcon);
					}
					break;
				}
			}
			// No activity detected?
			if (r == rl->end())
			{
				if (_midiDetect)
				{
					_midiDetect = false;
					iChanDetectLabel->setPixmap(*darkRedLedIcon);
				}
			}
            
            if (mp->device() && mp->device()->isSynthPlugin())
            {
                SynthPluginDevice* synth = (SynthPluginDevice*)mp->device();
                
                int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
                
                if (nprogram == program)
                {
                    nprogram = synth->getCurrentProgram();
                    if (nprogram == -1)
                        nprogram = CTRL_VAL_UNKNOWN;
                    if (nprogram != program)
                        mp->setHwCtrlState(outChannel, CTRL_PROGRAM, nprogram);
                }
                
                if (nprogram != program)
                {
                    if (nprogram == CTRL_VAL_UNKNOWN)
                    {
                        const QString n(tr("Select Patch"));
                        emit updateCurrentPatch(n);
                    }
                    else
                    {
                        MidiInstrument* instr = mp->instrument();
                        QString name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
                        if (name.isEmpty())
                        {
                            name = "???";
                        }
                        Patch *patch = instr->getPatch(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
                        if(patch)
                        {
                            emit patchChanged(patch);
                        }
                        else
                        {
                            emit patchChanged(new Patch);
                        }
                        emit updateCurrentPatch(name);
                    }
                    program = nprogram;
                }
            }
            else
            {
                int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
                
                if (nprogram == CTRL_VAL_UNKNOWN)
                {
                    if (program != CTRL_VAL_UNKNOWN)
                    {
                        //printf("Composer::midiConductorHeartBeat setting program to unknown\n");
                        
                        program = CTRL_VAL_UNKNOWN;
                        if (iHBank->value() != 0)
                        {
                            iHBank->blockSignals(true);
                            iHBank->setValue(0);
                            iHBank->blockSignals(false);
                        }
                        if (iLBank->value() != 0)
                        {
                            iLBank->blockSignals(true);
                            iLBank->setValue(0);
                            iLBank->blockSignals(false);
                        }
                        if (iProgram->value() != 0)
                        {
                            iProgram->blockSignals(true);
                            iProgram->setValue(0);
                            iProgram->blockSignals(false);
                        }
                    }
                    
                    nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
                    
                    if (nprogram == CTRL_VAL_UNKNOWN)
                    {
                        const QString n(tr("Select Patch"));
                        emit updateCurrentPatch(n);
                    }
                    else
                    {
                        MidiInstrument* instr = mp->instrument();
                        QString name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
                        if (name.isEmpty())
                        {
                            name = "???";
                        }
                        Patch *patch = instr->getPatch(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
                        if(patch)
                        {
                            emit patchChanged(patch);
                        }
                        else
                        {
                            emit patchChanged(new Patch);
                        }
                        emit updateCurrentPatch(name);
                    }
                }
                else if (program != nprogram)
                {
                    program = nprogram;
                    
                    MidiInstrument* instr = mp->instrument();
                    QString name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
                    Patch *patch = instr->getPatch(outChannel, program, song->mtype(), track->type() == Track::DRUM);
                    if(patch)
                    {
                        emit patchChanged(patch);
                    }
                    else
                    {
                        emit patchChanged(new Patch);
                    }
                    emit updateCurrentPatch(name);
                    
                    int hb = ((program >> 16) & 0xff) + 1;
                    if (hb == 0x100)
                        hb = 0;
                    int lb = ((program >> 8) & 0xff) + 1;
                    if (lb == 0x100)
                        lb = 0;
                    int pr = (program & 0xff) + 1;
                    if (pr == 0x100)
                        pr = 0;
                    
                    if (iHBank->value() != hb)
                    {
                        iHBank->blockSignals(true);
                        iHBank->setValue(hb);
                        iHBank->blockSignals(false);
                    }
                    if (iLBank->value() != lb)
                    {
                        iLBank->blockSignals(true);
                        iLBank->setValue(lb);
                        iLBank->blockSignals(false);
                    }
                    if (iProgram->value() != pr)
                    {
                        iProgram->blockSignals(true);
                        iProgram->setValue(pr);
                        iProgram->blockSignals(false);
                    }
                }
            }
                
			MidiController* mc = mp->midiController(CTRL_VOLUME);
			int mn = mc->minVal();
			int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
			if (v == CTRL_VAL_UNKNOWN)
				v = mn - 1;
			else
				v -= mc->bias();
			if (volume != v)
			{
				volume = v;
				if (iLautst->value() != v)
				{
					iLautst->blockSignals(true);
					iLautst->setValue(v);
					iLautst->blockSignals(false);
				}
			}

			mc = mp->midiController(CTRL_PANPOT);
			mn = mc->minVal();
			v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
			if (v == CTRL_VAL_UNKNOWN)
				v = mn - 1;
			else
				v -= mc->bias();
			if (pan != v)
			{
				pan = v;
				if (iPan->value() != v)
				{
					iPan->blockSignals(true);
					iPan->setValue(v);
					iPan->blockSignals(false);
				}
			}

			QList<PatchSequence*> *list = mp->patchSequences();
			if (_progRowNum != list->size())
			{
				//printf("Calling populate matrix from heartBeat()\n");
				populateMatrix();
				rebuildMatrix();
			}

			if(selected->isMidiTrack())/*{{{*/
			{
				int oPort = ((MidiTrack*) selected)->outPort();
				MidiPort* port = &midiPorts[oPort];

				if(port)
					btnInstrument->setToolTip(QString(tr("Change Instrument: ")).append(port->instrument()->iname()));
				else
					btnInstrument->setToolTip(tr("Change Instrument"));
    		    if (port->device() && port->device()->isSynthPlugin())
    		    {
                    btnShowGui->setEnabled(true);
					SynthPluginDevice* synth = (SynthPluginDevice*)port->device();
                    //Update the state as it was set elsewhere or the track was changed
                    if (synth->hasNativeGui())
                    {
                        if (btnShowGui->isChecked() != synth->nativeGuiVisible())
                        {
                            btnShowGui->blockSignals(true);
                            btnShowGui->setChecked(synth->nativeGuiVisible());
                            btnShowGui->blockSignals(false);
                        }
                    }
                    else
                    {
                        if (btnShowGui->isChecked() != synth->guiVisible())
                        {
                            btnShowGui->blockSignals(true);
                            btnShowGui->setChecked(synth->guiVisible());
                            btnShowGui->blockSignals(false);
                        }
                    }
    		    }
				else
				{
					btnShowGui->setEnabled(false);
				}
			}/*}}}*/
		}
			break;

		case Track::WAVE:
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			break;
	}
}/*}}}*/

void Conductor::toggleSynthGui(bool on)
{
	if(!selected)
		return;
	if(selected->isMidiTrack())/*{{{*/
	{
		int oPort = ((MidiTrack*) selected)->outPort();
		MidiPort* port = &midiPorts[oPort];

        if (port->device() && port->device()->isSynthPlugin())
        {
			SynthPluginDevice* synth = (SynthPluginDevice*)port->device();
            if(synth->hasNativeGui())
				audio->msgShowInstrumentNativeGui(port->instrument(), on);
            else
                audio->msgShowInstrumentGui(port->instrument(), on);
        }
	}/*}}}*/
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Conductor::configChanged()
{
	//printf("Conductor::configChanged\n");

	//if (config.canvasBgPixmap.isEmpty()) {
	//      canvas->setBg(config.partCanvasBg);
	//      canvas->setBg(QPixmap());
	//}
	//else {
	//      canvas->setBg(QPixmap(config.canvasBgPixmap));
	//}

	setFont(config.fonts[2]);
	//updateConductor(type);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Conductor::songChanged(int type)
{
	//printf("Conductor::songChanged() Type: %d\n", type);
	// Is it simply a midi controller value adjustment? Forget it.
	if (type == SC_MIDI_CONTROLLER)
		return;
	if (type == SC_SELECTION)
		return;
	if (!isVisible())
		return;
	if(type & (SC_TRACK_INSTRUMENT))
		setTrack(selected);
	if ((type & (SC_PATCH_UPDATED)) && !editing)
	{
		//printf("Calling populate matrix from songChanged()\n");
		populateMatrix();
		rebuildMatrix();
		return;
	}
	updateConductor(type);
}

//---------------------------------------------------------
//   iOutputChannelChanged
//---------------------------------------------------------

void Conductor::iOutputChannelChanged(int channel)
{
	--channel;
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;/*{{{*/
	if (channel != track->outChannel())
	{
		// Changed by T356.
		//track->setOutChannel(channel);
		audio->msgIdle(true);
		//audio->msgSetTrackOutChannel(track, channel);
		track->setOutChanAndUpdate(channel);
		audio->msgIdle(false);

		// may result in adding/removing mixer strip:
		//song->update(-1);
		song->update(SC_MIDI_TRACK_PROP);
	}/*}}}*/
}

//---------------------------------------------------------
//   iOutputPortChanged
//---------------------------------------------------------

void Conductor::iOutputPortChanged(int index)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	if (index == track->outPort())
		return;
	// Changed by T356.
	//track->setOutPort(index);
	audio->msgIdle(true);
	//audio->msgSetTrackOutPort(track, index);
	track->setOutPortAndUpdate(index);
	//_tableModel->clear();
	//printf("Calling populate matrix from iOutputPortChanged()\n");
	populatePatches();
	populateMatrix();
	rebuildMatrix();
	audio->msgIdle(false);

	song->update(SC_MIDI_TRACK_PROP);
}

void Conductor::updateCommentState(bool state, bool block)
{
	if(block)
		btnComments->blockSignals(true);
	btnComments->setChecked(state);
	if(block)
		btnComments->blockSignals(false);
}

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

//void Conductor::routingPopupMenuActivated(int n)

void Conductor::routingPopupMenuActivated(QAction* act)
{
	///if(!midiConductor || gRoutingPopupMenuMaster != midiConductor || !selected || !selected->isMidiTrack())
	if ((gRoutingPopupMenuMaster != this) || !selected || !selected->isMidiTrack())
		return;
	oom->routingPopupMenuActivated(selected, act->data().toInt());
}

#if 0
//---------------------------------------------------------
//   routingPopupViewActivated
//---------------------------------------------------------

void Conductor::routingPopupViewActivated(const QModelIndex& mdi)
{
	///if(!midiConductor || gRoutingPopupMenuMaster != midiConductor || !selected || !selected->isMidiTrack())
	if (gRoutingPopupMenuMaster != this || !selected || !selected->isMidiTrack())
		return;
	oom->routingPopupMenuActivated(selected, mdi.data().toInt());
}
#endif

//---------------------------------------------------------
//   inRoutesPressed
//---------------------------------------------------------

void Conductor::inRoutesPressed()/*{{{*/
{
	if (!selected)
		return;
	if (!selected->isMidiTrack())
		return;

	PopupMenu* pup = oom->prepareRoutingPopupMenu(selected, false);
	//PopupView* pup = oom->prepareRoutingPopupView(selected, false);

	if (!pup)
	{
		int ret = QMessageBox::warning(this, tr("No inputs"),
				tr("There are no midi inputs.\n"
				"Do you want to open the midi configuration dialog?"),
				QMessageBox::Ok | QMessageBox::Cancel,
				QMessageBox::Ok);
		if (ret == QMessageBox::Ok)
		{
			// printf("open config midi ports\n");
			oom->configMidiAssign(1);
		}
		return;
	}

	///gRoutingPopupMenuMaster = midiConductor;
	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	//connect(pup, SIGNAL(activated(const QModelIndex&)), SLOT(routingPopupViewActivated(const QModelIndex&)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	//connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupViewAboutToHide()));
	pup->popup(QCursor::pos());
	//pup->setVisible(true);
	iRButton->setDown(false);
	return;
}/*}}}*/

//---------------------------------------------------------
//   outRoutesPressed
//---------------------------------------------------------

void Conductor::outRoutesPressed()/*{{{*/
{
	if (!selected)
		return;
	if (!selected->isMidiTrack())
		return;

	PopupMenu* pup = oom->prepareRoutingPopupMenu(selected, true);
	if (!pup)
		return;

	///gRoutingPopupMenuMaster = midiConductor;
	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	pup->popup(QCursor::pos());
	///oRButton->setDown(false);
	return;
}/*}}}*/

//---------------------------------------------------------
//   iProgHBankChanged
//---------------------------------------------------------

void Conductor::iProgHBankChanged()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int channel = track->outChannel();
	int port = track->outPort();
	int hbank = iHBank->value();
	int lbank = iLBank->value();
	int prog = iProgram->value();

	if (hbank > 0 && hbank < 129)
		hbank -= 1;
	else
		hbank = 0xff;
	if (lbank > 0 && lbank < 129)
		lbank -= 1;
	else
		lbank = 0xff;
	if (prog > 0 && prog < 129)
		prog -= 1;
	else
		prog = 0xff;

	MidiPort* mp = &midiPorts[port];
	if (prog == 0xff && hbank == 0xff && lbank == 0xff)
	{
		program = CTRL_VAL_UNKNOWN;
		if (mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
		return;
	}

	int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
	if (np == CTRL_VAL_UNKNOWN)
	{
		np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
		if (np != CTRL_VAL_UNKNOWN)
		{
			lbank = (np & 0xff00) >> 8;
			prog = np & 0xff;
			if (prog == 0xff)
				prog = 0;
			int ilbnk = lbank;
			int iprog = prog;
			if (ilbnk == 0xff)
				ilbnk = -1;
			++ilbnk;
			++iprog;
			iLBank->blockSignals(true);
			iProgram->blockSignals(true);
			iLBank->setValue(ilbnk);
			iProgram->setValue(iprog);
			iLBank->blockSignals(false);
			iProgram->blockSignals(false);
		}
	}

	if (prog == 0xff && (hbank != 0xff || lbank != 0xff))
	{
		prog = 0;
		iProgram->blockSignals(true);
		iProgram->setValue(1);
		iProgram->blockSignals(false);
	}
	program = (hbank << 16) + (lbank << 8) + prog;
	MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program, selected);
	audio->msgPlayMidiEvent(&ev);

	MidiInstrument* instr = mp->instrument();
	emit updateCurrentPatch(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	Patch *patch = instr->getPatch(channel, program, song->mtype(), track->type() == Track::DRUM);
	if(patch)
	{
		emit patchChanged(patch);
	}
	else
	{
		emit patchChanged(new Patch);
	}
	//iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
}/*}}}*/

//---------------------------------------------------------
//   iProgLBankChanged
//---------------------------------------------------------

void Conductor::iProgLBankChanged()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int channel = track->outChannel();
	int port = track->outPort();
	int hbank = iHBank->value();
	int lbank = iLBank->value();
	int prog = iProgram->value();

	if (hbank > 0 && hbank < 129)
		hbank -= 1;
	else
		hbank = 0xff;
	if (lbank > 0 && lbank < 129)
		lbank -= 1;
	else
		lbank = 0xff;
	if (prog > 0 && prog < 129)
		prog -= 1;
	else
		prog = 0xff;

	MidiPort* mp = &midiPorts[port];
	if (prog == 0xff && hbank == 0xff && lbank == 0xff)
	{
		program = CTRL_VAL_UNKNOWN;
		if (mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
		return;
	}

	int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
	if (np == CTRL_VAL_UNKNOWN)
	{
		np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
		if (np != CTRL_VAL_UNKNOWN)
		{
			hbank = (np & 0xff0000) >> 16;
			prog = np & 0xff;
			if (prog == 0xff)
				prog = 0;
			int ihbnk = hbank;
			int iprog = prog;
			if (ihbnk == 0xff)
				ihbnk = -1;
			++ihbnk;
			++iprog;
			iHBank->blockSignals(true);
			iProgram->blockSignals(true);
			iHBank->setValue(ihbnk);
			iProgram->setValue(iprog);
			iHBank->blockSignals(false);
			iProgram->blockSignals(false);
		}
	}

	if (prog == 0xff && (hbank != 0xff || lbank != 0xff))
	{
		prog = 0;
		iProgram->blockSignals(true);
		iProgram->setValue(1);
		iProgram->blockSignals(false);
	}
	program = (hbank << 16) + (lbank << 8) + prog;
	MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program, selected);
	audio->msgPlayMidiEvent(&ev);

	MidiInstrument* instr = mp->instrument();
	emit updateCurrentPatch(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	//iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	Patch *patch = instr->getPatch(channel, program, song->mtype(), track->type() == Track::DRUM);
	if(patch)
	{
		emit patchChanged(patch);
	}
	else
	{
		emit patchChanged(new Patch);
	}
}/*}}}*/

//---------------------------------------------------------
//   iProgramChanged
//---------------------------------------------------------

void Conductor::iProgramChanged()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int channel = track->outChannel();
	int port = track->outPort();
	int hbank = iHBank->value();
	int lbank = iLBank->value();
	int prog = iProgram->value();

	if (hbank > 0 && hbank < 129)
		hbank -= 1;
	else
		hbank = 0xff;
	if (lbank > 0 && lbank < 129)
		lbank -= 1;
	else
		lbank = 0xff;
	if (prog > 0 && prog < 129)
		prog -= 1;
	else
		prog = 0xff;

	MidiPort *mp = &midiPorts[port];
	if (prog == 0xff)
	{
		program = CTRL_VAL_UNKNOWN;
		iHBank->blockSignals(true);
		iLBank->blockSignals(true);
		iHBank->setValue(0);
		iLBank->setValue(0);
		iHBank->blockSignals(false);
		iLBank->blockSignals(false);

		if (mp->hwCtrlState(channel, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, channel, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
		return;
	}
	else
	{
		int np = mp->hwCtrlState(channel, CTRL_PROGRAM);
		if (np == CTRL_VAL_UNKNOWN)
		{
			np = mp->lastValidHWCtrlState(channel, CTRL_PROGRAM);
			if (np != CTRL_VAL_UNKNOWN)
			{
				hbank = (np & 0xff0000) >> 16;
				lbank = (np & 0xff00) >> 8;
				int ihbnk = hbank;
				int ilbnk = lbank;
				if (ihbnk == 0xff)
					ihbnk = -1;
				if (ilbnk == 0xff)
					ilbnk = -1;
				++ihbnk;
				++ilbnk;
				iHBank->blockSignals(true);
				iLBank->blockSignals(true);
				iHBank->setValue(ihbnk);
				iLBank->setValue(ilbnk);
				iHBank->blockSignals(false);
				iLBank->blockSignals(false);
			}
		}
		program = (hbank << 16) + (lbank << 8) + prog;
		MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program, selected);
		audio->msgPlayMidiEvent(&ev);

		MidiInstrument* instr = mp->instrument();
		emit updateCurrentPatch(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
		Patch *patch = instr->getPatch(channel, program, song->mtype(), track->type() == Track::DRUM);
		if(patch)
		{
			emit patchChanged(patch);
		}
		else
		{
			emit patchChanged(new Patch);
		}
		//iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	}

	//      updateConductor();
}/*}}}*/

//---------------------------------------------------------
//   iLautstChanged
//---------------------------------------------------------

void Conductor::iLautstChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int outPort = track->outPort();
	int chan = track->outChannel();
	MidiPort* mp = &midiPorts[outPort];
	MidiController* mctl = mp->midiController(CTRL_VOLUME);
	if ((val < mctl->minVal()) || (val > mctl->maxVal()))
	{
		if (mp->hwCtrlState(chan, CTRL_VOLUME) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_VOLUME, CTRL_VAL_UNKNOWN);
	}
	else
	{
		val += mctl->bias();

		MidiPlayEvent ev(0, outPort, chan, ME_CONTROLLER, CTRL_VOLUME, val, selected);
		audio->msgPlayMidiEvent(&ev);
	}
	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   iTranspChanged
//---------------------------------------------------------

void Conductor::iTranspChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->transposition = val;
	song->update(SC_MIDI_TRACK_PROP);
}

void Conductor::transposeStateChanged(bool state)
{
	if(!selected)
		return;
	MidiTrack* track = (MidiTrack*)selected;
	track->transpose = state;
	if(!state)
	{
		btnTranspose->blockSignals(true);
		btnTranspose->setChecked(state);
		btnTranspose->blockSignals(false);
	}
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iAnschlChanged
//---------------------------------------------------------

void Conductor::iAnschlChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->velocity = val;
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

//---------------------------------------------------------
//   iVerzChanged
//---------------------------------------------------------

void Conductor::iVerzChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->delay = val;
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

//---------------------------------------------------------
//   iLenChanged
//---------------------------------------------------------

void Conductor::iLenChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->len = val;
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void Conductor::iKomprChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->compression = val;
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

//---------------------------------------------------------
//   iPanChanged
//---------------------------------------------------------

void Conductor::iPanChanged(int val)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	int chan = track->outChannel();
	MidiPort* mp = &midiPorts[port];
	MidiController* mctl = mp->midiController(CTRL_PANPOT);
	if ((val < mctl->minVal()) || (val > mctl->maxVal()))
	{
		if (mp->hwCtrlState(chan, CTRL_PANPOT) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_PANPOT, CTRL_VAL_UNKNOWN);
	}
	else
	{
		val += mctl->bias();

		// Realtime Change:
		MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, val, selected);
		audio->msgPlayMidiEvent(&ev);
	}
	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void Conductor::instrPopup()
{
	return;
}

//---------------------------------------------------------
//   recEchoToggled
//---------------------------------------------------------

void Conductor::recEchoToggled(bool v)/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->setRecEcho(v);
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

//---------------------------------------------------------
//   iProgramDoubleClicked
//---------------------------------------------------------

void Conductor::iProgramDoubleClicked()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	int chan = track->outChannel();
	MidiPort* mp = &midiPorts[port];
	MidiController* mctl = mp->midiController(CTRL_PROGRAM);

	if (!track || !mctl)
		return;

	int lastv = mp->lastValidHWCtrlState(chan, CTRL_PROGRAM);
	int curv = mp->hwCtrlState(chan, CTRL_PROGRAM);

	if (curv == CTRL_VAL_UNKNOWN)
	{
		// If no value has ever been set yet, use the current knob value
		//  (or the controller's initial value?) to 'turn on' the controller.
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			int kiv = mctl->initVal();
			//int kiv = lrint(_knob->value());
			if (kiv == CTRL_VAL_UNKNOWN)
				kiv = 0;
			//else
			//{
			//if(kiv < mctrl->minVal())
			//  kiv = mctrl->minVal();
			//if(kiv > mctrl->maxVal())
			//  kiv = mctrl->maxVal();
			//kiv += mctrl->bias();
			//}

			//MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, num, kiv);
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, kiv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, lastv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   iLautstDoubleClicked
//---------------------------------------------------------

void Conductor::iLautstDoubleClicked()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	int chan = track->outChannel();
	MidiPort* mp = &midiPorts[port];
	MidiController* mctl = mp->midiController(CTRL_VOLUME);

	if (!track || !mctl)
		return;

	int lastv = mp->lastValidHWCtrlState(chan, CTRL_VOLUME);
	int curv = mp->hwCtrlState(chan, CTRL_VOLUME);

	if (curv == CTRL_VAL_UNKNOWN)
	{
		// If no value has ever been set yet, use the current knob value
		//  (or the controller's initial value?) to 'turn on' the controller.
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			int kiv = mctl->initVal();
			//int kiv = lrint(_knob->value());
			if (kiv == CTRL_VAL_UNKNOWN)
				// Set volume to 78% of range, so that if range is 0 - 127, then value is 100.
				kiv = lround(double(mctl->maxVal() - mctl->minVal()) * 0.7874);
			else
			{
				if (kiv < mctl->minVal())
					kiv = mctl->minVal();
				if (kiv > mctl->maxVal())
					kiv = mctl->maxVal();
				kiv += mctl->bias();
			}

			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, kiv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, lastv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_VOLUME) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_VOLUME, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   iPanDoubleClicked
//---------------------------------------------------------

void Conductor::iPanDoubleClicked()/*{{{*/
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	int chan = track->outChannel();
	MidiPort* mp = &midiPorts[port];
	MidiController* mctl = mp->midiController(CTRL_PANPOT);

	if (!track || !mctl)
		return;

	int lastv = mp->lastValidHWCtrlState(chan, CTRL_PANPOT);
	int curv = mp->hwCtrlState(chan, CTRL_PANPOT);

	if (curv == CTRL_VAL_UNKNOWN)
	{
		// If no value has ever been set yet, use the current knob value
		//  (or the controller's initial value?) to 'turn on' the controller.
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			int kiv = mctl->initVal();
			//int kiv = lrint(_knob->value());
			if (kiv == CTRL_VAL_UNKNOWN)
				// Set volume to 50% of range, so that if range is 0 - 127, then value is 64.
				kiv = lround(double(mctl->maxVal() - mctl->minVal()) * 0.5);
			else
			{
				if (kiv < mctl->minVal())
					kiv = mctl->minVal();
				if (kiv > mctl->maxVal())
					kiv = mctl->maxVal();
				kiv += mctl->bias();
			}

			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, kiv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, lastv, selected);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_PANPOT) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_PANPOT, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

void Conductor::generateInstrumentMenu()/*{{{*/
{
	if(!selected || !selected->isMidiTrack())
		return;

	QMenu* p = new QMenu(this);
	TrackInstrumentMenu *imenu = new TrackInstrumentMenu(p, selected);
	connect(imenu, SIGNAL(instrumentSelected(qint64, const QString&, int)), this, SLOT(instrumentChangeRequested(qint64, const QString&, int)));

	p->addAction(imenu);
	p->exec(QCursor::pos());

	delete p;
}/*}}}*/

void Conductor::instrumentChangeRequested(qint64 id, const QString& instrument, int type)
{
	trackManager->setTrackInstrument(id, instrument, type);
}

//---------------------------------------------------------
//   updateConductor
//---------------------------------------------------------

void Conductor::updateConductor(int flags)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;
	if (flags == SC_SELECTION)
		return;

	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;

	if (flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))
		oom->updateRouteMenus(selected, this);

	if (flags & (SC_MIDI_TRACK_PROP))
	{
		iTransp->blockSignals(true);
		chkTranspose->blockSignals(true);
		iAnschl->blockSignals(true);
		iVerz->blockSignals(true);
		iLen->blockSignals(true);
		iKompr->blockSignals(true);
		iTransp->setValue(track->transposition);
		chkTranspose->setChecked(track->transpose);
		if(!track->transpose)
		{
			btnTranspose->blockSignals(true);
			btnTranspose->setChecked(false);
			btnTranspose->blockSignals(false);
		}
		iAnschl->setValue(track->velocity);
		iVerz->setValue(track->delay);
		iLen->setValue(track->len);
		iKompr->setValue(track->compression);
		iTransp->blockSignals(false);
		chkTranspose->blockSignals(false);
		iAnschl->blockSignals(false);
		iVerz->blockSignals(false);
		iLen->blockSignals(false);
		iKompr->blockSignals(false);

		int outChannel = track->outChannel();
	
		
		int outPort = track->outPort();
		iOutput->blockSignals(true);

		iOutput->clear();

		for (int i = 0; i < MIDI_PORTS; ++i)
		{
			QString name;
			name.sprintf("%d:%s", i + 1, midiPorts[i].portname().toLatin1().constData());
			iOutput->insertItem(i, name);
			if (i == outPort)
				iOutput->setCurrentIndex(i);
		}
		iOutput->blockSignals(false);
		
		iOutputChannel->blockSignals(true);
		iOutputChannel->setValue(outChannel + 1);
		iOutputChannel->blockSignals(false);

		// Set record echo.
		if (recEchoButton->isChecked() != track->recEcho())
		{
			recEchoButton->blockSignals(true);
			recEchoButton->setChecked(track->recEcho());
			recEchoButton->blockSignals(false);
		}
	}

	int outChannel = track->outChannel();
	int outPort = track->outPort();
	MidiPort* mp = &midiPorts[outPort];
	int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
	if (nprogram == CTRL_VAL_UNKNOWN)
	{
		iHBank->blockSignals(true);
		iLBank->blockSignals(true);
		iProgram->blockSignals(true);
		iHBank->setValue(0);
		iLBank->setValue(0);
		iProgram->setValue(0);
		iHBank->blockSignals(false);
		iLBank->blockSignals(false);
		iProgram->blockSignals(false);

		program = CTRL_VAL_UNKNOWN;
		nprogram = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
		if (nprogram == CTRL_VAL_UNKNOWN)
		{
			emit patchChanged(new Patch);
			emit updateCurrentPatch(tr("Select Patch"));
		}
		else
		{
			MidiInstrument* instr = mp->instrument();
			emit updateCurrentPatch(instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM));
			Patch *patch = instr->getPatch(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
			if(patch)
			{
				emit patchChanged(patch);
			}
			else
			{
				emit patchChanged(new Patch);
			}
		}
	}
	else
	{
		program = nprogram;

		MidiInstrument* instr = mp->instrument();
		emit updateCurrentPatch(instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM));
		Patch *patch = instr->getPatch(outChannel, program, song->mtype(), track->type() == Track::DRUM);
		if(patch)
		{
			emit patchChanged(patch);
		}
		else
		{
			emit patchChanged(new Patch);
		}

		int hb = ((program >> 16) & 0xff) + 1;
		if (hb == 0x100)
			hb = 0;
		int lb = ((program >> 8) & 0xff) + 1;
		if (lb == 0x100)
			lb = 0;
		int pr = (program & 0xff) + 1;
		if (pr == 0x100)
			pr = 0;
		iHBank->blockSignals(true);
		iLBank->blockSignals(true);
		iProgram->blockSignals(true);

		iHBank->setValue(hb);
		iLBank->setValue(lb);
		iProgram->setValue(pr);

		iHBank->blockSignals(false);
		iLBank->blockSignals(false);
		iProgram->blockSignals(false);
	}
    
    if (mp->device() && mp->device()->isSynthPlugin())
        return;

	MidiController* mc = mp->midiController(CTRL_VOLUME);
	int mn = mc->minVal();
	int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
	volume = v;
	if (v == CTRL_VAL_UNKNOWN)
		v = mn - 1;
	else	// Auto bias...
		v -= mc->bias();
	iLautst->blockSignals(true);
	iLautst->setRange(mn - 1, mc->maxVal());
	iLautst->setValue(v);
	iLautst->blockSignals(false);

	mc = mp->midiController(CTRL_PANPOT);
	mn = mc->minVal();
	v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
	pan = v;
	if (v == CTRL_VAL_UNKNOWN)
		v = mn - 1;
	else // Auto bias...
		v -= mc->bias();
	iPan->blockSignals(true);
	iPan->setRange(mn - 1, mc->maxVal());
	iPan->setValue(v);
	iPan->blockSignals(false);
	if(selected->isMidiTrack())/*{{{*/
	{
		int oPort = ((MidiTrack*) selected)->outPort();
		MidiPort* port = &midiPorts[oPort];

        if (port->device() && port->device()->isSynthPlugin())
        {
			SynthPluginDevice* synth = (SynthPluginDevice*)port->device();
            //Update the state as it was set elsewhere or the track was changed
            if (synth->hasNativeGui())
            {
                if (btnShowGui->isChecked() != synth->nativeGuiVisible())
                {
                    btnShowGui->blockSignals(true);
                    btnShowGui->setChecked(synth->nativeGuiVisible());
                    btnShowGui->blockSignals(false);
                }
            }
            else
            {
                if (btnShowGui->isChecked() != synth->guiVisible())
                {
                    btnShowGui->blockSignals(true);
                    btnShowGui->setChecked(synth->guiVisible());
                    btnShowGui->blockSignals(false);
                }
            }
        }
		else
		{
			btnShowGui->setEnabled(false);
		}
	}/*}}}*/
}

void Conductor::editorPartChanged(Part* p)
{
	if(p)
	{
		_resetProgram = true;
	}
}

//---------------------------------------------------------
//   progRecClicked
//---------------------------------------------------------

void Conductor::progRecClicked()
{
	if (!selected)
		return;
	progRecClicked(selected);
}

void Conductor::progRecClicked(Track* t)
{
	if (!t)
		return;
	MidiTrack* track = (MidiTrack*) t;
	int portno = track->outPort();
	int channel = track->outChannel();
	MidiPort* port = &midiPorts[portno];
	int program = port->hwCtrlState(channel, CTRL_PROGRAM);
	if (program == CTRL_VAL_UNKNOWN || program == 0xffffff)
		return;

	unsigned tick = song->cpos();
	Event a(Controller);
	a.setTick(tick);
	a.setA(CTRL_PROGRAM);
	a.setB(program);

	if(m_eventPart)
		song->recordEvent((MidiPart*)m_eventPart, a);
	else //Let song guess, its only that rec button we never use
		song->recordEvent(track, a);
}

//---------------------------------------------------------
//   volRecClicked
//---------------------------------------------------------

void Conductor::volRecClicked()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int portno = track->outPort();
	int channel = track->outChannel();
	MidiPort* port = &midiPorts[portno];
	int volume = port->hwCtrlState(channel, CTRL_VOLUME);
	if (volume == CTRL_VAL_UNKNOWN)
		return;

	unsigned tick = song->cpos();
	Event a(Controller);
	a.setTick(tick);
	a.setA(CTRL_VOLUME);
	a.setB(volume);

	song->recordEvent(track, a);
}

//---------------------------------------------------------
//   panRecClicked
//---------------------------------------------------------

void Conductor::panRecClicked()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int portno = track->outPort();
	int channel = track->outChannel();
	MidiPort* port = &midiPorts[portno];
	int pan = port->hwCtrlState(channel, CTRL_PANPOT);
	if (pan == CTRL_VAL_UNKNOWN)
		return;

	unsigned tick = song->cpos();
	Event a(Controller);
	a.setTick(tick);
	a.setA(CTRL_PANPOT);
	a.setB(pan);

	song->recordEvent(track, a);
}

//---------------------------------------------------------
//   recordClicked
//---------------------------------------------------------

void Conductor::recordClicked()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int portno = track->outPort();
	int channel = track->outChannel();
	MidiPort* port = &midiPorts[portno];
	unsigned tick = song->cpos();

	int program = port->hwCtrlState(channel, CTRL_PROGRAM);
	if (program != CTRL_VAL_UNKNOWN && program != 0xffffff)
	{
		Event a(Controller);
		a.setTick(tick);
		a.setA(CTRL_PROGRAM);
		a.setB(program);
		song->recordEvent(track, a);
	}
	int volume = port->hwCtrlState(channel, CTRL_VOLUME);
	if (volume != CTRL_VAL_UNKNOWN)
	{
		Event a(Controller);
		a.setTick(tick);
		a.setA(CTRL_VOLUME);
		a.setB(volume);
		song->recordEvent(track, a);
	}
	int pan = port->hwCtrlState(channel, CTRL_PANPOT);
	if (pan != CTRL_VAL_UNKNOWN)
	{
		Event a(Controller);
		a.setTick(tick);
		a.setA(CTRL_PANPOT);
		a.setB(pan);
		song->recordEvent(track, a);
	}
}

void Conductor::toggleAdvanced(int checked)
{
	if (checked == Qt::Checked)
	{
		frame->show();
	}
	else
	{
		frame->hide();
	}
}

void Conductor::rebuildMatrix()
{
	//printf("Entering rebuildMatrix()\n");
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	MidiPort* mp = &midiPorts[port];
	//Clear the matrix
	_matrix->erase(_matrix->begin(), _matrix->end());
	QList<int> rows = tableView->getSelectedRows();
	if (mp)
	{
		QList<PatchSequence*> *list = mp->patchSequences();
		if (list && !list->isEmpty())
		{
			if (!rows.isEmpty() && _useMatrix)
			{
				int row = rows.at(0);
				for (int i = row; i < _tableModel->rowCount(); ++i)
				{
					//PatchSequence *ps = list->at(i);
					QStandardItem *item = _tableModel->item(i);
					if (item && item->checkState() == Qt::Checked)
					{
						_matrix->append(item->row());
					}
				}
				for(int i = 0; i < row; ++i)
				{
					QStandardItem *item = _tableModel->item(i);
					if (item && item->checkState() == Qt::Checked)
					{
						_matrix->append(item->row());
					}
				}
			}
			else
			{
				for (int i = 0; i < list->size(); ++i)
				{
					PatchSequence *ps = list->at(i);
					if (ps->selected)
					{
						_matrix->append(i);
					}
				}
			}
			_selModel->blockSignals(true);
			tableView->selectRow(_selectedIndex);
			_selModel->blockSignals(false);
			/*if (!_matrix->isEmpty())
			{
				_selModel->blockSignals(true);
				tableView->selectRow(_matrix->at(0));
				_selModel->blockSignals(false);
			}
			else
			{
				_selModel->blockSignals(true);
				tableView->selectRow(0);
				_selModel->blockSignals(false);
			}*/
		}
	}
	//printf("Leaving rebuildMatrix()\n");
}

void Conductor::matrixItemChanged(QStandardItem* item)
{
	//printf("Entering matrixItemChanged()\n");
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	MidiPort* mp = &midiPorts[port];
	if (item && mp)
	{
		QList<PatchSequence*> *list = mp->patchSequences();
		int row = item->row();
		if (list && !list->isEmpty() && row < list->size())
		{
			PatchSequence *ps = list->at(row);
			if (ps)
			{
				ps->selected = item->checkState() == Qt::Checked ? true : false;
				_useMatrix = ps->selected;
			}
			//updateConductor(-1);
			editing = true;
			song->update(SC_PATCH_UPDATED);
			editing = false;
			song->dirty = true;
			_selectedIndex = row;
			rebuildMatrix();
		}
	}
	//printf("Leaving matrixItemChanged()\n");
}

void Conductor::insertMatrixEvent(Part* curPart, unsigned tick)
{
	if (!curPart)
		return;
	m_eventPart = curPart;
	MidiTrack* track = (MidiTrack*) curPart->track();
	int channel = track->outChannel();
	int port = track->outPort();
	//printf("Conductor::insertMatrixEvent() _matrix->size() = %d\n", _matrix->size());
	if (_matrix->size() == 1 || !_useMatrix)
	{
		//Get the QStandardItem in the hidden third column
		//This column contains the ID of the Patch
		int row;
		if(!_useMatrix)
		{
			QList<int> rows = tableView->getSelectedRows();
			if(!rows.isEmpty())
				row = rows.at(0);
			else
			{
				m_eventPart = 0;
				return; //Nothing is selected and we are in selection mode
			}
			//printf("Conductor::insertMatrixEvent() not using matrix\n");
		}
		else
			row = _matrix->at(0);
		QStandardItem* item = _tableModel->item(row);
		int id = item->data(ConductorPatchIdRole).toInt();
		if(curPart->lenTick() <= song->cpos())
		{
			curPart->setLenTick(tick);
		}
		if(song->len() <= tick)
		{
			song->setLen(tick);
		}
		MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id, (Track*)track);
		audio->msgPlayMidiEvent(&ev);
		_selectedIndex = item->row();
		updateConductor(-1);
		//_selModel->blockSignals(true);
		tableView->selectRow(item->row());
		//_selModel->blockSignals(false);
		progRecClicked(track);
	}
	else if (_matrix->size() > 1)
	{
		int row = _matrix->takeFirst();
		//_selModel->blockSignals(true);
		_selectedIndex = _matrix->at(0);
		tableView->selectRow(_matrix->at(0));
		//_selModel->blockSignals(false);
		//printf("Adding Program Change for row: %d\n", row);
		if (row != -1 && row < _tableModel->rowCount())
		{
			QStandardItem* item = _tableModel->item(row);
			int id = item->data(ConductorPatchIdRole).toInt();
			if(curPart->lenTick() <= song->cpos())
			{
				curPart->setLenTick(tick);
			}
			if(song->len() <= tick)
			{
				song->setLen(tick);
			}
			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id, (Track*)track);
			audio->msgPlayMidiEvent(&ev);
			updateConductor(-1);
			progRecClicked(track);
		}
		_matrix->push_back(row);
	}
	m_eventPart = 0;
}

void Conductor::populateMatrix()
{
	//printf("Conductor::populateMatrix() entering\n");
	_tableModel->clear();
	if (!selected)
		return;
	//printf("Conductor::populateMatrix() found track\n");
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	MidiPort* mp = &midiPorts[port];
	if (mp)
	{
		QList<PatchSequence*> *ps = mp->patchSequences();
		if (ps)
		{
			for (int i = 0; i < ps->size(); ++i)
			{
				//printf("Conductor::populateMatrix() found preset: %d\n", i);
				PatchSequence* p = ps->at(i);
				
				QStandardItem *patch = new QStandardItem(p->name);
				patch->setToolTip(p->name);
				patch->setData(p->id, ConductorPatchIdRole);
				patch->setEditable(false);
				patch->setCheckable(true);
				patch->setCheckState(p->selected ? Qt::Checked : Qt::Unchecked);
				
				_tableModel->blockSignals(true);
				_tableModel->insertRow(_tableModel->rowCount(), patch);
				_tableModel->blockSignals(false);
				_tableModel->emit_layoutChanged();
				tableView->setRowHeight(_tableModel->rowCount(), 50);
			}
			_progRowNum = ps->size();
		}
	}
	//rebuildMatrix();
	tableView->resizeRowsToContents();
	updateTableHeader();
}

void Conductor::deleteSelectedPatches(bool)
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = 0;
		MidiTrack* track = (MidiTrack*) selected;
		int port = track->outPort();
		MidiPort* mp = &midiPorts[port];
		if (mp)
		{
			QList<PatchSequence*> *list = mp->patchSequences();
			QList<PatchSequence*> dlist;
			for (int i = 0; i < rows.size(); ++i)
			{
				int r = rows.at(i);
				id = r;
				if (!list->isEmpty() && r < list->size())
				{
					//int mid = list->indexOf(i);
					//if(mid != -1)
					PatchSequence* p = list->at(r);
					dlist.append(p);
				}
			}
			if (!dlist.isEmpty())
			{
				for (int d = 0; d < dlist.size(); ++d)
					mp->deletePatchSequence(dlist.at(d));
			}
		}
		//updateTableHeader();

		//tableView->resizeRowsToContents();
		int c = _tableModel->rowCount();
		//printf("Row Count: %d - Deleted  Row:%d\n",c ,id);
		if (c > id)
		{
			//_selModel->blockSignals(true);
			tableView->selectRow(id);
			_selectedIndex = id;
			//_selModel->blockSignals(false);
		}
		else
		{
			//_selModel->blockSignals(true);
			tableView->selectRow(0);
			_selectedIndex = 0;
			//_selModel->blockSignals(false);
		}
		populateMatrix();
		rebuildMatrix();
	}
}

void Conductor::movePatchDown(bool)
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id + 1) >= _tableModel->rowCount())
			return;
		int row = (id + 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		foreach(QStandardItem* i, item)
			i->setEditable(false);
		_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->selectRow(row);
	}
}

void Conductor::movePatchUp(bool)
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id - 1) < 0)
			return;
		int row = (id - 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		foreach(QStandardItem* i, item)
			i->setEditable(false);
		_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->selectRow(row);
	}
}

void Conductor::updateTableHeader()/*{{{*/
{
	QStandardItem* hpatch = new QStandardItem(tr("Patch"));
	_tableModel->setHorizontalHeaderItem(0, hpatch);
	tableView->horizontalHeader()->setStretchLastSection(true);

	//update the patchList headers as well
	QStandardItem* pid = new QStandardItem("");
	QStandardItem* patch = new QStandardItem(tr("Select Patch"));
	patch->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	_patchModel->setHorizontalHeaderItem(0, patch);
	_patchModel->setHorizontalHeaderItem(1, pid);
	//patchList->setColumnWidth(0, 1);
	patchList->setColumnHidden(1, true);
}/*}}}*/

void Conductor::patchSequenceInserted(QModelIndex /*index*/, int start, int end)
{
	//printf("Entering patchSequenceInserted()\n");
	if (!selected)/*{{{*/
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	MidiPort* mp = &midiPorts[port];
	if (mp)
	{
		for (int i = start; i <= end; ++i)
		{
			QStandardItem* patch = _tableModel->item(i);
			PatchSequence* ps = new PatchSequence();
			if (patch)
			{
				ps->id = patch->data(ConductorPatchIdRole).toInt();
				ps->name = patch->text();
				ps->selected = (patch->checkState() == Qt::Checked);
				mp->insertPatchSequence(i, ps);
				_selectedIndex = i;
				tableView->selectRow(i);
			}
		}
		editing = true;
		song->update(SC_PATCH_UPDATED);
		editing = false;
		song->dirty = true;
		//rebuildMatrix();
	}/*}}}*/
	//printf("Leaving patchSequenceInserted()\n");
}

void Conductor::patchSequenceRemoved(QModelIndex /*index*/, int start, int end)
{
	//printf("Leaving patchSequenceDeleted()\n");
	if (!selected)/*{{{*/
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int port = track->outPort();
	MidiPort* mp = &midiPorts[port];
	if (mp)
	{
		QList<PatchSequence*> *sets = mp->patchSequences();
		QList<PatchSequence*> toBeDeleted;
		if (sets)
		{
			for (int i = start; i <= end; ++i)
			{
				PatchSequence* ps = sets->at(i);
				if (ps)
				{
					toBeDeleted.append(ps);
				}
			}
			if (!toBeDeleted.isEmpty())
			{
				for (int d = 0; d < toBeDeleted.size(); ++d)
				{
					mp->deletePatchSequence(toBeDeleted.at(d));
				}
			}
		}
		//rebuildMatrix();
		editing = true;
		song->update(SC_PATCH_UPDATED);
		editing = false;
		song->dirty = true;
	}/*}}}*/
	//printf("Leaving patchSequenceDeleted()\n");
}

void Conductor::matrixSelectionChanged(QItemSelection sel, QItemSelection)
{
	//if(sel == unsel)
	//	return;
	QModelIndexList ind = sel.indexes();
	if(ind.isEmpty())
		return;
	QModelIndex inx = ind.at(0);
	int row = inx.row();
	QStandardItem* item = _tableModel->item(row);
	if(item)
	{
		_selectedIndex = row;
		
		_useMatrix = (item->checkState() == Qt::Checked);
		rebuildMatrix();
	}
}

void Conductor::clonePatchSequence()
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int start = rows.at(0);
		int row = (start + 1);
		QStandardItem* iname = _tableModel->item(start);
		if(iname)
		{
			QStandardItem* item = new QStandardItem(iname->text());
			item->setData(iname->data(), ConductorPatchIdRole);
			item->setToolTip(iname->toolTip());
			item->setCheckable(true);
			item->setEditable(false);
			item->setCheckState(iname->checkState());

			_selectedIndex = row;
			_tableModel->insertRow(row, item);
			tableView->setRowHeight(row, 50);
			tableView->resizeRowsToContents();
			tableView->selectRow(row);
		}
	}
}

void Conductor::populatePatches()
{
	if(!selected)
	{
		_patchModel->clear();
		return;
	}
	MidiTrack* track = (MidiTrack*)selected;
    //if (track->wantsAutomation())
    //    return;
	int channel = track->outChannel();
	int port = track->outPort();
	MidiInstrument* instr = midiPorts[port].instrument();
    if (instr && _patchModel/* && instr != genericMidiInstrument*/)
        instr->populatePatchModel(_patchModel, channel, song->mtype(), track->type() == Track::DRUM);
}

void Conductor::patchDoubleClicked(QModelIndex index)/*{{{*/
{
	if(!selected)
		return;
	QStandardItem* nItem = _patchModel->itemFromIndex(index);//item(row, 0);

	if(nItem->hasChildren()) //Its a folder perform expand collapse
	{
		patchList->setExpanded(index, !patchList->isExpanded(index));
	}
	else
	{
		int row = nItem->row();
		QStandardItem* p = nItem->parent();
		QStandardItem *idItem;
		QString pg = "";
		if(p && p != _patchModel->invisibleRootItem() && p->columnCount() == 2)
		{
			//We are in group mode
			idItem = p->child(row, 1);
			pg = p->text();
		}
		else
		{
			idItem = _patchModel->item(row, 1);
		}
		int id = idItem->text().toInt();
		QString name = nItem->text();
		//printf("Found patch Name: %s - ID: %d\n",name.toUtf8().constData(), id);

		if (!name.isEmpty() && id >= 0)
		{
			MidiTrack* track = (MidiTrack*) selected;
			int channel = track->outChannel();
			int port = track->outPort();

			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id, selected);
			audio->msgPlayMidiEvent(&ev);

			if (!pg.isEmpty())
				pg = pg + ": \n  ";
			QString label = "  " + pg + name;


			QStandardItem* patch = new QStandardItem(label);
			patch->setData(idItem->text(), ConductorPatchIdRole);
			patch->setToolTip(label);
			patch->setEditable(false);
			patch->setCheckable(true);
			patch->setCheckState(Qt::Unchecked);

			int trow = _tableModel->rowCount();
			_selectedIndex = trow;
			_useMatrix = false;
			_tableModel->insertRow(trow, patch);
			tableView->setRowHeight(trow, 50);
			tableView->resizeRowsToContents();
			updateConductor(-1);
			updateTableHeader();
		}
	}
}/*}}}*/

void Conductor::patchClicked(QModelIndex index)/*{{{*/
{
	if(!selected)
		return;
	QStandardItem* nItem = _patchModel->itemFromIndex(index);//item(row, 0);

	if(!nItem->hasChildren())
	{
		int row = nItem->row();
		QStandardItem* p = nItem->parent();
		QStandardItem *idItem;
		QString pg = "";
		if(p && p != _patchModel->invisibleRootItem() && p->columnCount() == 2)
		{
			//We are in group mode
			idItem = p->child(row, 1);
			pg = p->text();
		}
		else
		{
			idItem = _patchModel->item(row, 1);
		}
		int id = idItem->text().toInt();
		QString name = nItem->text();
		//printf("Found patch Name: %s - ID: %d\n",name.toUtf8().constData(), id);

		if (!name.isEmpty() && id >= 0)
		{
			MidiTrack* track = (MidiTrack*) selected;
			int channel = track->outChannel();
			int port = track->outPort();

			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id);
			audio->msgPlayMidiEvent(&ev);
		}
	}
}/*}}}*/

	//void patchSelectionChanged(QItemSelection, QItemSelection);
void Conductor::patchSelectionChanged(QItemSelection index, QItemSelection)
{
	patchClicked(index.indexes().at(0));
}

void Conductor::patchSequenceClicked(QModelIndex index)/*{{{*/
{
	if(!selected)
		return;
	QStandardItem* item = _tableModel->itemFromIndex(index);

	if(item)
	{
		int id = item->data(ConductorPatchIdRole).toInt();
		if (id >= 0)
		{
			MidiTrack* track = (MidiTrack*) selected;
			int channel = track->outChannel();
			int port = track->outPort();

			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id, selected);
			audio->msgPlayMidiEvent(&ev);
		}
	}
}/*}}}*/

void Conductor::previewSelectedPatch()
{
	if(!selected)
		return;
	if(_patchSelModel->hasSelection())
	{
		//printf("Conductor::addSelectedPatch()\n");
		patchClicked(_patchSelModel->currentIndex());
	}
}

void Conductor::addSelectedPatch()
{
	if(!selected)
		return;
	if(_patchSelModel->hasSelection())
	{
		//printf("Conductor::addSelectedPatch()\n");
		patchDoubleClicked(_patchSelModel->currentIndex());
	}
}

void Conductor::updateSize()
{
	tableView->resizeRowsToContents();
}

void Conductor::showEvent(QShowEvent* /*evt*/)
{
	tableView->resizeRowsToContents();
	/*if(_autoExapand)
		chkAdvanced->setChecked(false);
	_autoExapand = false;*/
}


