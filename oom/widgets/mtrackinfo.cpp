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
#include <QTableWidget>    
#include <QToolButton>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <values.h>

#include "mtrackinfo.h"
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

#include "gcombo.h"
#include "traverso_shared/TConfig.h"

static int rasterTable[] = {
	//------                8    4     2
	1, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	1, 6, 12, 24, 48, 96, 192, 384, 768, 1536,
	1, 9, 18, 36, 72, 144, 288, 576, 1152, 2304
};

static const char* rasterStrings[] ={
	QT_TRANSLATE_NOOP("@default", "Off"), "2pp", "5pp", "64T", "32T", "16T", "8T", "4T", "2T", "1T",
	QT_TRANSLATE_NOOP("@default", "Off"), "3pp", "6pp", "64", "32", "16", "8", "4", "2", "1",
	QT_TRANSLATE_NOOP("@default", "Off"), "4pp", "7pp", "64.", "32.", "16.", "8.", "4.", "2.", "1."
};

static int quantTable[] = {
	1, 16, 32, 64, 128, 256, 512, 1024,
	1, 24, 48, 96, 192, 384, 768, 1536,
	1, 36, 72, 144, 288, 576, 1152, 2304
};

static const char* quantStrings[] = {
	QT_TRANSLATE_NOOP("@default", "Off"), "64T", "32T", "16T", "8T", "4T", "2T", "1T",
	QT_TRANSLATE_NOOP("@default", "Off"), "64", "32", "16", "8", "4", "2", "1",
	QT_TRANSLATE_NOOP("@default", "Off"), "64.", "32.", "16.", "8.", "4.", "2.", "1."
};

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void MidiTrackInfo::setTrack(Track* t)
{
	if (!t)
	{
		selected = 0;
		return;
	}

	if (!t->isMidiTrack())
		return;
	selected = t;

	QPalette pal;
	if (selected->type() == Track::DRUM)
		pal.setColor(trackNameLabel->backgroundRole(), config.drumTrackLabelBg);
	else
		pal.setColor(trackNameLabel->backgroundRole(), config.midiTrackLabelBg);
	trackNameLabel->setPalette(pal);

	populatePatches();
	updateTrackInfo(-1);
	//printf("Calling populate matrix from setTrack()\n");
	populateMatrix();
	rebuildMatrix();
}

//---------------------------------------------------------
//   midiTrackInfo
//---------------------------------------------------------

