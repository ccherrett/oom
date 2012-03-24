//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QClipboard>
#include <QMessageBox>
#include <QShortcut>
#include <QSignalMapper>
#include <QTimer>
#include <QWhatsThis>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QDomNode>
#include <QTextStream>
#include <QDockWidget>
#include <QProgressDialog>
#include <QSizeGrip>
#include <QtGui>
#include <QUndoStack>
#include <QUndoView>


#include "app.h"
#include "master/lmaster.h"
#include "al/dsp.h"
#include "AudioMixer.h"
#include "Composer.h"
#include "audio.h"
#include "audiodev.h"
#include "audioprefetch.h"
#include "apconfig.h"
#include "bigtime.h"
#include "cliplist/cliplist.h"
#include "conf.h"
#include "debug.h"
#include "didyouknow.h"
#include "filedialog.h"
#include "gatetime.h"
#include "gconfig.h"
#include "gui.h"
#include "icons.h"
#include "instruments/editinstrument.h"
#include "listedit.h"
#include "marker/markerview.h"
#include "master/masteredit.h"
#include "metronome.h"
#include "midiseq.h"
#include "midiport.h"
#include "mididev.h"
#include "driver/jackmidi.h"
#include "mixdowndialog.h"
#include "Performer.h"
#include "popupmenu.h"
#include "shortcuts.h"
#include "shortcutconfig.h"
#include "songinfo.h"
#include "ticksynth.h"
#include "transport.h"
#include "transpose.h"
#include "widgets/projectcreateimpl.h"
#include "midiassign.h"
#include "midimonitor.h"
#include "confmport.h"
#include "mixerdock.h"
#include "toolbars/transporttools.h"
#include "toolbars/edittools.h"
#include "toolbars/looptools.h"
#include "toolbars/feedbacktools.h"
#include "TrackManager.h"
#include "utils.h"

#include "ccinfo.h"
#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#ifdef VST_SUPPORT
#include "vst.h"
#endif

#include "traverso_shared/TConfig.h"
#include "network/LSThread.h"

//extern void cacheJackRouteNames();

static pthread_t watchdogThread;
//ErrorHandler *error;
static const char* fileOpenText =
		QT_TRANSLATE_NOOP("@default", "Click this button to open a <em>new song</em>.<br>"
		"You can also select the <b>Open command</b> from the File menu.");
static const char* fileSaveText =
		QT_TRANSLATE_NOOP("@default", "Click this button to save the song you are "
		"editing.  You will be prompted for a file name.\n"
		"You can also select the Save command from the File menu.");
static const char* fileNewText = QT_TRANSLATE_NOOP("@default", "Create New Song");

static const char* infoLoopButton = QT_TRANSLATE_NOOP("@default", "loop between left mark and right mark");
static const char* infoPunchinButton = QT_TRANSLATE_NOOP("@default", "record starts at left mark");
static const char* infoPunchoutButton = QT_TRANSLATE_NOOP("@default", "record stops at right mark");
static const char* infoStartButton = QT_TRANSLATE_NOOP("@default", "rewind to start position");
static const char* infoRewindButton = QT_TRANSLATE_NOOP("@default", "rewind current position");
static const char* infoForwardButton = QT_TRANSLATE_NOOP("@default", "move current position");
static const char* infoStopButton = QT_TRANSLATE_NOOP("@default", "stop sequencer");
static const char* infoPlayButton = QT_TRANSLATE_NOOP("@default", "start sequencer play");
static const char* infoRecordButton = QT_TRANSLATE_NOOP("@default", "to record press record and then play");
static const char* infoPanicButton = QT_TRANSLATE_NOOP("@default", "send note off to all midi channels");

#define PROJECT_LIST_LEN  6
static QString* projectList[PROJECT_LIST_LEN];

extern void initMidiSynth();
extern void exitJackAudio();
extern void exitDummyAudio();
// p3.3.39
extern void exitOSC();

#ifdef HAVE_LASH
#include <lash/lash.h>
lash_client_t * lash_client = 0;
extern snd_seq_t * alsaSeq;
#endif /* HAVE_LASH */

int watchAudio, watchAudioPrefetch, watchMidi;
pthread_t splashThread;
//static int performerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | DrawTool;


//PyScript *pyscript;
// void OOMidi::runPythonScript()
// {
//  QString script("test.py");
// // pyscript->runPythonScript(script);
// }

//---------------------------------------------------------
//   sleep function
//---------------------------------------------------------

void microSleep(long msleep)
{
	bool sleepOk = -1;

	while (sleepOk == -1)
		sleepOk = usleep(msleep);
}

void OOMidi::initGlobalInputPorts()/*{{{*/
{
	gInputListPorts.clear();
	if(gInputList.size())
	{
		for(int i = 0; i < gInputList.size(); ++i)
		{
			QPair<int, QString> input = gInputList.at(i);
			addGlobalInput(input);
		}
	}
}/*}}}*/

