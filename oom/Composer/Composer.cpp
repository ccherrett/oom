//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: Composer.cpp,v 1.33.2.21 2009/11/17 22:08:22 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "awl/sigedit.h"

#include "config.h"

#include <stdio.h>
#include <values.h>

#include <QtGui>
#include <QMainWindow>

#include "Composer.h"
#include "song.h"
#include "app.h"
#include "mtscale.h"
#include "sigscale.h"
//#include "sigedit.h"
#include "scrollscale.h"
#include "ComposerCanvas.h"
#include "xml.h"
#include "splitter.h"
#include "lcombo.h"
#include "conductor/Conductor.h"
#include "midiport.h"
#include "mididev.h"
#include "utils.h"
#include "globals.h"
#include "HeaderList.h"
#include "icons.h"
#include "utils.h"
#include "audio.h"
#include "event.h"
#include "midiseq.h"
#include "midictrl.h"
#include "mpevent.h"
#include "gconfig.h"
#include "spinbox.h"
#include "tvieweditor.h"
#include "traverso_shared/TConfig.h"
#include "tviewdock.h"
#include "commentdock.h"
#include "rmap.h"
#include "shortcuts.h"
#include "mixerdock.h"
#include "toolbars/tools.h"
#include "ClipList/AudioClipList.h"
#include "TimeHeader.h"
#include "TempoHeader.h"
#include "tempolabel.h"
#include "DoubleSlider.h"

static int rasterTable[] = {
	1, 0, 768, 384, 192, 96
};

//---------------------------------------------------------
//   Composer
//    is the central widget in app
//---------------------------------------------------------