MidiTrackInfo::MidiTrackInfo(QWidget* parent, Track* sel_track, int rast, int quant) : QFrame(parent)//QWidget(parent)
{
	setupUi(this);
	_midiDetect = false;
	_progRowNum = 0;
	_selectedIndex = 0;
	editing = false;
	_useMatrix = false;
	_autoExapand = true;
	_matrix = new QList<int>;
	_tableModel = new ProgramChangeTableModel(this);
	tableView = new ProgramChangeTable(this);
	tableView->setMinimumHeight(150);
	tableBox->addWidget(tableView);
	_selModel = new QItemSelectionModel(_tableModel);//tableView->selectionModel();
	_patchModel = new QStandardItemModel(0, 2, this);
	_patchSelModel = new QItemSelectionModel(_patchModel);

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
	recEchoIconSet.addPixmap(*midiThruOnIcon, QIcon::Normal, QIcon::On);
	recEchoIconSet.addPixmap(*midiThruOffIcon, QIcon::Normal, QIcon::Off);
	recEchoButton->setIcon(recEchoIconSet);
	recEchoButton->setIconSize(midiThruOnIcon->size());

	// OOMidi-2: AlignCenter and WordBreak are set in the ui(3) file, but not supported by QLabel. Turn them on here.
	trackNameLabel->setAlignment(Qt::AlignCenter);

	if (selected)
	{
		trackNameLabel->setObjectName(selected->cname());
		QPalette pal;
		//pal.setColor(trackNameLabel->backgroundRole(), QColor(0, 160, 255)); // Med blue
		if (selected->type() == Track::DRUM)
			pal.setColor(trackNameLabel->backgroundRole(), config.drumTrackLabelBg);
		else
			pal.setColor(trackNameLabel->backgroundRole(), config.midiTrackLabelBg);
		trackNameLabel->setPalette(pal);
	}
	//else
	//{
	//  pal.setColor(trackNameLabel->backgroundRole(), config.midiTrackLabelBg);
	//  trackNameLabel->setPalette(pal);
	//}

	//trackNameLabel->setStyleSheet(QString("background-color: ") + QColor(0, 160, 255).name()); // Med blue
	trackNameLabel->setWordWrap(true);
	trackNameLabel->setAutoFillBackground(true);
	trackNameLabel->setTextFormat(Qt::PlainText);
	trackNameLabel->setLineWidth(2);
	trackNameLabel->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
	trackNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));

	setLabelText();
	setLabelFont();


	tableView->setModel(_tableModel);
	tableView->setSelectionModel(_selModel);
	tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
	updateTableHeader();

	patchList->setModel(_patchModel);
	patchList->setSelectionModel(_patchSelModel);

	//HTMLDelegate *delegate = new HTMLDelegate;
	//tableView->setItemDelegateForColumn(1, delegate);

	btnUp->setIcon(*upPCIcon);
	btnDown->setIcon(*downPCIcon);
	btnDelete->setIcon(*garbagePCIcon);
	btnUp->setIconSize(upPCIcon->size());
	btnDown->setIconSize(downPCIcon->size());
	btnDelete->setIconSize(garbagePCIcon->size());
	btnCopy->setIcon(*duplicatePCIcon);
	btnCopy->setIconSize(duplicatePCIcon->size());

	//start tb1 merge
	//---------------------------------------------------
	//  Raster, Quant.
	//---------------------------------------------------

    rasterLabel = new GridCombo(this);
    quantLabel = new GridCombo(this);

	rlist = new QTableWidget(10, 3);
	qlist = new QTableWidget(8, 3);
	rlist->verticalHeader()->setDefaultSectionSize(22);
	rlist->horizontalHeader()->setDefaultSectionSize(32);
	rlist->setSelectionMode(QAbstractItemView::SingleSelection);
	rlist->verticalHeader()->hide();
	rlist->horizontalHeader()->hide();
	qlist->verticalHeader()->setDefaultSectionSize(22);
	qlist->horizontalHeader()->setDefaultSectionSize(32);
	qlist->setSelectionMode(QAbstractItemView::SingleSelection);
	qlist->verticalHeader()->hide();
	qlist->horizontalHeader()->hide();

	rlist->setMinimumWidth(96);
	qlist->setMinimumWidth(96);

	rasterLabel->setView(rlist);
	quantLabel->setView(qlist);

	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 10; i++)
			rlist->setItem(i, j, new QTableWidgetItem(tr(rasterStrings[i + j * 10])));
	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 8; i++)
			qlist->setItem(i, j, new QTableWidgetItem(tr(quantStrings[i + j * 8])));

	setRaster(rast);
	setQuant(quant);

    rasterLabel->setFixedHeight(22);
    quantLabel->setFixedHeight(22);

	controlsBox->addWidget(new QLabel(tr("Snap")));
	controlsBox->addWidget(rasterLabel);
	controlsBox->addWidget(new QLabel(tr("Quantize")));
	controlsBox->addWidget(quantLabel);

	//---------------------------------------------------
	//  To Menu
	//---------------------------------------------------
	QComboBox* toList = new QComboBox;
	toList->setFixedHeight(22);
	toList->insertItem(0, tr("All Events"));
	toList->insertItem(CMD_RANGE_LOOP, tr("Looped Ev."));
	toList->insertItem(CMD_RANGE_SELECTED, tr("Selected Ev."));
	toList->insertItem(CMD_RANGE_LOOP | CMD_RANGE_SELECTED, tr("Looped+Sel."));
	controlsBox->addWidget(new QLabel(tr("To")));
	controlsBox->addWidget(toList);

	connect(rasterLabel, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
	connect(quantLabel, SIGNAL(activated(int)), SLOT(_quantChanged(int)));
	connect(toList, SIGNAL(activated(int)), SIGNAL(toChanged(int)));
	//end tb1

	connect(tableView, SIGNAL(rowOrderChanged()), SLOT(rebuildMatrix()));
	connect(_selModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SLOT(matrixSelectionChanged(QItemSelection, QItemSelection)));
	connect(_tableModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(matrixItemChanged(QStandardItem*)));
	connect(_tableModel, SIGNAL(rowsInserted(QModelIndex, int, int)), SLOT(patchSequenceInserted(QModelIndex, int, int)));
	connect(_tableModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), SLOT(patchSequenceRemoved(QModelIndex, int, int)));
	connect(patchList, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT(patchDoubleClicked(const QModelIndex&) ) );
	connect(chkAdvanced, SIGNAL(stateChanged(int)), SLOT(toggleAdvanced(int)));
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(deleteSelectedPatches(bool)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(movePatchUp(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(movePatchDown(bool)));
	connect(btnCopy, SIGNAL(clicked(bool)), SLOT(clonePatchSequence()));

	//setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));

	connect(iPatch, SIGNAL(released()), SLOT(instrPopup()));

	///pop = new QMenu(iPatch);
	//pop->setCheckable(false); // not needed in Qt4

	// Removed by Tim. p3.3.9
	//connect(iName, SIGNAL(returnPressed()), SLOT(iNameChanged()));

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
	connect(iAnschl, SIGNAL(valueChanged(int)), SLOT(iAnschlChanged(int)));
	connect(iVerz, SIGNAL(valueChanged(int)), SLOT(iVerzChanged(int)));
	connect(iLen, SIGNAL(valueChanged(int)), SLOT(iLenChanged(int)));
	connect(iKompr, SIGNAL(valueChanged(int)), SLOT(iKomprChanged(int)));
	connect(iPan, SIGNAL(valueChanged(int)), SLOT(iPanChanged(int)));
	connect(iPan, SIGNAL(doubleClicked()), SLOT(iPanDoubleClicked()));
	connect(iOutput, SIGNAL(activated(int)), SLOT(iOutputPortChanged(int)));
	///connect(iInput, SIGNAL(textChanged(const QString&)), SLOT(iInputPortChanged(const QString&)));
	connect(recordButton, SIGNAL(clicked()), SLOT(recordClicked()));
	connect(progRecButton, SIGNAL(clicked()), SLOT(progRecClicked()));
	connect(volRecButton, SIGNAL(clicked()), SLOT(volRecClicked()));
	connect(panRecButton, SIGNAL(clicked()), SLOT(panRecClicked()));
	connect(recEchoButton, SIGNAL(toggled(bool)), SLOT(recEchoToggled(bool)));
	connect(iRButton, SIGNAL(pressed()), SLOT(inRoutesPressed()));

	// TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
	//oRButton->setEnabled(false);
	//oRButton->setVisible(false);
	//connect(oRButton, SIGNAL(pressed()), SLOT(outRoutesPressed()));

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));

	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
	bool adv = tconfig().get_property("MidiTrackInfo", "advanced", false).toBool();
	chkAdvanced->setChecked(adv);
}