void OOMidi::addGlobalInput(QPair<int, QString> input)
{

	QString devname = input.second;
	MidiPort* inport = 0;
	MidiDevice* indev = 0;
	QString inputDevName(QString("Input-").append(devname));
	int midiInPort = getFreeMidiPort();
	if(debugMsg)
		qDebug("OOMidi::addGlobalInput: createMidiInputDevice is set: %i", midiInPort);
	inport = &midiPorts[midiInPort];
	int devtype = input.first;
	oomMidiPorts.insert(inport->id(), inport);
	if(devtype == MidiDevice::ALSA_MIDI)
	{
		indev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
		if(indev)
		{
			if(debugMsg)
				qDebug("OOMidi::addGlobalInput: Found MIDI input device: ALSA_MIDI");
			int openFlags = 0;
			openFlags ^= 0x2;
			indev->setOpenFlags(openFlags);
			midiSeq->msgSetMidiDevice(inport, indev);
			gInputListPorts.append(midiInPort);
		}
	}
	else if(devtype == MidiDevice::JACK_MIDI)
	{
		indev = MidiJackDevice::createJackMidiDevice(inputDevName, 3);
		if(indev)
		{
			if(debugMsg)
				qDebug("OOMidi::addGlobalInput: Created MIDI input device: JACK_MIDI");
			int openFlags = 0;
			openFlags ^= 0x2;
			indev->setOpenFlags(openFlags);
			midiSeq->msgSetMidiDevice(inport, indev);
			gInputListPorts.append(midiInPort);
		}
	}
	if(indev && indev->deviceType() == MidiDevice::JACK_MIDI)
	{
		if(debugMsg)
			qDebug("OOMidi::addGlobalInput: MIDI input device configured, Adding input routes to MIDI port");
		Route srcRoute(devname, false, -1, Route::JACK_ROUTE);
		Route dstRoute(indev, -1);

		audio->msgAddRoute(srcRoute, dstRoute);

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}

//---------------------------------------------------------
//   seqStart
//---------------------------------------------------------

bool OOMidi::seqStart()
{
	if (audio->isRunning())
	{
		printf("seqStart(): already running\n");
		return true;
	}

	if (!audio->start())
	{
		QMessageBox::critical(oom, tr("Failed to start audio!"),
				tr("Was not able to start audio, check if jack is running.\n"));
		return false;
	}

	//
	// wait for jack callback
	//
	for (int i = 0; i < 60; ++i)
	{
		if (audio->isRunning())
			break;
		sleep(1);
	}
	if (!audio->isRunning())
	{
		QMessageBox::critical(oom, tr("Failed to start audio!"),
				tr("Timeout waiting for audio to run. Check if jack is running.\n"));
	}
	//
	// now its safe to ask the driver for realtime
	// priority

	realTimePriority = audioDevice->realtimePriority();
	if (debugMsg)
		printf("OOMidi::seqStart: getting audio driver realTimePriority:%d\n", realTimePriority);

	int pfprio = 0;
	int midiprio = 0;
	int monitorprio = 0;

	// NOTE: realTimeScheduling can be true (gotten using jack_is_realtime()),
	//  while the determined realTimePriority can be 0.
	// realTimePriority is gotten using pthread_getschedparam() on the client thread
	//  in JackAudioDevice::realtimePriority() which is a bit flawed - it reports there's no RT...
	if (realTimeScheduling)
	{
		//monitorprio = realTimePriority + 1;

		pfprio = realTimePriority + 1;

		midiprio = realTimePriority + 2;
	}

	if (midiRTPrioOverride > 0)
		midiprio = midiRTPrioOverride;

	// FIXME FIXME: The realTimePriority of the Jack thread seems to always be 5 less than the value passed to jackd command.
	//if(midiprio == realTimePriority)
	//  printf("OOMidi: WARNING: Midi realtime priority %d is the same as audio realtime priority %d. Try a different setting.\n",
	//         midiprio, realTimePriority);

	//Starting midiMonitor
	printf("Starting midiMonitor\n");
	midiMonitor->start(monitorprio);

	audioPrefetch->start(pfprio);

	audioPrefetch->msgSeek(0, true); // force

	midiSeq->start(midiprio);

	int counter = 0;
	while (++counter)
	{
		if (counter > 1000)
		{
			fprintf(stderr, "midi sequencer thread does not start!? Exiting...\n");
			exit(33);
		}
		midiSeqRunning = midiSeq->isRunning();
		if (midiSeqRunning)
			break;
		usleep(1000);
		if(debugMsg)
			printf("looping waiting for sequencer thread to start\n");
	}
	if (!midiSeqRunning)
	{
		fprintf(stderr, "midiSeq is not running! Exiting...\n");
		exit(33);
	}

	midiMonitor->populateList();

#ifdef LSCP_SUPPORT
	//emit lscpStartListener();
#endif
	return true;
}

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void OOMidi::seqStop()
{
	// label sequencer as disabled before it actually happened to minimize race condition
	midiSeqRunning = false;
#ifdef LSCP_SUPPORT
	//emit lscpStopListener();
#endif

	song->setStop(true);
	song->setStopPlay(false);
	//Stop the midiMonitor before the sequencer
	printf("Stoping midiMonitor\n");
	midiMonitor->stop(true);
	midiSeq->stop(true);
	audio->stop(true);
	audioPrefetch->stop(true);
    // close opened synths
    for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
    {
        MidiDevice* md = *i;
        if (md && md->isSynthPlugin())
            // can be safely closed even if not active
            md->close();
    }
	if (realTimeScheduling && watchdogThread)
		pthread_cancel(watchdogThread);

}

//---------------------------------------------------------
//   seqRestart
//---------------------------------------------------------

bool OOMidi::seqRestart()
{
	bool restartSequencer = audio->isRunning();
	if (restartSequencer)
	{
		if (audio->isPlaying())
		{
			audio->msgPlay(false);
			while (audio->isPlaying())
				qApp->processEvents();
		}
		seqStop();
	}
	if (!seqStart())
		return false;

	audioDevice->graphChanged();
	return true;
}

bool OOMidi::startServer()
{
	if(!server.listen(QHostAddress::Any, OOCMD_PORT))
	{
		printf("OOMidi CMS Error: %s\n",server.errorString().toLatin1().constData());
		return false;
	}
	else
	{
		printf("OOMidi Command Server Listening on port: %d\n", OOCMD_PORT);
		return true;
	}
}

void OOMidi::stopServer()
{
	if(server.isListening())
	{
		server.close();
	}
}

#ifdef LSCP_SUPPORT
void OOMidi::restartLSCPSubscribe()
{
	emit lscpStopListener();
	emit lscpStartListener();
}


void OOMidi::startLSCPClient()
{
	lsclient = new LSClient();
	connect(lsclient, SIGNAL(channelInfoChanged(const LSCPChannelInfo&)), SIGNAL(channelInfoChanged(const LSCPChannelInfo&)));
	connect(oom, SIGNAL(lscpStartListener()), lsclient, SLOT(subscribe()));
	connect(oom, SIGNAL(lscpStopListener()), lsclient, SLOT(unsubscribe()));
	//lsclient->start();
}

void OOMidi::stopLSCPClient()
{
	if(lsclient)
	{
		lsclient->stopClient();
	}
}
#endif

void OOMidi::pipelineStateChanged(int state)
{
	switch(state)
	{
		case 0:
			if(!pipelineBox)
			{
				pipelineBox = new QMessageBox(this);
				pipelineBox->setModal(true);
			}
			pipelineBox->setWindowTitle(tr("Pipeline Error"));
			pipelineBox->setText(tr("There has been a Pipeline error"));
			pipelineBox->setInformativeText(tr("One or more of the programs in your Pipeline has crashed\nPlease wait while we restore the Pipeline to a working state."));
			pipelineBox->show();
		break;
		case 1:
			if(pipelineBox)
			{
				pipelineBox->close();
				pipelineBox = 0;
			}
			song->closeJackBox();
		break;
		default:
			printf("Unknown state: %d\n", state);
		break;
	}
}
//---------------------------------------------------------
//   addProject
//---------------------------------------------------------

void addProject(const QString& name)
{
	for (int i = 0; i < PROJECT_LIST_LEN; ++i)
	{
		if (projectList[i] == 0)
			break;
		if (name == *projectList[i])
		{
			int dst = i;
			int src = i + 1;
			int n = PROJECT_LIST_LEN - i - 1;
			delete projectList[i];
			for (int k = 0; k < n; ++k)
				projectList[dst++] = projectList[src++];
			projectList[dst] = 0;
			break;
		}
	}
	QString** s = &projectList[PROJECT_LIST_LEN - 2];
	QString** d = &projectList[PROJECT_LIST_LEN - 1];
	if (*d)
		delete *d;
	for (int i = 0; i < PROJECT_LIST_LEN - 1; ++i)
		*d-- = *s--;
	projectList[0] = new QString(name);
}

//---------------------------------------------------------
//   populateAddTrack
//    this is also used in "mixer"
//---------------------------------------------------------

QActionGroup* populateAddTrack(QMenu* addTrack)
{
	QActionGroup* grp = new QActionGroup(addTrack);

	QAction* midi = addTrack->addAction(QIcon(*addMidiIcon),
			QT_TRANSLATE_NOOP("@default", "Add Midi Track"));
	midi->setData(Track::MIDI);
    grp->addAction(midi);

    /*QAction* drum = addTrack->addAction(QIcon(*addtrack_drumtrackIcon),
			QT_TRANSLATE_NOOP("@default", "Add Drum Track"));
	drum->setData(Track::DRUM);
	grp->addAction(drum);
    */

	QAction* wave = addTrack->addAction(QIcon(*addAudioIcon),
			QT_TRANSLATE_NOOP("@default", "Add Audio Track"));
	wave->setData(Track::WAVE);
	grp->addAction(wave);

    //QAction* synth = addTrack->addAction(QIcon(*addSynthIcon),
    //        QT_TRANSLATE_NOOP("@default", "Add Synth Track"));
    //synth->setData(Track::AUDIO_SOFTSYNTH);
    //grp->addAction(synth);

	QAction* aoutput = addTrack->addAction(QIcon(*addOutputIcon),
			QT_TRANSLATE_NOOP("@default", "Add Audio Output"));
	aoutput->setData(Track::AUDIO_OUTPUT);
	grp->addAction(aoutput);

	QAction* agroup = addTrack->addAction(QIcon(*addBussIcon),
			QT_TRANSLATE_NOOP("@default", "Add Audio Buss"));
	agroup->setData(Track::AUDIO_BUSS);
	grp->addAction(agroup);

	QAction* ainput = addTrack->addAction(QIcon(*addInputIcon),
			QT_TRANSLATE_NOOP("@default", "Add Audio Input"));
	ainput->setData(Track::AUDIO_INPUT);
	grp->addAction(ainput);

	QAction* aaux = addTrack->addAction(QIcon(*addAuxIcon),
			QT_TRANSLATE_NOOP("@default", "Add Aux Send"));
	aaux->setData(Track::AUDIO_AUX);
	grp->addAction(aaux);

	QObject::connect(addTrack, SIGNAL(triggered(QAction *)), song, SLOT(addNewTrack(QAction *)));

	return grp;
}

//---------------------------------------------------------
//   OOMidi
//---------------------------------------------------------

OOMidi::OOMidi(int argc, char** argv) : QMainWindow()
{

	// Very first thing we should do is loading global configuration values
	tconfig().check_and_load_configuration();

	//loadTheme(config.style);
	loadTheme("plastique");
	QApplication::setFont(config.fonts[0]);
	loadStyleSheetFile(config.styleSheetFile);

	setIconSize(ICON_SIZE);
	setFocusPolicy(Qt::WheelFocus);
	oom = this; // hack
	
	//Initialize the trackManager
	trackManager = new TrackManager;
	clipListEdit = 0;
	midiSyncConfig = 0;
	midiRemoteConfig = 0;
	midiPortConfig = 0;
	midiAssignDialog = 0;
	metronomeConfig = 0;
	audioConfig = 0;
	midiFileConfig = 0;
	midiFilterConfig = 0;
	midiInputTransform = 0;
	midiRhythmGenerator = 0;
	globalSettingsConfig = 0;
	markerView = 0;
	softSynthesizerConfig = 0;
	midiTransformerDialog = 0;
	shortcutConfig = 0;
	pipelineBox = 0;
	mixer1 = 0;
	mixer2 = 0;
	watchdogThread = 0;
	editInstrument = 0;
	routingPopupMenu = 0;
	routingDialog = 0;
	firstrun = true;
	m_externalCall = false;
	performer = 0;
	m_rasterVal = 0;

	g_trackColorListLine.insert(Track::AUDIO_INPUT, QColor(189,122,214));
	g_trackColorListLine.insert(Track::MIDI, QColor(1,230,238));
	g_trackColorListLine.insert(Track::DRUM, QColor(1,230,238));
	g_trackColorListLine.insert(Track::AUDIO_OUTPUT, QColor(252,118,118));
	g_trackColorListLine.insert(Track::WAVE, QColor(129,244,118));
	g_trackColorListLine.insert(Track::AUDIO_BUSS, QColor(252,164,36));
	g_trackColorListLine.insert(Track::AUDIO_AUX, QColor(227,233,114));
	g_trackColorListLine.insert(Track::AUDIO_SOFTSYNTH, QColor(129,244,118));

	g_trackColorList.insert(Track::AUDIO_INPUT, QColor(105,105,105));
	g_trackColorList.insert(Track::MIDI, QColor(105,105,105));
	g_trackColorList.insert(Track::DRUM, QColor(105,105,105));
	g_trackColorList.insert(Track::AUDIO_OUTPUT, QColor(105,105,105));
	g_trackColorList.insert(Track::WAVE, QColor(105,105,105));
	g_trackColorList.insert(Track::AUDIO_BUSS, QColor(105,105,105));
	g_trackColorList.insert(Track::AUDIO_AUX, QColor(105,105,105));
	g_trackColorList.insert(Track::AUDIO_SOFTSYNTH, QColor(105,105,105));

	g_trackColorListSelected.insert(Track::AUDIO_INPUT, QColor(189,122,214));
	g_trackColorListSelected.insert(Track::MIDI, QColor(1,230,238));
	g_trackColorListSelected.insert(Track::DRUM, QColor(1,230,238));
	g_trackColorListSelected.insert(Track::AUDIO_OUTPUT, QColor(252,118,118));
	g_trackColorListSelected.insert(Track::WAVE, QColor(129,244,118));
	g_trackColorListSelected.insert(Track::AUDIO_BUSS, QColor(252,164,36));
	g_trackColorListSelected.insert(Track::AUDIO_AUX, QColor(227,233,114));
	g_trackColorListSelected.insert(Track::AUDIO_SOFTSYNTH, QColor(129,244,118));

	g_trackDragImageList.insert(Track::AUDIO_INPUT, *dragMidiIcon);
	g_trackDragImageList.insert(Track::MIDI, *dragInputIcon);
	g_trackDragImageList.insert(Track::DRUM, *dragInputIcon);
	g_trackDragImageList.insert(Track::AUDIO_OUTPUT, *dragOutputIcon);
	g_trackDragImageList.insert(Track::WAVE, *dragAudioIcon);
	g_trackDragImageList.insert(Track::AUDIO_BUSS, *dragBussIcon);
	g_trackDragImageList.insert(Track::AUDIO_AUX, *dragAuxIcon);
	g_trackDragImageList.insert(Track::AUDIO_SOFTSYNTH, *dragAudioIcon);

	appName = QString("The Composer - OOStudio-").append(VERSION).append("     ");
	setWindowTitle(appName);

	qRegisterMetaType<CCInfo>("CCInfo");
	
	editSignalMapper = new QSignalMapper(this);
	midiPluginSignalMapper = new QSignalMapper(this);
	followSignalMapper = new QSignalMapper(this);
	_resourceDock = new QDockWidget(tr("The Orchestra Pit"), this);
	_resourceDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	_resourceDock->setObjectName("dockResourceCenter");
	addDockWidget(Qt::LeftDockWidgetArea, _resourceDock);

	m_undoStack = new QUndoStack(this);
	m_undoView = 0;

    toolbarComposerSettings = 0;
    toolbarSnap = 0;

	song = new Song(m_undoStack, "song");
	song->blockSignals(true);
	heartBeatTimer = new QTimer(this);
	heartBeatTimer->setObjectName("timer");
	connect(heartBeatTimer, SIGNAL(timeout()), song, SLOT(beat()));

#ifdef LSCP_SUPPORT
	lscpRestart = false;
#endif

#ifdef ENABLE_PYTHON
	//---------------------------------------------------
	//    Python bridge
	//---------------------------------------------------
	// Uncomment in order to enable OOMidi Python bridge:
	if (usePythonBridge)
	{
		printf("Initializing python bridge!\n");
		if (initPythonBridge() == false)
		{
			printf("Could not initialize Python bridge\n");
			exit(1);
		}
	}
#endif

	//---------------------------------------------------
	//    undo/redo
	//---------------------------------------------------

	undoRedo = new QActionGroup(this);
	undoRedo->setExclusive(false);
	undoAction = new QAction(QIcon(*undoIconS), tr("Und&o"), undoRedo);
	redoAction = new QAction(QIcon(*redoIconS), tr("Re&do"), undoRedo);

	undoAction->setWhatsThis(tr("undo last change to song"));
	redoAction->setWhatsThis(tr("redo last undo"));
	undoAction->setEnabled(false);
	redoAction->setEnabled(false);
	connect(redoAction, SIGNAL(triggered()), song, SLOT(redo()));
	connect(undoAction, SIGNAL(triggered()), song, SLOT(undo()));

	//---------------------------------------------------
	// Canvas Actions
	//---------------------------------------------------
	noteAlphaAction = new QAction(QIcon(*multiDisplayIconSet3), tr("multipart"), this);
	noteAlphaAction->setToolTip(tr("EPIC: Toggle Display of multiple parts"));
	noteAlphaAction->setCheckable(true);

	multiPartSelectionAction = new QAction(QIcon(*selectMultiIconSet3), tr("multiselection"), this);
	multiPartSelectionAction->setToolTip(tr("EPIC: Toggle ability to select multiple part notes"));
	multiPartSelectionAction->setCheckable(true);
	
	//---------------------------------------------------
	//    Transport
	//---------------------------------------------------

	transportAction = new QActionGroup(this);
	transportAction->setExclusive(false);

	replayAction = new QAction(QIcon(*auditionIconSet3), tr("Toggle Audition Mode"), transportAction);
	replayAction->setCheckable(true);
	connect(replayAction, SIGNAL(toggled(bool)), song, SLOT(setReplay(bool)));



	startAction = new QAction(QIcon(*startIconSet3), tr("Start"), transportAction);

	startAction->setWhatsThis(tr(infoStartButton));
	connect(startAction, SIGNAL(triggered()), song, SLOT(rewindStart()));

	rewindAction = new QAction(QIcon(*rewindIconSet3), tr("Rewind"), transportAction);

	rewindAction->setWhatsThis(tr(infoRewindButton));
	connect(rewindAction, SIGNAL(triggered()), song, SLOT(rewind()));

	forwardAction = new QAction(QIcon(*forwardIconSet3), tr("Forward"), transportAction);

	forwardAction->setWhatsThis(tr(infoForwardButton));
	connect(forwardAction, SIGNAL(triggered()), song, SLOT(forward()));

	stopAction = new QAction(QIcon(*stopIconSet3), tr("Stop"), transportAction);
	stopAction->setCheckable(true);

	stopAction->setWhatsThis(tr(infoStopButton));
	stopAction->setChecked(true);
	connect(stopAction, SIGNAL(toggled(bool)), song, SLOT(setStop(bool)));

	playAction = new QAction(QIcon(*playIconSet3), tr("Play"), transportAction);
	playAction->setCheckable(true);

	playAction->setWhatsThis(tr(infoPlayButton));
	playAction->setChecked(false);
	connect(playAction, SIGNAL(toggled(bool)), song, SLOT(setPlay(bool)));

	recordAction = new QAction(QIcon(*recordIconSet3), tr("Record"), transportAction);
	recordAction->setCheckable(true);
	recordAction->setWhatsThis(tr(infoRecordButton));
	connect(recordAction, SIGNAL(toggled(bool)), song, SLOT(setRecord(bool)));
	
	loopAction = new QAction(QIcon(*loopIconSet3), tr("Loop"), transportAction);/*{{{*/
	loopAction->setCheckable(true);

	loopAction->setWhatsThis(tr(infoLoopButton));
	connect(loopAction, SIGNAL(toggled(bool)), song, SLOT(setLoop(bool)));

	punchinAction = new QAction(QIcon(*punchinIconSet3), tr("Punchin"), transportAction);
	punchinAction->setCheckable(true);

	punchinAction->setWhatsThis(tr(infoPunchinButton));
	connect(punchinAction, SIGNAL(toggled(bool)), song, SLOT(setPunchin(bool)));

	punchoutAction = new QAction(QIcon(*punchoutIconSet3), tr("Punchout"), transportAction);
	punchoutAction->setCheckable(true);

	punchoutAction->setWhatsThis(tr(infoPunchoutButton));
	connect(punchoutAction, SIGNAL(toggled(bool)), song, SLOT(setPunchout(bool)));/*}}}*/

	panicAction = new QAction(QIcon(*panicIconSet3), tr("Panic"), this);
	panicAction->setShortcut(shortcuts[SHRT_MIDI_PANIC].key);

	panicAction->setWhatsThis(tr(infoPanicButton));
	connect(panicAction, SIGNAL(triggered()), song, SLOT(panic()));

	pcloaderAction = new QAction(QIcon(*pcloaderIconSet3), tr("Program Change Loader"), this);
	feedbackAction = new QAction(QIcon(*feedbackIconSet3), tr("Feedback"), this);
	feedbackAction->setCheckable(true);
	connect(feedbackAction, SIGNAL(toggled(bool)), song, SLOT(toggleFeedback(bool)));


	initMidiInstruments();
	initMidiPorts();
	::initMidiDevices();

	//----Actions
	//-------- File Actions

	fileNewAction = new QAction(QIcon(*filenewIcon), tr("&New"), this);
	fileNewAction->setToolTip(tr(fileNewText));
	fileNewAction->setWhatsThis(tr(fileNewText));

	fileOpenAction = new QAction(QIcon(*openIcon), tr("&Open"), this);

	fileOpenAction->setToolTip(tr(fileOpenText));
	fileOpenAction->setWhatsThis(tr(fileOpenText));

	openRecent = new QMenu(tr("Open &Recent"), this);

	fileSaveAction = new QAction(QIcon(*saveIcon), tr("&Save"), this);

	fileSaveAction->setToolTip(tr(fileSaveText));
	fileSaveAction->setWhatsThis(tr(fileSaveText));

	fileSaveAsAction = new QAction(tr("Save &As"), this);

	fileImportMidiAction = new QAction(tr("Import Midifile"), this);
	fileExportMidiAction = new QAction(tr("Export Midifile"), this);
	//fileImportPartAction = new QAction(tr("Import Part"), this);

	//fileImportWaveAction = new QAction(tr("Import Audio File"), this);

	quitAction = new QAction(tr("&Quit"), this);

	//-------- Edit Actions
	editCutAction = new QAction(QIcon(*editcutIconSet), tr("C&ut"), this);
	editCopyAction = new QAction(QIcon(*editcopyIconSet), tr("&Copy"), this);
	editPasteAction = new QAction(QIcon(*editpasteIconSet), tr("&Paste"), this);
	editInsertAction = new QAction(QIcon(*editpasteIconSet), tr("&Insert"), this);
	editPasteCloneAction = new QAction(QIcon(*editpasteCloneIconSet), tr("Paste c&lone"), this);
	editPaste2TrackAction = new QAction(QIcon(*editpaste2TrackIconSet), tr("Paste to &track"), this);
	editPasteC2TAction = new QAction(QIcon(*editpasteClone2TrackIconSet), tr("Paste clone to trac&k"), this);
	editInsertEMAction = new QAction(QIcon(*editpasteIconSet), tr("&Insert Empty Measure"), this);
	editDeleteSelectedAction = new QAction(QIcon(*edit_track_delIcon), tr("Delete Selected Tracks"), this);


	addTrack = new QMenu(tr("Add Track"), this);
	addTrack->setIcon(QIcon(*edit_track_addIcon));
	select = new QMenu(tr("Select"), this);
	select->setIcon(QIcon(*selectIcon));

	editSelectAllAction = new QAction(QIcon(*select_allIcon), tr("Select &All"), this);
	editDeselectAllAction = new QAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"), this);
	editInvertSelectionAction = new QAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"), this);
	editInsideLoopAction = new QAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"), this);
	editOutsideLoopAction = new QAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"), this);
	editAllPartsAction = new QAction(QIcon(*select_all_parts_on_trackIcon), tr("All &Parts on Track"), this);
	editSelectAllTracksAction = new QAction(QIcon(*select_allIcon), tr("Select All &Tracks"), this);
	

	startPianoEditAction = new QAction(*pianoIconSet, tr("Performer"), this);
	startListEditAction = new QAction(QIcon(*edit_listIcon), tr("List"), this);

	master = new QMenu(tr("Tempo Editor"), this);
	master->setIcon(QIcon(*edit_mastertrackIcon));
	masterGraphicAction = new QAction(QIcon(*mastertrack_graphicIcon), tr("Graphic"), this);
	masterListAction = new QAction(QIcon(*mastertrack_listIcon), tr("List"), this);
	masterEnableAction = new QAction(QIcon(*enabledIconSet3), tr("Enable usage of Tempo Editor"), this);
	masterEnableAction->setCheckable(true);
	masterEnableAction->setChecked(song->masterFlag());
	connect(masterEnableAction, SIGNAL(triggered(bool)), song, SLOT(setMasterFlag(bool)));

	midiEdit = new QMenu(tr("Midi"), this);
	midiEdit->setIcon(QIcon(*edit_midiIcon));

	midiTransposeAction = new QAction(QIcon(*midi_transposeIcon), tr("Transpose"), this);
	midiTransformerAction = new QAction(QIcon(*midi_transformIcon), tr("Midi &Transform"), this);

	//editSongInfoAction = new QAction(QIcon(*songInfoIcon), tr("Song Info"), this);

	//-------- View Actions
	viewTransportAction = new QAction(QIcon(*view_transport_windowIcon), tr("Transport Panel"), this);
	viewTransportAction->setCheckable(true);
	viewBigtimeAction = new QAction(QIcon(*view_bigtime_windowIcon), tr("Bigtime Window"), this);
	viewBigtimeAction->setCheckable(true);
	viewMixerAAction = new QAction(QIcon(*mixerSIcon), tr("Mixer"), this);
	viewMixerAAction->setCheckable(true);
	viewCliplistAction = new QAction(QIcon(*cliplistSIcon), tr("Cliplist"), this);
	viewCliplistAction->setCheckable(true);
	viewMarkerAction = new QAction(QIcon(*view_markerIcon), tr("Marker View"), this);
	viewMarkerAction->setCheckable(true);

    viewToolbars = new QMenu(tr("Toolbars"), this);
    viewToolbarOrchestra = new QAction(tr("The Orchestra Pit"), this);
    viewToolbarOrchestra->setCheckable(true);
    viewToolbarMixer = new QAction(tr("The Mixer Dock"), this);
    viewToolbarMixer->setCheckable(true);
    viewToolbarComposerSettings = new QAction(tr("The Composer Settings"), this);
    viewToolbarComposerSettings->setCheckable(true);
    viewToolbarSnap = new QAction(tr("Snap"), this);
    viewToolbarSnap->setCheckable(true);
    viewToolbarTransport = new QAction(tr("Transport Tools"), this);
    viewToolbarTransport->setCheckable(true);

	//-------- Structure Actions
	strGlobalCutAction = new QAction(tr("Global Cut"), this);
	strGlobalInsertAction = new QAction(tr("Global Insert"), this);
	strGlobalSplitAction = new QAction(tr("Global Split"), this);
	strCopyRangeAction = new QAction(tr("Copy Range"), this);
	strCopyRangeAction->setEnabled(false);
	strCutEventsAction = new QAction(tr("Cut Events"), this);
	strCutEventsAction->setEnabled(false);

	//-------- Midi Actions
	menuScriptPlugins = new QMenu(tr("&Plugins"), this);
	midiEditInstAction = new QAction(QIcon(*midi_edit_instrumentIcon), tr("Edit Instrument"), this);
	midiInputPlugins = new QMenu(tr("Input Plugins"), this);
	midiInputPlugins->setIcon(QIcon(*midi_inputpluginsIcon));
	midiTrpAction = new QAction(QIcon(*midi_inputplugins_transposeIcon), tr("Transpose"), this);
	midiInputTrfAction = new QAction(QIcon(*midi_inputplugins_midi_input_transformIcon), tr("Midi Input Transform"), this);
	midiInputFilterAction = new QAction(QIcon(*midi_inputplugins_midi_input_filterIcon), tr("Midi Input Filter"), this);
	midiRemoteAction = new QAction(QIcon(*midi_inputplugins_remote_controlIcon), tr("Midi Remote Control"), this);
#ifdef BUILD_EXPERIMENTAL
	midiRhythmAction = new QAction(QIcon(*midi_inputplugins_random_rhythm_generatorIcon), tr("Rhythm Generator"), this);
#endif
	midiResetInstAction = new QAction(QIcon(*midi_reset_instrIcon), tr("Reset Instr."), this);
	midiInitInstActions = new QAction(QIcon(*midi_init_instrIcon), tr("Init Instr."), this);
	midiLocalOffAction = new QAction(QIcon(*midi_local_offIcon), tr("Local Off"), this);

	//-------- Audio Actions
	audioBounce2TrackAction = new QAction(QIcon(*audio_bounce_to_trackIcon), tr("Bounce to Track"), this);
	audioBounce2FileAction = new QAction(QIcon(*audio_bounce_to_fileIcon), tr("Bounce to File"), this);
	audioRestartAction = new QAction(QIcon(*audio_restartaudioIcon), tr("Restart Audio"), this);

	//-------- Automation Actions
	autoMixerAction = new QAction(QIcon(*automation_mixerIcon), tr("Mixer Automation"), this);
	autoMixerAction->setCheckable(true);
	autoSnapshotAction = new QAction(QIcon(*automation_take_snapshotIcon), tr("Take Snapshot"), this);
	autoClearAction = new QAction(QIcon(*automation_clear_dataIcon), tr("Clear Automation Data"), this);
	autoClearAction->setEnabled(false);

	//-------- Settings Actions
	settingsGlobalAction = new QAction(QIcon(*settings_globalsettingsIcon), tr("Global Settings"), this);
	settingsShortcutsAction = new QAction(QIcon(*settings_configureshortcutsIcon), tr("Configure Shortcuts"), this);
	follow = new QMenu(tr("Follow Song"), this);
	dontFollowAction = new QAction(tr("Don't Follow Song"), this);
	dontFollowAction->setCheckable(true);
	followPageAction = new QAction(tr("Follow Page"), this);
	followPageAction->setCheckable(true);
	followPageAction->setChecked(true);
	followCtsAction = new QAction(tr("Follow Continuous"), this);
	followCtsAction->setCheckable(true);

	settingsMetronomeAction = new QAction(QIcon(*settings_metronomeIcon), tr("Metronome"), this);
	settingsMidiIOAction = new QAction(QIcon(*settings_midifileexportIcon), tr("Midi File Import/Export"), this);
	settingsMidiAssignAction = new QAction(QIcon(*settings_midiport_softsynthsIcon), tr("Connections Manager"), this);

	//-------- Help Actions
	helpManualAction = new QAction(tr("&Manual"), this);
	helpHomepageAction = new QAction(tr("&OOStudio Homepage"), this);
	helpReportAction = new QAction(tr("&Report Bug..."), this);
	helpAboutAction = new QAction(tr("&About OOStudio"), this);


	//---- Connections
	//-------- File connections

	connect(fileNewAction, SIGNAL(triggered()), SLOT(loadTemplate()));
	connect(fileOpenAction, SIGNAL(triggered()), SLOT(loadProject()));
	connect(openRecent, SIGNAL(aboutToShow()), SLOT(openRecentMenu()));
	connect(openRecent, SIGNAL(triggered(QAction*)), SLOT(selectProject(QAction*)));

	connect(fileSaveAction, SIGNAL(triggered()), SLOT(save()));
	connect(fileSaveAsAction, SIGNAL(triggered()), SLOT(saveAs()));

	connect(fileImportMidiAction, SIGNAL(triggered()), SLOT(importMidi()));
	connect(fileExportMidiAction, SIGNAL(triggered()), SLOT(exportMidi()));
	//connect(fileImportPartAction, SIGNAL(triggered()), SLOT(importPart()));

	//connect(fileImportWaveAction, SIGNAL(triggered()), SLOT(importWave()));
	connect(quitAction, SIGNAL(triggered()), SLOT(quitDoc()));

	//-------- Edit connections
	connect(editCutAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editCopyAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editPasteAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editInsertAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editPasteCloneAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editPaste2TrackAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editPasteC2TAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editInsertEMAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editDeleteSelectedAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));

	connect(editSelectAllAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editSelectAllTracksAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editDeselectAllAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editInvertSelectionAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editInsideLoopAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editOutsideLoopAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
	connect(editAllPartsAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));

	editSignalMapper->setMapping(editCutAction, CMD_CUT);
	editSignalMapper->setMapping(editCopyAction, CMD_COPY);
	editSignalMapper->setMapping(editPasteAction, CMD_PASTE);
	editSignalMapper->setMapping(editInsertAction, CMD_INSERT);
	editSignalMapper->setMapping(editPasteCloneAction, CMD_PASTE_CLONE);
	editSignalMapper->setMapping(editPaste2TrackAction, CMD_PASTE_TO_TRACK);
	editSignalMapper->setMapping(editPasteC2TAction, CMD_PASTE_CLONE_TO_TRACK);
	editSignalMapper->setMapping(editInsertEMAction, CMD_INSERTMEAS);
	editSignalMapper->setMapping(editDeleteSelectedAction, CMD_DELETE_TRACK);
	editSignalMapper->setMapping(editSelectAllAction, CMD_SELECT_ALL);
	editSignalMapper->setMapping(editSelectAllTracksAction, CMD_SELECT_ALL_TRACK);
	editSignalMapper->setMapping(editDeselectAllAction, CMD_SELECT_NONE);
	editSignalMapper->setMapping(editInvertSelectionAction, CMD_SELECT_INVERT);
	editSignalMapper->setMapping(editInsideLoopAction, CMD_SELECT_ILOOP);
	editSignalMapper->setMapping(editOutsideLoopAction, CMD_SELECT_OLOOP);
	editSignalMapper->setMapping(editAllPartsAction, CMD_SELECT_PARTS);

	connect(editSignalMapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

	connect(startPianoEditAction, SIGNAL(triggered()), SLOT(startPerformer()));
	connect(startListEditAction, SIGNAL(triggered()), SLOT(startListEditor()));

	connect(masterGraphicAction, SIGNAL(triggered()), SLOT(startMasterEditor()));
	connect(masterListAction, SIGNAL(triggered()), SLOT(startLMasterEditor()));

	connect(midiTransposeAction, SIGNAL(triggered()), SLOT(transpose()));
	connect(midiTransformerAction, SIGNAL(triggered()), SLOT(startMidiTransformer()));

	//-------- View connections
	connect(viewTransportAction, SIGNAL(toggled(bool)), SLOT(toggleTransport(bool)));
	connect(viewBigtimeAction, SIGNAL(toggled(bool)), SLOT(toggleBigTime(bool)));
	connect(viewMixerAAction, SIGNAL(toggled(bool)), SLOT(toggleMixer1(bool)));
	connect(viewCliplistAction, SIGNAL(toggled(bool)), SLOT(startClipList(bool)));
	connect(viewMarkerAction, SIGNAL(toggled(bool)), SLOT(toggleMarker(bool)));

    connect(viewToolbarOrchestra, SIGNAL(toggled(bool)), SLOT(showToolbarOrchestra(bool)));
    connect(viewToolbarMixer, SIGNAL(toggled(bool)), SLOT(showToolbarMixer(bool)));
    connect(viewToolbarComposerSettings, SIGNAL(toggled(bool)), SLOT(showToolbarComposerSettings(bool)));
    connect(viewToolbarSnap, SIGNAL(toggled(bool)), SLOT(showToolbarSnap(bool)));
    connect(viewToolbarTransport, SIGNAL(toggled(bool)), SLOT(showToolbarTransport(bool)));
    connect(viewToolbars, SIGNAL(aboutToShow()), SLOT(updateViewToolbarMenu()));

	//-------- Structure connections
	connect(strGlobalCutAction, SIGNAL(triggered()), SLOT(globalCut()));
	connect(strGlobalInsertAction, SIGNAL(triggered()), SLOT(globalInsert()));
	connect(strGlobalSplitAction, SIGNAL(triggered()), SLOT(globalSplit()));
	connect(strCopyRangeAction, SIGNAL(triggered()), SLOT(copyRange()));
	connect(strCutEventsAction, SIGNAL(triggered()), SLOT(cutEvents()));

	//-------- Midi connections
	connect(midiEditInstAction, SIGNAL(triggered()), SLOT(startEditInstrument()));
	connect(midiResetInstAction, SIGNAL(triggered()), SLOT(resetMidiDevices()));
	connect(midiInitInstActions, SIGNAL(triggered()), SLOT(initMidiDevices()));
	connect(midiLocalOffAction, SIGNAL(triggered()), SLOT(localOff()));

	connect(midiTrpAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
	connect(midiInputTrfAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
	connect(midiInputFilterAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
	connect(midiRemoteAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));

	midiPluginSignalMapper->setMapping(midiTrpAction, 0);
	midiPluginSignalMapper->setMapping(midiInputTrfAction, 1);
	midiPluginSignalMapper->setMapping(midiInputFilterAction, 2);
	midiPluginSignalMapper->setMapping(midiRemoteAction, 3);

#ifdef BUILD_EXPERIMENTAL
	connect(midiRhythmAction, SIGNAL(triggered()), midiPluginSignalMapper, SLOT(map()));
	midiPluginSignalMapper->setMapping(midiRhythmAction, 4);
#endif

	connect(midiPluginSignalMapper, SIGNAL(mapped(int)), this, SLOT(startMidiInputPlugin(int)));

	//-------- Audio connections
	connect(audioBounce2TrackAction, SIGNAL(triggered()), SLOT(bounceToTrack()));
	connect(audioBounce2FileAction, SIGNAL(triggered()), SLOT(bounceToFile()));
	connect(audioRestartAction, SIGNAL(triggered()), SLOT(seqRestart()));

	//-------- Automation connections
	connect(autoMixerAction, SIGNAL(triggered()), SLOT(switchMixerAutomation()));
	connect(autoSnapshotAction, SIGNAL(triggered()), SLOT(takeAutomationSnapshot()));
	connect(autoClearAction, SIGNAL(triggered()), SLOT(clearAutomation()));

	//-------- Settings connections
	connect(settingsGlobalAction, SIGNAL(triggered()), SLOT(configGlobalSettings()));
	connect(settingsShortcutsAction, SIGNAL(triggered()), SLOT(configShortCuts()));
	connect(settingsMetronomeAction, SIGNAL(triggered()), SLOT(configMetronome()));
	connect(settingsMidiIOAction, SIGNAL(triggered()), SLOT(configMidiFile()));
	connect(settingsMidiAssignAction, SIGNAL(triggered()), SLOT(configMidiAssign()));

	connect(dontFollowAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));
	connect(followPageAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));
	connect(followCtsAction, SIGNAL(triggered()), followSignalMapper, SLOT(map()));

	followSignalMapper->setMapping(dontFollowAction, CMD_FOLLOW_NO);
	followSignalMapper->setMapping(followPageAction, CMD_FOLLOW_JUMP);
	followSignalMapper->setMapping(followCtsAction, CMD_FOLLOW_CONTINUOUS);

	connect(followSignalMapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

	//-------- Help connections
	connect(helpManualAction, SIGNAL(triggered()), SLOT(startHelpBrowser()));
	connect(helpHomepageAction, SIGNAL(triggered()), SLOT(startHomepageBrowser()));
	connect(helpAboutAction, SIGNAL(triggered()), SLOT(about()));

	//--------------------------------------------------
	//    Miscellaneous shortcuts
	//--------------------------------------------------

	QShortcut* sc = new QShortcut(shortcuts[SHRT_DELETE].key, this);
	sc->setContext(Qt::WindowShortcut);
	connect(sc, SIGNAL(activated()), editSignalMapper, SLOT(map()));
	editSignalMapper->setMapping(sc, CMD_DELETE);


	if (realTimePriority < sched_get_priority_min(SCHED_FIFO))
		realTimePriority = sched_get_priority_min(SCHED_FIFO);
	else if (realTimePriority > sched_get_priority_max(SCHED_FIFO))
		realTimePriority = sched_get_priority_max(SCHED_FIFO);

	// If we requested to force the midi thread priority...
	if (midiRTPrioOverride > 0)
	{
		if (midiRTPrioOverride < sched_get_priority_min(SCHED_FIFO))
			midiRTPrioOverride = sched_get_priority_min(SCHED_FIFO);
		else if (midiRTPrioOverride > sched_get_priority_max(SCHED_FIFO))
			midiRTPrioOverride = sched_get_priority_max(SCHED_FIFO);
	}

	midiSeq = new MidiSeq("Midi");
	audio = new Audio();
	audioPrefetch = new AudioPrefetch("Prefetch");
	//Define the MidiMonitor
	midiMonitor = new MidiMonitor("MidiMonitor");

	//---------------------------------------------------
	//    Popups
	//---------------------------------------------------

	//-------------------------------------------------------------
	//    popup File
	//-------------------------------------------------------------

	menu_file = menuBar()->addMenu(tr("&File"));
	menu_file->addAction(fileNewAction);
	menu_file->addAction(fileOpenAction);
	menu_file->addMenu(openRecent);
	menu_file->addSeparator();
	menu_file->addAction(fileSaveAction);
	menu_file->addAction(fileSaveAsAction);
	menu_file->addSeparator();
	menu_file->addAction(fileImportMidiAction);
	menu_file->addAction(fileExportMidiAction);
	//menu_file->addAction(fileImportPartAction);
	//menu_file->addSeparator();
	//menu_file->addAction(fileImportWaveAction);
	menu_file->addSeparator();
	menu_file->addAction(quitAction);
	menu_file->addSeparator();

	//-------------------------------------------------------------
	//    popup Edit
	//-------------------------------------------------------------

	menuEdit = menuBar()->addMenu(tr("&Edit"));
	menuEdit->addActions(undoRedo->actions());
	menuEdit->addSeparator();

	menuEdit->addAction(editCutAction);
	menuEdit->addAction(editCopyAction);
	menuEdit->addAction(editPasteAction);
	menuEdit->addAction(editInsertAction);
	menuEdit->addAction(editPasteCloneAction);
	menuEdit->addAction(editPaste2TrackAction);
	menuEdit->addAction(editPasteC2TAction);
	menuEdit->addAction(editInsertEMAction);
	menuEdit->addAction(strGlobalSplitAction);
	menuEdit->addSeparator();
	menuEdit->addAction(editDeleteSelectedAction);

	menuEdit->addMenu(addTrack);
	menuEdit->addMenu(select);
	select->addAction(editSelectAllTracksAction);
	select->addAction(editSelectAllAction);
	select->addAction(editDeselectAllAction);
	select->addAction(editInvertSelectionAction);
	select->addAction(editInsideLoopAction);
	select->addAction(editOutsideLoopAction);
	select->addAction(editAllPartsAction);
	menuEdit->addSeparator();

	menuEdit->addAction(startPianoEditAction);
	menuEdit->addAction(startListEditAction);

	menuEdit->addSeparator();

	//-------------------------------------------------------------
	//    popup View
	//-------------------------------------------------------------

	menuView = menuBar()->addMenu(tr("View"));

	menuView->addMenu(master);
	master->addAction(masterGraphicAction);
	master->addAction(settingsMetronomeAction);
	master->addAction(masterListAction);
	menuView->addAction(viewTransportAction);
	menuView->addAction(viewMixerAAction);
	menuView->addAction(viewBigtimeAction);
	menuView->addAction(viewCliplistAction);
	menuView->addAction(viewMarkerAction);

    menuView->addSeparator();
    menuView->addMenu(viewToolbars);
    viewToolbars->addAction(viewToolbarOrchestra);
    viewToolbars->addAction(viewToolbarMixer);
    //viewToolbars->addSeparator();
    viewToolbars->addAction(viewToolbarComposerSettings);
    viewToolbars->addAction(viewToolbarSnap);
    viewToolbars->addAction(viewToolbarTransport);

	//-------------------------------------------------------------
	//    popup Midi
	//-------------------------------------------------------------

	menu_functions = menuBar()->addMenu(tr("&Midi"));
	menu_functions->addAction(midiEditInstAction);
#ifdef BUILD_EXPERIMENTAL
	midiInputPlugins->addAction(midiRhythmAction);
#endif

	menu_functions->addSeparator();
	menu_functions->addAction(midiLocalOffAction);

	//-------------------------------------------------------------
	//    popup Audio
	//-------------------------------------------------------------

	menu_audio = menuBar()->addMenu(tr("&Audio"));
	menu_audio->addAction(audioBounce2TrackAction);
	menu_audio->addAction(audioBounce2FileAction);
	menu_audio->addSeparator();
	menu_audio->addAction(audioRestartAction);


	//-------------------------------------------------------------
	//    popup Settings
	//-------------------------------------------------------------

	menuSettings = menuBar()->addMenu(tr("Settings"));
	menuSettings->addAction(settingsGlobalAction);
	menuSettings->addAction(settingsShortcutsAction);
	menuSettings->addMenu(follow);
	follow->addAction(dontFollowAction);
	follow->addAction(followPageAction);
	follow->addAction(followCtsAction);
	menuSettings->addSeparator();
	menuSettings->addAction(settingsMidiIOAction);
	menuSettings->addSeparator();
	menuSettings->addAction(settingsMidiAssignAction);

	//---------------------------------------------------
	//    popup Help
	//---------------------------------------------------

	menu_help = menuBar()->addMenu(tr("&Help"));
	menu_help->addAction(helpManualAction);
	menu_help->addAction(helpHomepageAction);
	menu_help->addSeparator();
	menu_help->addAction(helpAboutAction);

	QWidget *faketitle = new QWidget();
	m_mixerDock = new QDockWidget(tr("The Mixer Dock"), this);
	m_mixerDock->setTitleBarWidget(faketitle);
	m_mixerDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	m_mixerDock->setObjectName("m_mixerDock");
	addDockWidget(Qt::BottomDockWidgetArea, m_mixerDock);

	m_mixerWidget = new MixerDock(m_mixerDock);
	m_mixerWidget->setObjectName("MixerDock");
	connect(m_mixerDock, SIGNAL(visibilityChanged(bool)), m_mixerWidget, SLOT(updateConnections(bool)));
	connect(song, SIGNAL(composerViewChanged()), m_mixerWidget, SLOT(composerViewChanged()));
	m_mixerDock->setVisible(false);
	m_mixerDock->setWidget(m_mixerWidget);

	//---------------------------------------------------
	//    Central Widget
	//---------------------------------------------------

	composer = new Composer(this, "composer");
	setCentralWidget(composer);
	addTransportToolbar();

	connect(heartBeatTimer, SIGNAL(timeout()), composer, SLOT(heartBeat()));
	connect(composer, SIGNAL(editPart(Track*)), SLOT(startEditor(Track*)));
	connect(composer, SIGNAL(dropSongFile(const QString&)), SLOT(loadProjectFile(const QString&)));
	connect(composer, SIGNAL(dropMidiFile(const QString&)), SLOT(importMidi(const QString&)));
	connect(composer, SIGNAL(startEditor(PartList*, int)), SLOT(startEditor(PartList*, int)));
	connect(this, SIGNAL(configChanged()), composer, SLOT(configChanged()));
	connect(pcloaderAction, SIGNAL(triggered()), composer, SLOT(preloadControllers()));
	connect(composer, SIGNAL(trackSelectionChanged(qint64)), m_mixerWidget, SLOT(scrollSelectedToView(qint64)));

	connect(composer, SIGNAL(setUsedTool(int)), SLOT(setUsedTool(int)));

	//---------------------------------------------------
	//  read list of "Recent Projects"
	//---------------------------------------------------

	QString prjPath(QString(configPath).append(QDir::separator()).append("projects"));
	QFile projFile(prjPath);
	if (!projFile.open(QIODevice::ReadOnly|QIODevice::Text))
	{
		for (int i = 0; i < PROJECT_LIST_LEN; ++i)
			projectList[i] = 0;
	}
	else
	{
		QTextStream in(&projFile);
		QString path = in.readLine();
		int i = 0;
		while(!path.isNull() && i < PROJECT_LIST_LEN)
		{
			QFileInfo f(path);
			if(f.exists())
			{
				projectList[i] = new QString(path);
				++i;
			}
			path = in.readLine();
		}
		for(;i < PROJECT_LIST_LEN; ++i)
			projectList[i] = 0;
	}

	initMidiSynth();

	QActionGroup *grp = populateAddTrack(addTrack);

	trackMidiAction = grp->actions()[0];
	trackWaveAction = grp->actions()[1];
	trackAOutputAction = grp->actions()[2];
	trackAGroupAction = grp->actions()[3];
	trackAInputAction = grp->actions()[4];
	trackAAuxAction = grp->actions()[5];

	transport = new Transport(this, "transport");
	bigtime = 0;

	QClipboard* cb = QApplication::clipboard();
	connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
	connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));
	connect(composer, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

	//---------------------------------------------------
	//  load project
	//    if no songname entered on command line:
	//    startMode: 0  - load last song
	//               1  - load default template
	//               2  - load configured start song
	//---------------------------------------------------

	m_useTemplate = false;
	if (argc >= 2)/*{{{*/
		m_initProjectName = argv[0];
	else if (config.startMode == 0)
	{
		if (argc < 2)
			m_initProjectName = projectList[0] ? *projectList[0] : QString("untitled");
		else
			m_initProjectName = argv[0];
		printf("starting with selected song %s\n", config.startSong.toLatin1().constData());
	}
	else if (config.startMode == 1)
	{
		printf("starting with default template\n");
		m_initProjectName = oomGlobalShare + QString("/templates/default.oom");
		m_useTemplate = true;
	}
	else if (config.startMode == 2)
	{
		printf("starting with pre configured song %s\n", config.startSong.toLatin1().constData());
		m_initProjectName = config.startSong;
	}/*}}}*/

	/**
	 * Load linuxsampler 
	 */
	if(config.lsClientStartLS)
	{
		if(gSamplerStarted)
			loadInitialProject();
		else
			lsStartupFailed();
		//connect(&gLSThread, SIGNAL(startupSuccess()), this, SLOT(loadInitialProject()));
		//connect(&gLSThread, SIGNAL(startupFailed()), this, SLOT(lsStartupFailed()));
		//gLSThread.start();
	}
	else
	{
		loadInitialProject();
	}

	QSize defaultScreenSize = tconfig().get_property("Interface", "size", QSize(0, 0)).toSize();
	int dw = qApp->desktop()->width();
	int dh = qApp->desktop()->height();
	if(defaultScreenSize.height())
	{
		if(defaultScreenSize.height() <= dh && defaultScreenSize.width() <= dw)
		{
			restoreGeometry(tconfig().get_property("Interface", "geometry", "").toByteArray());
		}
		else
		{
			showMaximized();
		}
	}
	else
	{ //maximize the window
		showMaximized();
	}
	restoreState(tconfig().get_property("Interface", "windowstate", "").toByteArray());
	//restoreDockWidget(_resourceDock);
	//restoreDockWidget(m_mixerDock);


	setCorner(Qt::BottomLeftCorner, Qt:: LeftDockWidgetArea);
}

OOMidi::~OOMidi()
{
	//printf("OOMidi::~OOMidi\n");
	tconfig().set_property("Interface", "size", size());
	tconfig().set_property("Interface", "fullScreen", isFullScreen());
	tconfig().set_property("Interface", "pos", pos());
	tconfig().set_property("Interface", "windowstate", saveState());
	tconfig().set_property("Interface", "geometry", saveGeometry());
    // Save the new global settings to the configuration file
    tconfig().save();
}

void OOMidi::loadInitialProject()
{
	//qDebug("Entering OOMidi::loadInitialProject~~~~~~~~~~~~~~~~~~~~~~~~~~");
	if(config.lsClientAutoStart)
	{
		bool started = false;
		int retry = 0;
		while(!started && retry <= 10)
		{
			lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
			lsClientStarted = lsClient->startClient();
			if(config.lsClientResetOnStart && lsClientStarted)
			{
				lsClient->resetSampler();
			}
			started = lsClient && lsClient->isClientStarted();
			sleep(1);
			retry++;
		}
		if(retry >= 10 && !started)
		{
			QMessageBox::critical(this, tr("LinuxSampler Client Failed!!"), tr("You have configured OOStudio to start a LinuxSampler\n however the LinuxSampler client failed to start"));
		}
	}

	song->blockSignals(false);
	loadProjectFile(m_initProjectName, m_useTemplate, true);
	changeConfig(false);
	readInstrumentTemplates();
	song->update();
	//qDebug("Leaving OOMidi::loadInitialProject~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

void OOMidi::lsStartupFailed()
{
	//qDebug("Entering OOMidi::lsStartupFailed~~~~~~~~~~~~~~~~~~~~~~~~~~");
	m_initProjectName = oomGlobalShare + QString("/templates/default.oom");
	m_useTemplate = true;
	song->blockSignals(false);
	loadProjectFile(m_initProjectName, m_useTemplate, true);
	changeConfig(false);
	readInstrumentTemplates();
	song->update();
	//qDebug("Leaving OOMidi::lsStartupFailed~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

void OOMidi::showUndoView()
{
}

Tool OOMidi::getCurrentTool()
{
    if (composer && composer->getCanvas())
        return composer->getCanvas()->tool();
    // return an invalid tool
    return (Tool)0;
}

/**
 * Called from within the after Composer Constructor
 */
void OOMidi::addTransportToolbar()
{
    tools = new QToolBar(tr("Transport Tools"));
	tools->setAllowedAreas(Qt::BottomToolBarArea);
	tools->setFloatable(false);
	tools->setMovable(false);
	tools->setIconSize(QSize(29, 25));
	tools->setObjectName("transTools");
	addToolBar(Qt::BottomToolBarArea, tools);

	QWidget* spacer15 = new QWidget();
	spacer15->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	spacer15->setFixedWidth(15);
	tools->addWidget(spacer15);

	FeedbackTools * feedbackBar = new FeedbackTools(this);
	tools->addWidget(feedbackBar);
	
	QWidget* spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer->setMaximumWidth(205);
	tools->addWidget(spacer);

	bool showPanic = true;
	bool showMuteSolo = false;

	TransportToolbar *transportbar = new TransportToolbar(this, showPanic, showMuteSolo);
	tools->addWidget(transportbar);
	
	QWidget* tspacer = new QWidget();
	tspacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tspacer->setMaximumWidth(15);
	tools->addWidget(tspacer);
	
	tools1 = new EditToolBar(this, composerTools);
	//addToolBar(Qt::BottomToolBarArea, tools1);
	tools1->setObjectName("tbEditTools");
	connect(tools1, SIGNAL(toolChanged(int)), composer, SLOT(setTool(int)));
	connect(tools1, SIGNAL(toolChanged(int)), composer, SIGNAL(updateHeaderTool(int)));
	connect(composer, SIGNAL(toolChanged(int)), tools1, SLOT(set(int)));
	connect(composer, SIGNAL(updateFooterTool(int)), tools1, SLOT(set(int)));
	//toolByHeaderChanged
	tools->addWidget(tools1);
	QWidget* tspacer2 = new QWidget();
	tspacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tspacer2->setMaximumWidth(15);
	tools->addWidget(tspacer2);

	LoopToolbar* loopBar = new LoopToolbar(Qt::Horizontal, this);
	tools->addWidget(loopBar);
	QWidget* spacer55555 = new QWidget();
	spacer55555->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer55555->setMaximumWidth(15);
	tools->addWidget(spacer55555);

    QSizeGrip* corner = new QSizeGrip(tools);
	QWidget* spacer4444 = new QWidget();
	spacer4444->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tools->addWidget(spacer4444);
	tools->addWidget(corner);
	QWidget* spacer3 = new QWidget();
	spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer3->setMaximumWidth(5);
	tools->addWidget(spacer3);
}

//---------------------------------------------------------
//   setComposerAndSnapToolbars
//---------------------------------------------------------

void OOMidi::setComposerAndSnapToolbars(QToolBar* comp, QToolBar* snap)
{
    toolbarComposerSettings = comp;
    toolbarSnap = snap;
}

//---------------------------------------------------------
//   setHeartBeat
//---------------------------------------------------------

void OOMidi::setHeartBeat()
{
	heartBeatTimer->start(1000 / config.guiRefresh);
}

//---------------------------------------------------------
//   resetDevices
//---------------------------------------------------------

void OOMidi::resetMidiDevices()
{
	audio->msgResetMidiDevices();
}

//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void OOMidi::initMidiDevices()
{
	// Added by T356
	//audio->msgIdle(true);

	audio->msgInitMidiDevices();

	// Added by T356
	//audio->msgIdle(false);
}

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void OOMidi::localOff()
{
	audio->msgLocalOff();
}

//---------------------------------------------------------
//   loadProjectFile
//    load *.oom, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//---------------------------------------------------------

// for drop:

void OOMidi::loadProjectFile(const QString& name)
{
	loadProjectFile(name, false, false);
}


void OOMidi::loadProjectFile(const QString& name, bool songTemplate, bool loadAll)
{
	//FIXME: Check this the file exists before going further
	QFileInfo info(name);
	if(!info.exists())
	{
		QMessageBox::critical(this, tr("Song Open Error"), QString(tr("Failed to open song\nNo such file:")).append(info.filePath()));
		return;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	song->invalid = true;
	//
	// stop audio threads if running
	//
	bool restartSequencer = audio->isRunning();
	if (restartSequencer)
	{
		if (audio->isPlaying())
		{
			audio->msgPlay(false);
			while (audio->isPlaying())
				qApp->processEvents();
		}
		seqStop();
	}
	microSleep(100000);

	loadProjectFile1(name, songTemplate, loadAll);
	microSleep(100000);
	if (restartSequencer)
		seqStart();
	song->invalid = false;
	song->update();

	//if (song->getSongInfo().length() > 0)
	//	startSongInfo(false);
	QApplication::restoreOverrideCursor();
}

//---------------------------------------------------------
//   loadProjectFile
//    load *.oom, *.mid, *.kar
//
//    template - if true, load file but do not change
//                project name
//    loadAll  - load song data + configuration data
//---------------------------------------------------------

void OOMidi::loadProjectFile1(const QString& name, bool songTemplate, bool loadAll)
{
	if (mixer1)
		mixer1->clear();
	m_mixerWidget->clear();
	composer->clear(); // clear track info
	//Clear the ID based oomMidiPorts hash, it will be repopulated when the song loads
	oomMidiPorts.clear();
	if (clearSong())
	{
		return;
	}

	//Reset Linuxsampler if user selected reset on song load
	if(config.lsClientResetOnSongStart && !firstrun)
	{
		//Start Linuxsampler
		if(config.lsClientStartLS)
		{
			if(gLSThread)
			{
				if(lsClient)
					lsClient->stopClient();
				lsClient = 0;
				gLSThread->quit();
				gLSThread = 0;
				gSamplerStarted = false;
			}
			gLSThread = new LSThread();
			gLSThread->start();
		}
		bool started = false;
		int retry = 0;
		while(!started && retry <= 10)
		{
			lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
			lsClientStarted = lsClient->startClient();
			if(config.lsClientResetOnStart && lsClientStarted)
			{
				lsClient->resetSampler();
			}
			started = lsClient && lsClient->isClientStarted();
			sleep(1);
			retry++;
		}
		if(retry >= 10 && !started)
		{
			QMessageBox::critical(this, tr("LinuxSampler Client Failed!!"), tr("You have configured OOStudio to start a LinuxSampler\n however the LinuxSampler client failed to start"));
		}
	}
	//Configure the my input list

	QFileInfo fi(name);
	if (songTemplate)
	{
		if (!fi.isReadable())
		{
			QMessageBox::critical(this, QString("OOStudio"),
					tr("Cannot read template"));
			return;
		}
		project.setFile("untitled");
		oomProject = oomProjectInitPath;
		oomProjectFile = project.filePath();
	}
	else
	{
		printf("Setting project path to %s\n", fi.absolutePath().toLatin1().constData());
		oomProject = fi.absolutePath();
		project.setFile(name);
		oomProjectFile = project.filePath();
	}
	QString ex = fi.completeSuffix().toLower();
	QString mex = ex.section('.', -1, -1);
	if ((mex == "gz") || (mex == "bz2"))
		mex = ex.section('.', -2, -2);

	if (ex.isEmpty() || mex == "oom")
	{
		//
		//  read *.oom file
		//
		bool popenFlag;
		FILE* f = fileOpen(this, fi.filePath(), QString(".oom"), "r", popenFlag, true);
		if (f == 0)
		{
			if (errno != ENOENT)
			{
				QMessageBox::critical(this, QString("OOStudio"),
						tr("File open error"));
				setUntitledProject();
			}
			else
				setConfigDefaults();
		}
		else
		{
	   		 // Load the .oom file into a QDomDocument.
	   		 // the xml parser of QDomDocument then will be able to tell us
	   		 // if the .oom file didn't get corrupted in some way, cause the
	   		 // internal xml parser of oom can't do that.
	   		 QDomDocument doc("OOMProject");
	   		 QFile file(fi.filePath());

	   		 if (!file.open(QIODevice::ReadOnly)) {
	   		     printf("Could not open file %s readonly\n", file.fileName().toLatin1().data());
	   		 }

	   		 QString errorMsg;
	   		 if (!doc.setContent(&file, &errorMsg)) 
	   		 {
	   		 	printf("Failed to set xml content (Error: %s)\n", errorMsg.toLatin1().data());

	   		 	if (QMessageBox::critical(this,
	   		 			      QString("OOStudio Load Project"),
	   		 			      tr("Failed to parse file:\n\n %1 \n\n\n Error Message:\n\n %2 \n\n"
	   		 				 "Suggestion: \n\nmove the %1 file to another location, and rename the %1.backup to %1"
	   		 				 " and reload the project\n")
	   		 			      .arg(file.fileName())
	   		 			      .arg(errorMsg),
	   		 			      "OK")) {
	   		 		setUntitledProject();
	   		 		// is it save to return; here ?
	   		 		return;
	   		 	}
	   		 }

	   		 // OK, seems the xml file contained valid xml, start loading the real thing here
	   		 // using the internal xml parser for now.

			//TODO: Flush all ports at this point so each song loads with a fresh LS state

			initGlobalInputPorts();
			Xml xml(f);
			if(debugMsg)
				qDebug("OOMidi::loadProjectFile1 Before OOMidi::read()\n");
			read(xml, !loadAll);
			if(debugMsg)
				qDebug("OOMidi::loadProjectFile1 After OOMidi::read()\n");
			bool fileError = ferror(f);
			popenFlag ? pclose(f) : fclose(f);
			if (fileError)
			{
				QMessageBox::critical(this, QString("OOStudio"), tr("File read error"));
				setUntitledProject();
			}
		}
	}
	else if (mex == "mid" || mex == "kar")
	{
		setConfigDefaults();
		if (!importMidi(name, false))
			setUntitledProject();
	}
	else
	{
		QMessageBox::critical(this, QString("OOStudio"), tr("Unknown File Format: ") + ex);
		setUntitledProject();
	}
	if (!songTemplate)
	{
		addProject(project.absoluteFilePath());
		setWindowTitle(QString("The Composer - OOStudio-").append(VERSION).append(":     ") + project.completeBaseName() + QString("     "));
	}
	song->dirty = false;

	viewTransportAction->setChecked(config.transportVisible);
	viewBigtimeAction->setChecked(config.bigTimeVisible);
	viewMarkerAction->setChecked(config.markerVisible);

	autoMixerAction->setChecked(automation);

	if (loadAll)
	{
		showBigtime(config.bigTimeVisible);
		showMixer1(config.mixer1Visible);

		if (config.transportVisible)
			transport->show();
		transport->move(config.geometryTransport.topLeft());
		showTransport(config.transportVisible);
	}
	song->addMasterTrack();
	song->addOOMVerb();
	transport->setMasterFlag(song->masterFlag());
	punchinAction->setChecked(song->punchin());
	punchoutAction->setChecked(song->punchout());
	loopAction->setChecked(song->loop());
	song->update();
	song->updatePos();
	clipboardChanged(); // enable/disable "Paste"
	selectionChanged(); // enable/disable "Copy" & "Paste"

	// Try this AFTER the song update above which does a mixer update... Tested OK - mixers resize properly now.
	if (loadAll)
	{
		// Moved here from above due to crash with a song loaded and then File->New.
		// Marker view list was not updated, had non-existent items from marker list (cleared in ::clear()).
		showMarker(config.markerVisible);
	}
	firstrun = false;
	emit viewReady();
}

//---------------------------------------------------------
//   setUntitledProject
//---------------------------------------------------------

void OOMidi::setUntitledProject()
{
	setConfigDefaults();
	QString name("untitled");
	oomProject = "./"; //QFileInfo(name).absolutePath();
	project.setFile(name);
	oomProjectFile = project.filePath();
	//setWindowTitle(tr("OOMidi: Song: ") + project.completeBaseName());
	setWindowTitle(QString("The Composer - OOStudio-").append(VERSION).append(":     ") + project.completeBaseName() + QString("     "));
}

//---------------------------------------------------------
//   setConfigDefaults
//---------------------------------------------------------

void OOMidi::setConfigDefaults()
{
	readConfiguration(); // used for reading midi files
#if 0
	if (readConfiguration())
	{
		//
		// failed to load config file
		// set buildin defaults
		//
		configTransportVisible = false;
		configBigTimeVisible = false;

		for (int channel = 0; channel < 2; ++channel)
			song->addTrack(Track::AUDIO_BUSS);
		AudioTrack* out = (AudioTrack*) song->addTrack(Track::AUDIO_OUTPUT);
		AudioTrack* in = (AudioTrack*) song->addTrack(Track::AUDIO_INPUT);

		// set some default routes
		std::list<QString> il = audioDevice->inputPorts();
		int channel = 0;
		for (std::list<QString>::iterator i = il.begin(); i != il.end(); ++i, ++channel)
		{
			if (channel == 2)
				break;
			audio->msgAddRoute(Route(out, channel), Route(*i, channel));
		}
		channel = 0;
		std::list<QString> ol = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = ol.begin(); i != ol.end(); ++i, ++channel)
		{
			if (channel == 2)
				break;
			audio->msgAddRoute(Route(*i, channel), Route(in, channel));
		}
	}
#endif
	song->dirty = false;
}

//---------------------------------------------------------
//   setFollow
//---------------------------------------------------------

void OOMidi::setFollow()
{
	Song::FollowMode fm = song->follow();

	dontFollowAction->setChecked(fm == Song::NO);
	followPageAction->setChecked(fm == Song::JUMP);
	followCtsAction->setChecked(fm == Song::CONTINUOUS);
}

//---------------------------------------------------------
//   OOMidi::loadProject
//---------------------------------------------------------

void OOMidi::loadProject()
{
	bool loadAll;
	QString fn = getOpenFileName(QString(""), med_file_pattern, this,
			tr("OOStudio: load project"), &loadAll);
	if (!fn.isEmpty())
	{
		oomProject = QFileInfo(fn).absolutePath();
		oomProjectFile = QFileInfo(fn).filePath();
		loadProjectFile(fn, false, loadAll);
	}
}

//---------------------------------------------------------
//   loadTemplate
//---------------------------------------------------------

void OOMidi::loadTemplate()
{
	QString fn = getOpenFileName(QString("templates"), med_file_pattern, this,
			tr("OOStudio: load template"), 0, MFileDialog::GLOBAL_VIEW);
	if (!fn.isEmpty())
	{
		// oomProject = QFileInfo(fn).absolutePath();
		loadProjectFile(fn, true, true);
		setUntitledProject();
	}
}

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool OOMidi::save()
{
	if (project.completeBaseName() == "untitled")
		return saveAs();
	else
		return save(project.filePath(), false);
}

//---------------------------------------------------------
//   save
//---------------------------------------------------------

bool OOMidi::save(const QString& name, bool overwriteWarn)
{
	QString backupCommand;

	// By T356. Cache the jack in/out route names BEFORE saving.
	// Because jack often shuts down during save, causing the routes to be lost in the file.
	// Not required any more...
	//cacheJackRouteNames();

	if (QFile::exists(name))
	{
		backupCommand.sprintf("cp \"%s\" \"%s.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
	}
	else if (QFile::exists(name + QString(".oom")))
	{
		backupCommand.sprintf("cp \"%s.oom\" \"%s.oom.backup\"", name.toLatin1().constData(), name.toLatin1().constData());
	}
	if (!backupCommand.isEmpty())
	{
		int tmp = system(backupCommand.toLatin1().constData());
		if(debugMsg)
			printf("Creating project backup: %d", tmp);
	}

	bool popenFlag;
	FILE* f = fileOpen(this, name, QString(".oom"), "w", popenFlag, false, overwriteWarn);
	if (f == 0)
		return false;
	Xml xml(f);
	write(xml);
	if (ferror(f))
	{
		QString s = "Write File\n" + name + "\nfailed: "
				//+ strerror(errno);
				+ QString(strerror(errno)); // p4.0.0
		QMessageBox::critical(this,
				tr("OOStudio: Write File failed"), s);
		popenFlag ? pclose(f) : fclose(f);
		unlink(name.toLatin1().constData());
		//FIXME: We should probably restore the backup file at this point as the saved file 
		//has been found to be currupt, why force the user to make the restore, we already told them it failed
		return false;
	}
	else
	{
		popenFlag ? pclose(f) : fclose(f);
		//We should also use QDomDocument to parse the file like we do in openProject and verify after the fact that
		//it did actually save a good file before returning true below
		//Lets save config when the user saves to make sure everything is in sync on next launch
		changeConfig(true);
		song->dirty = false;
		return true;
	}
}

bool OOMidi::saveRouteMapping(QString name, QString notes)
{
	bool popenFlag;
	FILE* f = fileOpen(this, name, QString(".orm"), "w", popenFlag, false, false);
	if (f == 0)
		return false;
	Xml xml(f);
	xml.header();

	int level = 0;
	xml.tag(level++, "orm version=\"2.0\"");
	xml.put(level++,"<notes text=\"%s\" />", notes.toLatin1().constData());
	//Write out the routing map to the xml here
	for (ciTrack i = song->tracks()->begin(); i != song->tracks()->end(); ++i)
	{
		(*i)->writeRouting(level, xml);
	}
	// Write midi device routing.
	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		(*i)->writeRouting(level, xml);
	}
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		midiPorts[i].writeRouting(level, xml);
	}
	xml.tag(level, "/orm");
	if (ferror(f))
	{
		QString s = "Write File\n" + name + "\nfailed: " + QString(strerror(errno));
		QMessageBox::critical(this, tr("OOStudio: Write File failed"), s);
		popenFlag ? pclose(f) : fclose(f);
		unlink(name.toLatin1().constData());
		return false;
	}
	else
	{
		popenFlag ? pclose(f) : fclose(f);
		song->dirty = false;
		return true;
	}
	return true;
}

bool OOMidi::updateRouteMapping(QString name, QString note)/*{{{*/
{
	QFileInfo fi(name);
	QDomDocument doc("OOMRouteMap");/*{{{*/
	QFile file(fi.filePath());

	if (!file.open(QIODevice::ReadOnly)) {
	    printf("Could not open file %s read/write\n", file.fileName().toLatin1().data());
		return false;
	}

	QString errorMsg;
	if (!doc.setContent(&file, &errorMsg)) {
	    printf("Failed to set xml content (Error: %s)\n", errorMsg.toLatin1().data());

	    if (QMessageBox::critical(this,
				  QString("OOStudio Load Routing Map"),
				  tr("Failed to parse file:\n\n %1 \n\n\n Error Message:\n\n %2 \n")
				  .arg(file.fileName())
				  .arg(errorMsg),
				  "OK"))
		{
		return false;
	    }
	}/*}}}*/
	file.close();

	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
	    printf("Could not open file %s read/write\n", file.fileName().toLatin1().data());
		return false;
	}
	QDomElement root = doc.documentElement();
	QDomNode n = root.firstChild();
	while(!n.isNull())
	{
		QDomElement e = n.toElement();
		if( !e.isNull() && e.tagName() == "notes")
		{
			e.setAttribute("text", note);
			//QDomText t = n.toText();
			//t.setData(note);
			QTextStream ts( &file );
			ts << doc.toString();
			break;
		}
		n = n.nextSibling();
	}
	file.close();
	return true;
}/*}}}*/

QString OOMidi::noteForRouteMapping(QString name)/*{{{*/
{
	QString rv;
	QFileInfo fi(name);
	QDomDocument doc("OOMRouteMap");/*{{{*/
	QFile file(fi.filePath());

	if (!file.open(QIODevice::ReadOnly)) {
	    printf("Could not open file %s readonly\n", file.fileName().toLatin1().data());
		return rv;
	}

	QString errorMsg;
	if (!doc.setContent(&file, &errorMsg)) {
	    printf("Failed to set xml content (Error: %s)\n", errorMsg.toLatin1().data());

	    if (QMessageBox::critical(this,
				  QString("OOStudio Load Routing Map"),
				  tr("Failed to parse file:\n\n %1 \n\n\n Error Message:\n\n %2 \n")
				  .arg(file.fileName())
				  .arg(errorMsg),
				  "OK"))
		{
		return rv;
	    }
	}/*}}}*/
	QDomElement root = doc.documentElement();
	QDomNode n = root.firstChild();
	while(!n.isNull())
	{
		QDomElement e = n.toElement();
		if( !e.isNull() && e.tagName() == "notes")
		{
			rv = e.attribute("text", "");//text();
			break;
		}
		n = n.nextSibling();
	}
	file.close();
	return rv;
}/*}}}*/

bool OOMidi::loadRouteMapping(QString name)
{
	//Make sure we stop the song
	song->setStop(true);
	//Start the sequencer before we start loading routes into the engine
	if(!audio->isRunning())
	{
		printf("Sequencer is not running, Restarting\n");
		seqRestart();
	}

	QFileInfo fi(name);/*{{{*/
	if (!fi.isReadable())
	{
		QMessageBox::critical(this, QString("OOStudio"), tr("Cannot read routing map"));
		return false;
	}
	QString ex = fi.completeSuffix().toLower();
	QString mex = ex.section('.', -1, -1);

	if (ex.isEmpty() || mex == "orm")
	{
		bool popenFlag;
		FILE* f = fileOpen(this, fi.filePath(), QString(".orm"), "r", popenFlag, true);
		if (f == 0)
		{
			if (errno != ENOENT)
			{
				QMessageBox::critical(this, QString("OOStudio"), tr("File open error: Could not open Route Map"));
				return false;
			}
		}
		else
		{
	    QDomDocument doc("OOMRouteMap");
	    QFile file(fi.filePath());

	    if (!file.open(QIODevice::ReadOnly)) {
		printf("Could not open file %s readonly\n", file.fileName().toLatin1().data());
				return false;
	    }

	    QString errorMsg;
	    if (!doc.setContent(&file, &errorMsg)) {
		printf("Failed to set xml content (Error: %s)\n", errorMsg.toLatin1().data());

		if (QMessageBox::critical(this,
				      QString("OOStudio Load Routing Map"),
				      tr("Failed to parse file:\n\n %1 \n\n\n Error Message:\n\n %2 \n")
				      .arg(file.fileName())
				      .arg(errorMsg),
				      "OK"))
				{
			return false;
		}
	    }

			Xml xml(f);

			//Flush the routing list for all tracks
			for (ciTrack i = song->tracks()->begin(); i != song->tracks()->end(); ++i)
			{
				(*i)->inRoutes()->clear();
				(*i)->outRoutes()->clear();
			}
			//Flush the routing list for all midi devices
			for (iMidiDevice imd = midiDevices.begin(); imd != midiDevices.end(); ++imd)
			{
				(*imd)->inRoutes()->clear();
				(*imd)->outRoutes()->clear();
			}

			bool lld = true;
			while (lld)/*{{{*/
			{
				Xml::Token token = xml.parse();
				const QString& tag = xml.s1();
				switch (token)
				{
					case Xml::Error:
					case Xml::End:
						lld = false;
						break;
					case Xml::TagStart:
						if (tag == "orm")
						{
							//xml.skip(tag);
							break;
						}
						else if (tag == "notes")
						{
							break;
						}
						else if (tag == "Route")
						{
							song->readRoute(xml);
							audio->msgUpdateSoloStates();
						}
						else
							xml.unknown("orm");
						break;
					case Xml::Attribut:
						break;
					case Xml::TagEnd:
						if (tag == "orm")
							lld = false;
							break;
					default:
						break;
				}
			}/*}}}*/
			bool fileError = ferror(f);
			popenFlag ? pclose(f) : fclose(f);
			if (fileError)
			{
				QMessageBox::critical(this, QString("OOStudio"), tr("File read error"));
				return false;
			}
		}
	}/*}}}*/
	song->dirty = true;
	//Restart all the audio connections
	seqRestart();
	song->update(SC_CONFIG);
	return true;
}

void OOMidi::connectDefaultSongPorts()
{
	if(!song->associatedRoute.isEmpty())
	{
		loadRouteMapping(song->associatedRoute);
	}
	else
	{
		printf("Current Song has no default route\n");
	}
}

//---------------------------------------------------------
//   quitDoc
//---------------------------------------------------------

void OOMidi::quitDoc(bool external)
{
	m_externalCall = external;
	close();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void OOMidi::closeEvent(QCloseEvent* event)
{
	if(performer)
	{
		performer->hide();
		delete performer;
	}
	song->setStop(true);
	//
	// wait for sequencer
	//
	while (audio->isPlaying())
	{
		qApp->processEvents();
	}
	if (song->dirty)
	{
		int n = 0;
		if(m_externalCall)
		{
			n = QMessageBox::warning(this, appName,
					tr("A Remote shutdown sequence was called\n"
					"However the current Project contains unsaved data\n"
					"Save Current Project?"),
					tr("&Save"), tr("&Don't Save"), 0, 1);
			if (n == 0)
			{
				save();
			}
		}
		else
		{
			n = QMessageBox::warning(this, appName,
					tr("The current Project contains unsaved data\n"
					"Save Current Project?"),
					tr("&Save"), tr("&Don't Save"), tr("&Cancel"), 0, 2);
			if (n == 0)
			{
				if (!save()) // dont quit if save failed
				{
					event->ignore();
					return;
				}
			}
			else if (n == 2)
			{
				event->ignore();
				return;
			}
		}
	}
	seqStop();

	if(config.lsClientStartLS)
	{
		if(gLSThread)
			gLSThread->quit();
	}
	WaveTrackList* wt = song->waves();
	for (iWaveTrack iwt = wt->begin(); iwt != wt->end(); ++iwt)
	{
		WaveTrack* t = *iwt;
		if (t->recFile() && t->recFile()->samples() == 0)
		{
			t->recFile()->remove();
		}
	}

	// save "Open Recent" list
	QString prjPath(QString(configPath).append(QDir::separator()).append("projects"));
	FILE* f = fopen(prjPath.toLatin1().constData(), "w");
	if (f)
	{
		QList<QString*> uniq;
		for (int i = 0; i < PROJECT_LIST_LEN; ++i)
		{
			if(projectList[i] && (uniq.isEmpty() || !uniq.contains(projectList[i])))
				fprintf(f, "%s\n", projectList[i]->toLatin1().constData());
		}
		fclose(f);
	}
	if (debugMsg)
		printf("OOStudio: Exiting JackAudio\n");
	exitJackAudio();
	if (debugMsg)
		printf("OOStudio: Exiting DummyAudio\n");
	exitDummyAudio();
	if (debugMsg)
		printf("OOStudio: Exiting Metronome\n");
	exitMetronome();

	// Make sure to clear the menu, which deletes any sub menus.
	if (routingPopupMenu)
		routingPopupMenu->clear();

	song->cleanupForQuit();

	if (debugMsg)
		printf("OOMidi: Cleaning up temporary wavefiles + peakfiles\n");
	// Cleanup temporary wavefiles + peakfiles used for undo
	for (std::list<QString>::iterator i = temporaryWavFiles.begin(); i != temporaryWavFiles.end(); i++)
	{
		QString filename = *i;
		QFileInfo f(filename);
		QDir d = f.dir();
		d.remove(filename);
		d.remove(f.completeBaseName() + ".wca");
	}

	// Added by Tim. p3.3.14

#ifdef HAVE_LASH
	// Disconnect gracefully from LASH.
	if (lash_client)
	{
		if (debugMsg)
			printf("OOMidi: Disconnecting from LASH\n");
		lash_event_t* lashev = lash_event_new_with_type(LASH_Quit);
		lash_send_event(lash_client, lashev);
	}
#endif

	if (debugMsg)
		printf("OOMidi: Exiting Dsp\n");
	AL::exitDsp();

	if (debugMsg)
		printf("OOMidi: Exiting OSC\n");
	exitOSC();

	// p3.3.47
	delete midiMonitor;
	delete audioPrefetch;
	delete audio;
	delete midiSeq;
	delete song;

	qApp->quit();
}

//---------------------------------------------------------
//   toggleMarker
//---------------------------------------------------------

void OOMidi::toggleMarker(bool checked)
{
	showMarker(checked);
}

//---------------------------------------------------------
//   showMarker
//---------------------------------------------------------

void OOMidi::showMarker(bool flag)
{
	//printf("showMarker %d\n",flag);
	if (markerView == 0)
	{
		markerView = new MarkerView(this);

		// Removed p3.3.43
		// Song::addMarker() already emits a 'markerChanged'.
		//connect(composer, SIGNAL(addMarker(int)), markerView, SLOT(addMarker(int)));

		connect(markerView, SIGNAL(closed()), SLOT(markerClosed()));
		toplevels.push_back(Toplevel(Toplevel::MARKER, (unsigned long) (markerView), markerView));
		markerView->show();
	}
	markerView->setVisible(flag);
	viewMarkerAction->setChecked(flag);
}

//---------------------------------------------------------
//   markerClosed
//---------------------------------------------------------

void OOMidi::markerClosed()
{
	viewMarkerAction->setChecked(false);
}

//---------------------------------------------------------
//   toggleTransport
//---------------------------------------------------------

void OOMidi::toggleTransport(bool checked)
{
	showTransport(checked);
}

//---------------------------------------------------------
//   showTransport
//---------------------------------------------------------

void OOMidi::showTransport(bool flag)
{
	transport->setVisible(flag);
	viewTransportAction->setChecked(flag);
}

//---------------------------------------------------------
//   getRoutingPopupMenu
//---------------------------------------------------------

PopupMenu* OOMidi::getRoutingPopupMenu()
{
	if (!routingPopupMenu)
		routingPopupMenu = new PopupMenu(this);
	return routingPopupMenu;
}

//---------------------------------------------------------
//   updateRouteMenus
//---------------------------------------------------------

void OOMidi::updateRouteMenus(Track* track, QObject* master)
{
	// NOTE: The puropse of this routine is to make sure the items actually reflect
	//  the routing status. And with OOMidi-1 QT3, it was also required to actually
	//  check the items since QT3 didn't do it for us.
	// But now with OOMidi-2 and QT4, QT4 checks an item when it is clicked.
	// So this routine is less important now, since 99% of the time, the items
	//  will be in the right checked state.
	// But we still need this in case for some reason a route could not be
	//  added (or removed). Then the item will be properly un-checked (or checked) here.

	//if(!track || track != gRoutingPopupMenuMaster || track->type() == Track::AUDIO_AUX)
	//if(!track || track->type() == Track::AUDIO_AUX)
	if (!track || gRoutingPopupMenuMaster != master) // p3.3.50
		return;

	PopupMenu* pup = getRoutingPopupMenu();

	if (pup->actions().isEmpty())
		return;

	if (!pup->isVisible())
		return;

	//AudioTrack* t = (AudioTrack*)track;
	RouteList* rl = gIsOutRoutingPopupMenu ? track->outRoutes() : track->inRoutes();

	iRouteMenuMap imm = gRoutingMenuMap.begin();
	for (; imm != gRoutingMenuMap.end(); ++imm)
	{
		// p3.3.50 Ignore the 'toggle' items.
		if (imm->second.type == Route::MIDI_PORT_ROUTE &&
				imm->first >= (MIDI_PORTS * MIDI_CHANNELS) && imm->first < (MIDI_PORTS * MIDI_CHANNELS + MIDI_PORTS))
			continue;

		//bool found = false;
		iRoute irl = rl->begin();
		for (; irl != rl->end(); ++irl)
		{
			if (imm->second.type == Route::MIDI_PORT_ROUTE) // p3.3.50 Is the map route a midi port route?
			{
				if (irl->type == Route::MIDI_PORT_ROUTE && irl->midiPort == imm->second.midiPort // Is the track route a midi port route?
						&& (irl->channel & imm->second.channel) == imm->second.channel) // Is the exact channel mask bit(s) set?
				{
					//found = true;
					break;
				}
			}
			else
				if (*irl == imm->second)
			{
				//found = true;
				break;
			}
		}
		//pup->setItemChecked(imm->first, found);
		//printf("OOMidi::updateRouteMenus setItemChecked\n");
		// TODO: OOMidi-2: Convert this, fastest way is to change the routing map, otherwise this requires a lookup.
		//if(pup->isItemChecked(imm->first) != (irl != rl->end()))
		//  pup->setItemChecked(imm->first, irl != rl->end());
		QAction* act = pup->findActionFromData(imm->first);
		if (act && act->isChecked() != (irl != rl->end()))
			act->setChecked(irl != rl->end());
	}
}

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void OOMidi::routingPopupMenuActivated(Track* track, int n)
{
	if (!track)
		return;

	if (track->isMidiTrack())
	{
		PopupMenu* pup = getRoutingPopupMenu();

		if (pup->actions().isEmpty())
			return;

		RouteList* rl = gIsOutRoutingPopupMenu ? track->outRoutes() : track->inRoutes();

		if (n == -1)
			return;

		iRouteMenuMap imm = gRoutingMenuMap.find(n);
		if (imm == gRoutingMenuMap.end())
			return;
		if (imm->second.type != Route::MIDI_PORT_ROUTE)
			return;
		Route &aRoute = imm->second;
		int chbit = aRoute.channel;
		Route bRoute(track, chbit);
		int mdidx = aRoute.midiPort;

		MidiPort* mp = &midiPorts[mdidx];
		MidiDevice* md = mp->device();
		if (!md)
			return;

		//if(!(md->rwFlags() & 2))
		if (!(md->rwFlags() & (gIsOutRoutingPopupMenu ? 1 : 2)))
			return;

		//printf("Routing menu clicked with gid: %d\n", n);
		if(n >= 50000 && !gIsOutRoutingPopupMenu)
		{
			for(iMidiTrack it = song->midis()->begin(); it != song->midis()->end(); ++it)
			{
				Route tRoute(*it, chbit);
				audio->msgAddRoute(aRoute, tRoute);
				//printf("Adding route for track %s\n", (*it)->name().toLatin1().constData());
			}
		}
		else
		{
			int chmask = 0;
			iRoute iir = rl->begin();
			for (; iir != rl->end(); ++iir)
			{
				if (iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx) // p3.3.50 Is there already a route to this port?
				{
					chmask = iir->channel; // p3.3.50 Grab the channel mask.
					break;
				}
			}
			if ((chmask & chbit) == chbit) // p3.3.50 Is the channel's bit(s) set?
			{
				if (gIsOutRoutingPopupMenu)
					audio->msgRemoveRoute(bRoute, aRoute);
				else
					audio->msgRemoveRoute(aRoute, bRoute);
			}
			else
			{
				// connect
				if (gIsOutRoutingPopupMenu)
					audio->msgAddRoute(bRoute, aRoute);
				else
					audio->msgAddRoute(aRoute, bRoute);
			}
		}

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}

//---------------------------------------------------------
//   routingPopupMenuAboutToHide
//---------------------------------------------------------

void OOMidi::routingPopupMenuAboutToHide()
{
	gRoutingMenuMap.clear();
	gRoutingPopupMenuMaster = 0;
}

//---------------------------------------------------------
//   prepareRoutingPopupMenu
//---------------------------------------------------------

PopupMenu* OOMidi::prepareRoutingPopupMenu(Track* track, bool dst)
{
	if (!track)
		return 0;

	if (track->isMidiTrack())
	{

		RouteList* rl = dst ? track->outRoutes() : track->inRoutes();

		PopupMenu* pup = getRoutingPopupMenu();
		pup->disconnect();

		int gid = 0;
		QAction* act = 0;

		// Routes can't be re-read until the message sent from msgAddRoute1()
		//  has had time to be sent and actually affected the routes.
		///_redisplay:

		pup->clear();
		gRoutingMenuMap.clear();
		gid = 0;

		for (int i = 0; i < MIDI_PORTS; ++i)
		{
			//MidiInPort* track = *i;
			// NOTE: Could possibly list all devices, bypassing ports, but no, let's stick with ports.
			MidiPort* mp = &midiPorts[i];
			MidiDevice* md = mp->device();
			if (!md)
				continue;

			//if (!(md->rwFlags() & (dst ? 1 : 2)))
			if (!(md->openFlags() & (dst ? 1 : 2)))
				continue;

			//printf("OOMidi::prepareRoutingPopupMenu adding submenu portnum:%d\n", i);

			PopupMenu* subp = new PopupMenu(pup);
			subp->setTitle(md->name());

			int chanmask = 0;
			// p3.3.50 To reduce number of routes required, from one per channel to just one containing a channel mask.
			// Look for the first route to this midi port. There should always be only a single route for each midi port, now.
			for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
			{
				if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i)
				{
					// We have a route to the midi port. Grab the channel mask.
					chanmask = ir->channel;
					break;
				}
			}

			for (int ch = 0; ch < MIDI_CHANNELS; ++ch)
			{
				gid = i * MIDI_CHANNELS + ch;

				//printf("OOMidi::prepareRoutingPopupMenu inserting gid:%d\n", gid);

				act = subp->addAction(QString("Channel %1").arg(ch + 1));
				act->setCheckable(true);
				act->setData(gid);

				int chbit = 1 << ch;
				Route srcRoute(i, chbit); // p3.3.50 In accordance with new channel mask, use the bit position.

				gRoutingMenuMap.insert(pRouteMenuMap(gid, srcRoute));

				if (chanmask & chbit) // p3.3.50 Is the channel already set? Show item check mark.
					act->setChecked(true);
			}

			if(!dst)
			{
				//Add global menu toggle
				int ch1bit = 1 << 0;
				Route myRoute(i, ch1bit); // p3.3.50 In accordance with new channel mask, use the bit position.
				gid = 50000 + i;
				act = subp->addAction(QString("Set Global Channel 1"));
				act->setData(gid);
				gRoutingMenuMap.insert(pRouteMenuMap(gid, myRoute));
			}

			// p3.3.50 One route with all channel bits set.
			gid = MIDI_PORTS * MIDI_CHANNELS + i; // Make sure each 'toggle' item gets a unique id.
			act = subp->addAction(QString("Toggle all"));
			//act->setCheckable(true);
			act->setData(gid);
			Route togRoute(i, (1 << MIDI_CHANNELS) - 1); // Set all channel bits.
			gRoutingMenuMap.insert(pRouteMenuMap(gid, togRoute));

			pup->addMenu(subp);
		}


		if (pup->actions().isEmpty())
		{
			gRoutingPopupMenuMaster = 0;
			gRoutingMenuMap.clear();
			return 0;
		}

		gIsOutRoutingPopupMenu = dst;
		return pup;
	}

	return 0;
}


//---------------------------------------------------------
//   saveAs
//---------------------------------------------------------

bool OOMidi::saveAs()
{
	QString name;
	if (oomProject == oomProjectInitPath)
	{
		printf("config.useProjectSaveDialog=%d\n", config.useProjectSaveDialog);
		if (config.useProjectSaveDialog)
		{
			ProjectCreateImpl pci(oom);
			if (pci.exec() == QDialog::Rejected)
			{
				return false;
			}
			song->setSongInfo(pci.getSongInfo());
			name = pci.getProjectPath();
		}
		else
		{
			name = getSaveFileName(QString(""), med_file_save_pattern, this, tr("OOStudio: Save As"));
			if (name.isEmpty())
				return false;
		}
		oomProject = QFileInfo(name).absolutePath();
		oomProjectFile = QFileInfo(name).filePath();
		QDir dirmanipulator;
		if (!dirmanipulator.mkpath(oomProject))
		{
			QMessageBox::warning(this, "Path error", "Can't create project path", QMessageBox::Ok);
			return false;
		}
	}
	else
	{
		name = getSaveFileName(QString(""), med_file_save_pattern, this, tr("OOStudio: Save As"));
	}
	bool ok = false;
	if (!name.isEmpty())
	{
		QString tempOldProj = oomProject;
		oomProject = QFileInfo(name).absolutePath();
		ok = save(name, true);
		if (ok)
		{
			project.setFile(name);
			oomProjectFile = project.filePath();
			//setWindowTitle(tr("OOMidi: Song: ") + project.completeBaseName());
			setWindowTitle(QString("The Composer - OOStudio-").append(VERSION).append(":     ") + project.completeBaseName() + QString("     "));
			addProject(name);
		}
		else
			oomProject = tempOldProj;
	}

	return ok;
}

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void OOMidi::startEditor(PartList* pl, int type)
{
	switch (type)
	{
		case 0: startPerformer(pl, true);
			break;
		case 1: startListEditor(pl);
			break;
		default:
			break;
	}
}

//---------------------------------------------------------
//   startEditor
//---------------------------------------------------------

void OOMidi::startEditor(Track* t)
{
	switch (t->type())
	{
		case Track::MIDI:
		case Track::DRUM: startPerformer(); 
			break;
		default:
			break;
	}
}

//---------------------------------------------------------
//   getMidiPartsToEdit
//---------------------------------------------------------

PartList* OOMidi::getMidiPartsToEdit()
{
	PartList* pl = song->getSelectedMidiParts();
	if (pl->empty())
	{
		QMessageBox::critical(this, QString("OOStudio"), tr("Nothing to edit"));
		return 0;
	}
	return pl;
}

//---------------------------------------------------------
//   startPerformer
//---------------------------------------------------------

void OOMidi::startPerformer()
{
	if(composer->isEditing())
	{
		composer->endEditing();
		return;
	}
	PartList* pl = getMidiPartsToEdit();
	if (pl == 0)
		return;
	startPerformer(pl, true);
}

void OOMidi::performerClosed()
{
	if(performer)
		delete performer;
	performer = 0;
}

void OOMidi::startPerformer(PartList* pl, bool /*showDefaultCtrls*/)
{
	if(!performer)
	{
		performer = new Performer(pl, this, 0, composer->cursorValue());
		performer->setWindowRole("performer");
    	// Be able to open the List Editor from the Piano Roll
    	// with the application global shortcut to open the L.E.
    	performer->addAction(startListEditAction);
    	// same for save shortcut
    	performer->addAction(fileSaveAction);
		//performer->addAction(playAction);

		//performer->restoreState(tconfig().get_property("PerformerEdit", "windowstate", "").toByteArray());
		performer->show();
		Song::movePlaybackToPart(pl->begin()->second);

		connect(performer, SIGNAL(deleted()), SLOT(performerClosed()));
		connect(oom, SIGNAL(configChanged()), performer, SLOT(configChanged()));
	}
	else
	{
		//Add part list to performer, Should the list already there be cleared?
		if(performer->isMinimized())
			performer->showNormal();
		performer->addParts(pl);
		performer->setCurCanvasPart(pl->begin()->second);
		Song::movePlaybackToPart(pl->begin()->second);
		performer->activateWindow();
		performer->raise();
	}

}

//---------------------------------------------------------
//   startListenEditor
//---------------------------------------------------------

void OOMidi::startListEditor()
{
	PartList* pl = getMidiPartsToEdit();
	if (pl == 0)
		return;
	startListEditor(pl);
}

void OOMidi::startListEditor(PartList* pl)
{
	ListEdit* listEditor = new ListEdit(pl);
	listEditor->show();
	toplevels.push_back(Toplevel(Toplevel::LISTE, (unsigned long) (listEditor), listEditor));
	connect(listEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
	connect(oom, SIGNAL(configChanged()), listEditor, SLOT(configChanged()));
}

//---------------------------------------------------------
//   startMasterEditor
//---------------------------------------------------------

void OOMidi::startMasterEditor()
{
	MasterEdit* masterEditor = new MasterEdit();
	masterEditor->installEventFilter(oom);
	masterEditor->setWindowRole("tempo_editor");
	masterEditor->show();
	toplevels.push_back(Toplevel(Toplevel::MASTER, (unsigned long) (masterEditor), masterEditor));
	connect(masterEditor, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
}

//---------------------------------------------------------
//   startLMasterEditor
//---------------------------------------------------------

void OOMidi::startLMasterEditor()
{
	LMaster* lmaster = new LMaster();
	lmaster->setWindowRole("tempo_editor_list");
	lmaster->show();
	toplevels.push_back(Toplevel(Toplevel::LMASTER, (unsigned long) (lmaster), lmaster));
	connect(lmaster, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
	connect(oom, SIGNAL(configChanged()), lmaster, SLOT(configChanged()));
}

//---------------------------------------------------------
//   startDrumEditor
//---------------------------------------------------------

void OOMidi::startDrumEditor()
{
	return;
}

void OOMidi::startDrumEditor(PartList*, bool)
{
	return;
}

//---------------------------------------------------------
//   startWaveEditor
//---------------------------------------------------------

void OOMidi::startWaveEditor()
{
	return;
}

void OOMidi::startWaveEditor(PartList*)
{
	return;
}


//---------------------------------------------------------
//   startSongInfo
//---------------------------------------------------------

void OOMidi::startSongInfo(bool editable)
{
	printf("startSongInfo!!!!\n");
	SongInfoWidget info;
	info.songInfoText->setPlainText(song->getSongInfo());
	info.songInfoText->setReadOnly(!editable);
	info.show();
	if (info.exec() == QDialog::Accepted)
	{
		if (editable)
			song->setSongInfo(info.songInfoText->toPlainText());
	}

}

//---------------------------------------------------------
//   showDidYouKnowDialog
//---------------------------------------------------------

void OOMidi::showDidYouKnowDialog()
{
	if ((bool)config.showDidYouKnow == true)
	{
		printf("show did you know dialog!!!!\n");
		DidYouKnowWidget dyk;
		dyk.tipText->setText("To get started with OOStudio why don't you try some demo songs available at http://www.openoctave.org/");
		dyk.show();
		if (dyk.exec())
		{
			if (dyk.dontShowCheckBox->isChecked())
			{
				printf("disables dialog!\n");
				config.showDidYouKnow = false;
				oom->changeConfig(true); // save settings
			}
		}
	}
}
//---------------------------------------------------------
//   startDefineController
//---------------------------------------------------------


//---------------------------------------------------------
//   startClipList
//---------------------------------------------------------

void OOMidi::startClipList(bool checked)
{
	if (clipListEdit == 0)
	{
		//clipListEdit = new ClipListEdit();
		clipListEdit = new ClipListEdit(this);
		toplevels.push_back(Toplevel(Toplevel::CLIPLIST, (unsigned long) (clipListEdit), clipListEdit));
		connect(clipListEdit, SIGNAL(deleted(unsigned long)), SLOT(toplevelDeleted(unsigned long)));
	}
	clipListEdit->show();
	viewCliplistAction->setChecked(checked);
}

//---------------------------------------------------------
//   fileMenu
//---------------------------------------------------------

void OOMidi::openRecentMenu()
{
	openRecent->clear();
	for (int i = 0; i < PROJECT_LIST_LEN; ++i)
	{
		if (projectList[i] == 0)
			break;
		QByteArray ba = projectList[i]->toLatin1();
		const char* path = ba.constData();
		const char* p = strrchr(path, '/');
		if (p == 0)
			p = path;
		else
			++p;
		QAction *act = openRecent->addAction(QString(p));
		act->setData(i);
	}
}

//---------------------------------------------------------
//   selectProject
//---------------------------------------------------------

void OOMidi::selectProject(QAction* act)
{
	if (!act)
		return;
	int id = act->data().toInt();
	assert(id < PROJECT_LIST_LEN);
	QString* name = projectList[id];
	if (name == 0)
		return;
	loadProjectFile(*name, false, true);
}

//---------------------------------------------------------
//   toplevelDeleted
//---------------------------------------------------------

void OOMidi::toplevelDeleted(unsigned long tl)
{
	for (iToplevel i = toplevels.begin(); i != toplevels.end(); ++i)
	{
		if (i->object() == tl)
		{
			switch (i->type())
			{
				case Toplevel::MARKER:
					break;
				case Toplevel::CLIPLIST:
					// ORCAN: This needs to be verified. aid2 used to correspond to Cliplist:
					//menu_audio->setItemChecked(aid2, false);
					viewCliplistAction->setChecked(false);
					return;
					//break;
					// the followin editors can exist in more than
					// one instantiation:
				case Toplevel::PIANO_ROLL:
				case Toplevel::LISTE:
				case Toplevel::DRUM:
				case Toplevel::MASTER:
				case Toplevel::WAVE:
				case Toplevel::LMASTER:
					break;
			}
			toplevels.erase(i);
			return;
		}
	}
	printf("topLevelDeleted: top level %lx not found\n", tl);
	//assert(false);
}

//---------------------------------------------------------
//   ctrlChanged
//    midi ctrl value changed
//---------------------------------------------------------

#if 0

void OOMidi::ctrlChanged()
{
	composer->updateInspector();
}
#endif

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void OOMidi::keyPressEvent(QKeyEvent* event)
{
	// Pass it on to composer part canvas.
	composer->getCanvas()->redirKeypress(event);
}

bool OOMidi::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = 0;
	int key = 0;

	if (event->type() == QEvent::KeyPress) {
		keyEvent = static_cast<QKeyEvent *>(event);
		key = keyEvent->key();

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

	}

	//QAbstractItemView* absItemView = qobject_cast<QAbstractItemView*>(obj);
	//if (absItemView && keyEvent)
	if (keyEvent)
	{
		if (key == shortcuts[SHRT_PLAY_TOGGLE].key)
		{
			kbAccel(key);
			return true;
		}
		else if(key == shortcuts[SHRT_PLAY_REPEAT].key)
		{
			kbAccel(key);
			return true;
		}
		else if (key == shortcuts[SHRT_START_REC].key)
		{
			kbAccel(key);
			return true;
		}
	}

	return QObject::eventFilter(obj, event);
}

//---------------------------------------------------------
//   kbAccel
//---------------------------------------------------------

void OOMidi::kbAccel(int key)
{
	if (key == shortcuts[SHRT_TOGGLE_METRO].key)
	{
		song->setClick(!song->click());
	}
	else if (key == shortcuts[SHRT_PLAY_TOGGLE].key)
	{
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
	}
	else if (key == shortcuts[SHRT_STOP].key)
	{
		//song->setPlay(false);
		song->setStop(true);
	}
	else if (key == shortcuts[SHRT_GOTO_START].key)
	{
		Pos p(0, true);
		song->setPos(0, p);
	}
	else if (key == shortcuts[SHRT_PLAY_SONG].key)
	{
		song->setPlay(true);
	}

		// p4.0.10 Tim. Normally each editor window handles these, to inc by the editor's raster snap value.
		// But users were asking for a global version - "they don't work when I'm in mixer or transport".
		// Since no editor claimed the key event, we don't know a specific editor's snap setting,
		//  so adopt a policy where the composer is the 'main' raster reference, I guess...
	else if (key == shortcuts[SHRT_POS_DEC].key)
	{
		int spos = song->cpos();
		if (spos > 0)
		{
			spos -= 1; // Nudge by -1, then snap down with raster1.
			spos = AL::sigmap.raster1(spos, song->composerRaster());
		}
		if (spos < 0)
			spos = 0;
		Pos p(spos, true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC].key)
	{
		int spos = AL::sigmap.raster2(song->cpos() + 1, song->composerRaster()); // Nudge by +1, then snap up with raster2.
		Pos p(spos, true);
		song->setPos(0, p, true, true, true); //CDW
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key)
	{
		int spos = song->cpos() - AL::sigmap.rasterStep(song->cpos(), song->composerRaster());
		if (spos < 0)
			spos = 0;
		Pos p(spos, true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key)
	{
		Pos p(song->cpos() + AL::sigmap.rasterStep(song->cpos(), song->composerRaster()), true);
		song->setPos(0, p, true, true, true);
		return;
	}

	else if (key == shortcuts[SHRT_GOTO_LEFT].key)
	{
		if (!song->record())
			song->setPos(0, song->lPos());
	}
	else if (key == shortcuts[SHRT_GOTO_RIGHT].key)
	{
		if (!song->record())
			song->setPos(0, song->rPos());
	}
	else if (key == shortcuts[SHRT_TOGGLE_LOOP].key)
	{
		song->setLoop(!song->loop());
	}
	else if (key == shortcuts[SHRT_START_REC].key)
	{
		if (!audio->isPlaying())
		{
			song->setRecord(!song->record());
		}
	}
	else if (key == shortcuts[SHRT_REC_CLEAR].key)
	{
		if (!audio->isPlaying())
		{
			song->clearTrackRec();
		}
	}
	else if(key == shortcuts[SHRT_TOGGLE_PLAY_REPEAT].key)
	{
		replayAction->toggle();
		return;
	}
	else if(key == shortcuts[SHRT_PLAY_REPEAT].key)
	{
		if(!replayAction->isChecked())
		{
			replayAction->toggle();
		}
		else
		{
			song->updateReplayPos();
		}
		return;
	}
	else if (key == shortcuts[SHRT_OPEN_TRANSPORT].key)
	{
		toggleTransport(!viewTransportAction->isChecked());
	}
	else if (key == shortcuts[SHRT_OPEN_BIGTIME].key)
	{
		toggleBigTime(!viewBigtimeAction->isChecked());
	}
	else if (key == shortcuts[SHRT_OPEN_MIXER].key)
	{
		toggleMixer1(!viewMixerAAction->isChecked());
	}
	else if (key == shortcuts[SHRT_NEXT_MARKER].key)
	{
		if (markerView)
			markerView->nextMarker();
	}
	else if (key == shortcuts[SHRT_PREV_MARKER].key)
	{
		if (markerView)
			markerView->prevMarker();
	}
	else
	{
		if (debugMsg)
			printf("unknown kbAccel 0x%x\n", key);
	}
}

//---------------------------------------------------------
//   cmd
//    some cmd's from pulldown menu
//---------------------------------------------------------

void OOMidi::cmd(int cmd)
{
	TrackList* tracks = song->tracks();
	int l = song->lpos();
	int r = song->rpos();

	switch (cmd)
	{
		case CMD_CUT:
			composer->cmd(Composer::CMD_CUT_PART);
			break;
		case CMD_COPY:
		{
			//TODO: Hook right here and get the current tool from the composer
			//copy automation if the automation tool is selected.
			ComposerCanvas *canvas = composer->getCanvas();
			if(canvas && canvas->tool() == AutomationTool)
			{
				//printf("Automation copy\n");
				composer->cmd(Composer::CMD_COPY_AUTOMATION_NODES);
			}
			else
			{
				composer->cmd(Composer::CMD_COPY_PART);
			}
		}
			break;
		case CMD_PASTE:
		{
			//TODO: Same as above
			ComposerCanvas *canvas = composer->getCanvas();
			if(canvas && canvas->tool() == AutomationTool)
			{
				//printf("Automation paste\n");
				composer->cmd(Composer::CMD_PASTE_AUTOMATION_NODES);
			}
			else
			{
				composer->cmd(Composer::CMD_PASTE_PART);
			}
		}
			break;
		case CMD_PASTE_CLONE:
			composer->cmd(Composer::CMD_PASTE_CLONE_PART);
			break;
		case CMD_PASTE_TO_TRACK:
			composer->cmd(Composer::CMD_PASTE_PART_TO_TRACK);
			break;
		case CMD_PASTE_CLONE_TO_TRACK:
			composer->cmd(Composer::CMD_PASTE_CLONE_PART_TO_TRACK);
			break;
		case CMD_INSERT:
			composer->cmd(Composer::CMD_INSERT_PART);
			break;
		case CMD_INSERTMEAS:
			composer->cmd(Composer::CMD_INSERT_EMPTYMEAS);
			break;
		case CMD_DELETE:
			if (composer->getCanvas()->tool() == AutomationTool)
			{
				composer->cmd(Composer::CMD_REMOVE_SELECTED_AUTOMATION_NODES);
			}
			else
			{
				QList<Part*> parts = song->selectedParts();
				if (parts.size())
				{
					song->startUndo();
					audio->msgRemoveParts(parts);
					song->endUndo(SC_PART_REMOVED);
					break;
				}
				else
				{
					trackManager->removeSelectedTracks();
					/*QString msg(tr("You are about to delete \n%1 \nAre you sure this is what you want?"));
					if(QMessageBox::question(this, 
						tr("Delete Track"),
						msg.arg("all selected tracks"),
						QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
					{
						audio->msgRemoveTracks();
					}*/
				}
				//song->endUndo(SC_TRACK_REMOVED);

			}
			break;
		case CMD_DELETE_TRACK:
			trackManager->removeSelectedTracks();
			//song->startUndo();
			//audio->msgRemoveTracks();
			//song->endUndo(SC_TRACK_REMOVED);
			audio->msgUpdateSoloStates();
			break;

		case CMD_SELECT_ALL_TRACK:
		{
			TrackList* tl = song->visibletracks();
			TrackList selectedTracks = song->getSelectedTracks();
			bool select = true;
			if (selectedTracks.size() == tl->size())
			{
				select = false;
			}

			for (iTrack t = tl->begin(); t != tl->end(); ++t)
			{
				(*t)->setSelected(select);
			}
			song->update(SC_SELECTION);
			break;
		}
		case CMD_SELECT_ALL:
		case CMD_SELECT_NONE:
		case CMD_SELECT_INVERT:
		case CMD_SELECT_ILOOP:
		case CMD_SELECT_OLOOP:
		{
			ComposerCanvas *canvas = composer->getCanvas();
			if(canvas && canvas->tool() == AutomationTool)
			{
				//printf("Automation copy\n");
				composer->cmd(Composer::CMD_SELECT_ALL_AUTOMATION);
			}
			else
			{
				for (iTrack i = tracks->begin(); i != tracks->end(); ++i)
				{
					PartList* parts = (*i)->parts();
					for (iPart p = parts->begin(); p != parts->end(); ++p)
					{
						bool f = false;
						int t1 = p->second->tick();
						int t2 = t1 + p->second->lenTick();
						bool inside =
								((t1 >= l) && (t1 < r))
								|| ((t2 > l) && (t2 < r))
								|| ((t1 <= l) && (t2 > r));
						switch (cmd)
						{
							case CMD_SELECT_INVERT:
								f = !p->second->selected();
								break;
							case CMD_SELECT_NONE:
								f = false;
								break;
							case CMD_SELECT_ALL:
								f = true;
								break;
							case CMD_SELECT_ILOOP:
								f = inside;
								break;
							case CMD_SELECT_OLOOP:
								f = !inside;
								break;
						}
						p->second->setSelected(f);
					}
				}
				song->update();
			}
		}
		break;
		case CMD_SELECT_PARTS:
			for (iTrack i = tracks->begin(); i != tracks->end(); ++i)
			{
				if (!(*i)->selected())
					continue;
				PartList* parts = (*i)->parts();
				for (iPart p = parts->begin(); p != parts->end(); ++p)
					p->second->setSelected(true);
			}
			song->update();
			break;
		case CMD_FOLLOW_NO:
			song->setFollow(Song::NO);
			setFollow();
			break;
		case CMD_FOLLOW_JUMP:
			song->setFollow(Song::JUMP);
			setFollow();
			break;
		case CMD_FOLLOW_CONTINUOUS:
			song->setFollow(Song::CONTINUOUS);
			setFollow();
			break;
	}
}

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void OOMidi::clipboardChanged()
{
	bool flag = false;
	if (QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-midipartlist")) ||
			QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-wavepartlist")) ||
			QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-mixedpartlist")) ||
			QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-automationcurve")))
		flag = true;

	editPasteAction->setEnabled(flag);
	editInsertAction->setEnabled(flag);
	editPasteCloneAction->setEnabled(flag);
	editPaste2TrackAction->setEnabled(flag);
	editPasteC2TAction->setEnabled(flag);
}

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void OOMidi::selectionChanged()
{
	//bool flag = composer->isSingleSelection();  // -- Hmm, why only single?
	bool flag = true;//composer->selectionSize() > 0; // -- Test OK cut and copy. For oom2. Tim.
	editCutAction->setEnabled(flag);
	editCopyAction->setEnabled(flag);
}

//---------------------------------------------------------
//   transpose
//---------------------------------------------------------

void OOMidi::transpose()
{
	Transpose *w = new Transpose();
	w->show();
}

//---------------------------------------------------------
//   modifyGateTime
//---------------------------------------------------------

void OOMidi::modifyGateTime()
{
	GateTime* w = new GateTime(this);
	w->show();
}

//---------------------------------------------------------
//   modifyVelocity
//---------------------------------------------------------

void OOMidi::modifyVelocity()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   crescendo
//---------------------------------------------------------

void OOMidi::crescendo()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   thinOut
//---------------------------------------------------------

void OOMidi::thinOut()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   eraseEvent
//---------------------------------------------------------

void OOMidi::eraseEvent()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   noteShift
//---------------------------------------------------------

void OOMidi::noteShift()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   moveClock
//---------------------------------------------------------

void OOMidi::moveClock()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   copyMeasure
//---------------------------------------------------------

void OOMidi::copyMeasure()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   eraseMeasure
//---------------------------------------------------------

void OOMidi::eraseMeasure()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   deleteMeasure
//---------------------------------------------------------

void OOMidi::deleteMeasure()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   createMeasure
//---------------------------------------------------------

void OOMidi::createMeasure()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   mixTrack
//---------------------------------------------------------

void OOMidi::mixTrack()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   loadTheme
//---------------------------------------------------------

void OOMidi::loadTheme(const QString& s)
{
	if (style()->objectName() != s)
		QApplication::setStyle(s);
}

//---------------------------------------------------------
//   loadStyleSheetFile
//---------------------------------------------------------

void OOMidi::loadStyleSheetFile(const QString& s)
{
	if (s.isEmpty())
	{
		qApp->setStyleSheet(s);
		return;
	}

	QFile cf(s);
	if (cf.open(QIODevice::ReadOnly))
	{
		QByteArray ss = cf.readAll();
		QString sheet(QString::fromUtf8(ss.data()));
		qApp->setStyleSheet(sheet);
		cf.close();
	}
	else
		printf("loading style sheet <%s> failed\n", qPrintable(s));
}

//---------------------------------------------------------
//   configChanged
//    - called whenever configuration has changed
//    - when configuration has changed by user, call with
//      writeFlag=true to save configuration in ~/.config/OOMidi
//---------------------------------------------------------

void OOMidi::changeConfig(bool writeFlag)
{
	if (writeFlag)
		writeGlobalConfiguration();

	emit configChanged();
	updateConfiguration();
}

//---------------------------------------------------------
//   configMetronome
//---------------------------------------------------------

void OOMidi::configMetronome()
{
	if (!metronomeConfig)
		metronomeConfig = new MetronomeConfig;

	if (metronomeConfig->isVisible())
	{
		metronomeConfig->raise();
		metronomeConfig->activateWindow();
	}
	else
		metronomeConfig->show();
}


//---------------------------------------------------------
//   configShortCuts
//---------------------------------------------------------

void OOMidi::configShortCuts()
{
	if (!shortcutConfig)
		shortcutConfig = new ShortcutConfig(this);
	shortcutConfig->_config_changed = false;
	if (shortcutConfig->exec())
		changeConfig(true);
}

//---------------------------------------------------------
//   globalCut
//    - remove area between left and right locator
//    - do not touch muted track
//    - cut master track
//---------------------------------------------------------

void OOMidi::globalCut()
{
	int lpos = song->lpos();
	int rpos = song->rpos();
	if ((lpos - rpos) >= 0)
		return;

	song->startUndo();
	TrackList* tracks = song->tracks();
	for (iTrack it = tracks->begin(); it != tracks->end(); ++it)
	{
		MidiTrack* track = dynamic_cast<MidiTrack*> (*it);
		if (track == 0 || track->mute())
			continue;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			Part* part = p->second;
			int t = part->tick();
			int l = part->lenTick();
			if (t + l <= lpos)
				continue;
			if ((t >= lpos) && ((t + l) <= rpos))
			{
				audio->msgRemovePart(part, false);
			}
			else if ((t < lpos) && ((t + l) > lpos) && ((t + l) <= rpos))
			{
				// remove part tail
				int len = lpos - t;
				MidiPart* nPart = new MidiPart(*(MidiPart*) part);
				nPart->setLenTick(len);
				//
				// cut Events in nPart
				EventList* el = nPart->events();
				iEvent ie = el->lower_bound(t + len);
				for (; ie != el->end();)
				{
					iEvent i = ie;
					++ie;
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgDeleteEvent(i->second, nPart, false);
					audio->msgDeleteEvent(i->second, nPart, false, false, false);
				}
				// Indicate no undo, and do port controller values and clone parts.
				//audio->msgChangePart(part, nPart, false);
				audio->msgChangePart(part, nPart, false, true, true);
			}
			else if ((t < lpos) && ((t + l) > lpos) && ((t + l) > rpos))
			{
				//----------------------
				// remove part middle
				//----------------------

				MidiPart* nPart = new MidiPart(*(MidiPart*) part);
				EventList* el = nPart->events();
				iEvent is = el->lower_bound(lpos);
				iEvent ie = el->upper_bound(rpos);
				for (iEvent i = is; i != ie;)
				{
					iEvent ii = i;
					++i;
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgDeleteEvent(ii->second, nPart, false);
					audio->msgDeleteEvent(ii->second, nPart, false, false, false);
				}

				ie = el->lower_bound(rpos);
				for (; ie != el->end();)
				{
					iEvent i = ie;
					++ie;
					Event event = i->second;
					Event nEvent = event.clone();
					nEvent.setTick(nEvent.tick() - (rpos - lpos));
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(event, nEvent, nPart, false);
					audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
				}
				nPart->setLenTick(l - (rpos - lpos));
				// Indicate no undo, and do port controller values and clone parts.
				//audio->msgChangePart(part, nPart, false);
				audio->msgChangePart(part, nPart, false, true, true);
			}
			else if ((t >= lpos) && (t < rpos) && (t + l) > rpos)
			{
				// TODO: remove part head
			}
			else if (t >= rpos)
			{
				MidiPart* nPart = new MidiPart(*(MidiPart*) part);
				int nt = part->tick();
				nPart->setTick(nt - (rpos - lpos));
				// Indicate no undo, and do port controller values but not clone parts.
				//audio->msgChangePart(part, nPart, false);
				audio->msgChangePart(part, nPart, false, true, false);
			}
		}
	}
	// TODO: cut tempo track
	// TODO: process marker
	song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED);
}

//---------------------------------------------------------
//   globalInsert
//    - insert empty space at left locator position upto
//      right locator
//    - do not touch muted track
//    - insert in master track
//---------------------------------------------------------

void OOMidi::globalInsert()
{
	unsigned lpos = song->lpos();
	unsigned rpos = song->rpos();
	if (lpos >= rpos)
		return;

	song->startUndo();
	TrackList* tracks = song->tracks();
	for (iTrack it = tracks->begin(); it != tracks->end(); ++it)
	{
		MidiTrack* track = dynamic_cast<MidiTrack*> (*it);
		//
		// process only non muted midi tracks
		//
		if (track == 0 || track->mute())
			continue;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			Part* part = p->second;
			unsigned t = part->tick();
			int l = part->lenTick();
			if (t + l <= lpos)
				continue;
			if (lpos >= t && lpos < (t + l))
			{
				MidiPart* nPart = new MidiPart(*(MidiPart*) part);
				nPart->setLenTick(l + (rpos - lpos));
				EventList* el = nPart->events();

				iEvent i = el->end();
				while (i != el->begin())
				{
					--i;
					if (i->first < lpos)
						break;
					Event event = i->second;
					Event nEvent = i->second.clone();
					nEvent.setTick(nEvent.tick() + (rpos - lpos));
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(event, nEvent, nPart, false);
					audio->msgChangeEvent(event, nEvent, nPart, false, false, false);
				}
				// Indicate no undo, and do port controller values and clone parts.
				//audio->msgChangePart(part, nPart, false);
				audio->msgChangePart(part, nPart, false, true, true);
			}
			else if (t > lpos)
			{
				MidiPart* nPart = new MidiPart(*(MidiPart*) part);
				nPart->setTick(t + (rpos - lpos));
				// Indicate no undo, and do port controller values but not clone parts.
				//audio->msgChangePart(part, nPart, false);
				audio->msgChangePart(part, nPart, false, true, false);
			}
		}
	}
	// TODO: process tempo track
	// TODO: process marker
	song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_REMOVED);
}

//---------------------------------------------------------
//   globalSplit
//    - split all parts at the song position pointer
//    - do not touch muted track
//---------------------------------------------------------

void OOMidi::globalSplit()
{
	int pos = song->cpos();
	song->startUndo();
	TrackList* tracks = song->tracks();
	for (iTrack it = tracks->begin(); it != tracks->end(); ++it)
	{
		Track* track = *it;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			Part* part = p->second;
			int p1 = part->tick();
			int l0 = part->lenTick();
			if (pos > p1 && pos < (p1 + l0))
			{
				Part* p1;
				Part* p2;
				track->splitPart(part, pos, p1, p2);
				// Indicate no undo, and do port controller values but not clone parts.
				//audio->msgChangePart(part, p1, false);
				audio->msgChangePart(part, p1, false, true, false);
				audio->msgAddPart(p2, false);
				break;
			}
		}
	}
	song->endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
}

//---------------------------------------------------------
//   copyRange
//    - copy space between left and right locator position
//      to song position pointer
//    - dont process muted tracks
//    - create a new part for every track containing the
//      copied events
//---------------------------------------------------------

void OOMidi::copyRange()
{
	QMessageBox::critical(this,
			tr("OOStudio: Copy Range"),
			tr("not implemented")
			);
}

//---------------------------------------------------------
//   cutEvents
//    - make sure that all events in a part end where the
//      part ends
//    - process only marked parts
//---------------------------------------------------------

void OOMidi::cutEvents()
{
	QMessageBox::critical(this,
			tr("OOStudio: Cut Events"),
			tr("not implemented")
			);
}

//---------------------------------------------------------
//   checkRegionNotNull
//    return true if (rPos - lPos) <= 0
//---------------------------------------------------------

bool OOMidi::checkRegionNotNull()
{
	int start = song->lPos().frame();
	int end = song->rPos().frame();
	if (end - start <= 0)
	{
		QMessageBox::critical(this,
				tr("OOStudio: Bounce"),
				tr("set left/right marker for bounce range")
				);
		return true;
	}
	return false;
}

//---------------------------------------------------------
//   bounceToTrack
//---------------------------------------------------------

void OOMidi::bounceToTrack()
{
	if (audio->bounce())
		return;

	song->bounceOutput = 0;

	if (song->waves()->empty())
	{
		QMessageBox::critical(this,
				tr("OOStudio: Bounce to Track"),
				tr("No wave tracks found")
				);
		return;
	}

	OutputList* ol = song->outputs();
	if (ol->empty())
	{
		QMessageBox::critical(this,
				tr("OOStudio: Bounce to Track"),
				tr("No audio output tracks found")
				);
		return;
	}

	if (checkRegionNotNull())
		return;

	AudioOutput* out = 0;
	// If only one output, pick it, else pick the first selected.
	if (ol->size() == 1)
		out = ol->front();
	else
	{
		for (iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
		{
			AudioOutput* o = *iao;
			if (o->selected())
			{
				if (out)
				{
					out = 0;
					break;
				}
				out = o;
			}
		}
		if (!out)
		{
			QMessageBox::critical(this,
					tr("OOStudio: Bounce to Track"),
					tr("Select one audio output track,\nand one target wave track")
					);
			return;
		}
	}

	// search target track
	TrackList* tl = song->tracks();
	WaveTrack* track = 0;

	for (iTrack it = tl->begin(); it != tl->end(); ++it)
	{
		Track* t = *it;
		if (t->selected())
		{
			if (t->type() != Track::WAVE && t->type() != Track::AUDIO_OUTPUT)
			{
				track = 0;
				break;
			}
			if (t->type() == Track::WAVE)
			{
				if (track)
				{
					track = 0;
					break;
				}
				track = (WaveTrack*) t;
			}

		}
	}

	if (track == 0)
	{
		if (ol->size() == 1)
		{
			QMessageBox::critical(this,
					tr("OOStudio: Bounce to Track"),
					tr("Select one target wave track")
					);
			return;
		}
		else
		{
			QMessageBox::critical(this,
					tr("OOStudio: Bounce to Track"),
					tr("Select one target wave track,\nand one audio output track")
					);
			return;
		}
	}
	song->setPos(0,song->lPos(),0,true,true);
	song->bounceOutput = out;
	song->bounceTrack = track;
	song->setRecord(true);
	song->setRecordFlag(track, true);
	track->prepareRecording();
	audio->msgBounce();
	song->setPlay(true);
}

//---------------------------------------------------------
//   bounceToFile
//---------------------------------------------------------

void OOMidi::bounceToFile(AudioOutput* ao)
{
	if (audio->bounce())
		return;
	song->bounceOutput = 0;
	if (!ao)
	{
		OutputList* ol = song->outputs();
		if (ol->empty())
		{
			QMessageBox::critical(this,
					tr("OOStudio: Bounce to File"),
					tr("No audio output tracks found")
					);
			return;
		}
		// If only one output, pick it, else pick the first selected.
		if (ol->size() == 1)
			ao = ol->front();
		else
		{
			for (iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
			{
				AudioOutput* o = *iao;
				if (o->selected())
				{
					if (ao)
					{
						ao = 0;
						break;
					}
					ao = o;
				}
			}
			if (ao == 0)
			{
				QMessageBox::critical(this,
						tr("OOStudio: Bounce to File"),
						tr("Select one audio output track")
						);
				return;
			}
		}
	}

	if (checkRegionNotNull())
		return;

	SndFile* sf = getSndFile(0, this);
	if (sf == 0)
		return;

	song->setPos(0,song->lPos(),0,true,true);
	song->bounceOutput = ao;
	ao->setRecFile(sf);
	song->setRecord(true, false);
	song->setRecordFlag(ao, true);
	ao->prepareRecording();
	audio->msgBounce();
	song->setPlay(true);
}

#ifdef HAVE_LASH
//---------------------------------------------------------
//   lash_idle_cb
//---------------------------------------------------------
#include <iostream>

void
OOMidi::lash_idle_cb()
{
	lash_event_t * event;
	if (!lash_client)
		return;

	while ((event = lash_get_event(lash_client)))
	{
		switch (lash_event_get_type(event))
		{
			case LASH_Save_File:
			{
				/* save file */
				QString ss = QString(lash_event_get_string(event)) + QString("/lash-project-oom.oom");
				int ok = save(ss.toAscii(), false);
				if (ok)
				{
					project.setFile(ss.toAscii());
					//setWindowTitle(tr("OOMidi: Song: ") + project.completeBaseName());
					setWindowTitle(QString("The Composer - OOStudio-").append(VERSION).append(":     ") + project.completeBaseName()  + QString("     "));
					addProject(ss.toAscii());
					oomProject = QFileInfo(ss.toAscii()).absolutePath();
					oomProjectFile = QFileInfo(ss.toAscii()).filePath();
				}
				lash_send_event(lash_client, event);
			}
				break;

			case LASH_Restore_File:
			{
				/* load file */
				QString sr = QString(lash_event_get_string(event)) + QString("/lash-project-oom.oom");
				loadProjectFile(sr.toAscii(), false, true);
				lash_send_event(lash_client, event);
			}
				break;

			case LASH_Quit:
			{
				/* quit oom */
				std::cout << "OOMidi::lash_idle_cb Received LASH_Quit"
						<< std::endl;
				lash_event_destroy(event);
			}
				break;

			default:
			{
				std::cout << "OOMidi::lash_idle_cb Received unknown LASH event of type "
						<< lash_event_get_type(event)
						<< std::endl;
				lash_event_destroy(event);
			}
				break;
		}
	}
}
#endif /* HAVE_LASH */

//---------------------------------------------------------
//   clearSong
//    return true if operation aborted
//    called with sequencer stopped
//---------------------------------------------------------

bool OOMidi::clearSong()
{
	if (song->dirty)
	{
		int n = 0;
		n = QMessageBox::warning(this, appName,
				tr("The current Project contains unsaved data\n"
				"Load overwrites current Project:\n"
				"Save Current Project?"),
				tr("&Save"), tr("&Don't Save"), tr("&Cancel"), 0, 2);
		switch (n)
		{
			case 0:
				if (!save()) // abort if save failed
					return true;
				break;
			case 1:
				break;
			case 2:
				return true;
			default:
				printf("InternalError: gibt %d\n", n);
		}
	}
	if (audio->isPlaying())
	{
		audio->msgPlay(false);
		while (audio->isPlaying())
			qApp->processEvents();
	}
	microSleep(100000);

	for (iToplevel i = toplevels.begin(); i != toplevels.end(); ++i)
	{
		Toplevel tl = *i;
		unsigned long obj = tl.object();
		switch (tl.type())
		{
			case Toplevel::CLIPLIST:
			case Toplevel::MARKER:
			case Toplevel::PIANO_ROLL:
			case Toplevel::LISTE:
			case Toplevel::DRUM:
			case Toplevel::MASTER:
			case Toplevel::WAVE:
			case Toplevel::LMASTER:
			{
				((QWidget*) (obj))->blockSignals(true);
				((QWidget*) (obj))->close();
			}
				//goto again;
		}
	}
	toplevels.clear();
	if(performer)
	{
		performer->hide();
		delete performer;
		performer = 0;
	}
	//printf("OOMidi::clearSong() TopLevel.size(%d) \n", (int)toplevels.size());
	microSleep(100000);
	emit songClearCalled();
	song->clear(false);
	microSleep(200000);
	return false;
}

//---------------------------------------------------------
//   startEditInstrument
//---------------------------------------------------------

void OOMidi::startEditInstrument()
{
	if (editInstrument == 0)
	{
		editInstrument = new EditInstrument(this);
		editInstrument->show();
	}
	else
	{
		if (!editInstrument->isHidden())
			editInstrument->hide();
		else
			editInstrument->show();
	}

}

//---------------------------------------------------------
//   switchMixerAutomation
//---------------------------------------------------------

void OOMidi::switchMixerAutomation()
{
	automation = !automation;
	// Clear all pressed and touched and rec event lists.
	song->clearRecAutomation(true);

	// printf("automation = %d\n", automation);
	autoMixerAction->setChecked(automation);
}

//---------------------------------------------------------
//   clearAutomation
//---------------------------------------------------------

void OOMidi::clearAutomation()
{
	printf("not implemented\n");
}

//---------------------------------------------------------
//   takeAutomationSnapshot
//---------------------------------------------------------

void OOMidi::takeAutomationSnapshot()
{
	int frame = song->cPos().frame();
	TrackList* tracks = song->tracks();
	for (iTrack i = tracks->begin(); i != tracks->end(); ++i)
	{
		if ((*i)->isMidiTrack())
			continue;
		AudioTrack* track = (AudioTrack*) * i;
		CtrlListList* cll = track->controller();
		for (iCtrlList icl = cll->begin(); icl != cll->end(); ++icl)
		{
			double val = icl->second->curVal();
			icl->second->add(frame, val);
		}
	}
}

//---------------------------------------------------------
//   updateConfiguration
//    called whenever the configuration has changed
//---------------------------------------------------------

void OOMidi::updateConfiguration()
{
	fileOpenAction->setShortcut(shortcuts[SHRT_OPEN].key);
	fileNewAction->setShortcut(shortcuts[SHRT_NEW].key);
	fileSaveAction->setShortcut(shortcuts[SHRT_SAVE].key);
	fileSaveAsAction->setShortcut(shortcuts[SHRT_SAVE_AS].key);

	//menu_file->setShortcut(shortcuts[SHRT_OPEN_RECENT].key, menu_ids[CMD_OPEN_RECENT]);    // Not used.
	fileImportMidiAction->setShortcut(shortcuts[SHRT_IMPORT_MIDI].key);
	fileExportMidiAction->setShortcut(shortcuts[SHRT_EXPORT_MIDI].key);
	//fileImportPartAction->setShortcut(shortcuts[SHRT_IMPORT_PART].key);
	//fileImportWaveAction->setShortcut(shortcuts[SHRT_IMPORT_AUDIO].key);
	quitAction->setShortcut(shortcuts[SHRT_QUIT].key);

	//menu_file->setShortcut(shortcuts[SHRT_LOAD_TEMPLATE].key, menu_ids[CMD_LOAD_TEMPLATE]);  // Not used.

	undoAction->setShortcut(shortcuts[SHRT_UNDO].key);
	redoAction->setShortcut(shortcuts[SHRT_REDO].key);

	editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
	editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
	editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
	editInsertAction->setShortcut(shortcuts[SHRT_INSERT].key);
	editPasteCloneAction->setShortcut(shortcuts[SHRT_PASTE_CLONE].key);
	editPaste2TrackAction->setShortcut(shortcuts[SHRT_PASTE_TO_TRACK].key);
	editPasteC2TAction->setShortcut(shortcuts[SHRT_PASTE_CLONE_TO_TRACK].key);
	editInsertEMAction->setShortcut(shortcuts[SHRT_INSERTMEAS].key);

	//editDeleteSelectedAction has no acceleration

	trackMidiAction->setShortcut(shortcuts[SHRT_ADD_MIDI_TRACK].key);
	//trackDrumAction->setShortcut(shortcuts[SHRT_ADD_DRUM_TRACK].key);
	trackWaveAction->setShortcut(shortcuts[SHRT_ADD_WAVE_TRACK].key);
	trackAOutputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_OUTPUT].key);
	trackAGroupAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_BUSS].key);
	trackAInputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_INPUT].key);
	trackAAuxAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_AUX].key);

	editSelectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
	editSelectAllTracksAction->setShortcut(shortcuts[SHRT_SEL_ALL_TRACK].key);
	editDeselectAllAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
	editInvertSelectionAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
	editInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
	editOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
	editAllPartsAction->setShortcut(shortcuts[SHRT_SELECT_PRTSTRACK].key);

	startPianoEditAction->setShortcut(shortcuts[SHRT_OPEN_PIANO].key);
	startListEditAction->setShortcut(shortcuts[SHRT_OPEN_LIST].key);

	masterGraphicAction->setShortcut(shortcuts[SHRT_OPEN_GRAPHIC_MASTER].key);
	masterListAction->setShortcut(shortcuts[SHRT_OPEN_LIST_MASTER].key);

	midiTransposeAction->setShortcut(shortcuts[SHRT_TRANSPOSE].key);
	midiTransformerAction->setShortcut(shortcuts[SHRT_OPEN_MIDI_TRANSFORM].key);

	viewTransportAction->setShortcut(shortcuts[SHRT_OPEN_TRANSPORT].key);
	viewBigtimeAction->setShortcut(shortcuts[SHRT_OPEN_BIGTIME].key);
	viewMixerAAction->setShortcut(shortcuts[SHRT_OPEN_MIXER].key);
	viewMarkerAction->setShortcut(shortcuts[SHRT_OPEN_MARKER].key);

	strGlobalCutAction->setShortcut(shortcuts[SHRT_GLOBAL_CUT].key);
	strGlobalInsertAction->setShortcut(shortcuts[SHRT_GLOBAL_INSERT].key);
	strGlobalSplitAction->setShortcut(shortcuts[SHRT_GLOBAL_SPLIT].key);
	strCopyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
	strCutEventsAction->setShortcut(shortcuts[SHRT_CUT_EVENTS].key);

	midiResetInstAction->setShortcut(shortcuts[SHRT_MIDI_RESET].key);
	midiInitInstActions->setShortcut(shortcuts[SHRT_MIDI_INIT].key);
	midiLocalOffAction->setShortcut(shortcuts[SHRT_MIDI_LOCAL_OFF].key);
	midiTrpAction->setShortcut(shortcuts[SHRT_MIDI_INPUT_TRANSPOSE].key);
	midiInputTrfAction->setShortcut(shortcuts[SHRT_MIDI_INPUT_TRANSFORM].key);
	midiInputFilterAction->setShortcut(shortcuts[SHRT_MIDI_INPUT_FILTER].key);
	midiRemoteAction->setShortcut(shortcuts[SHRT_MIDI_REMOTE_CONTROL].key);
