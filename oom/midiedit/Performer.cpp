//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: Performer.cpp,v 1.25.2.15 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================


// if app.h is included before the other headers
// if fails to compile, why ?
#include "app.h"

#include <QLayout>
#include <QSizeGrip>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QToolTip>
#include <QMenu>
#include <QSignalMapper>
#include <QMenuBar>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QAction>
#include <QKeySequence>
#include <QKeyEvent>
#include <QGridLayout>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QWidgetAction>
#include <QDockWidget>
#include <QTabWidget>
#include <QtGui>

#include <stdio.h>

#include "xml.h"
#include "mtscale.h"
#include "pcscale.h"
#include "PerformerCanvas.h"
#include "Performer.h"
#include "poslabel.h"
#include "pitchlabel.h"
#include "scrollscale.h"
#include "Piano.h"
#include "../ctrl/ctrledit.h"
#include "splitter.h"
#include "ttoolbar.h"
//#include "tb1.h"
#include "utils.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"
#include "midiport.h"
#include "instruments/minstrument.h"

#include "cmd.h"
#include "quantconfig.h"
#include "shortcuts.h"

#include "conductor/Conductor.h"
#include "tracklistview.h"
#include "toolbars/transporttools.h"
#include "toolbars/edittools.h"
#include "toolbars/epictools.h"
#include "toolbars/looptools.h"
#include "toolbars/misctools.h"

#include "traverso_shared/TConfig.h"

int Performer::_quantInit = 96;
int Performer::_rasterInit = 96;
int Performer::_widthInit = 600;
int Performer::_heightInit = 400;
int Performer::_quantStrengthInit = 80; // 1 - 100%
int Performer::_quantLimitInit = 50; // tick value
bool Performer::_quantLenInit = false;
int Performer::_toInit = 0;
int Performer::colorModeInit = 2;

static const int xscale = -10;
static const int yscale = 1;
static const int pianoWidth = 40;
static int performerTools = PointerTool | PencilTool | RubberTool | DrawTool;


//---------------------------------------------------------
//   Performer
//---------------------------------------------------------