MidiTrackInfo::~MidiTrackInfo()
{
	tconfig().set_property("MidiTrackInfo", "advanced", chkAdvanced->isChecked());
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiTrackInfo::heartBeat()
{
	///if(!showTrackinfoFlag || !selected)
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
			///int ichMask    = track->inChannelMask();
			//int iptMask    = track->inPortMask();
			///unsigned int iptMask    = track->inPortMask();

			MidiPort* mp = &midiPorts[outPort];

			// Set record echo.
			//if(recEchoButton->isChecked() != track->recEcho())
			//{
			//  recEchoButton->blockSignals(true);
			//  recEchoButton->setChecked(track->recEcho());
			//  recEchoButton->blockSignals(false);
			//}

			// Check for detection of midi general activity on chosen channels...
			int mpt = 0;
			//int mch = 0;
			RouteList* rl = track->inRoutes();

			ciRoute r = rl->begin();
			//for( ; mpt < MIDI_PORTS; ++mpt)
			for (; r != rl->end(); ++r)
			{
				//if(!r->isValid() || ((r->type != Route::ALSA_MIDI_ROUTE) && (r->type != Route::JACK_MIDI_ROUTE)))
				//if(!r->isValid() || (r->type != Route::MIDI_DEVICE_ROUTE))
				if (!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE)) // p3.3.49
					continue;

				// NOTE: TODO: Code for channelless events like sysex, ** IF we end up using the 'special channel 17' method.
				//if(r->channel == -1)
				if (r->channel == -1 || r->channel == 0) // p3.3.50
					continue;

				// No port assigned to the device?
				//mpt = r->device->midiPort();
				mpt = r->midiPort; // p3.3.49
				if (mpt < 0 || mpt >= MIDI_PORTS)
					continue;

				//for(; mch < MIDI_CHANNELS; ++mch)
				//{
				//if(midiPorts[mpt].syncInfo().actDetect(mch) && (iptMask & (1 << mpt)) && (ichMask & (1 << mch)) )
				//if((iptMask & bitShiftLU[mpt]) && (midiPorts[mpt].syncInfo().actDetectBits() & ichMask) )
				//if(midiPorts[mpt].syncInfo().actDetectBits() & bitShiftLU[r->channel])
				if (midiPorts[mpt].syncInfo().actDetectBits() & r->channel) // p3.3.50 Use new channel mask.
				{
					//if(iChanTextLabel->paletteBackgroundColor() != green)
					//  iChanTextLabel->setPaletteBackgroundColor(green);
					//if(iChanDetectLabel->pixmap() != greendotIcon)
					if (!_midiDetect)
					{
						//printf("Arranger::midiTrackInfoHeartBeat setting green icon\n");

						_midiDetect = true;
						//iChanDetectLabel->setPixmap(*greendotIcon);
						iChanDetectLabel->setPixmap(*redLedIcon);
					}
					break;
				}
				//}
			}
			// No activity detected?
			//if(mch == MIDI_CHANNELS)
			//if(mpt == MIDI_PORTS)
			if (r == rl->end())
			{
				//if(iChanTextLabel->paletteBackgroundColor() != darkGreen)
				//  iChanTextLabel->setPaletteBackgroundColor(darkGreen);
				//if(iChanDetectLabel->pixmap() != darkgreendotIcon)
				if (_midiDetect)
				{
					//printf("Arranger::midiTrackInfoHeartBeat setting darkgreen icon\n");

					_midiDetect = false;
					//iChanDetectLabel->setPixmap(*darkgreendotIcon);
					iChanDetectLabel->setPixmap(*darkRedLedIcon);
				}
			}

			int nprogram = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
			if (nprogram == CTRL_VAL_UNKNOWN)
			{
				if (program != CTRL_VAL_UNKNOWN)
				{
					//printf("Arranger::midiTrackInfoHeartBeat setting program to unknown\n");

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
					//const char* n = "<unknown>";
					const QString n(tr("Select Patch"));
					//if(strcmp(iPatch->text().toLatin1().constData(), n) != 0)
					if (iPatch->text() != n)
					{
						//printf("Arranger::midiTrackInfoHeartBeat setting patch <unknown>\n");

						iPatch->setText(n);
					}
				}
				else
				{
					MidiInstrument* instr = mp->instrument();
					QString name = instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM);
					if (name.isEmpty())
					{
						const QString n("???");
						if (iPatch->text() != n)
							iPatch->setText(n);
					}
					else
						if (iPatch->text() != name)
					{
						//printf("Arranger::midiTrackInfoHeartBeat setting patch name\n");

						iPatch->setText(name);
					}
				}
			}
			else
				if (program != nprogram)
			{
				program = nprogram;

				//int hb, lb, pr;
				//if (program == CTRL_VAL_UNKNOWN) {
				//      hb = lb = pr = 0;
				//      iPatch->setText("---");
				//      }
				//else
				//{
				MidiInstrument* instr = mp->instrument();
				QString name = instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM);
				if (iPatch->text() != name)
					iPatch->setText(name);

				int hb = ((program >> 16) & 0xff) + 1;
				if (hb == 0x100)
					hb = 0;
				int lb = ((program >> 8) & 0xff) + 1;
				if (lb == 0x100)
					lb = 0;
				int pr = (program & 0xff) + 1;
				if (pr == 0x100)
					pr = 0;
				//}

				//printf("Arranger::midiTrackInfoHeartBeat setting program\n");

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

			MidiController* mc = mp->midiController(CTRL_VOLUME);
			int mn = mc->minVal();
			int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
			if (v == CTRL_VAL_UNKNOWN)
				//{
				//v = mc->initVal();
				//if(v == CTRL_VAL_UNKNOWN)
				//  v = 0;
				v = mn - 1;
				//}
			else
				// Auto bias...
				v -= mc->bias();
			if (volume != v)
			{
				volume = v;
				if (iLautst->value() != v)
				{
					//printf("Arranger::midiTrackInfoHeartBeat setting volume\n");

					iLautst->blockSignals(true);
					//iLautst->setRange(mn - 1, mc->maxVal());
					iLautst->setValue(v);
					iLautst->blockSignals(false);
				}
			}

			mc = mp->midiController(CTRL_PANPOT);
			mn = mc->minVal();
			v = mp->hwCtrlState(outChannel, CTRL_PANPOT);
			if (v == CTRL_VAL_UNKNOWN)
				//{
				//v = mc->initVal();
				//if(v == CTRL_VAL_UNKNOWN)
				//  v = 0;
				v = mn - 1;
				//}
			else
				// Auto bias...
				v -= mc->bias();
			if (pan != v)
			{
				pan = v;
				if (iPan->value() != v)
				{
					//printf("Arranger::midiTrackInfoHeartBeat setting pan\n");

					iPan->blockSignals(true);
					//iPan->setRange(mn - 1, mc->maxVal());
					iPan->setValue(v);
					iPan->blockSignals(false);
				}
			}

			// Does it include a midi controller value adjustment? Then handle it...
			//if(flags & SC_MIDI_CONTROLLER)
			//  seek();

			/*
			if(iTransp->value() != track->transposition)
			  iTransp->setValue(track->transposition);
			if(iAnschl->value() != track->velocity)
			  iAnschl->setValue(track->velocity);
			if(iVerz->value() != track->delay)
			  iVerz->setValue(track->delay);
			if(iLen->value() != track->len)
			  iLen->setValue(track->len);
			if(iKompr->value() != track->compression)
			  iKompr->setValue(track->compression);
			 */
			QList<PatchSequence*> *list = mp->patchSequences();
			if (_progRowNum != list->size())
			{
				//printf("Calling populate matrix from heartBeat()\n");
				populateMatrix();
				rebuildMatrix();
			}
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
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void MidiTrackInfo::configChanged()
{
	//printf("MidiTrackInfo::configChanged\n");

	//if (config.canvasBgPixmap.isEmpty()) {
	//      canvas->setBg(config.partCanvasBg);
	//      canvas->setBg(QPixmap());
	//}
	//else {
	//      canvas->setBg(QPixmap(config.canvasBgPixmap));
	//}

	setFont(config.fonts[2]);
	//updateTrackInfo(type);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiTrackInfo::songChanged(int type)
{
	//printf("MidiTrackInfo::songChanged() Type: %d\n", type);
	// Is it simply a midi controller value adjustment? Forget it.
	if (type == SC_MIDI_CONTROLLER)
		return;
	if (type == SC_SELECTION)
		return;
	if (!isVisible())
		return;
	if (type == SC_PATCH_UPDATED && !editing)
	{
		//printf("Calling populate matrix from songChanged()\n");
		populateMatrix();
		rebuildMatrix();
		return;
	}
	updateTrackInfo(type);
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void MidiTrackInfo::setLabelText()
{
	MidiTrack* track = (MidiTrack*) selected;
	if (track)
		trackNameLabel->setText(track->name());
	else
		trackNameLabel->setText(QString());
}

//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void MidiTrackInfo::setLabelFont()
{
	//if(!selected)
	//  return;
	//MidiTrack* track = (MidiTrack*)selected;

	// Use the new font #6 I created just for these labels (so far).
	// Set the label's font.
	trackNameLabel->setFont(config.fonts[6]);
	// Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
	autoAdjustFontSize(trackNameLabel, trackNameLabel->text(), false, true, config.fonts[6].pointSize(), 5);
}

//---------------------------------------------------------
//   iOutputChannelChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputChannelChanged(int channel)
{
	--channel;
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
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
	}
}

//---------------------------------------------------------
//   iOutputPortChanged
//---------------------------------------------------------

void MidiTrackInfo::iOutputPortChanged(int index)
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

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

//void MidiTrackInfo::routingPopupMenuActivated(int n)

void MidiTrackInfo::routingPopupMenuActivated(QAction* act)
{
	///if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
	if ((gRoutingPopupMenuMaster != this) || !selected || !selected->isMidiTrack())
		return;
	oom->routingPopupMenuActivated(selected, act->data().toInt());
}

#if 0
//---------------------------------------------------------
//   routingPopupViewActivated
//---------------------------------------------------------

void MidiTrackInfo::routingPopupViewActivated(const QModelIndex& mdi)
{
	///if(!midiTrackInfo || gRoutingPopupMenuMaster != midiTrackInfo || !selected || !selected->isMidiTrack())
	if (gRoutingPopupMenuMaster != this || !selected || !selected->isMidiTrack())
		return;
	oom->routingPopupMenuActivated(selected, mdi.data().toInt());
}
#endif

//---------------------------------------------------------
//   inRoutesPressed
//---------------------------------------------------------

void MidiTrackInfo::inRoutesPressed()
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
			oom->configMidiPorts();
		}
		return;
	}

	///gRoutingPopupMenuMaster = midiTrackInfo;
	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	//connect(pup, SIGNAL(activated(const QModelIndex&)), SLOT(routingPopupViewActivated(const QModelIndex&)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	//connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupViewAboutToHide()));
	pup->popup(QCursor::pos());
	//pup->setVisible(true);
	iRButton->setDown(false);
	return;
}