#ifdef BUILD_EXPERIMENTAL
	midiRhythmAction->setShortcut(shortcuts[SHRT_RANDOM_RHYTHM_GENERATOR].key);
#endif

	audioBounce2TrackAction->setShortcut(shortcuts[SHRT_AUDIO_BOUNCE_TO_TRACK].key);
	audioBounce2FileAction->setShortcut(shortcuts[SHRT_AUDIO_BOUNCE_TO_FILE].key);
	audioRestartAction->setShortcut(shortcuts[SHRT_AUDIO_RESTART].key);

	autoMixerAction->setShortcut(shortcuts[SHRT_MIXER_AUTOMATION].key);
	autoSnapshotAction->setShortcut(shortcuts[SHRT_MIXER_SNAPSHOT].key);
	autoClearAction->setShortcut(shortcuts[SHRT_MIXER_AUTOMATION_CLEAR].key);

	settingsGlobalAction->setShortcut(shortcuts[SHRT_GLOBAL_CONFIG].key);
	settingsShortcutsAction->setShortcut(shortcuts[SHRT_CONFIG_SHORTCUTS].key);
	settingsMetronomeAction->setShortcut(shortcuts[SHRT_CONFIG_METRONOME].key);
	settingsMidiAssignAction->setShortcut(shortcuts[SHRT_CONFIG_MIDI_PORTS].key);


	dontFollowAction->setShortcut(shortcuts[SHRT_FOLLOW_NO].key);
	followPageAction->setShortcut(shortcuts[SHRT_FOLLOW_JUMP].key);
	followCtsAction->setShortcut(shortcuts[SHRT_FOLLOW_CONTINUOUS].key);

	helpManualAction->setShortcut(shortcuts[SHRT_OPEN_HELP].key);

	
	loopAction->setShortcut(shortcuts[SHRT_TOGGLE_LOOP].key);
	//punchinAction->setShortcut(shortcuts[].key);
	//punchoutAction->setShortcut(shortcuts[].key);
	startAction->setShortcut(shortcuts[SHRT_GOTO_START].key);
	//rewindAction->setShortcut(shortcuts[].key);
	//forwardAction->setShortcut(shortcuts[].key);
	stopAction->setShortcut(shortcuts[SHRT_STOP].key);
	playAction->setShortcut(shortcuts[SHRT_PLAY_SONG].key);
	//recordAction->setShortcut(shortcuts[].key);
	panicAction->setShortcut(shortcuts[SHRT_MIDI_PANIC].key);
	
}