Performer::Performer(PartList* pl, QWidget* parent, const char* name, unsigned initPos)
	: AbstractMidiEditor(_quantInit, _rasterInit, pl, parent, name)
{
	deltaMode = false;
	// Set size stored in global config, or use defaults.
	/*int w = tconfig().get_property("PerformerEdit", "widgetwidth", 800).toInt();
	int h = tconfig().get_property("PerformerEdit", "widgetheigth", 650).toInt();
	int dw = qApp->desktop()->width();
	int dh = qApp->desktop()->height();
		resize(w, h);*/
	/*if(h <= dh && w <= dw)
	{
		printf("Restoring window state\n");
		resize(w, h);
	}
	else
	{
		printf("Desktop size too large for saved state\n");
		showMaximized();
	}*/

	selPart = 0;
	quantConfig = 0;
	_playEvents = false;
	_quantStrength = _quantStrengthInit;
	_quantLimit = _quantLimitInit;
	_quantLen = _quantLenInit;
	_to = _toInit;
	colorMode = tconfig().get_property("PerformerEdit", "colormode", colorModeInit).toInt();
	_stepQwerty = false;

	m_prDock = new QDockWidget(tr("The Orchestra Pit"), this);
	m_prDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_prDock->setObjectName("dockResourceCenter");
	m_prDock->setMinimumWidth(190);
	addDockWidget(Qt::LeftDockWidgetArea, m_prDock);

	m_tabs = new QTabWidget(m_prDock);
	m_tabs->setObjectName("tabControlCenter");
	m_tabs->setTabPosition(QTabWidget::West);
	m_tabs->setTabShape(QTabWidget::Triangular);
	//m_tabs->setMinimumSize(QSize(200, 150));
	m_tabs->setMinimumWidth(250);
	//connect(m_rtabs, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
	QWidget* dockWidget = new QWidget(this);
	dockWidget->setMinimumWidth(250);
	QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
	dockLayout->setContentsMargins(0,0,0,0);
	QLabel* logoLabel = new QLabel(dockWidget);
	logoLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	logoLabel->setPixmap(QPixmap(":/images/icons/oomidi_icon_the_performer.png"));
	dockLayout->addWidget(logoLabel);
	dockLayout->addWidget(m_tabs);
	m_prDock->setWidget(dockWidget);
	connect(m_prDock, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), SLOT(dockAreaChanged(Qt::DockWidgetArea)));

	QSignalMapper* mapper = new QSignalMapper(this);
	QSignalMapper* colorMapper = new QSignalMapper(this);

	//---------Menu----------------------------------

	menuEdit = menuBar()->addMenu(tr("&Edit"));

	menuEdit->addActions(undoRedo->actions());

	menuEdit->addSeparator();

	editCutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("C&ut"));
	mapper->setMapping(editCutAction, PerformerCanvas::CMD_CUT);
	connect(editCutAction, SIGNAL(triggered()), mapper, SLOT(map()));

	editCopyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("&Copy"));
	mapper->setMapping(editCopyAction, PerformerCanvas::CMD_COPY);
	connect(editCopyAction, SIGNAL(triggered()), mapper, SLOT(map()));

	editPasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("&Paste"));
	mapper->setMapping(editPasteAction, PerformerCanvas::CMD_PASTE);
	connect(editPasteAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuEdit->addSeparator();

	editDelEventsAction = menuEdit->addAction(tr("Delete &Events"));
	mapper->setMapping(editDelEventsAction, PerformerCanvas::CMD_DEL);
	connect(editDelEventsAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuEdit->addSeparator();

	menuSelect = menuEdit->addMenu(QIcon(*selectIcon), tr("&Select"));

	selectAllAction = menuSelect->addAction(QIcon(*select_allIcon), tr("Select &All"));
	mapper->setMapping(selectAllAction, PerformerCanvas::CMD_SELECT_ALL);
	connect(selectAllAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectNoneAction = menuSelect->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
	mapper->setMapping(selectNoneAction, PerformerCanvas::CMD_SELECT_NONE);
	connect(selectNoneAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectInvertAction = menuSelect->addAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"));
	mapper->setMapping(selectInvertAction, PerformerCanvas::CMD_SELECT_INVERT);
	connect(selectInvertAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuSelect->addSeparator();

	selectInsideLoopAction = menuSelect->addAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"));
	mapper->setMapping(selectInsideLoopAction, PerformerCanvas::CMD_SELECT_ILOOP);
	connect(selectInsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectOutsideLoopAction = menuSelect->addAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"));
	mapper->setMapping(selectOutsideLoopAction, PerformerCanvas::CMD_SELECT_OLOOP);
	connect(selectOutsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuSelect->addSeparator();

	//selectPrevPartAction = select->addAction(tr("&Previous Part"));
	selectPrevPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Previous Part"));
	mapper->setMapping(selectPrevPartAction, PerformerCanvas::CMD_SELECT_PREV_PART);
	connect(selectPrevPartAction, SIGNAL(triggered()), mapper, SLOT(map()));

	//selNextPartAction = select->addAction(tr("&Next Part"));
	selectNextPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Next Part"));
	mapper->setMapping(selectNextPartAction, PerformerCanvas::CMD_SELECT_NEXT_PART);
	connect(selectNextPartAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuConfig = menuBar()->addMenu(tr("&Config"));

	eventColor = menuConfig->addMenu(tr("&Event Color"));

	QActionGroup* actgrp = new QActionGroup(this);
	actgrp->setExclusive(true);

	//evColorBlueAction = eventColor->addAction(tr("&Blue"));
	evColorBlueAction = actgrp->addAction(tr("&Blue"));
	evColorBlueAction->setCheckable(true);
	colorMapper->setMapping(evColorBlueAction, 0);

	//evColorPitchAction = eventColor->addAction(tr("&Pitch colors"));
	evColorPitchAction = actgrp->addAction(tr("&Pitch colors"));
	evColorPitchAction->setCheckable(true);
	colorMapper->setMapping(evColorPitchAction, 1);

	//evColorVelAction = eventColor->addAction(tr("&Velocity colors"));
	evColorVelAction = actgrp->addAction(tr("&Velocity colors"));
	evColorVelAction->setCheckable(true);
	colorMapper->setMapping(evColorVelAction, 2);

	connect(evColorBlueAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
	connect(evColorPitchAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
	connect(evColorVelAction, SIGNAL(triggered()), colorMapper, SLOT(map()));

	eventColor->addActions(actgrp->actions());

	connect(colorMapper, SIGNAL(mapped(int)), this, SLOT(eventColorModeChanged(int)));

	menuFunctions = menuBar()->addMenu(tr("&Functions"));

	menuFunctions->setTearOffEnabled(true);

	funcOverQuantAction = menuFunctions->addAction(tr("Over Quantize"));
	mapper->setMapping(funcOverQuantAction, PerformerCanvas::CMD_OVER_QUANTIZE);
	connect(funcOverQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteOnQuantAction = menuFunctions->addAction(tr("Note On Quantize"));
	mapper->setMapping(funcNoteOnQuantAction, PerformerCanvas::CMD_ON_QUANTIZE);
	connect(funcNoteOnQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteOnOffQuantAction = menuFunctions->addAction(tr("Note On/Off Quantize"));
	mapper->setMapping(funcNoteOnOffQuantAction, PerformerCanvas::CMD_ONOFF_QUANTIZE);
	connect(funcNoteOnOffQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcIterQuantAction = menuFunctions->addAction(tr("Iterative Quantize"));
	mapper->setMapping(funcIterQuantAction, PerformerCanvas::CMD_ITERATIVE_QUANTIZE);
	connect(funcIterQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuFunctions->addSeparator();

	funcConfigQuantAction = menuFunctions->addAction(tr("Config Quant..."));
	connect(funcConfigQuantAction, SIGNAL(triggered()), this, SLOT(configQuant()));

	menuFunctions->addSeparator();

	funcGateTimeAction = menuFunctions->addAction(tr("Modify Gate Time"));
	mapper->setMapping(funcGateTimeAction, PerformerCanvas::CMD_MODIFY_GATE_TIME);
	connect(funcGateTimeAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcModVelAction = menuFunctions->addAction(tr("Modify Velocity"));
	mapper->setMapping(funcModVelAction, PerformerCanvas::CMD_MODIFY_VELOCITY);
	connect(funcModVelAction, SIGNAL(triggered()), mapper, SLOT(map()));
/*
	funcCrescendoAction = menuFunctions->addAction(tr("Crescendo"));
	mapper->setMapping(funcCrescendoAction, PerformerCanvas::CMD_CRESCENDO);
	funcCrescendoAction->setEnabled(false);
	connect(funcCrescendoAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcTransposeAction = menuFunctions->addAction(tr("Transpose"));
	mapper->setMapping(funcTransposeAction, PerformerCanvas::CMD_TRANSPOSE);
	funcTransposeAction->setEnabled(false);
	connect(funcTransposeAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcThinOutAction = menuFunctions->addAction(tr("Thin Out"));
	mapper->setMapping(funcThinOutAction, PerformerCanvas::CMD_THIN_OUT);
	funcThinOutAction->setEnabled(false);
	connect(funcThinOutAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcEraseEventAction = menuFunctions->addAction(tr("Erase Event"));
	mapper->setMapping(funcEraseEventAction, PerformerCanvas::CMD_ERASE_EVENT);
	funcEraseEventAction->setEnabled(false);
	connect(funcEraseEventAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteShiftAction = menuFunctions->addAction(tr("Note Shift"));
	mapper->setMapping(funcNoteShiftAction, PerformerCanvas::CMD_NOTE_SHIFT);
	funcNoteShiftAction->setEnabled(false);
	connect(funcNoteShiftAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcMoveClockAction = menuFunctions->addAction(tr("Move Clock"));
	mapper->setMapping(funcMoveClockAction, PerformerCanvas::CMD_MOVE_CLOCK);
	funcMoveClockAction->setEnabled(false);
	connect(funcMoveClockAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcCopyMeasureAction = menuFunctions->addAction(tr("Copy Measure"));
	mapper->setMapping(funcCopyMeasureAction, PerformerCanvas::CMD_COPY_MEASURE);
	funcCopyMeasureAction->setEnabled(false);
	connect(funcCopyMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcEraseMeasureAction = menuFunctions->addAction(tr("Erase Measure"));
	mapper->setMapping(funcEraseMeasureAction, PerformerCanvas::CMD_ERASE_MEASURE);
	funcEraseMeasureAction->setEnabled(false);
	connect(funcEraseMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcDelMeasureAction = menuFunctions->addAction(tr("Delete Measure"));
	mapper->setMapping(funcDelMeasureAction, PerformerCanvas::CMD_DELETE_MEASURE);
	funcDelMeasureAction->setEnabled(false);
	connect(funcDelMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcCreateMeasureAction = menuFunctions->addAction(tr("Create Measure"));
	mapper->setMapping(funcCreateMeasureAction, PerformerCanvas::CMD_CREATE_MEASURE);
	funcCreateMeasureAction->setEnabled(false);
	connect(funcCreateMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));
*/
	funcSetFixedLenAction = menuFunctions->addAction(tr("Set Fixed Length"));
	mapper->setMapping(funcSetFixedLenAction, PerformerCanvas::CMD_FIXED_LEN);
	connect(funcSetFixedLenAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcDelOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));
	mapper->setMapping(funcDelOverlapsAction, PerformerCanvas::CMD_DELETE_OVERLAPS);
	connect(funcDelOverlapsAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuPlugins = menuBar()->addMenu(tr("&Plugins"));
	song->populateScriptMenu(menuPlugins, this);

	connect(mapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

	//---------ToolBar----------------------------------
	//tools = addToolBar(tr("Performer tools"));
	//tools->setObjectName("tbPRtools");
	//tools->addActions(undoRedo->actions());
	//tools->addSeparator();
	//tools->setIconSize(QSize(22, 22));
	
	m_stepAction = new QAction(this);
	m_stepAction->setToolTip(tr("Step Record"));
	m_stepAction->setIcon(*stepIconSet3);
	m_stepAction->setCheckable(true);

    /*srec = new QToolButton();
	srec->setToolTip(tr("Step Record"));
	srec->setIcon(*stepIconSet3);
    srec->setCheckable(true);*/
	//srec->setObjectName("StepRecord");
	//tools->addWidget(srec);

    midiin = new QToolButton();
	midiin->setToolTip(tr("Midi Input"));
	midiin->setIcon(*midiinIcon);
	midiin->setCheckable(true);
	//tools->addWidget(midiin);

    /*speaker = new QToolButton();
	speaker->setToolTip(tr("Play Events"));
	speaker->setIcon(*speakerIconSet3);
	speaker->setCheckable(true);*/
	//tools->addWidget(speaker);
	
	m_speakerAction = new QAction(this);
	m_speakerAction->setToolTip(tr("Play Events"));
	m_speakerAction->setIcon(*speakerIconSet3);
	m_speakerAction->setCheckable(true);

	solo = new QToolButton();
	///solo->setIcon(*soloIconSet3);
	//solo->setIconSize(soloIconOn->size());
	//solo->setToolTip(tr("Solo"));
	//solo->setCheckable(true);
	m_soloAction = new QAction(this);
	m_soloAction->setIcon(*soloIconSet3);
	m_soloAction->setToolTip(tr("Solo"));
	m_soloAction->setCheckable(true);
	solo->setDefaultAction(m_soloAction);


    //m_globalKey = new QToolButton();
	m_globalKeyAction = new QAction(this);
	m_globalKeyAction->setToolTip(tr("EPIC: Enable editing across all parts"));
	m_globalKeyAction->setIcon(*globalKeysIconSet3);
	m_globalKeyAction->setCheckable(true);
	//m_globalKey->setDefaultAction(globalKeyAction);

    //m_globalArm = new QToolButton();
	m_globalArmAction = new QAction(this);
	m_globalArmAction->setToolTip(tr("EPIC: Globally record arm all parts"));
	m_globalArmAction->setIcon(*globalArmIconSet3);
	//m_globalArm->setDefaultAction(globalArmAction);

    m_mutePart = new QToolButton();
	m_muteAction = new QAction(this);
	m_muteAction->setShortcut(shortcuts[SHRT_PART_TOGGLE_MUTE].key);
	m_muteAction->setToolTip(tr("Mute current part"));
	//m_muteAction->setIconSize(soloIconOn->size());
	m_muteAction->setIcon(*muteIconSet3);
	m_muteAction->setCheckable(true);
	m_mutePart->setDefaultAction(m_muteAction);

	QToolBar *cursorBar = new QToolBar(tr("Cursor"));
	posLabel = new PosLabel(0, "pos");
	posLabel->setFixedHeight(25);
	posLabel->setObjectName("Cursor");
	cursorBar->setObjectName("CursorBar");
	cursorBar->addWidget(posLabel);

	pitchLabel = new PitchLabel(0);
	pitchLabel->setFixedHeight(25);
	pitchLabel->setObjectName("pitchLabel");
	cursorBar->addWidget(pitchLabel);

	patchLabel = new QLabel();
	patchLabel->setObjectName("patchLabel");
	patchLabel->setMaximumSize(QSize(180, 22));
	patchLabel->setFixedWidth(280);
	//patchLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	patchLabel->setFixedHeight(22);
	menuBar()->setCornerWidget(patchLabel, Qt::TopRightCorner);
	patchLabel->show();
	//cursorBar->addWidget(patchLabel);

	addToolBar(Qt::BottomToolBarArea, cursorBar);
	cursorBar->setFloatable(false);
	cursorBar->setMovable(false);
	cursorBar->setAllowedAreas(Qt::BottomToolBarArea);
	
	tools22 = new EditToolBar(this, performerTools);
	//tools22->setVisible(false);
    tools2 = new QToolBar(tr("Edit Tools"));
	tools2->setObjectName("PREditToolBar");
	tools2->setIconSize(QSize(29, 25));
	addToolBar(Qt::BottomToolBarArea, tools2);
	tools2->setFloatable(false);
	tools2->setMovable(false);
	tools2->setAllowedAreas(Qt::BottomToolBarArea);
	QWidget* tspacer = new QWidget();
	tspacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tspacer->setMaximumWidth(170);
	tools2->addWidget(tspacer);

	bool showPanic = false;
	bool showMuteSolo = true;
	TransportToolbar *transportbar = new TransportToolbar(this,showPanic,showMuteSolo);
	transportbar->setMuteAction(m_muteAction);
	transportbar->setSoloAction(m_soloAction);
	tools2->addWidget(transportbar);
	connect(transportbar, SIGNAL(recordTriggered(bool)), this, SLOT(checkPartLengthForRecord(bool)));
	QWidget* spacer55 = new QWidget();
	spacer55->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer55->setMaximumWidth(15);
	tools2->addWidget(spacer55);
	//EditTools *edittools = new EditTools(tools22->getActions(), this);
	tools2->addWidget(tools22);
	QWidget* spacer555 = new QWidget();
	spacer555->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer555->setMaximumWidth(15);
	tools2->addWidget(spacer555);
	QList<QAction*> epicList;
	epicList.append(m_globalKeyAction);
	epicList.append(m_globalArmAction);

	EpicToolbar* epicBar = new EpicToolbar(epicList, this);
	tools2->addWidget(epicBar);
	noteAlphaAction->setChecked(true);
	QWidget* spacer5555 = new QWidget();
	spacer5555->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer5555->setMaximumWidth(15);
	tools2->addWidget(spacer5555);
	LoopToolbar* loopBar = new LoopToolbar(Qt::Horizontal, this);
	tools2->addWidget(loopBar);
	QWidget* spacer55555 = new QWidget();
	spacer55555->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer55555->setMaximumWidth(15);
	tools2->addWidget(spacer55555);
	
	QList<QAction*> miscList;
	miscList.append(m_stepAction);
	miscList.append(m_speakerAction);

	MiscToolbar* miscBar = new MiscToolbar(miscList, this);
	tools2->addWidget(miscBar);
	QWidget* spacer5 = new QWidget();
	spacer5->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer5->setMaximumWidth(15);
	tools2->addWidget(spacer5);
	//tools2->addWidget(srec);
	//tools2->addWidget(speaker);
	//tools2->addAction(panicAction);
	/*#ifdef LSCP_SUPPORT
	QToolButton *btnLSCP = new QToolButton();
	btnLSCP->setText(tr("L"));
	btnLSCP->setToolTip(tr("Click the refresh the LSCP Event subscription"));
	tools2->addWidget(btnLSCP);
	connect(btnLSCP, SIGNAL(clicked()), oom, SLOT(restartLSCPSubscribe()));
#endif*/
    QSizeGrip* corner = new QSizeGrip(mainw);
	QWidget* spacer3 = new QWidget();
	spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//spacer3->setMaximumWidth(5);
	tools2->addWidget(spacer3);
	tools2->addWidget(corner);


	//transport->setAllowedAreas(Qt::BottomToolBarArea);
	//transport->setFloatable(false);
	//transport->setMovable(false);
	//transport->setIconSize(QSize(22, 22));

	//addToolBarBreak();
	//toolbar = new Toolbar1(this, _rasterInit, _quantInit);
	//addToolBar(toolbar);

	//addToolBarBreak();
	info = new NoteInfo(this);
	//addToolBar(Qt::TopToolBarArea, info);
	//info->setAllowedAreas(Qt::TopToolBarArea);
	//info->setFloatable(false);
	//info->setMovable(false);
	//info->hide();

	//---------------------------------------------------
	//    split
	//---------------------------------------------------

	splitter = new Splitter(Qt::Vertical, this, "splitter");
	splitter->setHandleWidth(2);

	hsplitter = new Splitter(Qt::Horizontal, mainw, "hsplitter");
	hsplitter->setChildrenCollapsible(true);
	hsplitter->setHandleWidth(2);

	QPushButton* ctrl = new QPushButton(tr(""), mainw);
	ctrl->setObjectName("Ctrl");
	ctrl->setFont(config.fonts[3]);
	ctrl->setToolTip(tr("Add Controller Lane"));
	ctrl->setIcon(*plus_OffIcon);
	ctrl->setIconSize(QSize(25,25));
	ctrl->setFixedSize(QSize(25,25));
	hscroll = new ScrollScale(-25, -2, xscale, 20000, Qt::Horizontal, mainw);
	ctrl->setFixedSize(pianoWidth, hscroll->sizeHint().height());
	//ctrl->setFixedSize(pianoWidth / 2, hscroll->sizeHint().height());  // Tim.

	midiConductor = new Conductor(this, 0, _rasterInit, _quantInit);
	midiConductor->setObjectName("prTrackInfo");
	midiConductor->btnPrev->setIcon(*previousIconSet3);
	midiConductor->btnPrev->setIconSize(QSize(25, 25));
	midiConductor->btnPrev->setFixedSize(QSize(25, 25));
	midiConductor->btnPrev->setToolTip(tr("Select Previous Part"));
	midiConductor->btnNext->setIcon(*nextIconSet3);
	midiConductor->btnNext->setIconSize(QSize(25, 25));
	midiConductor->btnNext->setFixedSize(QSize(25, 25));
	midiConductor->btnNext->setToolTip(tr("Select Next Part"));
	midiConductor->btnPrev->setVisible(true);
	midiConductor->btnNext->setVisible(true);
	connect(midiConductor->btnPrev, SIGNAL(clicked()), this, SLOT(selectPrevPart()));
	connect(midiConductor->btnNext, SIGNAL(clicked()), this, SLOT(selectNextPart()));
	int mtiw = 280; //midiConductor->width(); // Save this.
	//midiConductor->setMinimumWidth(100);
	midiConductor->setMinimumSize(QSize(190,100));
	//midiConductor->setMaximumWidth(300);
	// Catch left/right arrow key events for this widget so we
	// can easily move the focus back from this widget to the canvas.
	installEventFilter(this);
	midiConductor->installEventFilter(this);
	midiConductor->getView()->installEventFilter(this);
	midiConductor->getPatchListview()->installEventFilter(this);

	connect(hsplitter, SIGNAL(splitterMoved(int, int)), midiConductor, SLOT(updateSize()));
	connect(hsplitter, SIGNAL(splitterMoved(int, int)),  SLOT(splitterMoved(int, int)));

	m_trackListView = new TrackListView(this ,this);

	m_tabs->addTab(midiConductor, tr("   Conductor   "));
	m_tabs->addTab(m_trackListView, tr("   Track List   "));
	m_tabs->addTab(info, tr("   Note Info   "));
	//hsplitter->addWidget(midiConductor);
	hsplitter->addWidget(splitter);

	mainGrid->setRowStretch(0, 100);
	mainGrid->setColumnStretch(1, 100);
	mainGrid->addWidget(hsplitter, 0, 1, 1, 3);

	QWidget* split1 = new QWidget(splitter);
	split1->setObjectName("split1");
	QGridLayout* gridS1 = new QGridLayout(split1);
	gridS1->setContentsMargins(0, 0, 0, 0);
	gridS1->setSpacing(0);
	//Defined and configure your program change bar here.
	//This may well be a copy of MTScale extended for our needs
	pcbar = new PCScale(&_raster, split1, this, xscale);
	//pcbar->setAudio(audio);
	//pcbar->setEditor(this);
	time = new MTScale(&_raster, split1, xscale);
	/*Piano*/ piano = new Piano(split1, yscale, this);
    canvas = new PerformerCanvas(this, split1, xscale, yscale);
    vscroll = new ScrollScale(-1, 7, yscale, KH * 75, Qt::Vertical, split1);

    int offset = -(config.division / 4);
    canvas->setOrigin(offset, 0);
    canvas->setCanvasTools(performerTools);
    canvas->setFocus();
    connect(canvas, SIGNAL(toolChanged(int)), tools22, SLOT(set(int)));
    time->setOrigin(offset, 0);
    pcbar->setOrigin(offset, 0);

    gridS1->setRowStretch(2, 100);
    gridS1->setColumnStretch(1, 100);

    gridS1->addWidget(pcbar, 0, 1, 1, 2);
    gridS1->addWidget(time, 1, 1, 1, 2);
    gridS1->addWidget(hLine(split1), 2, 0, 1, 3);
    gridS1->addWidget(piano, 3, 0);
    gridS1->addWidget(canvas, 3, 1);
    gridS1->addWidget(vscroll, 3, 2);

    ctrlLane = new Splitter(Qt::Vertical, splitter, "ctrllane");
    QWidget* split2 = new QWidget(splitter);
    split2->setMaximumHeight(hscroll->sizeHint().height());
    split2->setMinimumHeight(hscroll->sizeHint().height());
    QGridLayout* gridS2 = new QGridLayout(split2);
    gridS2->setContentsMargins(0, 0, 0, 0);
    gridS2->setSpacing(0);
    gridS2->setRowStretch(0, 100);
    gridS2->setColumnStretch(1, 100);
    gridS2->addWidget(ctrl, 0, 0);
    gridS2->addWidget(hscroll, 0, 1);

    //gridS2->addWidget(corner, 0, 2, Qt::AlignBottom | Qt::AlignRight);
    //splitter->setChildrenCollapsible(false);
    splitter->setCollapsible(2, false);

    piano->setFixedWidth(pianoWidth);

    // Tim.
    QList<int> mops;
    mops.append(mtiw); // 30 for possible scrollbar
    mops.append(width() - mtiw);
    hsplitter->setSizes(mops);
    hsplitter->setStretchFactor(0, 0);
    hsplitter->setStretchFactor(1, 15);

    connect(tools22, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));

	connect(noteAlphaAction, SIGNAL(toggled(bool)), canvas, SLOT(update()));
    //connect(midiConductor, SIGNAL(outputPortChanged(int)), list, SLOT(redraw()));
	connect(pcbar, SIGNAL(drawSelectedProgram(int, bool)), canvas, SLOT(drawSelectedProgram(int, bool)));
    connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));
    connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));
    connect(info, SIGNAL(enablePartLines(bool)), canvas, SLOT(setDrawPartEndLine(bool)));
    connect(vscroll, SIGNAL(scrollChanged(int)), piano, SLOT(setYPos(int)));
    connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
    connect(vscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setYMag(float)));
    connect(vscroll, SIGNAL(scaleChanged(float)), piano, SLOT(setYMag(float)));

    connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
    connect(hscroll, SIGNAL(scrollChanged(int)), time, SLOT(setXPos(int)));
    connect(hscroll, SIGNAL(scrollChanged(int)), pcbar, SLOT(setXPos(int)));

    connect(hscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setXMag(float)));
    connect(hscroll, SIGNAL(scaleChanged(float)), time, SLOT(setXMag(float)));
    connect(hscroll, SIGNAL(scaleChanged(float)), pcbar, SLOT(setXMag(float)));

    connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
    connect(canvas, SIGNAL(pitchChanged(int)), piano, SLOT(setPitch(int)));
    connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
    connect(canvas, SIGNAL(horizontalScroll(unsigned)), hscroll, SLOT(setPos(unsigned)));
    connect(canvas, SIGNAL(horizontalScrollNoLimit(unsigned)), hscroll, SLOT(setPosNoLimit(unsigned)));
    connect(canvas, SIGNAL(selectionChanged(int, Event&, Part*)), this,
		  SLOT(setSelection(int, Event&, Part*)));

    connect(piano, SIGNAL(keyPressed(int, int, bool)), canvas, SLOT(pianoPressed(int, int, bool)));
    connect(piano, SIGNAL(keyReleased(int, bool)), canvas, SLOT(pianoReleased(int, bool)));
	connect(piano, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
    connect(m_stepAction, SIGNAL(toggled(bool)), SLOT(setSteprec(bool)));
    //connect(midiin, SIGNAL(toggled(bool)), canvas, SLOT(setMidiin(bool)));
    connect(m_speakerAction, SIGNAL(toggled(bool)), SLOT(setSpeaker(bool)));
    connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));
	connect(m_globalArmAction, SIGNAL(triggered()), canvas, SLOT(recordArmAll()));
	connect(m_globalKeyAction, SIGNAL(toggled(bool)), canvas, SLOT(setGlobalKey(bool)));
	connect(m_globalKeyAction, SIGNAL(toggled(bool)), midiConductor, SLOT(setGlobalState(bool)));
	connect(m_globalKeyAction, SIGNAL(toggled(bool)), this, SLOT(toggleEpicEdit(bool)));
	connect(multiPartSelectionAction, SIGNAL(toggled(bool)), this, SLOT(toggleMultiPartSelection(bool)));
	connect(midiConductor, SIGNAL(globalTransposeClicked(bool)), canvas, SLOT(globalTransposeClicked(bool)));
	connect(midiConductor, SIGNAL(toggleComments(bool)), canvas, SLOT(toggleComments(bool)));
	connect(midiConductor, SIGNAL(toggleComments(bool)), canvas, SLOT(toggleComments(bool)));
	connect(m_muteAction, SIGNAL(triggered(bool)), this, SLOT(toggleMuteCurrentPart(bool)));

    connect(hscroll, SIGNAL(scaleChanged(float)), SLOT(updateHScrollRange()));
    piano->setYPos(KH * 30);
    canvas->setYPos(KH * 30);
    vscroll->setPos(KH * 30);
    //setSelection(0, 0, 0); //Really necessary? Causes segfault when only 1 item selected, replaced by the following:
    info->enableTools(false);
	connect(info, SIGNAL(alphaChanged()), canvas, SLOT(update()));

    connect(song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));
    connect(song, SIGNAL(punchinChanged(bool)), canvas, SLOT(update()));
    connect(song, SIGNAL(punchoutChanged(bool)), canvas, SLOT(update()));
    connect(song, SIGNAL(loopChanged(bool)), canvas, SLOT(update()));
    //connect(song, SIGNAL(playbackStateChanged(bool)), SLOT(playStateChanged(bool)));

    setWindowTitle("The Performer:     " + canvas->getCaption());

    updateHScrollRange();
    // connect to toolbar
    connect(canvas, SIGNAL(pitchChanged(int)), pitchLabel, SLOT(setPitch(int)));
    connect(canvas, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
    connect(piano, SIGNAL(pitchChanged(int)), pitchLabel, SLOT(setPitch(int)));
    connect(time, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
    //connect(pcbar, SIGNAL(selectInstrument()), midiConductor, SLOT(instrPopup()));
    connect(pcbar, SIGNAL(addProgramChange(Part*, unsigned)), midiConductor, SLOT(insertMatrixEvent(Part*, unsigned)));
    connect(midiConductor, SIGNAL(quantChanged(int)), SLOT(setQuant(int)));
    connect(midiConductor, SIGNAL(rasterChanged(int)), SLOT(setRaster(int)));
    connect(midiConductor, SIGNAL(toChanged(int)), SLOT(setTo(int)));
    connect(midiConductor, SIGNAL(updateCurrentPatch(QString)), patchLabel, SLOT(setText(QString)));
    connect(canvas, SIGNAL(partChanged(Part*)), midiConductor, SLOT(editorPartChanged(Part*)));
    connect(m_soloAction, SIGNAL(triggered(bool)), SLOT(soloChanged(bool)));
	connect(midiConductor, SIGNAL(patchChanged(Patch*)), this ,SLOT(setKeyBindings(Patch*)));
    //connect(oom, SIGNAL(channelInfoChanged(const LSCPChannelInfo&)), this, SLOT(setKeyBindings(const LSCPChannelInfo&)));

    setFocusPolicy(Qt::StrongFocus);
    setEventColorMode(colorMode);
    canvas->setMidiin(true);
    midiin->setChecked(true);
    canvas->playEvents(true);
    m_speakerAction->setChecked(true);

    QClipboard* cb = QApplication::clipboard();
    connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

    clipboardChanged(); // enable/disable "Paste"
    selectionChanged(); // enable/disable "Copy" & "Paste"
    initShortcuts(); // initialize shortcuts

    const Pos cpos = song->cPos();
    canvas->setPos(0, cpos.tick(), true);
    //	canvas->selectAtTick(cpos.tick());
    //canvas->selectFirst();
    //
    if (canvas->track())
    {
		song->setRecordFlag(canvas->track(), true);
		song->deselectTracks();
		canvas->track()->setSelected(true);
		song->update(SC_SELECTION);

	 	updateConductor();
	 	m_soloAction->blockSignals(true);
	 	m_soloAction->setChecked(canvas->track()->solo());
	 	m_soloAction->blockSignals(false);
		Part* part = curCanvasPart();
		if(part)
		{
			m_muteAction->blockSignals(true);
			m_muteAction->setChecked(part->mute());
			m_muteAction->blockSignals(false);
		}
    }

    unsigned pos;
    if (initPos >= MAXINT)
	  pos = song->cpos();
    else
	  pos = initPos;
    if (pos > MAXINT)
	  pos = MAXINT;

	bool showcomment = tconfig().get_property("PerformerEdit", "showcomments", false).toBool();
	//printf("Canvas show comments: %d\n", showcomment);
	midiConductor->updateCommentState(showcomment, false);
	CtrlEdit* mainVol = addCtrl();
	if (!mainVol->setType(QString("MainVolume")))
		removeCtrl(mainVol);
	addCtrl(); //Velocity
	CtrlEdit* modctrl = addCtrl();
	modctrl->setType(QString("Modulation"));
	restoreState(tconfig().get_property("PerformerEdit", "windowstate", "").toByteArray());
	/*if (!modctrl->setType(QString("Modulation")))
	{
		modctrl->hide();
		performer->removeCtrl(modctrl);
	}*/

    // At this point in time the range of the canvas hasn't
    // been calculated right ?
    // Also, why wanting to restore some initPos, what is initPos?
    // To me, it seems to make a lot more sense to use the actual
    // current song cpos.
    // This is now done via the showEvent();

    //      hscroll->setOffset((int)pos); // changed that to:
}
void Performer::toggleMuteCurrentPart(bool mute)
{
	if(canvas)
	{
		Part* part = curCanvasPart();
		if(part)
		{
			part->setMute(mute);
			song->update(SC_SELECTION);
		}
	}
}

void Performer::checkPartLengthForRecord(bool rec)/*{{{*/
{
	if(!rec)
		return;
	PartList* pl = parts();
	for(iPart p = pl->begin(); p != pl->end(); ++p)
	{
		Part* part = p->second; //curCanvasPart();
		if(part && part->track()->recordFlag())
		{
			unsigned spos = song->cpos();
			unsigned ptick = part->tick();
			unsigned pend = ptick + part->lenTick();
			int diff = spos - pend;
			if(diff > 0)
			{//resize part
				Part* newPart = part->clone();
				unsigned ntick = newPart->lenTick() + diff;
				newPart->setLenTick(ntick + (rasterStep(ntick)*2));
				// Indicate no undo, and do port controller values but not clone parts.
				audio->msgChangePart(part, newPart, true, true, false);
				setCurCanvasPart(newPart);
				song->update(SC_PART_MODIFIED);
			}
		}
	}
}/*}}}*/

void Performer::setCurCanvasPart(Part* part)
{
	if (canvas)
	{
		//printf("Performer::setCurCanvasPart\n");
		canvas->setCurrentPart(part);
		m_muteAction->blockSignals(true);
		m_muteAction->setChecked(part->mute());
		m_muteAction->blockSignals(false);
	}
	updateConductor();
	song->update(SC_SELECTION);
}

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void Performer::songChanged1(int bits)
{

	//if (bits & SC_SOLO)
	//{
		m_soloAction->blockSignals(true);
		m_soloAction->setChecked(canvas->track()->solo());
		m_soloAction->blockSignals(false);
	//	return;
	//}
	songChanged(bits);
	//midiConductor->songChanged(bits);
	// We'll receive SC_SELECTION if a different part is selected.
	if (bits & SC_SELECTION)
		updateConductor();
	if (bits & SC_MUTE)
	{
		Part* part = curCanvasPart();
		if(part)
		{
			m_muteAction->blockSignals(true);
			m_muteAction->setChecked(part->mute());
			m_muteAction->blockSignals(false);
		}
	}	
}

void Performer::selectPrevPart()
{
	cmd(PerformerCanvas::CMD_SELECT_PREV_PART);
}

void Performer::selectNextPart()
{
	cmd(PerformerCanvas::CMD_SELECT_NEXT_PART);
}

void Performer::dockAreaChanged(Qt::DockWidgetArea area)
{
	switch(area)
	{
		case Qt::LeftDockWidgetArea:
			m_tabs->setTabPosition(QTabWidget::West);
		break;
		case Qt::RightDockWidgetArea:
			m_tabs->setTabPosition(QTabWidget::East);
		break;
		default:
		break;
	}
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Performer::configChanged()
{
	initShortcuts();
	//midiConductor->updateConductor();
}

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void Performer::updateHScrollRange()
{
	int s, e;
	canvas->range(&s, &e);
	// Show one more measure.
	e += AL::sigmap.ticksMeasure(e);
	// Show another quarter measure due to imprecise drawing at canvas end point.
	e += AL::sigmap.ticksMeasure(e) / 4;
	// Compensate for the fixed piano and vscroll widths.
	e += canvas->rmapxDev(pianoWidth - vscroll->width());
	int s1, e1;
	hscroll->range(&s1, &e1);
	if (s != s1 || e != e1)
		hscroll->setRange(s, e);
}

void Performer::updateConductor()
{
	if(selected != curCanvasPart()->track())
	{
		selected = curCanvasPart()->track();
		if (selected->isMidiTrack())
		{
			midiConductor->setTrack(selected);
			///midiConductor->updateConductor(-1);
		}
	}
	midiConductor->updateCommentState(canvas->showComments(), false);
}

//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void Performer::follow(int pos)
{
	int s, e;
	canvas->range(&s, &e);

	if (pos < e && pos >= s)
		hscroll->setOffset(pos);
	if (pos < s)
		hscroll->setOffset(s);
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void Performer::setTime(unsigned tick)
{
	if (tick != MAXINT)
		posLabel->setValue(tick);
	time->setPos(3, tick, false);
    pcbar->setPos(3, tick, false);
}

//---------------------------------------------------------
//   ~Performer
//---------------------------------------------------------

Performer::~Performer()
{
	// undoRedo->removeFrom(tools);  // p4.0.6 Removed
	// store widget size to global config
	tconfig().set_property("PerformerEdit", "widgetwidth", width());
	tconfig().set_property("PerformerEdit", "widgetheigth", height());
	tconfig().set_property("PerformerEdit", "hscale", hscroll->mag());
	tconfig().set_property("PerformerEdit", "yscale", vscroll->mag());
	tconfig().set_property("PerformerEdit", "ypos", vscroll->pos());
	tconfig().set_property("PerformerEdit", "colormode", colorMode);
	tconfig().set_property("PerformerEdit", "showcomments", canvas->showComments());
	tconfig().set_property("PerformerEdit", "windowstate", saveState());
	//tconfig().set_property("PerformerEdit", "showghostpart", noteAlphaAction->isChecked());
	//printf("Canvas show comments: %d\n", canvas->showComments());
    tconfig().save();
	for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();i != ctrlEditList.end(); ++i)
	{
		ctrlEditList.erase(i);
		break;
	}
}

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void Performer::cmd(int cmd)
{
	((PerformerCanvas*) canvas)->cmd(cmd, _quantStrength, _quantLimit, _quantLen, _to);
}

void Performer::toggleMultiPartSelection(bool toggle)
{
	if(toggle)
	{
		m_globalKeyAction->setChecked(!toggle);
		tools22->set(PointerTool);
	}	
}

void Performer::toggleEpicEdit(bool toggle)
{
	if(toggle)
		multiPartSelectionAction->setChecked(!toggle);
}

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void Performer::setSelection(int tick, Event& e, Part* p)
{
	int selections = canvas->selectionSize();

	selEvent = e;
	selPart = (MidiPart*) p;
	selTick = tick;

	if (selections > 1)
	{
		info->enableTools(true);
		info->setDeltaMode(true);
		if (!deltaMode)
		{
			deltaMode = true;
			info->setValues(0, 0, 0, 0, 0);
			tickOffset = 0;
			lenOffset = 0;
			pitchOffset = 0;
			veloOnOffset = 0;
			veloOffOffset = 0;
		}
	}
	else if (selections == 1)
	{
		deltaMode = false;
		info->enableTools(true);
		info->setDeltaMode(false);
		info->setValues(tick,
						selEvent.lenTick(),
						selEvent.pitch(),
						selEvent.velo(),
						selEvent.veloOff());
	}
	else
	{
		deltaMode = false;
		info->enableTools(false);
	}
	selectionChanged();
}

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void Performer::noteinfoChanged(NoteInfo::ValType type, int val)
{
	int selections = canvas->selectionSize();

	if (selections == 0)
	{
		printf("noteinfoChanged while nothing selected\n");
	}
	else if (selections == 1)
	{
		Event event = selEvent.clone();
		switch (type)
		{
		case NoteInfo::VAL_TIME:
			event.setTick(val - selPart->tick());
			break;
		case NoteInfo::VAL_LEN:
			event.setLenTick(val);
			break;
		case NoteInfo::VAL_VELON:
			event.setVelo(val);
			break;
		case NoteInfo::VAL_VELOFF:
			event.setVeloOff(val);
			break;
		case NoteInfo::VAL_PITCH:
			event.setPitch(val);
			break;
		}
		// Indicate do undo, and do not do port controller values and clone parts.
		//audio->msgChangeEvent(selEvent, event, selPart);
		audio->msgChangeEvent(selEvent, event, selPart, true, false, false);
	}
	else
	{
		// multiple events are selected; treat noteinfo values
		// as offsets to event values

		int delta = 0;
		switch (type)
		{
		case NoteInfo::VAL_TIME:
			delta = val - tickOffset;
			tickOffset = val;
			break;
		case NoteInfo::VAL_LEN:
			delta = val - lenOffset;
			lenOffset = val;
			break;
		case NoteInfo::VAL_VELON:
			delta = val - veloOnOffset;
			veloOnOffset = val;
			break;
		case NoteInfo::VAL_VELOFF:
			delta = val - veloOffOffset;
			veloOffOffset = val;
			break;
		case NoteInfo::VAL_PITCH:
			delta = val - pitchOffset;
			pitchOffset = val;
			break;
		}
		if (delta)
			canvas->modifySelected(type, delta);
	}
}

void Performer::updateCanvas()
{
	for(std::list<CtrlEdit*>::iterator i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
	{
		CtrlEdit* edit = (CtrlEdit*)*i;
		if(edit)
			edit->updateCanvas();
	}
	canvas->update();
}

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* Performer::addCtrl()
{
	///CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, xscale, false, "pianoCtrlEdit");
	CtrlEdit* ctrlEdit = new CtrlEdit(ctrlLane/*splitter*/, this, xscale, false, "pianoCtrlEdit"); // ccharrett
	connect(tools22, SIGNAL(toolChanged(int)), ctrlEdit, SLOT(setTool(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scaleChanged(float)), ctrlEdit, SLOT(setXMag(float)));
	connect(ctrlEdit, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
	connect(ctrlEdit, SIGNAL(yposChanged(int)), pitchLabel, SLOT(setInt(int)));
	connect(info, SIGNAL(alphaChanged()), ctrlEdit, SLOT(updateCanvas()));
	connect(noteAlphaAction, SIGNAL(toggled(bool)), ctrlEdit, SLOT(updateCanvas()));

	ctrlEdit->setTool(tools22->curTool());
	ctrlEdit->setXPos(hscroll->pos());
	ctrlEdit->setXMag(hscroll->getScaleValue());

	ctrlEdit->show();
	ctrlEditList.push_back(ctrlEdit);
	return ctrlEdit;
}

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void Performer::removeCtrl(CtrlEdit* ctrl)
{
	for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
	i != ctrlEditList.end(); ++i)
	{
		if (*i == ctrl)
		{
			ctrlEditList.erase(i);
			break;
		}
	}
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void Performer::closeEvent(QCloseEvent* e)
{
	tconfig().set_property("PerformerEdit", "widgetwidth", width());
	tconfig().set_property("PerformerEdit", "widgetheigth", height());
	tconfig().set_property("PerformerEdit", "hscale", hscroll->mag());
	tconfig().set_property("PerformerEdit", "yscale", vscroll->mag());
	tconfig().set_property("PerformerEdit", "ypos", vscroll->pos());
	tconfig().set_property("PerformerEdit", "colormode", colorMode);
	tconfig().set_property("PerformerEdit", "showcomments", canvas->showComments());
	//tconfig().set_property("PerformerEdit", "showghostpart", noteAlphaAction->isChecked());
	//printf("Canvas show comments: %d\n", canvas->showComments());
    tconfig().save();
	emit deleted((unsigned long) this);
	e->accept();
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void Performer::readConfiguration(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
		case Xml::TagStart:
			if (tag == "quant")
				_quantInit = xml.parseInt();
			else if (tag == "raster")
				_rasterInit = xml.parseInt();
			else if (tag == "quantStrength")
				_quantStrengthInit = xml.parseInt();
			else if (tag == "quantLimit")
				_quantLimitInit = xml.parseInt();
			else if (tag == "quantLen")
				_quantLenInit = xml.parseInt();
			else if (tag == "to")
				_toInit = xml.parseInt();
			else if (tag == "colormode")
				colorModeInit = xml.parseInt();
			else if (tag == "width")
				_widthInit = xml.parseInt();
			else if (tag == "height")
				_heightInit = xml.parseInt();
			else
				xml.unknown("Performer");
			break;
		case Xml::TagEnd:
			if (tag == "performer")
				return;
		default:
			break;
		}
	}
}

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void Performer::writeConfiguration(int level, Xml& xml)
{
	xml.tag(level++, "performer");
	xml.intTag(level, "quant", _quantInit);
	xml.intTag(level, "raster", _rasterInit);
	xml.intTag(level, "quantStrength", _quantStrengthInit);
	xml.intTag(level, "quantLimit", _quantLimitInit);
	xml.intTag(level, "quantLen", _quantLenInit);
	xml.intTag(level, "to", _toInit);
	xml.intTag(level, "width", _widthInit);
	xml.intTag(level, "height", _heightInit);
	xml.intTag(level, "colormode", colorModeInit);
    xml.etag(--level, "performer");
}

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void Performer::soloChanged(bool flag)
{
	audio->msgSetSolo(canvas->track(), flag);
	song->update(SC_SOLO);
}

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Performer::setRaster(int val)
{
	_rasterInit = val;
	AbstractMidiEditor::setRaster(val);
	canvas->redrawGrid();
	canvas->setFocus(); // give back focus after kb input

    // falkTX. force update of control canvas
    // FIXME - there should be a better way to do this..
    song->update(0);
}

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void Performer::setQuant(int val)
{
	_quantInit = val;
	AbstractMidiEditor::setQuant(val);
	canvas->setFocus();
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Performer::writeStatus(int level, Xml& xml) const
{
	writePartList(level, xml);
	xml.tag(level++, "performer");
	AbstractMidiEditor::writeStatus(level, xml);
	splitter->writeStatus(level, xml);
	hsplitter->writeStatus(level, xml);

	for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
	i != ctrlEditList.end(); ++i)
	{
		(*i)->writeStatus(level, xml);
	}

	xml.intTag(level, "steprec", canvas->steprec());
	xml.intTag(level, "midiin", canvas->midiin());
	xml.intTag(level, "tool", int(canvas->tool()));
	xml.intTag(level, "quantStrength", _quantStrength);
	xml.intTag(level, "quantLimit", _quantLimit);
	xml.intTag(level, "quantLen", _quantLen);
	xml.intTag(level, "playEvents", _playEvents);
	xml.intTag(level, "xpos", hscroll->pos());
	xml.intTag(level, "xmag", hscroll->mag());
	xml.intTag(level, "ypos", vscroll->pos());
	xml.intTag(level, "ymag", vscroll->mag());
    xml.tag(--level, "/performer");
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Performer::readStatus(Xml& xml)
{
	printf("readstatus\n");
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
		case Xml::TagStart:
			if (tag == "steprec")
			{
				int val = xml.parseInt();
				canvas->setSteprec(val);
				m_stepAction->setChecked(val);
			}
			else if (tag == "midiin")
			{
				int val = xml.parseInt();
				canvas->setMidiin(val);
				midiin->setChecked(val);
			}
			else if (tag == "tool")
			{
				int tool = xml.parseInt();
				canvas->setTool(tool);
				tools22->set(tool);
			}
			else if (tag == "midieditor")
				AbstractMidiEditor::readStatus(xml);
			else if (tag == "ctrledit")
			{
				CtrlEdit* ctrl = addCtrl();
				ctrl->readStatus(xml);
			}
			else if (tag == splitter->objectName())
				splitter->readStatus(xml);
			else if (tag == hsplitter->objectName())
				hsplitter->readStatus(xml);
			else if (tag == "quantStrength")
				_quantStrength = xml.parseInt();
			else if (tag == "quantLimit")
				_quantLimit = xml.parseInt();
			else if (tag == "quantLen")
				_quantLen = xml.parseInt();
			else if (tag == "playEvents")
			{
				_playEvents = xml.parseInt();
				canvas->playEvents(_playEvents);
				m_speakerAction->setChecked(_playEvents);
			}
			else if (tag == "xmag")
				hscroll->setMag(xml.parseInt());
			else if (tag == "xpos")
				hscroll->setPos(xml.parseInt());
			else if (tag == "ymag")
				vscroll->setMag(xml.parseInt());
			else if (tag == "ypos")
				vscroll->setPos(xml.parseInt());
			else
				xml.unknown("Performer");
			break;
			case Xml::TagEnd:
			if (tag == "pianoroll" || tag == "performer")
			{
				_quantInit = _quant;
				_rasterInit = _raster;
				midiConductor->setRaster(_raster);
				midiConductor->setQuant(_quant);
				canvas->redrawGrid();
				return;
			}
			default:
			break;
		}
	}
}

static int rasterTable[] = {
	//-9----8-  7    6     5     4    3(1/4)     2   1
	4, 8, 16, 32, 64, 128, 256, 512, 1024, // triple
	6, 12, 24, 48, 96, 192, 384, 768, 1536,
	9, 18, 36, 72, 144, 288, 576, 1152, 2304 // dot
};

bool Performer::isGlobalEdit()
{
	return m_globalKeyAction->isChecked();
}

bool Performer::eventFilter(QObject *obj, QEvent *event)
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
			if (canvas->hasFocus())
			{
				midiConductor->getPatchListview()->setFocus();
			}
			else if (midiConductor->getPatchListview()->hasFocus())
			{
				midiConductor->getView()->setFocus();
			}
			else
			{
				canvas->setFocus(Qt::MouseFocusReason);
			}
			return true;
		}
		if (key == shortcuts[SHRT_TOGGLE_STEPRECORD].key)
		{
			m_stepAction->toggle();
			return true;
		}
		if (key == shortcuts[SHRT_MIDI_PANIC].key)
		{
			song->panic();
			return true;
		}
		if(key == shortcuts[SHRT_SEL_INSTRUMENT].key)
		{
			midiConductor->addSelectedPatch();
			return true;
		}
		if(key == shortcuts[SHRT_PREVIEW_INSTRUMENT].key)
		{
			midiConductor->previewSelectedPatch();
			return true;
		}
		else if (key == shortcuts[SHRT_ADD_PROGRAM].key)
		{
			unsigned utick = song->cpos() + rasterStep(song->cpos());
			if(m_globalKeyAction->isChecked())
			{
				for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
				{
					Part* part = ip->second;
					midiConductor->insertMatrixEvent(part, utick);
				}
			}
			else
			{
				midiConductor->insertMatrixEvent(curCanvasPart(), utick);
			}
			return true;
		}
		else if (key == shortcuts[SHRT_POS_INC].key)
		{
			PerformerCanvas* pc = (PerformerCanvas*) canvas;
			pc->pianoCmd(CMD_RIGHT);
			return true;
		}
		else if (key == shortcuts[SHRT_POS_DEC].key)
		{
			PerformerCanvas* pc = (PerformerCanvas*) canvas;
			pc->pianoCmd(CMD_LEFT);
			return true;
		}
		else if (key == shortcuts[SHRT_ADD_REST].key)
		{
			PerformerCanvas* pc = (PerformerCanvas*) canvas;
			pc->pianoCmd(CMD_RIGHT);
			return true;
		}
	}

	// standard event processing
	return QObject::eventFilter(obj, event);
}

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void Performer::keyPressEvent(QKeyEvent* event)
{

	// Force left/right arrow key events to move the focus
	// back on the canvas if it doesn't have the focus.
	if (!canvas->hasFocus())
	{
		if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
		{
			canvas->setFocus(Qt::MouseFocusReason);
			event->accept();
			return;
		}
	}

	int index;
	int n = sizeof (rasterTable) / sizeof (*rasterTable);
	for (index = 0; index < n; ++index)
		if (rasterTable[index] == raster())
			break;
	if (index == n)
	{
		index = 0;
		// raster 1 is not in table
	}
	int off = (index / 9) * 9;
	index = index % 9;

	int val = 0;

	int key = event->key();

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



	PerformerCanvas* pc = (PerformerCanvas*) canvas;

	if (_stepQwerty && pc->steprec())
	{
		if (key == shortcuts[SHRT_OCTAVE_QWERTY_0].key) {
			pc->setOctaveQwerty(0);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_1].key) {
			pc->setOctaveQwerty(1);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_2].key) {
			pc->setOctaveQwerty(2);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_3].key) {
			pc->setOctaveQwerty(3);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_4].key) {
			pc->setOctaveQwerty(4);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_5].key) {
			pc->setOctaveQwerty(5);
			return;
		} else if (key == shortcuts[SHRT_OCTAVE_QWERTY_6].key) {
			pc->setOctaveQwerty(6);
			return;
		}

		if(pc->stepInputQwerty(event))
		{
			return;
		}
	}


	if (key == Qt::Key_Escape)
	{
		close();
		return;
	}
	else if (key == shortcuts[SHRT_LE_INS_POLY_AFTERTOUCH].key)
	{
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_POINTER].key)
	{
		tools22->set(PointerTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_PENCIL].key)
	{
		tools22->set(PencilTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_RUBBER].key)
	{
		tools22->set(RubberTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key)
	{
		tools22->set(DrawTool);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC].key)
	{
		pc->pianoCmd(CMD_RIGHT);
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC].key)
	{
		pc->pianoCmd(CMD_LEFT);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key)
	{
		pc->pianoCmd(CMD_RIGHT_NOSNAP);
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key)
	{
		pc->pianoCmd(CMD_LEFT_NOSNAP);
		return;
	}
	else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key)
	{
		pc->pianoCmd(CMD_INSERT);
		return;
	}
	else if (key == Qt::Key_Delete)
	{
		pc->pianoCmd(CMD_DELETE);
		return;
	}
	else if (key == shortcuts[SHRT_ZOOM_IN].key)
	{
		int mag = hscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl < 23)
			zoomlvl++;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		hscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_ZOOM_OUT].key)
	{
		int mag = hscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl > 1)
			zoomlvl--;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		hscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_VZOOM_IN].key)
	{
		int mag = vscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl < 23)
			zoomlvl++;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		vscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_VZOOM_OUT].key)
	{
		int mag = vscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl > 1)
			zoomlvl--;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		vscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_GOTO_CPOS].key)
	{
		PartList* p = this->parts();
		Part* first = p->begin()->second;
		hscroll->setPos(song->cpos() - first->tick());
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_LEFT].key)
	{
		int pos = hscroll->pos() - config.division;
		if (pos < 0)
			pos = 0;
		hscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_RIGHT].key)
	{
		int pos = hscroll->pos() + config.division;
		hscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_UP].key)
	{
		int pos = vscroll->pos() - (config.division/2);
		if (pos < 0)
			pos = 0;
		vscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_DOWN].key)
	{
		int pos = vscroll->pos() + (config.division/2);
		vscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SEL_INSTRUMENT].key)
	{
		midiConductor->addSelectedPatch();
		return;
	}
	else if (key == shortcuts[SHRT_ADD_PROGRAM].key)
	{
		unsigned utick = song->cpos() + rasterStep(song->cpos());
		if(m_globalKeyAction->isChecked())
		{
			for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
			{
				Part* part = ip->second;
				midiConductor->insertMatrixEvent(part, utick);
			}
		}
		else
		{
			midiConductor->insertMatrixEvent(curCanvasPart(), utick);
		}
		//midiConductor->insertMatrixEvent(curCanvasPart(), utick); //progRecClicked();
		return;
	}
	if(key == shortcuts[SHRT_COPY_PROGRAM].key)
	{
		pcbar->copySelected(true);
		return;
	}
	if(key == shortcuts[SHRT_SEL_PROGRAM].key)
	{
		pcbar->selectProgramChange(song->cpos());
		return;
	}
	if(key == shortcuts[SHRT_LMOVE_PROGRAM].key)
	{
		pcbar->moveSelected(-1);
		return;
	}
	if(key == shortcuts[SHRT_RMOVE_PROGRAM].key)
	{
		pcbar->moveSelected(1);
		return;
	}
	else if (key == shortcuts[SHRT_DEL_PROGRAM].key)
	{
		//printf("Delete KeyStroke recieved\n");
		int x = song->cpos();
		if(m_globalKeyAction->isChecked())
		{
			for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
			{
				Part* mprt = ip->second;
				EventList* eventList = mprt->events();
				if(eventList && !eventList->empty())
				{
					for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
					{
						//Get event type.
						Event pcevt = evt->second;
						if (!pcevt.isNote())
						{
							if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
							{
								int xp = pcevt.tick() + mprt->tick();
								if (xp >= x && xp <= (x + 50))
								{
									pcbar->deleteProgramChange(pcevt);
									song->startUndo();
									audio->msgDeleteEvent(evt->second, mprt/*p->second*/, true, true, false);
									song->endUndo(SC_EVENT_MODIFIED);
									break;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			Part* mprt = curCanvasPart();/*{{{*/
			EventList* eventList = mprt->events();
			if(eventList && !eventList->empty())
			{
				for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
				{
					//Get event type.
					Event pcevt = evt->second;
					if (!pcevt.isNote())
					{
						if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
						{
							int xp = pcevt.tick() + mprt->tick();
							if (xp >= x && xp <= (x + 50))
							{
								pcbar->deleteProgramChange(pcevt);
								song->startUndo();
								audio->msgDeleteEvent(evt->second, mprt/*p->second*/, true, true, false);
								song->endUndo(SC_EVENT_MODIFIED);
								break;
							}
						}
					}
				}
			}/*}}}*/
		}
		pcbar->update();
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_1].key)
		val = rasterTable[8 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_2].key)
		val = rasterTable[7 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_3].key)
		val = rasterTable[6 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_4].key)
		val = rasterTable[5 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_5].key)
		val = rasterTable[4 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_6].key)
		val = rasterTable[3 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_7].key)
		val = rasterTable[2 + off];
	else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
		val = rasterTable[index + ((off == 0) ? 9 : 0)];
	else if (key == shortcuts[SHRT_EVENT_COLOR].key)
	{
		if (colorMode == 0)
			colorMode = 1;
		else if (colorMode == 1)
			colorMode = 2;
		else
			colorMode = 0;
		setEventColorMode(colorMode);
		return;
	}
	else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
		val = rasterTable[index + ((off == 18) ? 9 : 18)];

	else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key)
	{//CDW
		if ((off == 18) && (index > 2))
		{
			val = rasterTable[index + 9 - 1];
		}
		else if ((off == 9) && (index < 8))
		{
			val = rasterTable[index + 18 + 1];
		}
		else
			return;
	}
	else if (key == shortcuts[SHRT_TOGGLE_STEPRECORD].key)
	{
		m_stepAction->toggle();
		return;
	}
	else if (key == shortcuts[SHRT_TOGGLE_STEPQWERTY].key)
	{
		_stepQwerty = !_stepQwerty;
		if (_stepQwerty && !m_stepAction->isChecked()) {
			m_stepAction->toggle();
		}
		if (!_stepQwerty && m_stepAction->isChecked()) {
			m_stepAction->toggle();
		}

		return;
	}
	else if (key == shortcuts[SHRT_NOTE_VELOCITY_UP].key)
	{
		canvas->modifySelected(NoteInfo::VAL_VELON, 5, true);
		/*CItemList list = canvas->getSelectedItemsForCurrentPart();

		song->startUndo();
		for (iCItem k = list.begin(); k != list.end(); ++k)
		{
			NEvent* nevent = (NEvent*) (k->second);
			Event event = nevent->event();
			if (event.type() != Note)
				continue;

			int velo = event.velo();
			velo += 5;

			if (velo <= 0)
				velo = 1;
			if (velo > 127)
				velo = 127;
			if (event.velo() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVelo(velo);
				// Indicate no undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, nevent->part(), false);
				audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
			}
		}
		song->endUndo(SC_EVENT_MODIFIED);*/
		return;

	}
	else if (key == shortcuts[SHRT_NOTE_VELOCITY_DOWN].key)
	{
		canvas->modifySelected(NoteInfo::VAL_VELON, -5, true);
		/*CItemList list = canvas->getSelectedItemsForCurrentPart();

		song->startUndo();
		for (iCItem k = list.begin(); k != list.end(); ++k)
		{
			NEvent* nevent = (NEvent*) (k->second);
			Event event = nevent->event();
			if (event.type() != Note)
				continue;

			int velo = event.velo();
			velo -= 5;

			if (velo <= 0)
				velo = 1;
			if (velo > 127)
				velo = 127;
			if (event.velo() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVelo(velo);
				// Indicate no undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, nevent->part(), false);
				audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
			}
		}
		song->endUndo(SC_EVENT_MODIFIED);*/
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_TOGGLE_SOLO].key)
	{
		if (canvas->part()) {
			Track* t = canvas->part()->track();
			audio->msgSetSolo(t, !t->solo());
			song->update(SC_SOLO);
		}
		return;
	}
	else if (key == shortcuts[SHRT_TOGGLE_SOUND].key)
	{
		m_speakerAction->toggle();
		return;
	}
	else if (key == shortcuts[SHRT_START_REC].key)
	{
		//
		if (!audio->isPlaying())
		{
			checkPartLengthForRecord(!song->record());
			song->setRecord(!song->record());
		}
		//printf("Record key pressed from PR\n");
		return;
	}
	else if (key == shortcuts[SHRT_PLAY_TOGGLE].key)/*{{{*/
	{
		checkPartLengthForRecord(song->record());
		if (audio->isPlaying())
			//song->setStopPlay(false);
			song->setStop(true);
		else if (!config.useOldStyleStopShortCut)
			song->setPlay(true);
		else if (song->cpos() != song->lpos())
			song->setPos(0, song->lPos());
		else
		{
			Pos p(0, true);
			song->setPos(0, p);
		}
		return;
	}/*}}}*/
	else
	{ //Default:
		event->ignore();
		return;
	}
	setQuant(val);
	setRaster(val);
	midiConductor->setQuant(_quant);
	midiConductor->setRaster(_raster);
}