Composer::Composer(QMainWindow* parent, const char* name)
: QWidget(parent)
{
	setObjectName(name);
	_raster = 0; // measure
	selected = 0;
	setMinimumSize(600, 50);

	cursVal = MAXINT;
	
	m_tempoStart = tconfig().get_property("TempoRange", "start", "80.0").toDouble();
	m_tempoEnd = tconfig().get_property("TempoRange", "end", "180.0").toDouble();
	m_startTempo = 0;
	m_endTempo = 0;
	m_headerToolBox = 0;
	m_sigEdit = 0;
	m_tempoRange = 0;

	//setFocusPolicy(Qt::StrongFocus);

	//---------------------------------------------------
	//  ToolBar
	//    create toolbar in toplevel widget
	//---------------------------------------------------

	parent->addToolBarBreak();
	QToolBar* toolbar = parent->addToolBar(tr("The Composer Settings"));
	toolbar->setObjectName("tbComposer");
	toolbar->setMovable(false);
	toolbar->setFloatable(false);
	QToolBar* toolbar2 = new QToolBar(tr("Snap"));
	parent->addToolBar(Qt::BottomToolBarArea, toolbar2);
	toolbar2->setObjectName("tbSnap");
	toolbar2->setMovable(false);
	toolbar2->setFloatable(false);
	toolbar2->setFixedHeight(24);
    ((OOMidi*)parent)->setComposerAndSnapToolbars(toolbar, toolbar2);

	_rtabs = new QTabWidget(oom->resourceDock());
	_rtabs->setObjectName("tabControlCenter");
	_rtabs->setTabPosition(QTabWidget::West);
	_rtabs->setTabShape(QTabWidget::Triangular);
	//_rtabs->setMinimumSize(QSize(200, 150));
	_rtabs->setMinimumWidth(250);
	connect(_rtabs, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
	QWidget* dockWidget = new QWidget(this);
	QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
	dockLayout->setContentsMargins(0,0,0,0);
	QLabel* logoLabel = new QLabel(dockWidget);
	logoLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	logoLabel->setPixmap(QPixmap(":/images/icons/oomidi_icon_the_composer.png"));
	dockLayout->addWidget(logoLabel);
	dockLayout->addWidget(_rtabs);
	oom->resourceDock()->setWidget(dockWidget);
	connect(oom->resourceDock(), SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), SLOT(resourceDockAreaChanged(Qt::DockWidgetArea)));


	/*cursorPos = new PosLabel(0);
	cursorPos->setEnabled(false);
	cursorPos->setFixedHeight(22);
	cursorPos->setObjectName("composerCursor");
	toolbar2->addWidget(cursorPos);*/

	const char* rastval[] = {
		QT_TRANSLATE_NOOP("@default", "Off"), QT_TRANSLATE_NOOP("@default", "Bar"), "1/2", "1/4", "1/8", "1/16"
	};

    raster = new QComboBox();
	for (int i = 0; i < 6; i++)
		raster->insertItem(i, tr(rastval[i]));
	raster->setCurrentIndex(1);
	// Set the audio record part snapping. Set to 0 (bar), the same as this combo box intial raster.
	song->setComposerRaster(0);
	toolbar2->addWidget(raster);
	connect(raster, SIGNAL(activated(int)), SLOT(_setRaster(int)));
	connect(raster, SIGNAL(currentIndexChanged(int)), SLOT(_setRaster(int)));
	connect(raster, SIGNAL(activated(int)), oom, SLOT(setRaster(int)));
	connect(raster, SIGNAL(currentIndexChanged(int)), oom, SLOT(setRaster(int)));
	///raster->setFocusPolicy(Qt::NoFocus);
	raster->setFocusPolicy(Qt::TabFocus);

	// Song len
	QLabel* label = new QLabel(tr("Len"));
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setIndent(3);
	toolbar->addWidget(label);

	// song length is limited to 10000 bars; the real song len is limited
	// by overflows in tick computations
	//
	lenEntry = new SpinBox(1, 10000, 1);
	lenEntry->setValue(song->len());
	lenEntry->setToolTip(tr("song length - bars"));
	lenEntry->setWhatsThis(tr("song length - bars"));
	toolbar->addWidget(lenEntry);
	connect(lenEntry, SIGNAL(valueChanged(int)), SLOT(songlenChanged(int)));

	typeBox = new LabelCombo(tr("Type"), 0);
	typeBox->insertItem(0, tr("NO"));
	typeBox->insertItem(1, tr("GM"));
	typeBox->insertItem(2, tr("GS"));
	typeBox->insertItem(3, tr("XG"));
	typeBox->setCurrentIndex(0);
	typeBox->setToolTip(tr("midi song type"));
	typeBox->setWhatsThis(tr("midi song type"));
	///typeBox->setFocusPolicy(Qt::NoFocus);
	typeBox->setFocusPolicy(Qt::TabFocus);
	toolbar->addWidget(typeBox);
	connect(typeBox, SIGNAL(activated(int)), SLOT(modeChange(int)));

	label = new QLabel(tr("Pitch"));
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setIndent(3);
	toolbar->addWidget(label);

	globalPitchSpinBox = new SpinBox(-127, 127, 1);
	globalPitchSpinBox->setValue(song->globalPitchShift());
	globalPitchSpinBox->setToolTip(tr("midi pitch"));
	globalPitchSpinBox->setWhatsThis(tr("global midi pitch shift"));
	toolbar->addWidget(globalPitchSpinBox);
	connect(globalPitchSpinBox, SIGNAL(valueChanged(int)), SLOT(globalPitchChanged(int)));

	label = new QLabel(tr("Tempo"));
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setIndent(3);
	toolbar->addWidget(label);

	globalTempoSpinBox = new SpinBox(50, 200, 1, toolbar);
	globalTempoSpinBox->setSuffix(QString("%"));
	globalTempoSpinBox->setValue(tempomap.globalTempo());
	globalTempoSpinBox->setToolTip(tr("midi tempo"));
	globalTempoSpinBox->setWhatsThis(tr("midi tempo"));
	toolbar->addWidget(globalTempoSpinBox);
	connect(globalTempoSpinBox, SIGNAL(valueChanged(int)), SLOT(globalTempoChanged(int)));

	QToolButton* tempo50 = new QToolButton();
	tempo50->setText(QString("50%"));
	toolbar->addWidget(tempo50);
	connect(tempo50, SIGNAL(clicked()), SLOT(setTempo50()));

	QToolButton* tempo100 = new QToolButton();
	tempo100->setText(tr("N"));
	toolbar->addWidget(tempo100);
	connect(tempo100, SIGNAL(clicked()), SLOT(setTempo100()));

	QToolButton* tempo200 = new QToolButton();
	tempo200->setText(QString("200%"));
	toolbar->addWidget(tempo200);
	connect(tempo200, SIGNAL(clicked()), SLOT(setTempo200()));
	toolbar->hide();

	QVBoxLayout* box = new QVBoxLayout(this);
	box->setContentsMargins(0, 0, 0, 0);
	box->setSpacing(0);
	//box->addWidget(hLine(this), Qt::AlignTop);

	//---------------------------------------------------
	//  Tracklist
	//---------------------------------------------------

	int xscale = -100;
	int yscale = 1;

	//m_splitter = new QSplitter(Qt::Vertical, this);
	m_splitter = 0;
	split = new QSplitter(Qt::Horizontal, this);
	split->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	split->setHandleWidth(2);

	m_trackheader = new HeaderList(this, "trackHeaderList");
	m_trackheader->setMinimumWidth(MIN_HEADER_WIDTH);

	QWidget* newtlist = new QWidget(split);
	QVBoxLayout *trackLayout = new QVBoxLayout(newtlist);
	trackLayout->setContentsMargins(0, 0, 0, 0);
	trackLayout->setSpacing(0);
	listScroll = new QScrollArea(newtlist);
	listScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listScroll->setWidgetResizable(true);
	listScroll->setWidget(m_trackheader);
	listScroll->setMouseTracking(true);
	listScroll->setMinimumWidth(MIN_HEADER_WIDTH);
	listScroll->setMaximumWidth(MAX_HEADER_WIDTH);

	m_timeHeader = new TimeHeader(this);
	//m_timeHeader->setFixedWidth(MIN_HEADER_WIDTH);
	trackLayout->addWidget(m_timeHeader);

	connect(song, SIGNAL(posChanged(int, unsigned, bool)), m_timeHeader, SLOT(setPos(int, unsigned, bool)));
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(posChanged(int, unsigned, bool)));

	edittools = new EditToolBar(this, composerTools, true);
	edittools->setFixedHeight(32);
	connect(edittools, SIGNAL(toolChanged(int)), this, SLOT(setTool(int)));
	connect(edittools, SIGNAL(toolChanged(int)), SIGNAL(updateFooterTool(int)));
	connect(this, SIGNAL(toolChanged(int)), edittools, SLOT(set(int)));
	connect(this, SIGNAL(updateHeaderTool(int)), edittools, SLOT(setNoUpdate(int)));
	trackLayout->addWidget(edittools);
	
	//trackLayout->addItem(new QSpacerItem(0, 32, QSizePolicy::Fixed, QSizePolicy::Fixed));
	trackLayout->addWidget(listScroll);

	split->setStretchFactor(split->indexOf(newtlist), 0);

	QWidget* editor = new QWidget(split);
	split->setStretchFactor(split->indexOf(editor), 1);
	QSizePolicy epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	epolicy.setHorizontalStretch(255);
	epolicy.setVerticalStretch(100);
	editor->setSizePolicy(epolicy);

	// Do this now that the list is available.
	createDockMembers();

	if(_tvdock)
	{
		connect(m_trackheader, SIGNAL(trackInserted(int)), _tvdock, SLOT(selectStaticView(int)));
	}
	
	connect(m_trackheader, SIGNAL(selectionChanged(Track*)), SLOT(trackSelectionChanged()));
	connect(m_trackheader, SIGNAL(selectionChanged(Track*)), midiConductor, SLOT(setTrack(Track*)));

	//---------------------------------------------------
	//    Editor
	//---------------------------------------------------

	//int offset = AL::sigmap.ticksMeasure(0);
	int offset = -(config.division / 4);
	hscroll = new ScrollScale(-1000, -10, xscale, song->len(), Qt::Horizontal, this, offset);
	hscroll->setFocusPolicy(Qt::NoFocus);

	vscroll = new QScrollBar(Qt::Vertical, this);
	listScroll->setVerticalScrollBar(vscroll);
	vscroll->setMinimum(0);
	vscroll->setMaximum(20 * 20);
	vscroll->setSingleStep(5);
	vscroll->setPageStep(25);
	vscroll->setValue(0);

	QVBoxLayout *editorBox = new QVBoxLayout(editor);
	editorBox->setContentsMargins(0,0,0,0);
	editorBox->setSpacing(0);

	QHBoxLayout *canvasBox = new QHBoxLayout;
	canvasBox->setContentsMargins(0,0,0,0);
	canvasBox->setSpacing(0);

	time = new MTScale(&_raster, this, xscale);
	time->setOrigin(-offset, 0);

	m_sigRuler = new SigScale(&_raster, this, xscale);
	m_sigRuler->setOrigin(-offset, 0);

	canvas = new ComposerCanvas(&_raster, this, xscale, yscale);
	canvas->setBg(config.partCanvasBg);
	canvas->setCanvasTools(composerTools);
	canvas->setOrigin(-offset, 0);
	canvas->setFocus();

	m_headerTabs = new QTabWidget(this);
	m_headerTabs->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_headerTabs->setFixedHeight(80);
	editorBox->addWidget(m_headerTabs);

	virtualScroll.setCanvas(canvas);
	m_tempoHeader = new TempoHeader(this, xscale);
	m_tempoHeader->setOrigin(-offset, 0);

	m_headerTabs->addTab(&virtualScroll, QString(tr("Navigator")));
	m_headerTabs->addTab(m_tempoHeader, QString(tr("Tempo")));
	m_headerTabs->setCornerWidget(headerCornerWidget(1));
	connect(m_headerTabs, SIGNAL(currentChanged(int)), this, SLOT(headerTabChanged(int)));

	m_tempoHeader->setStartTempo(m_tempoStart);
	m_tempoHeader->setEndTempo(m_tempoEnd);

	connect(oom, SIGNAL(viewReady()), &virtualScroll, SLOT(updateSpacing()));
	connect(canvas, SIGNAL(selectionChanged()), &virtualScroll, SLOT(updateSelections()));
	connect(canvas, SIGNAL(trackHeightChanged()), &virtualScroll, SLOT(updateParts()));
	connect(m_trackheader, SIGNAL(trackHeightChanged()), &virtualScroll, SLOT(updateParts()));
	connect(&virtualScroll, SIGNAL(updateScroll(int, int)), this, SLOT(updateScroll(int, int)));
	
	connect(m_tempoHeader, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), m_tempoHeader, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scaleChanged(float)), m_tempoHeader, SLOT(setXMag(float)));
	connect(edittools, SIGNAL(toolChanged(int)), m_tempoHeader, SLOT(toolChanged(int)));
	connect(this, SIGNAL(updateHeaderTool(int)), m_tempoHeader, SLOT(toolChanged(int)));
	connect(m_tempoHeader, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(m_sigRuler, SIGNAL(timeChanged(unsigned)), SLOT(setTimeFromSig(unsigned)));
	//connect(m_tempoHeader, SIGNAL(tempoChanged(int), SLOT(setTempo(int)));

	connect(hscroll, SIGNAL(scrollChanged(int)), m_sigRuler, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scaleChanged(float)), m_sigRuler, SLOT(setXMag(float)));

	editorBox->addWidget(time);
	editorBox->addWidget(m_sigRuler);
	editorBox->addWidget(hLine(this));
	canvasBox->addWidget(canvas, 100);
	canvasBox->addWidget(vscroll);
	editorBox->addLayout(canvasBox, 100);
	editorBox->addWidget(hscroll);

	box->addWidget(split);

	connect(canvas, SIGNAL(setUsedTool(int)), this, SIGNAL(setUsedTool(int)));
	connect(canvas, SIGNAL(trackChanged(Track*)), m_trackheader, SLOT(selectTrack(Track*)));
	connect(canvas, SIGNAL(renameTrack(Track*)), m_trackheader, SLOT(renameTrack(Track*)));
	connect(m_trackheader, SIGNAL(keyPressExt(QKeyEvent*)), canvas, SLOT(redirKeypress(QKeyEvent*)));
	connect(canvas, SIGNAL(selectTrackAbove()), m_trackheader, SLOT(selectTrackAbove()));
	connect(canvas, SIGNAL(selectTrackBelow()), m_trackheader, SLOT(selectTrackBelow()));
	connect(canvas, SIGNAL(moveSelectedTracks(int)), m_trackheader, SLOT(moveSelectedTrack(int)));

	connect(this, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
	connect(m_trackheader, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));

	connect(vscroll, SIGNAL(valueChanged(int)), canvas, SLOT(setYPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scrollChanged(int)), time, SLOT(setXPos(int))); //
	connect(hscroll, SIGNAL(scaleChanged(float)), time, SLOT(setXMag(float)));
	connect(canvas, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(canvas, SIGNAL(verticalScroll(unsigned)), SLOT(verticalScrollSetYpos(unsigned)));
	connect(canvas, SIGNAL(horizontalScroll(unsigned)), hscroll, SLOT(setPos(unsigned)));
	connect(canvas, SIGNAL(horizontalScrollNoLimit(unsigned)), hscroll, SLOT(setPosNoLimit(unsigned)));
	connect(time, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));

	connect(canvas, SIGNAL(tracklistChanged()), m_trackheader, SLOT(tracklistChanged()));
	connect(canvas, SIGNAL(dclickPart(Track*)), SIGNAL(editPart(Track*)));
	connect(canvas, SIGNAL(startEditor(PartList*, int)), SIGNAL(startEditor(PartList*, int)));

	connect(song, SIGNAL(markerChanged(int)), SLOT(markerChanged(int)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(song, SIGNAL(composerViewChanged()), SLOT(composerViewChanged()));
    connect(song, SIGNAL(punchinChanged(bool)), canvas, SLOT(update()));
    connect(song, SIGNAL(punchoutChanged(bool)), canvas, SLOT(update()));
    connect(song, SIGNAL(loopChanged(bool)), canvas, SLOT(update()));
	connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
	connect(canvas, SIGNAL(selectionChanged()), SIGNAL(selectionChanged()));
	connect(canvas, SIGNAL(dropSongFile(const QString&)), SIGNAL(dropSongFile(const QString&)));
	connect(canvas, SIGNAL(dropMidiFile(const QString&)), SIGNAL(dropMidiFile(const QString&)));

	connect(canvas, SIGNAL(toolChanged(int)), SIGNAL(toolChanged(int)));
	//connect(canvas, SIGNAL(toolChanged(int)), edittools, SLOT(setTool(int)));
	connect(split, SIGNAL(splitterMoved(int, int)),  SLOT(splitterMoved(int, int)));

	configChanged(); // set configuration values
	if (canvas->part())
		midiConductor->setTrack(canvas->part()->track());
	updateConductor(-1);

	// Take care of some tabbies!
	setTabOrder(tempo200, m_trackheader);
	setTabOrder(m_trackheader, canvas);

	QList<int> vl;
	vl << MIN_HEADER_WIDTH << 200;
	QByteArray state = tconfig().get_property("composerSettings", "splitterState", "").toByteArray();
	if(state.isNull() || state.isEmpty())
		split->setSizes(vl);
	else
		split->restoreState(state);
}

Composer::~Composer()
{
	tconfig().set_property("composerSettings", "spliterState", split->saveState());
	tconfig().set_property("TempoRange", "start", m_tempoStart);
	tconfig().set_property("TempoRange", "end", m_tempoEnd);

	tconfig().save();
}

QWidget* Composer::headerCornerWidget(int tab)
{
	m_headerToolBox = new QStackedWidget(this);
	m_headerToolBox->addWidget(new QLabel());

	QWidget* w = new QWidget(this);
	QHBoxLayout* layout = new QHBoxLayout(w);
	layout->setSpacing(0);
	layout->setContentsMargins(0,0,0,0);

	//layout->addWidget(new QLabel(tr("Tempo Range ")));

	//m_startTempo = new TempoEdit(w);
	//m_startTempo->setToolTip(tr("Lowest visible Tempo"));
	//layout->addWidget(m_startTempo);
	
	//m_endTempo = new TempoEdit(w);
	//m_endTempo->setToolTip(tr("Highest visible Tempo"));
	//layout->addWidget(m_endTempo);

	m_tempoRange = new DoubleSlider(true, this);
	m_tempoRange->setRange(30.0, 250.0);
	m_tempoRange->setCurrentMin(m_tempoStart);
	m_tempoRange->setCurrentMax(m_tempoEnd);
	m_tempoRange->setFixedHeight(12);
	layout->addWidget(m_tempoRange);
	//m_headerToolBox->addWidget(m_tempoRange);
	
	//TODO: Add sigedit
	m_sigEdit = new SigEdit(0);
	m_sigEdit->setValue(AL::TimeSignature(4, 4));
	m_sigEdit->setToolTip(tr("Time signature at current position"));
	connect(m_sigEdit, SIGNAL(valueChanged(const AL::TimeSignature&)), song, SLOT(setSig(const AL::TimeSignature&)));
	layout->addSpacing(4);
	layout->addWidget(m_sigEdit);
	layout->addSpacing(6);

	w->setFixedHeight(16);
	m_headerToolBox->addWidget(w);

	connect(m_tempoRange, SIGNAL(minChanged(double)), m_tempoHeader, SLOT(setStartTempo(double)));
	connect(m_tempoRange, SIGNAL(maxChanged(double)), m_tempoHeader, SLOT(setEndTempo(double)));
	connect(m_tempoRange, SIGNAL(minChanged(double)), this, SLOT(setStartTempo(double)));
	connect(m_tempoRange, SIGNAL(maxChanged(double)), this, SLOT(setEndTempo(double)));
	
	m_headerToolBox->setCurrentIndex(tab);
	m_headerTabs->setCurrentIndex(tab);
	return m_headerToolBox;
}

void Composer::headerTabChanged(int tab)
{
	if(m_headerToolBox)
		m_headerToolBox->setCurrentIndex(tab);
}

void Composer::setStartTempo(double tempo)
{
	if(tempo >= m_tempoEnd)
	{
		if(m_tempoRange)
			m_tempoRange->setCurrentMin(m_tempoStart);
	}
	else
		m_tempoStart = tempo;
}

void Composer::setEndTempo(double tempo)
{
	if(tempo <= m_tempoStart)
	{
		if(m_tempoRange)
			m_tempoRange->setCurrentMax(m_tempoEnd);
	}
	else
		m_tempoEnd = tempo;
}

void Composer::posChanged(int idx, unsigned val, bool)
{
	if (idx == 0)
	{
		if(m_sigEdit)
		{
			int z, n;
			AL::sigmap.timesig(val, z, n);
			m_sigEdit->blockSignals(true);
			m_sigEdit->setValue(AL::TimeSignature(z, n));
			m_sigEdit->blockSignals(false);
		}
	}
	cursVal = val;
	m_timeHeader->setTime(val);
	time->setPos(3, val, false);
	canvas->setPos(3, val, false);
	m_tempoHeader->setPos(3, val, false);
	//m_sigRuler->setPos(3, val, false);
}

void Composer::heartBeat()
{
	virtualScroll.advancePlayhead();
}

void Composer::currentTabChanged(int tab)
{
	switch(tab)
	{
		case 1: //patch sequencer
		{
			if(midiConductor)
			{
				//printf("PatchSequencer Tab clicked\n");
				midiConductor->update();
				if(selected && selected->isMidiTrack())
					midiConductor->setTrack(selected);
			}
			if(m_clipList)
			{
				m_clipList->setActive(false);
			}
		}
		break;
		case 2: //Clip List
		{
			if(m_clipList)
			{
				m_clipList->setActive(true);
				m_clipList->refresh();
				m_clipList->loadBookmarks();
			}
		}
		break;
		default:
		{
			if(m_clipList)
			{
				m_clipList->setActive(false);
			}
		}
		break;
	}
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void Composer::setTime(unsigned tick)/*{{{*/
{
	/*if (tick == MAXINT)
		return;
	else
	{*/
		cursVal = tick;
		m_timeHeader->setTime(tick);
		time->setPos(3, tick, false);
		m_sigRuler->setPos(3, tick, false);
		canvas->setPos(3, tick, false);
		m_tempoHeader->setPos(3, tick, false);
	//}
}/*}}}*/

void Composer::setTimeFromSig(unsigned tick)/*{{{*/
{
	//if (tick == MAXINT)
	//	return;
	//else
	//{
		cursVal = tick;
		m_timeHeader->setTime(tick);
		time->setPos(3, tick, false);
		canvas->setPos(3, tick, false);
		m_tempoHeader->setPos(3, tick, false);
	//}
}/*}}}*/

//---------------------------------------------------------
//   toolChange
//---------------------------------------------------------

void Composer::setTool(int t)
{
	canvas->setTool(t);
}

//---------------------------------------------------------
//   dclickPart
//---------------------------------------------------------

void Composer::dclickPart(Track* t)
{
	emit editPart(t);
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Composer::configChanged()
{
	//printf("Composer::configChanged\n");

	if (config.canvasBgPixmap.isEmpty())
	{
		canvas->setBg(config.partCanvasBg);
		canvas->setBg(QPixmap());
		//printf("Composer::configChanged - no bitmap!\n");
	}
	else
	{

		//printf("Composer::configChanged - bitmap %s!\n", config.canvasBgPixmap.ascii());
		canvas->setBg(QPixmap(config.canvasBgPixmap));
	}
	///midiConductor->setFont(config.fonts[2]);
	//updateConductor(type);
}

//---------------------------------------------------------
//   songlenChanged
//---------------------------------------------------------

void Composer::songlenChanged(int n)
{
	int newLen = AL::sigmap.bar2tick(n, 0, 0);
	//printf("New Song Length: %d - %d \n", n, newLen);
	song->setLen(newLen);
}

void Composer::composerViewChanged()
{
	updateAll();
	canvas->trackViewChanged();
	updateConductor(SC_VIEW_CHANGED);
	virtualScroll.updateParts();
}

void Composer::updateAll()
{
	unsigned endTick = song->len();
	int offset = AL::sigmap.ticksMeasure(endTick);
	/*hscroll->setRange(-offset, endTick + offset); //DEBUG
	canvas->setOrigin(-offset, 0);
	time->setOrigin(-offset, 0);*/

	//int offset = -(config.division / 4);
	hscroll->setRange(offset, endTick + offset); //DEBUG
	canvas->setOrigin(-offset, 0);
	time->setOrigin(-offset, 0);
	m_sigRuler->setOrigin(-offset, 0);
	m_tempoHeader->setOrigin(-offset, 0);

	int bar, beat;
	unsigned tick;
	AL::sigmap.tickValues(endTick, &bar, &beat, &tick);
	if (tick || beat)
		++bar;
	lenEntry->blockSignals(true);
	lenEntry->setValue(bar);
	lenEntry->blockSignals(false);


	trackSelectionChanged();
	canvas->partsChanged();
	typeBox->setCurrentIndex(int(song->mtype()));
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Composer::songChanged(int type)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (type != SC_MIDI_CONTROLLER)
	{
		updateAll();
		if (type & (SC_TRACK_REMOVED | SC_VIEW_CHANGED))
		{
			canvas->trackViewChanged();
		}
		
		if (type & SC_SONG_TYPE) 
			setMode(song->mtype());
	
		if (type & SC_SIG)
			time->redraw();
		if (type & SC_TEMPO)
			setGlobalTempo(tempomap.globalTempo());
		if(type & SC_VIEW_CHANGED)
		{//Scroll to top
			//canvas->setYPos(0);
			//vscroll->setValue(0);
		}
		if(type & (SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | SC_VIEW_CHANGED | SC_TRACK_REMOVED | SC_TRACK_INSERTED))
		{//Scroll to top
			virtualScroll.updateParts();
		}
		//if (type & SC_SELECTION)
		//	virtualScroll.updateSelections();
		if (m_sigEdit && (type & SC_SIG))
		{
			int z, n;
			AL::sigmap.timesig(song->cpos(), z, n);
			m_sigEdit->blockSignals(true);
			m_sigEdit->setValue(AL::TimeSignature(z, n));
			m_sigEdit->blockSignals(false);
			m_sigRuler->redraw();
		}
	}

	updateConductor(type);
}

void Composer::markerChanged(int flag)
{
	switch(flag)
	{
		case Song::MARKER_ADD:
		case Song::MARKER_REMOVE:
			virtualScroll.updateParts();
		break;
		case Song::MARKER_TICK:
			virtualScroll.updateMarkers();
		break;
		//case Song::MARKER_CUR:
		//case Song::MARKER_NAME:
		//case Song::MARKER_LOCK
		default:
		break;
	}
}

void Composer::splitterMoved(int pos, int)
{
	if(pos > listScroll->maximumSize().width())
	{
		QList<int> def;
		def.append(listScroll->maximumSize().width());
		def.append(50);
		split->setSizes(def);
	}
}

//---------------------------------------------------------
//   trackSelectionChanged
//---------------------------------------------------------

void Composer::trackSelectionChanged()
{
	TrackList* tracks = song->visibletracks();
	Track* track = 0;
	
	for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
	{
		track = *t;
		if (track && track->selected())
		{
			break;
		}
	}
	if (track == selected)
		return;
	selected = track;
	updateConductor(-1);

	// Check if the selected track is inside the view, if not
	// scroll the track to the center of the view
	if(selected)
	{
		int vScrollValue = vscroll->value();
		int trackYPos = canvas->track2Y(selected);
		if (trackYPos > (vScrollValue + canvas->height()) || trackYPos < vScrollValue)
		{
			vscroll->setValue(trackYPos - (canvas->height() / 2));
		}
		if(selected->isMidiTrack())
		{
			raster->setCurrentIndex(config.midiRaster);
			//printf("Setting midi raster in trackSelectionChanged(%d)\n", config.midiRaster);
		}
		else
		{
			raster->setCurrentIndex(config.audioRaster);
			//printf("Setting audio raster in trackSelectionChanged(%d)\n", config.audioRaster);
		}
		emit trackSelectionChanged(selected->id());
	}
}

CItem* Composer::addCanvasPart(Track* t)
{
	CItem* item = canvas->addPartAtCursor(t);
	canvas->newItem(item, false);
	return item;
}

//---------------------------------------------------------
//   modeChange
//---------------------------------------------------------

void Composer::modeChange(int mode)
{
	song->setMType(MType(mode));
	updateConductor(-1);
}

//---------------------------------------------------------
//   setMode
//---------------------------------------------------------

void Composer::setMode(int mode)
{
	typeBox->blockSignals(true); //
	// This will only set if different.
	typeBox->setCurrentIndex(mode);
	typeBox->blockSignals(false); //
}

void Composer::showTrackViews()
{
	TrackViewEditor* ted = new TrackViewEditor(this);
	ted->show();
}

void Composer::resourceDockAreaChanged(Qt::DockWidgetArea area)
{
	switch(area)
	{
		case Qt::LeftDockWidgetArea:
			_rtabs->setTabPosition(QTabWidget::West);
		break;
		case Qt::RightDockWidgetArea:
			_rtabs->setTabPosition(QTabWidget::East);
		break;
		default:
		break;
	}
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Composer::writeStatus(int level, Xml& xml)
{
	xml.tag(level++, "composer");
	//split->writeStatus(level, xml);
	xml.intTag(level, "xpos", hscroll->pos());
	xml.intTag(level, "xmag", hscroll->mag());
	xml.intTag(level, "ypos", vscroll->value());
    xml.etag(--level, "composer");
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Composer::readStatus(Xml& xml)
{
	//printf("Composer::readStatus() entering\n");
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
				if (tag == "info")
					xml.skip(tag);
				else if(tag == "split") //backwards compat
					xml.skip(tag);
				else if (tag == split->objectName())
					xml.skip(tag);
				else if (tag == "list")
					xml.skip(tag);
				else if (tag == "xmag")
					hscroll->setMag(xml.parseInt());
				else if (tag == "xpos")
				{
					int hpos = xml.parseInt();
					hscroll->setPos(hpos);
				}
				else if (tag == "ypos")
					vscroll->setValue(xml.parseInt());
				else
					xml.unknown("Composer");
				break;
			case Xml::TagEnd:
				if (tag == "composer")
				{
					//printf("Composer::readStatus() leaving end tag\n");
					return;
				}
			default:
				break;
		}
	}
	//printf("Composer::readStatus() leaving\n");
}

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Composer::_setRaster(int index, bool setDefault)
{
	_raster = rasterTable[index];
	// Set the audio record part snapping.
	song->setComposerRaster(_raster);
	if(selected && setDefault)
	{
		if(selected->isMidiTrack())
			config.midiRaster = index;
		else
			config.audioRaster = index;
	}
	canvas->redraw();
}

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Composer::reset()
{
	canvas->setXPos(0);
	canvas->setYPos(0);
	hscroll->setPos(0);
	vscroll->setValue(0);
	time->setXPos(0);
	time->setYPos(0);
	m_sigRuler->setXPos(0);
}

void Composer::updateScroll(int x, int y)
{
	hscroll->setPos(x);
	vscroll->setValue(y);
}

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void Composer::cmd(int cmd)
{
	int ncmd;
	switch (cmd)
	{
		case CMD_CUT_PART:
			ncmd = ComposerCanvas::CMD_CUT_PART;
			break;
		case CMD_COPY_PART:
			ncmd = ComposerCanvas::CMD_COPY_PART;
			break;
		case CMD_PASTE_PART:
			ncmd = ComposerCanvas::CMD_PASTE_PART;
			break;
		case CMD_PASTE_CLONE_PART:
			ncmd = ComposerCanvas::CMD_PASTE_CLONE_PART;
			break;
		case CMD_PASTE_PART_TO_TRACK:
			ncmd = ComposerCanvas::CMD_PASTE_PART_TO_TRACK;
			break;
		case CMD_PASTE_CLONE_PART_TO_TRACK:
			ncmd = ComposerCanvas::CMD_PASTE_CLONE_PART_TO_TRACK;
			break;
		case CMD_INSERT_PART:
			ncmd = ComposerCanvas::CMD_INSERT_PART;
			break;
		case CMD_INSERT_EMPTYMEAS:
			ncmd = ComposerCanvas::CMD_INSERT_EMPTYMEAS;
			break;
		case CMD_REMOVE_SELECTED_AUTOMATION_NODES:
			ncmd = ComposerCanvas::CMD_REMOVE_SELECTED_AUTOMATION_NODES;
			break;
		case CMD_COPY_AUTOMATION_NODES:
			ncmd = ComposerCanvas::CMD_COPY_AUTOMATION_NODES;
			break;
		case CMD_PASTE_AUTOMATION_NODES:
			ncmd = ComposerCanvas::CMD_PASTE_AUTOMATION_NODES;
			break;
		case CMD_SELECT_ALL_AUTOMATION:
			ncmd = ComposerCanvas::CMD_SELECT_ALL_AUTOMATION;
		break;
		default:
			return;
	}
	canvas->cmd(ncmd);
}

//---------------------------------------------------------
//   globalPitchChanged
//---------------------------------------------------------

void Composer::globalPitchChanged(int val)
{
	song->setGlobalPitchShift(val);
}

//---------------------------------------------------------
//   globalTempoChanged
//---------------------------------------------------------

void Composer::globalTempoChanged(int val)
{
	audio->msgSetGlobalTempo(val);
	song->tempoChanged();
}

//---------------------------------------------------------
//   setTempo50
//---------------------------------------------------------

void Composer::setTempo50()
{
	setGlobalTempo(50);
}

//---------------------------------------------------------
//   setTempo100
//---------------------------------------------------------

void Composer::setTempo100()
{
	setGlobalTempo(100);
}

//---------------------------------------------------------
//   setTempo200
//---------------------------------------------------------

void Composer::setTempo200()
{
	setGlobalTempo(200);
}

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void Composer::setGlobalTempo(int val)
{
	if (val != globalTempoSpinBox->value())
		globalTempoSpinBox->setValue(val);
}

//---------------------------------------------------------
//   verticalScrollSetYpos
//---------------------------------------------------------

void Composer::verticalScrollSetYpos(unsigned ypos)
{
	vscroll->setValue(ypos);
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Composer::clear()
{
	selected = 0;
	midiConductor->setTrack(0);
	if (canvas)
	{
		canvas->setCurrentPart(0);
	}
	m_trackheader->clear();
}

void Composer::wheelEvent(QWheelEvent* ev)
{
	emit redirectWheelEvent(ev);
}

void Composer::controllerChanged(Track *t)
{
	canvas->controllerChanged(t);
}

//---------------------------------------------------------
//  createDockMembers
//---------------------------------------------------------

void Composer::createDockMembers()
{
	midiConductor = new Conductor(this);
	foreach(QObject* obj, oom->resourceDock()->children())
	{
		obj->installEventFilter(this);
	}
	midiConductor->performerToolsFrame->hide();

	_tvdock = new TrackViewDock(this);
	m_clipList = new AudioClipList(this);
	//Set to true if this is the first cliplist viewable tab
	//When false the directory watcher will not constantly update the view
	m_clipList->setActive(false);
	_commentdock = new CommentDock(this);
	_rtabs->addTab(_tvdock, tr("   EPIC Views   "));
	_rtabs->addTab(midiConductor, tr("   Conductor   "));
	_rtabs->addTab(m_clipList, tr("  Clips  "));
	_rtabs->addTab(_commentdock, tr("  Comments  "));

}

//---------------------------------------------------------
//   updateConductor
//---------------------------------------------------------

void Composer::updateConductor(int flags)
{
	_commentdock->setTrack(selected);
	if (selected == 0)
	{
		updateTabs();
		return;
	}
	if (selected->isMidiTrack())
	{
		if ((flags & SC_SELECTION) || (flags & SC_TRACK_REMOVED))
			updateTabs();
		// If a new part was selected, and only if it's different.
		if ((flags & SC_SELECTION) && midiConductor->track() != selected)
			// Set a new track and do a complete update.
			midiConductor->setTrack(selected);
		else
			// Otherwise just regular update with specific flags.
			midiConductor->updateConductor(flags);
	}
	else
	{
		if ((flags & SC_SELECTION) || (flags & SC_TRACK_REMOVED))
			updateTabs();
	}
}

//---------------------------------------------------------
//   updateTabs
//---------------------------------------------------------

void Composer::updateTabs()/*{{{*/
{
    midiConductor->update();
	if(selected)
	{
		if(selected->isMidiTrack())
		{
			_rtabs->setTabEnabled(1, true);
		}
		else
		{
			if(_rtabs->currentIndex() == 1)
				_rtabs->setCurrentIndex(0);
			_rtabs->setTabEnabled(1, false);
		}
	}
	else
	{
		if(_rtabs->currentIndex() == 1)
			_rtabs->setCurrentIndex(0);
		_rtabs->setTabEnabled(1, false);
	}
}/*}}}*/

void Composer::preloadControllers()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	audio->preloadControllers();
	QApplication::restoreOverrideCursor();
}

bool Composer::isEditing()
{
	return (m_trackheader->isEditing() || canvas->isEditing());
}

void Composer::endEditing()
{
	/*if(m_trackheader->isEditing())
	{
		m_trackheader->returnPressed();
	}*/
	if(canvas->isEditing())
	{
		canvas->returnPressed();
	}
}

bool Composer::eventFilter(QObject *obj, QEvent *event)
{
	// Force left/right arrow key events to move the focus
	// back on the canvas if it doesn't have the focus.
	// Currently the object that we're filtering is the
	// midiConductor.
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		int key = keyEvent->key();
		//if (event->state() & Qt::ShiftButton)
		if (((QInputEvent*) event)->modifiers() & Qt::ShiftModifier)
			key += Qt::SHIFT;
		//if (event->state() & Qt::AltButton)
		if (((QInputEvent*) event)->modifiers() & Qt::AltModifier)
			key += Qt::ALT;
		//if (event->state() & Qt::ControlButton)
		if (((QInputEvent*) event)->modifiers() & Qt::ControlModifier)
			key += Qt::CTRL;
		///if (event->state() & Qt::MetaButton)
		if (((QInputEvent*) event)->modifiers() & Qt::MetaModifier)
			key += Qt::META;

		if (key == shortcuts[SHRT_NAVIGATE_TO_CANVAS].key)
		{
			canvas->setFocus(Qt::MouseFocusReason);
			return true;
		}
	}

	// standard event processing
	return QObject::eventFilter(obj, event);
}