//---------------------------------------------------------
//   showBigtime
//---------------------------------------------------------

void OOMidi::showBigtime(bool on)
{
	if (on && bigtime == 0)
	{
		bigtime = new BigTime(0);
		bigtime->setPos(0, song->cpos(), false);
		connect(song, SIGNAL(posChanged(int, unsigned, bool)), bigtime, SLOT(setPos(int, unsigned, bool)));
		connect(oom, SIGNAL(configChanged()), bigtime, SLOT(configChanged()));
		connect(bigtime, SIGNAL(closed()), SLOT(bigtimeClosed()));
		bigtime->resize(config.geometryBigTime.size());
		bigtime->move(config.geometryBigTime.topLeft());
	}
	if (bigtime)
		bigtime->setVisible(on);
	viewBigtimeAction->setChecked(on);
}

//---------------------------------------------------------
//   toggleBigTime
//---------------------------------------------------------

void OOMidi::toggleBigTime(bool checked)
{
	showBigtime(checked);
}

//---------------------------------------------------------
//   bigtimeClosed
//---------------------------------------------------------

void OOMidi::bigtimeClosed()
{
	viewBigtimeAction->setChecked(false);
}

//---------------------------------------------------------
//   showMixer1
//---------------------------------------------------------

void OOMidi::showMixer1(bool on)
{
	if (on && mixer1 == 0)
	{
		mixer1 = new AudioMixer("Mixer1", this);
		mixer1->setObjectName("Mixer1");
		mixer1->setWindowRole("Mixer1");
		connect(mixer1, SIGNAL(closed()), SLOT(mixer1Closed()));
		//mixer1->resize(config.mixer1.geometry.size());
		//mixer1->move(config.mixer1.geometry.topLeft());
	}
	if (mixer1)
		mixer1->setVisible(on);
	viewMixerAAction->setChecked(on);
}