//---------------------------------------------------------
//   configQuant
//---------------------------------------------------------

void Performer::configQuant()
{
	if (!quantConfig)
	{
		quantConfig = new QuantConfig(_quantStrength, _quantLimit, _quantLen);
		connect(quantConfig, SIGNAL(setQuantStrength(int)), SLOT(setQuantStrength(int)));
		connect(quantConfig, SIGNAL(setQuantLimit(int)), SLOT(setQuantLimit(int)));
		connect(quantConfig, SIGNAL(setQuantLen(bool)), SLOT(setQuantLen(bool)));
	}
	quantConfig->show();
}

//---------------------------------------------------------
//   setSteprec
//---------------------------------------------------------

void Performer::setSteprec(bool flag)
{
	canvas->setSteprec(flag);
	//if (flag == false)
	//      midiin->setChecked(flag);
}

//---------------------------------------------------------
//   eventColorModeChanged
//---------------------------------------------------------

void Performer::eventColorModeChanged(int mode)
{
	colorMode = mode;
	colorModeInit = colorMode;

	((PerformerCanvas*) (canvas))->setColorMode(colorMode);
}

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void Performer::setEventColorMode(int mode)
{
	colorMode = mode;
	colorModeInit = colorMode;

	///eventColor->setItemChecked(0, mode == 0);
	///eventColor->setItemChecked(1, mode == 1);
	///eventColor->setItemChecked(2, mode == 2);
	evColorBlueAction->setChecked(mode == 0);
	evColorPitchAction->setChecked(mode == 1);
	evColorVelAction->setChecked(mode == 2);

	((PerformerCanvas*) (canvas))->setColorMode(colorMode);
}

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void Performer::clipboardChanged()
{
	editPasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-eventlist")));
}

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void Performer::selectionChanged()
{
	bool flag = canvas->selectionSize() > 0;
	editCutAction->setEnabled(flag);
	editCopyAction->setEnabled(flag);
	editDelEventsAction->setEnabled(flag);
}