//---------------------------------------------------------
//   outRoutesPressed
//---------------------------------------------------------

void MidiTrackInfo::outRoutesPressed()
{
	if (!selected)
		return;
	if (!selected->isMidiTrack())
		return;

	PopupMenu* pup = oom->prepareRoutingPopupMenu(selected, true);
	if (!pup)
		return;

	///gRoutingPopupMenuMaster = midiTrackInfo;
	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	pup->popup(QCursor::pos());
	///oRButton->setDown(false);
	return;
}

//---------------------------------------------------------
//   iProgHBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgHBankChanged()
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
	MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
	audio->msgPlayMidiEvent(&ev);

	MidiInstrument* instr = mp->instrument();
	iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	//      updateTrackInfo();
}

//---------------------------------------------------------
//   iProgLBankChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgLBankChanged()
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
	MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
	audio->msgPlayMidiEvent(&ev);

	MidiInstrument* instr = mp->instrument();
	iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	//      updateTrackInfo();
}

//---------------------------------------------------------
//   iProgramChanged
//---------------------------------------------------------

void MidiTrackInfo::iProgramChanged()
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
		MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, program);
		audio->msgPlayMidiEvent(&ev);

		MidiInstrument* instr = mp->instrument();
		iPatch->setText(instr->getPatchName(channel, program, song->mtype(), track->type() == Track::DRUM));
	}

	//      updateTrackInfo();
}