//---------------------------------------------------------
//   showMixer2
//---------------------------------------------------------

void OOMidi::showMixer2(bool)
{
	/*if (on && mixer2 == 0)
	{
		mixer2 = new AudioMixer(this, &(config.mixer2));
		mixer2->setObjectName("Mixer2");
		mixer2->setWindowRole("Mixer2");
		connect(mixer2, SIGNAL(closed()), SLOT(mixer2Closed()));
		//mixer2->resize(config.mixer2.geometry.size());
		//mixer2->move(config.mixer2.geometry.topLeft());
	}
	if (mixer2)
		mixer2->setVisible(on);
	viewMixerBAction->setChecked(on);
	*/
}

AudioPortConfig* OOMidi::getRoutingDialog(bool)
{
	configMidiAssign(0);
	return midiAssignDialog->getAudioPortConfig();
}

//---------------------------------------------------------
//   toggleMixer1
//---------------------------------------------------------

void OOMidi::toggleMixer1(bool checked)
{
	showMixer1(checked);
}

//---------------------------------------------------------
//   toggleMixer2
//---------------------------------------------------------

void OOMidi::toggleMixer2(bool checked)
{
	showMixer2(checked);
}

//---------------------------------------------------------
//   mixer1Closed
//---------------------------------------------------------