//---------------------------------------------------------
//   setSpeaker
//---------------------------------------------------------

void Performer::setSpeaker(bool val)
{
	_playEvents = val;
	canvas->playEvents(_playEvents);
}


void Performer::setKeyBindings(Patch* p)
{
	//if(!audio->isPlaying())
	//{
		if(debugMsg)
			printf("Debug: Updating patch - keys: %d, switches: %d\n", p->keys.size(), p->keyswitches.size());
		piano->setMIDIKeyBindings(p->keys, p->keyswitches);
		canvas->update();
	//}
}

//---------------------------------------------------------
//   setKeyBindings
//---------------------------------------------------------

#ifdef LSCP_SUPPORT
void Performer::setKeyBindings(LSCPChannelInfo info)/*{{{*/
{
	printf("entering Performer::setKeyBindings\n");
	if(!selected || audio->isPlaying())
		return;
	printf("not playing and selected\n");
	//check if the lscp information is pertaining to this track and port
	Track *t = curCanvasPart()->track();
	//RouteList *rl = t->inRoutes();
	printf("info.hbank = %d, info.lbank = %d, info.program = %d\n", info.hbank, info.lbank, info.program);
	MidiTrack* midiTrack = static_cast<MidiTrack*>(t);
	if (!midiTrack)
	{
		printf("not a midi track\n"); // Remon: fixed typo :)
		return;
	}
	else
	{
		printf("found midi track\n");
	}
	//const char* pname = info.midi_portname;

	printf("info midi portname %s\n", info.midi_portname.toAscii().data());
    MidiPort* mp = &midiPorts[midiTrack->outPort()];
	MidiDevice* dev = mp->device();
	if (!dev)
	{
		return;
	}
    RouteList *rl = dev->outRoutes();

	//printf("rl.size() %d\n", rl->size());

	//for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
    for(ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
	{
		printf("oom-port-name: %s, lscp-port-name: %s\n", (*ir).name().toAscii().data(), info.midi_portname.toAscii().data());

		QStringList tmp2 = (*ir).name().split(":", QString::SkipEmptyParts);
		if(tmp2.size() > 1)
		{
			QString portname = tmp2.at(1).trimmed();
			if(portname	== info.midi_portname)
			{
				printf("port names match\n");
				if(isCurrentPatch(info.hbank, info.lbank, info.program))
				{
					printf("is current patch calling setMIDIKeyBindings\n");
					piano->setMIDIKeyBindings(info.key_bindings, info.keyswitch_bindings);
				}
				else
				{
					printf("hbank, lbank and program did not match\n");
				}
				break;
			}
			else
			{
				printf("no match\n");
			}
		}
	}
}/*}}}*/
#endif

bool Performer::isCurrentPatch(int hbank, int lbank, int prog)/*{{{*/
{
	printf("entering Performer::isCurrentPatch\n");
	if(!selected)
		return false;
	MidiTrack *tr = (MidiTrack*)selected;
	int outChannel = tr->outChannel();
	int outPort = tr->outPort();
	MidiPort* mp = &midiPorts[outPort];
	int program = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
	if (program == CTRL_VAL_UNKNOWN)
	{
		program = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
	}

	if (program == CTRL_VAL_UNKNOWN)
	{
		return false;
	}
	int hb = ((program >> 16) & 0xff);
	if (hb == 0x100)
		hb = 0;
	int lb = ((program >> 8) & 0xff);
	if (lb == 0x100)
		lb = 0;
	int pr = (program & 0xff);
	if (pr == 0x100)
		pr = 0;


	printf("leaving Performer::isCurrentPatch\n");

	return (hb == hbank && lb == lbank && pr == prog);

}/*}}}*/

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Performer::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);
	_widthInit = ev->size().width();
	_heightInit = ev->size().height();
}