//---------------------------------------------------------
//   iLautstChanged
//---------------------------------------------------------

void MidiTrackInfo::iLautstChanged(int val)
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

		MidiPlayEvent ev(0, outPort, chan,
				ME_CONTROLLER, CTRL_VOLUME, val);
		audio->msgPlayMidiEvent(&ev);
	}
	song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   iTranspChanged
//---------------------------------------------------------

void MidiTrackInfo::iTranspChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->transposition = val;
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iAnschlChanged
//---------------------------------------------------------

void MidiTrackInfo::iAnschlChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->velocity = val;
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iVerzChanged
//---------------------------------------------------------

void MidiTrackInfo::iVerzChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->delay = val;
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iLenChanged
//---------------------------------------------------------

void MidiTrackInfo::iLenChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->len = val;
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iKomprChanged
//---------------------------------------------------------

void MidiTrackInfo::iKomprChanged(int val)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->compression = val;
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iPanChanged
//---------------------------------------------------------

void MidiTrackInfo::iPanChanged(int val)
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
		MidiPlayEvent ev(0, port, chan,
				ME_CONTROLLER, CTRL_PANPOT, val);
		audio->msgPlayMidiEvent(&ev);
	}
	song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void MidiTrackInfo::instrPopup()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int channel = track->outChannel();
	int port = track->outPort();
	MidiInstrument* instr = midiPorts[port].instrument();
	QMenu* pup = new QMenu;
	///instr->populatePatchPopup(pop, channel, song->mtype(), track->type() == Track::DRUM);
	instr->populatePatchPopup(pup, channel, song->mtype(), track->type() == Track::DRUM);

	///if(pop->actions().count() == 0)
	///  return;
	if (pup->actions().count() == 0)
	{
		delete pup;
		return;
	}

	QAction *act = pup->exec(iPatch->mapToGlobal(QPoint(10, 5)));
	if (act)
	{
		//int rv = act->data().toInt();
		QVariant _data = act->data();
		QStringList lst = _data.toStringList();
		if (!lst.isEmpty())
		{
			QString str = lst.at(0);
			QString pg = ""; //lst.at(1);
			int rv = str.toInt();

			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, rv);
			audio->msgPlayMidiEvent(&ev);

			//At this point we add the event to the list.
			if (lst.size() > 1)
			{
				pg = lst.at(1);
			}
			if (!pg.isEmpty())
				pg = pg + ": \n  ";
			QString label = "  " + pg + act->text();
			/*for (int i = 0; i < _tableModel->rowCount(); ++i)
			{
				QStandardItem* item = _tableModel->item(i, 1);
				item->setCheckState(Qt::Unchecked);
			}*/
			QList<QStandardItem*> rowData;
			QStandardItem* chk = new QStandardItem(true);
			chk->setCheckable(true);
			chk->setCheckState(Qt::Unchecked);
			chk->setToolTip(tr("Add to patch sequence"));

			QStandardItem* patch = new QStandardItem(label);
			patch->setToolTip(label);
			patch->setEditable(false);
			rowData.append(new QStandardItem(str));
			rowData.append(chk);
			rowData.append(patch);

			int row = _tableModel->rowCount();
			_selectedIndex = row;
			_useMatrix = false;
			_tableModel->insertRow(row, rowData);
			tableView->setRowHeight(row, 50);
			tableView->resizeRowsToContents();
			updateTrackInfo(-1);
			updateTableHeader();
			//_selModel->blockSignals(true);
			//printf("Calling selectedRow after insert\n");
			//tableView->selectRow(row);
			//_selModel->blockSignals(false);
		}
	}

	delete pup;
}

//---------------------------------------------------------
//   recEchoToggled
//---------------------------------------------------------