void OOMidi::mixer1Closed()
{
	viewMixerAAction->setChecked(false);
}

//---------------------------------------------------------
//   mixer2Closed
//---------------------------------------------------------

void OOMidi::mixer2Closed()
{
	//viewMixerBAction->setChecked(false);
}


//QWidget* OOMidi::mixerWindow()     { return audioMixer; }

QWidget* OOMidi::mixer1Window()
{
	return mixer1;
}

QWidget* OOMidi::mixer2Window()
{
	return 0;
}

QWidget* OOMidi::transportWindow()
{
	return transport;
}

QWidget* OOMidi::bigtimeWindow()
{
	return bigtime;
}

//---------------------------------------------------------
//   focusInEvent
//---------------------------------------------------------

void OOMidi::focusInEvent(QFocusEvent* ev)
{
	//if (audioMixer)
	//      audioMixer->raise();
	if (mixer1)
		mixer1->raise();
	//if (mixer2)
	//	mixer2->raise();
	raise();
	QMainWindow::focusInEvent(ev);
}

//---------------------------------------------------------
//   setUsedTool
//---------------------------------------------------------

void OOMidi::setUsedTool(int tool)
{
	tools1->set(tool);
}

void OOMidi::configMidiAssign(int tab)
{
	if(!midiAssignDialog)
	{
		midiAssignDialog = new MidiAssignDialog(this);
	}
	midiAssignDialog->show();
	midiAssignDialog->raise();
	midiAssignDialog->activateWindow();
	if(tab >= 0)
		midiAssignDialog->switchTabs(tab);
}

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------