//---------------------------------------------------------
//   showEvent
//   Now that every gui element is created, including
//   the scroll bars, what about updating the scrollbars
//   so that the play cursor is in the center of the viewport?
//---------------------------------------------------------

void Performer::showEvent(QShowEvent *)
{

	int w = tconfig().get_property("PerformerEdit", "widgetwidth", 924).toInt();
	int h = tconfig().get_property("PerformerEdit", "widgetheigth", 650).toInt();
	int dw = qApp->desktop()->width();
	int dh = qApp->desktop()->height();
	if(h <= dh && w <= dw)
	{
		//printf("Restoring window state\n");
		resize(w, h);
	}
	else
	{
		//printf("Desktop size too large for saved state\n");
		showMaximized();
	}
		
	// maybe add a bool flag to follow: centered ?
	// couldn't find a function that does that directly.
	follow(song->cpos());
	// now that the cursor is in the view, move the view
	// half the canvas width so the cursor is centered.

	hscroll->setPos(hscroll->pos() - (canvas->width() / 2));
	int hScale = tconfig().get_property("PerformerEdit", "hscale", 346).toInt();
	int vScale = tconfig().get_property("PerformerEdit", "yscale", 286).toInt();
	int yPos = tconfig().get_property("PerformerEdit", "ypos", 0).toInt();
	hscroll->setMag(hScale);
	vscroll->setMag(vScale);
	vscroll->setPos(yPos);
	QList<int> vl2;
	QString str2 = tconfig().get_property("splitter", "sizes", "347 218 33").toString();
	QStringList sl2 = str2.split(QString(" "), QString::SkipEmptyParts);
	for (QStringList::Iterator it2 = sl2.begin(); it2 != sl2.end(); ++it2)
	{
		int val = (*it2).toInt();
		vl2.append(val);
	}
	splitter->setSizes(vl2);
	QList<int> vl;
	QString str = tconfig().get_property("hsplitter", "sizes", "200").toString();
	QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
	for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it)
	{
		int val = (*it).toInt();
		vl.append(val);
	}
	hsplitter->setSizes(vl);
	
	QList<int> vl3;/*{{{*/
	//str = "78 50 78 ";//tconfig().get_property("ctrllane", "sizes", "78 50 78 ").toString();
	str = tconfig().get_property("ctrllane", "sizes", "78 50 78 ").toString();
	//printf("Control Lane Sizes: %s\n", str.toUtf8().constData());
	sl = str.split(QString(" "), QString::SkipEmptyParts);
	for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it)
	{
		int val = (*it).toInt();
		vl3.append(val);
	}
	ctrlLane->setSizes(vl3);/*}}}*/
	//ctrlLane
}

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void Performer::initShortcuts()
{
	editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
	editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
	editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
	editDelEventsAction->setShortcut(shortcuts[SHRT_DELETE].key);

	selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
	selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
	selectInvertAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
	selectInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
	selectOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
	selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
	selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);

	eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
	//evColorBlueAction->setShortcut(shortcuts[  ].key);
	//evColorPitchAction->setShortcut(shortcuts[  ].key);
	//evColorVelAction->setShortcut(shortcuts[  ].key);

	funcOverQuantAction->setShortcut(shortcuts[SHRT_OVER_QUANTIZE].key);
	funcNoteOnQuantAction->setShortcut(shortcuts[SHRT_ON_QUANTIZE].key);
	funcNoteOnOffQuantAction->setShortcut(shortcuts[SHRT_ONOFF_QUANTIZE].key);
	funcIterQuantAction->setShortcut(shortcuts[SHRT_ITERATIVE_QUANTIZE].key);

	funcConfigQuantAction->setShortcut(shortcuts[SHRT_CONFIG_QUANT].key);

	funcGateTimeAction->setShortcut(shortcuts[SHRT_MODIFY_GATE_TIME].key);
	funcModVelAction->setShortcut(shortcuts[SHRT_MODIFY_VELOCITY].key);
	/*
	funcCrescendoAction->setShortcut(shortcuts[SHRT_CRESCENDO].key);
	funcTransposeAction->setShortcut(shortcuts[SHRT_TRANSPOSE].key);
	funcThinOutAction->setShortcut(shortcuts[SHRT_THIN_OUT].key);
	funcEraseEventAction->setShortcut(shortcuts[SHRT_ERASE_EVENT].key);
	funcNoteShiftAction->setShortcut(shortcuts[SHRT_NOTE_SHIFT].key);
	funcMoveClockAction->setShortcut(shortcuts[SHRT_MOVE_CLOCK].key);
	funcCopyMeasureAction->setShortcut(shortcuts[SHRT_COPY_MEASURE].key);
	funcEraseMeasureAction->setShortcut(shortcuts[SHRT_ERASE_MEASURE].key);
	funcDelMeasureAction->setShortcut(shortcuts[SHRT_DELETE_MEASURE].key);
	funcCreateMeasureAction->setShortcut(shortcuts[SHRT_CREATE_MEASURE].key);
	*/
	funcSetFixedLenAction->setShortcut(shortcuts[SHRT_FIXED_LEN].key);
	funcDelOverlapsAction->setShortcut(shortcuts[SHRT_DELETE_OVERLAPS].key);

}

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------

void Performer::execDeliveredScript(int id)
{
	//QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
	QString scriptfile = song->getScriptPath(id, true);
	song->executeScript(scriptfile.toAscii().data(), parts(), quant(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------

void Performer::execUserScript(int id)
{
	QString scriptfile = song->getScriptPath(id, false);
	song->executeScript(scriptfile.toAscii().data(), parts(), quant(), true);
}

//---------------------------------------------------------
//   newCanvasWidth
//---------------------------------------------------------

void Performer::newCanvasWidth(int /*w*/)
{
	/*
		  int nw = w + (vscroll->width() - 18); // 18 is the fixed width of the CtlEdit VScale widget.
		  if(nw < 1)
			nw = 1;

		  for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
			 i != ctrlEditList.end(); ++i) {
				// Changed by Tim. p3.3.7
				//(*i)->setCanvasWidth(w);
				(*i)->setCanvasWidth(nw);
				}

		  updateHScrollRange();
	 */
}

void Performer::splitterMoved(int pos, int)
{
	if(pos < midiConductor->minimumSize().width())
	{
		QList<int> def;
		def.append(midiConductor->minimumSize().width());
		def.append(50);
		hsplitter->setSizes(def);
	}
}