void MidiTrackInfo::recEchoToggled(bool v)
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	track->setRecEcho(v);
	song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   iProgramDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iProgramDoubleClicked()
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
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, kiv);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PROGRAM, lastv);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_PROGRAM) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_PROGRAM, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   iLautstDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iLautstDoubleClicked()
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

			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, kiv);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_VOLUME, lastv);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_VOLUME) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_VOLUME, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   iPanDoubleClicked
//---------------------------------------------------------

void MidiTrackInfo::iPanDoubleClicked()
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

			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, kiv);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			MidiPlayEvent ev(0, port, chan, ME_CONTROLLER, CTRL_PANPOT, lastv);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, CTRL_PANPOT) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, CTRL_PANPOT, CTRL_VAL_UNKNOWN);
	}

	song->update(SC_MIDI_CONTROLLER);
}


//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void MidiTrackInfo::updateTrackInfo(int flags)
{
	//printf("MidiTrackInfo::updateTrackInfo(%d) called\n", flags);
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;
	if (flags == SC_SELECTION)
		return;

	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;

	// p3.3.47 Update the routing popup menu if anything relevant changes.
	//if(gRoutingPopupMenuMaster == midiTrackInfo && selected && (flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG)))
	if (flags & (SC_ROUTE | SC_CHANNELS | SC_CONFIG)) // p3.3.50
		// Use this handy shared routine.
		//oom->updateRouteMenus(selected);
		///oom->updateRouteMenus(selected, midiTrackInfo);   // p3.3.50
		oom->updateRouteMenus(selected, this);

	// Added by Tim. p3.3.9
	setLabelText();
	setLabelFont();

	if (flags & (SC_MIDI_TRACK_PROP))
	{
		iTransp->blockSignals(true);
		iAnschl->blockSignals(true);
		iVerz->blockSignals(true);
		iLen->blockSignals(true);
		iKompr->blockSignals(true);
		iTransp->setValue(track->transposition);
		iAnschl->setValue(track->velocity);
		iVerz->setValue(track->delay);
		iLen->setValue(track->len);
		iKompr->setValue(track->compression);
		iTransp->blockSignals(false);
		iAnschl->blockSignals(false);
		iVerz->blockSignals(false);
		iLen->blockSignals(false);
		iKompr->blockSignals(false);

		int outChannel = track->outChannel();
		///int inChannel  = track->inChannelMask();
		int outPort = track->outPort();
		//int inPort     = track->inPortMask();
		///unsigned int inPort     = track->inPortMask();

		iOutput->blockSignals(true);
		//iInput->clear();
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

		//iInput->setText(bitmap2String(inPort));
		///iInput->setText(u32bitmap2String(inPort));

		//iInputChannel->setText(bitmap2String(inChannel));

		// Removed by Tim. p3.3.9
		//if (iName->text() != selected->name()) {
		//      iName->setText(selected->name());
		//      iName->home(false);
		//      }

		iOutputChannel->blockSignals(true);
		iOutputChannel->setValue(outChannel + 1);
		iOutputChannel->blockSignals(false);
		///iInputChannel->setText(bitmap2String(inChannel));

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
			//iPatch->setText(QString("<unknown>"));
			iPatch->setText(tr("Select Patch"));
		else
		{
			MidiInstrument* instr = mp->instrument();
			iPatch->setText(instr->getPatchName(outChannel, nprogram, song->mtype(), track->type() == Track::DRUM));
		}
	}
	else
		//if (program != nprogram)
	{
		program = nprogram;

		//int hb, lb, pr;
		//if (program == CTRL_VAL_UNKNOWN) {
		//      hb = lb = pr = 0;
		//      iPatch->setText("---");
		//      }
		//else
		//{
		MidiInstrument* instr = mp->instrument();
		iPatch->setText(instr->getPatchName(outChannel, program, song->mtype(), track->type() == Track::DRUM));

		int hb = ((program >> 16) & 0xff) + 1;
		if (hb == 0x100)
			hb = 0;
		int lb = ((program >> 8) & 0xff) + 1;
		if (lb == 0x100)
			lb = 0;
		int pr = (program & 0xff) + 1;
		if (pr == 0x100)
			pr = 0;
		//}
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

	MidiController* mc = mp->midiController(CTRL_VOLUME);
	int mn = mc->minVal();
	int v = mp->hwCtrlState(outChannel, CTRL_VOLUME);
	volume = v;
	if (v == CTRL_VAL_UNKNOWN)
		//{
		//v = mc->initVal();
		//if(v == CTRL_VAL_UNKNOWN)
		//  v = 0;
		v = mn - 1;
		//}
	else
		// Auto bias...
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
		//{
		//v = mc->initVal();
		//if(v == CTRL_VAL_UNKNOWN)
		//  v = 0;
		v = mn - 1;
		//}
	else
		// Auto bias...
		v -= mc->bias();
	iPan->blockSignals(true);
	iPan->setRange(mn - 1, mc->maxVal());
	iPan->setValue(v);
	iPan->blockSignals(false);
	//}
	//printf("Before populateMatrix()\n");
	//populateMatrix();
	//rebuildMatrix();
}

//---------------------------------------------------------
//   progRecClicked
//---------------------------------------------------------

void MidiTrackInfo::progRecClicked()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
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

	song->recordEvent(track, a);
}

//---------------------------------------------------------
//   volRecClicked
//---------------------------------------------------------

void MidiTrackInfo::volRecClicked()
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

void MidiTrackInfo::panRecClicked()
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

void MidiTrackInfo::recordClicked()
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

void MidiTrackInfo::toggleAdvanced(int checked)
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