void OOMidi::execDeliveredScript(int id)
{
	//QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
	song->executeScript(song->getScriptPath(id, true).toLatin1().constData(), song->getSelectedMidiParts(), 0, false); // TODO: get quant from composer
}
//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------

void OOMidi::execUserScript(int id)
{
	song->executeScript(song->getScriptPath(id, false).toLatin1().constData(), song->getSelectedMidiParts(), 0, false); // TODO: get quant from composer
}

//---------------------------------------------------------
// Instrument Template code
//
//---------------------------------------------------------

void OOMidi::insertInstrumentTemplate(TrackView* tv, int idx)
{
	Q_UNUSED(idx);
	m_instrumentTemplates.insert(tv->id(), tv);
	emit instrumentTemplateAdded(tv->id());
}

void OOMidi::removeInstrumentTemplate(qint64 id)
{
	if(m_instrumentTemplates.contains(id))
	{
		m_instrumentTemplates.erase(m_instrumentTemplates.find(id));
		emit instrumentTemplateRemoved(id);
	}
}

TrackView* OOMidi::addInstrumentTemplate()
{
	TrackView* tv = new TrackView(true);
	tv->setDefaultName();
	m_instrumentTemplates.insert(tv->id(), tv);
	//emit instrumentTemplateAdded(tv->id());

	return tv;
}