void MidiTrackInfo::rebuildMatrix()
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
					QStandardItem *item = _tableModel->item(i, 1);
					if (item && item->checkState() == Qt::Checked)
					{
						_matrix->append(item->row());
					}
				}
				for(int i = 0; i < row; ++i)
				{
					QStandardItem *item = _tableModel->item(i, 1);
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

void MidiTrackInfo::matrixItemChanged(QStandardItem* item)
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
		if (list && !list->isEmpty() && row < list->size() && item->column() == 1)
		{
			PatchSequence *ps = list->at(row);
			if (ps)
			{
				ps->selected = item->checkState() == Qt::Checked ? true : false;
				_useMatrix = ps->selected;
			}
			//updateTrackInfo(-1);
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

void MidiTrackInfo::insertMatrixEvent()
{
	if (!selected)
		return;
	MidiTrack* track = (MidiTrack*) selected;
	int channel = track->outChannel();
	int port = track->outPort();
	//printf("MidiTrackInfo::insertMatrixEvent() _matrix->size() = %d\n", _matrix->size());
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
				return; //Nothing is selected and we are in selection mode
			//printf("MidiTrackInfo::insertMatrixEvent() not using matrix\n");
		}
		else
			row = _matrix->at(0);
		QStandardItem* item = _tableModel->item(row, 0);
		int id = item->text().toInt();
		MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id);
		audio->msgPlayMidiEvent(&ev);
		_selectedIndex = item->row();
		updateTrackInfo(-1);
		//_selModel->blockSignals(true);
		tableView->selectRow(item->row());
		//_selModel->blockSignals(false);
		progRecClicked();
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
			QStandardItem* item = _tableModel->item(row, 0);
			int id = item->text().toInt();
			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id);
			audio->msgPlayMidiEvent(&ev);
			updateTrackInfo(-1);
			progRecClicked();
		}
		_matrix->push_back(row);
	}
}

void MidiTrackInfo::populateMatrix()
{
	//printf("MidiTrackInfo::populateMatrix() entering\n");
	_tableModel->clear();
	if (!selected)
		return;
	//printf("MidiTrackInfo::populateMatrix() found track\n");
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
				//printf("MidiTrackInfo::populateMatrix() found preset: %d\n", i);
				QList<QStandardItem*> rowData;
				PatchSequence* p = ps->at(i);
				QStandardItem *id = new QStandardItem(QString::number(p->id));
				id->setEditable(false);
				QStandardItem *patch = new QStandardItem(p->name);
				patch->setToolTip(p->name);
				patch->setEditable(false);
				QStandardItem *chk = new QStandardItem(p->selected);
				chk->setEditable(false);
				chk->setCheckable(true);
				if (p->selected)
				{
					//_useMatrix = true;
					chk->setCheckState(Qt::Checked);
				}
				else
					chk->setCheckState(Qt::Unchecked);
				chk->setToolTip(tr("Add to patch sequence"));
				rowData.append(id);
				rowData.append(chk);
				rowData.append(patch);
				_tableModel->blockSignals(true);
				_tableModel->insertRow(_tableModel->rowCount(), rowData);
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

void MidiTrackInfo::deleteSelectedPatches(bool)
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

void MidiTrackInfo::movePatchDown(bool)
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id + 1) >= _tableModel->rowCount())
			return;
		int row = (id + 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		QStandardItem* txt = item.at(2);
		txt->setEditable(false);
		_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->setColumnWidth(1, 20);
		tableView->setColumnWidth(0, 1);
		//_selModel->blockSignals(true);
		tableView->selectRow(row);
		//_selModel->blockSignals(false);
	}
}

void MidiTrackInfo::movePatchUp(bool)
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id - 1) < 0)
			return;
		int row = (id - 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		QStandardItem* txt = item.at(2);
		txt->setEditable(false);
		_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->setColumnWidth(1, 20);
		tableView->setColumnWidth(0, 1);
		//_selModel->blockSignals(true);
		tableView->selectRow(row);
		//_selModel->blockSignals(false);
	}
}

void MidiTrackInfo::updateTableHeader()
{
	QStandardItem* hid = new QStandardItem(tr("I"));
	QStandardItem* hstat = new QStandardItem(true);
	hstat->setCheckable(true);
	hstat->setCheckState(Qt::Unchecked);
	QStandardItem* hpatch = new QStandardItem(tr("Patch"));
	_tableModel->setHorizontalHeaderItem(0, hid);
	_tableModel->setHorizontalHeaderItem(1, hstat);
	_tableModel->setHorizontalHeaderItem(2, hpatch);
	tableView->setColumnWidth(1, 20);
	tableView->setColumnHidden(0, true);
	tableView->horizontalHeader()->setStretchLastSection(true);

	//update the patchList headers as well
	QStandardItem* pid = new QStandardItem("");
	QStandardItem* patch = new QStandardItem(tr("Select Patch"));
	patch->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	_patchModel->setHorizontalHeaderItem(0, patch);
	_patchModel->setHorizontalHeaderItem(1, pid);
	//patchList->setColumnWidth(0, 1);
	patchList->setColumnHidden(1, true);
}

void MidiTrackInfo::patchSequenceInserted(QModelIndex /*index*/, int start, int end)
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
			QStandardItem* id = _tableModel->item(i, 0);
			QStandardItem* chk = _tableModel->item(i, 1);
			QStandardItem* patch = _tableModel->item(i, 2);
			PatchSequence* ps = new PatchSequence();
			if (id && chk && patch)
			{
				ps->id = id->text().toInt();
				ps->name = patch->text();
				ps->selected = false;
				if (chk->checkState() == Qt::Checked)
					ps->selected = true;
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

void MidiTrackInfo::patchSequenceRemoved(QModelIndex /*index*/, int start, int end)
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

void MidiTrackInfo::matrixSelectionChanged(QItemSelection sel, QItemSelection)
{
	//if(sel == unsel)
	//	return;
	QModelIndexList ind = sel.indexes();
	if(ind.isEmpty())
		return;
	QModelIndex inx = ind.at(0);
	int row = inx.row();
	QStandardItem* item = _tableModel->item(row, 1);
	if(item)
	{
		_selectedIndex = row;
		if(item->checkState() == Qt::Checked)
		{
			_useMatrix = true;
			rebuildMatrix();
		}
		else
		{
			_useMatrix = false;
			rebuildMatrix();
		}
	}
}

void MidiTrackInfo::clonePatchSequence()
{
	QList<int> rows = tableView->getSelectedRows();
	if (!rows.isEmpty())
	{
		int start = rows.at(0);
		int row = (start + 1);
		QStandardItem* iid = _tableModel->item(start, 0);
		QStandardItem* ichk = _tableModel->item(start, 1);
		QStandardItem* iname = _tableModel->item(start, 2);
		if(iid && ichk && iname)
		{
			QList<QStandardItem*> item;

			QStandardItem* id = new QStandardItem();
			id->setText(iid->text());
			//id->setToolTip(iid->toolTip());
			
			QStandardItem* chk = new QStandardItem();
			chk->setEditable(ichk->isEditable());
			chk->setCheckable(ichk->isCheckable());
			chk->setCheckState(ichk->checkState());

			
			QStandardItem* name = new QStandardItem();
			name->setEditable(iname->isEditable());
			name->setText(iname->text());
			name->setToolTip(iname->toolTip());
			
			item.append(id);
			item.append(chk);
			item.append(name);
			_selectedIndex = row;
			_tableModel->insertRow(row, item);
			tableView->setRowHeight(row, 50);
			tableView->resizeRowsToContents();
			tableView->setColumnWidth(1, 20);
			tableView->setColumnWidth(0, 1);
			//_selModel->blockSignals(true);
			tableView->selectRow(row);
			//_selModel->blockSignals(false);
		}
	}
}

void MidiTrackInfo::populatePatches()
{
	if(!selected)
	{
		_patchModel->clear();
		return;
	}
	MidiTrack* track = (MidiTrack*)selected;
	int channel = track->outChannel();
	int port = track->outPort();
	MidiInstrument* instr = midiPorts[port].instrument();
	instr->populatePatchModel(_patchModel, channel, song->mtype(), track->type() == Track::DRUM);
}

void MidiTrackInfo::patchDoubleClicked(QModelIndex index)
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

		if (!name.isEmpty() && id)
		{
			MidiTrack* track = (MidiTrack*) selected;
			int channel = track->outChannel();
			int port = track->outPort();

			MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, id);
			audio->msgPlayMidiEvent(&ev);

			if (!pg.isEmpty())
				pg = pg + ": \n  ";
			QString label = "  " + pg + name;

			QList<QStandardItem*> rowData;
			QStandardItem* chk = new QStandardItem(true);
			chk->setCheckable(true);
			chk->setCheckState(Qt::Unchecked);
			chk->setToolTip(tr("Add to patch sequence"));

			QStandardItem* patch = new QStandardItem(label);
			patch->setToolTip(label);
			patch->setEditable(false);
			rowData.append(new QStandardItem(idItem->text()));
			rowData.append(chk);
			rowData.append(patch);

			int trow = _tableModel->rowCount();
			_selectedIndex = trow;
			_useMatrix = false;
			_tableModel->insertRow(trow, rowData);
			tableView->setRowHeight(trow, 50);
			tableView->resizeRowsToContents();
			updateTrackInfo(-1);
			updateTableHeader();
		}
	}
}

void MidiTrackInfo::updateSize()
{
	tableView->resizeRowsToContents();
}

void MidiTrackInfo::showEvent(QShowEvent* /*evt*/)
{
	tableView->resizeRowsToContents();
	/*if(_autoExapand)
		chkAdvanced->setChecked(false);
	_autoExapand = false;*/
}

//---------------------------------------------------------/*{{{*/
//   rasterChanged
//---------------------------------------------------------

void MidiTrackInfo::_rasterChanged(int /*i*/)
{
	emit rasterChanged(rasterTable[rlist->currentRow() + rlist->currentColumn() * 10]);
}

//---------------------------------------------------------
//   quantChanged
//---------------------------------------------------------

void MidiTrackInfo::_quantChanged(int /*i*/)
{
	emit quantChanged(quantTable[qlist->currentRow() + qlist->currentColumn() * 8]);
}

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void MidiTrackInfo::setRaster(int val)
{
	for (unsigned i = 0; i < sizeof (rasterTable) / sizeof (*rasterTable); i++)
	{
		if (val == rasterTable[i])
		{
			rasterLabel->setCurrentIndex(i);
			return;
		}
	}
	printf("setRaster(%d) not defined\n", val);
	rasterLabel->setCurrentIndex(0);
}

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void MidiTrackInfo::setQuant(int val)
{
	for (unsigned i = 0; i < sizeof (quantTable) / sizeof (*quantTable); i++)
	{
		if (val == quantTable[i])
		{
			quantLabel->setCurrentIndex(i);
			return;
		}
	}
	printf("setQuant(%d) not defined\n", val);
	quantLabel->setCurrentIndex(0);
}/*}}}*/