TrackView* OOMidi::findInstrumentTemplateById(qint64 id) const
{
	if(m_instrumentTemplates.contains(id))
		return m_instrumentTemplates.value(id);
	return 0;
}

//---------------------------------------------------------
// View Toolbars code
//---------------------------------------------------------

void OOMidi::showToolbarOrchestra(bool yesno)
{
    if (_resourceDock)
        _resourceDock->setVisible(yesno);
}

void OOMidi::showToolbarMixer(bool yesno)
{
    if (m_mixerDock)
        m_mixerDock->setVisible(yesno);
}

void OOMidi::showToolbarComposerSettings(bool yesno)
{
    if (toolbarComposerSettings)
        toolbarComposerSettings->setVisible(yesno);
}

void OOMidi::showToolbarSnap(bool yesno)
{
    if (toolbarSnap)
        toolbarSnap->setVisible(yesno);
}

void OOMidi::showToolbarTransport(bool yesno)
{
    if (tools)
        tools->setVisible(yesno);
}

void OOMidi::updateViewToolbarMenu()
{
    if (_resourceDock)
    {
        viewToolbarOrchestra->setEnabled(true);
        viewToolbarOrchestra->setChecked(_resourceDock->isVisible());
    }
    else
        viewToolbarOrchestra->setEnabled(false);
    
    if (m_mixerDock)
    {
        viewToolbarMixer->setEnabled(true);
        viewToolbarMixer->setChecked(m_mixerDock->isVisible());
    }
    else
        viewToolbarMixer->setEnabled(false);

    if (toolbarComposerSettings)
    {
        viewToolbarComposerSettings->setEnabled(true);
        viewToolbarComposerSettings->setChecked(toolbarComposerSettings->isVisible());
    }
    else
        viewToolbarComposerSettings->setEnabled(false);

    if (toolbarSnap)
    {
        viewToolbarSnap->setEnabled(true);
        viewToolbarSnap->setChecked(toolbarSnap->isVisible());
    }
    else
        viewToolbarSnap->setEnabled(false);

    if (tools)
    {
        viewToolbarTransport->setEnabled(true);
        viewToolbarTransport->setChecked(tools->isVisible());
    }
    else
        viewToolbarTransport->setEnabled(false);
}

void OOMidi::showEvent(QShowEvent*)
{
	emit viewReady();
}
