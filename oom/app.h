//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: app.h,v 1.34.2.14 2009/11/16 11:29:33 lunar_shuttle Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __APP_H__
#define __APP_H__

#include "config.h"
#include "cobject.h"
#include "toolbars/tools.h"
#include "cserver.h"
#ifdef LSCP_SUPPORT
#include "network/lsclient.h"
#endif
#include "trackview.h"
#include <QFileInfo>
#include <QPair>

#include "al/sig.h"

class QCloseEvent;
class QFocusEvent;
class QMainWindow;
class QMenu;
class QPoint;
class QRect;
class QScrollArea;
class QSignalMapper;
class QString;
class QToolBar;
class QToolButton;
class QActionGroup;
class QDockWidget;
class QMessageBox;
class QUndoStack;
class QUndoView;
class QShowEvent;

class Part;
class PartList;
class Transport;
class BigTime;
class Composer;
class Instrument;
class PopupMenu;
class PopupView;
class Track;
class PrinterConfig;
class MidiSyncConfig;
class MRConfig;
class MetronomeConfig;
class AudioConf;
class Xml;
class AudioMixer;
class ClipListEdit;
class AudioRecord;
class MidiFileConfig;
class MidiFilterConfig;
class MarkerView;
class GlobalSettingsConfig;
class MidiControllerEditDialog;
class MidiInputTransformDialog;
class MidiTransformerDialog;
class SynthI;
class RhythmGen;
class MidiTrack;
class MidiInstrument;
class MidiPort;
class ShortcutConfig;
class WaveTrack;
class AudioOutput;
class EditInstrument;
class AudioPortConfig;
class QDocWidget;
class MidiAssignDialog;
class MidiPlayEvent;
class MixerDock;
class VirtualTrack;
class Performer;

#define MENU_ADD_SYNTH_ID_BASE 0x1000
#define OOCMD_PORT 8415

//---------------------------------------------------------
//   OOMidi
//---------------------------------------------------------

class OOMidi : public QMainWindow
{

    Q_OBJECT
    enum
    {
	CMD_CUT, CMD_COPY, CMD_PASTE, CMD_INSERT, CMD_INSERTMEAS, CMD_PASTE_CLONE,
	CMD_PASTE_TO_TRACK, CMD_PASTE_CLONE_TO_TRACK, CMD_DELETE,
	CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
	CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
	CMD_FOLLOW_NO, CMD_FOLLOW_JUMP, CMD_FOLLOW_CONTINUOUS,
	CMD_DELETE_TRACK, CMD_SELECT_ALL_TRACK
    };

    //File menu items:

    enum
    {
	CMD_OPEN_RECENT = 0, CMD_LOAD_TEMPLATE, CMD_SAVE_AS, CMD_IMPORT_MIDI,
	CMD_EXPORT_MIDI, CMD_IMPORT_PART, CMD_IMPORT_AUDIO, CMD_QUIT, CMD_OPEN_DRUMS, CMD_OPEN_WAVE,
	CMD_OPEN_LIST, CMD_OPEN_LIST_MASTER, CMD_GLOBAL_CONFIG,
	CMD_OPEN_GRAPHIC_MASTER, CMD_OPEN_MIDI_TRANSFORM, CMD_TRANSPOSE,
	CMD_GLOBAL_CUT, CMD_GLOBAL_INSERT, CMD_GLOBAL_SPLIT, CMD_COPY_RANGE,
	CMD_CUT_EVENTS, CMD_CONFIG_SHORTCUTS, CMD_CONFIG_METRONOME, CMD_CONFIG_MIDISYNC,
	CMD_MIDI_FILE_CONFIG, CMD_APPEARANCE_SETTINGS, CMD_CONFIG_MIDI_PORTS, CMD_CONFIG_AUDIO_PORTS,
	CMD_MIDI_EDIT_INSTRUMENTS, CMD_MIDI_RESET, CMD_MIDI_INIT, CMD_MIDI_LOCAL_OFF,
	CMD_MIXER_SNAPSHOT, CMD_MIXER_AUTOMATION_CLEAR, CMD_OPEN_HELP, CMD_OPEN_HOMEPAGE,
	CMD_OPEN_BUG, CMD_START_WHATSTHIS,
	CMD_AUDIO_BOUNCE_TO_FILE, CMD_AUDIO_BOUNCE_TO_TRACK, CMD_AUDIO_RESTART,
	CMD_LAST
    };

    //int menu_ids[CMD_LAST];

    // File menu actions
    QAction *fileSaveAction, *fileOpenAction, *fileNewAction, *testAction;
    QAction *fileSaveAsAction, *fileImportMidiAction, *fileExportMidiAction, *fileImportPartAction, *fileImportWaveAction, *quitAction;

    // Edit Menu actions
    QAction *editCutAction, *editCopyAction, *editPasteAction, *editInsertAction, *editPasteCloneAction, *editPaste2TrackAction;
    QAction *editPasteC2TAction, *editInsertEMAction, *editDeleteSelectedAction, *editSelectAllAction, *editDeselectAllAction, *editSelectAllTracksAction;
    QAction *editInvertSelectionAction, *editInsideLoopAction, *editOutsideLoopAction, *editAllPartsAction;
    QAction *trackMidiAction, *trackDrumAction, *trackWaveAction, *trackAOutputAction, *trackAGroupAction;
    QAction *trackAInputAction, *trackAAuxAction;
    QAction *startPianoEditAction, *startDrumEditAction, *startListEditAction;
    QAction *masterGraphicAction, *masterListAction;
    QAction *midiTransposeAction;
    QAction *midiTransformerAction;

    // View Menu actions
    QAction *viewTransportAction, *viewBigtimeAction, *viewMixerAAction, *viewCliplistAction, *viewMarkerAction;
    QAction *viewToolbarOrchestra, *viewToolbarMixer, *viewToolbarComposerSettings, *viewToolbarSnap, *viewToolbarTransport;

    // Structure Menu actions
    QAction *strGlobalCutAction, *strGlobalInsertAction, *strGlobalSplitAction, *strCopyRangeAction, *strCutEventsAction;

    // Midi Menu Actions
    QAction *midiEditInstAction, *midiResetInstAction, *midiInitInstActions, *midiLocalOffAction;
    QAction *midiTrpAction, *midiInputTrfAction, *midiInputFilterAction, *midiRemoteAction;
#ifdef BUILD_EXPERIMENTAL
    QAction *midiRhythmAction;
#endif

    // Audio Menu Actions
    QAction *audioBounce2TrackAction, *audioBounce2FileAction, *audioRestartAction;

    // Automation Menu Actions
    QAction *autoMixerAction, *autoSnapshotAction, *autoClearAction;

    // Settings Menu Actions
    QAction *settingsGlobalAction, *settingsShortcutsAction, *settingsMetronomeAction;
    QAction *settingsMidiIOAction, *settingsMidiAssignAction;
    QAction *dontFollowAction, *followPageAction, *followCtsAction;

    // Help Menu Actions
    QAction *helpManualAction, *helpHomepageAction, *helpReportAction, *helpAboutAction;

    QString appName;

    QFileInfo project;
    QToolBar *tools;
    EditToolBar *tools1;

    Transport* transport;
    BigTime* bigtime;
    EditInstrument* editInstrument;
	Performer* performer;

    QToolBar* toolbarComposerSettings;
    QToolBar* toolbarSnap;

    QMenu *menu_file, *menuView, *menuSettings, *menu_help;
    QMenu *menuEdit, *menuStructure;
    QMenu* menu_audio, *menuAutomation;
    QMenu* menu_functions, *menuScriptPlugins;
    QMenu* select, *master, *viewToolbars, *midiEdit, *addTrack;

    // Special 'stay-open' menu for routes.
    PopupMenu* routingPopupMenu;

    QMenu* follow;
    QMenu* midiInputPlugins;

	MidiAssignDialog* midiAssignDialog;
    QWidget* midiPortConfig;
    QWidget* softSynthesizerConfig;
    MidiSyncConfig* midiSyncConfig;
    MRConfig* midiRemoteConfig;
    RhythmGen* midiRhythmGenerator;
    MetronomeConfig* metronomeConfig;
    AudioConf* audioConfig;
    MidiFileConfig* midiFileConfig;
    GlobalSettingsConfig* globalSettingsConfig;
    MidiFilterConfig* midiFilterConfig;
    MidiInputTransformDialog* midiInputTransform;
    ShortcutConfig* shortcutConfig;
    //Appearance* appearance;
    AudioMixer* mixer1;
    AudioMixer* mixer2;
    AudioPortConfig* routingDialog;

    ToplevelList toplevels;
    ClipListEdit* clipListEdit;
    MarkerView* markerView;
    MidiTransformerDialog* midiTransformerDialog;
    QMenu* openRecent;

	QDockWidget* _resourceDock;
	MixerDock *m_mixerWidget;
	QDockWidget *m_mixerDock;
	QMessageBox *pipelineBox;

	QUndoStack* m_undoStack;
	QUndoView* m_undoView;

	bool m_externalCall;

    QSignalMapper *editSignalMapper;
    QSignalMapper *midiPluginSignalMapper;
    QSignalMapper *followSignalMapper;
	OOMCommandServer server;
#ifdef LSCP_SUPPORT
	LSClient *lsclient;
	bool lscpRestart;
#endif
	
	QString m_initProjectName;
	bool m_useTemplate;
	int m_rasterVal;
	
	//-------------------------------------------------------------
	// Instrument Templates
	//-------------------------------------------------------------
	TrackViewList m_instrumentTemplates;


    bool readMidi(FILE*);
    void read(Xml& xml, bool skipConfig);
    void processTrack(MidiTrack* track);

    void write(Xml& xml) const;
    bool clearSong();
    bool save(const QString&, bool);
    void setUntitledProject();
    void setConfigDefaults();

    void setFollow();
    void readConfigParts(Xml& xml);
    void readToplevels(Xml& xml);
    PartList* getMidiPartsToEdit();
    Part* readPart(Xml& xml);
    bool checkRegionNotNull();
    void loadProjectFile1(const QString&, bool songTemplate, bool loadAll);
    void writeGlobalConfiguration(int level, Xml&) const;
    void writeConfiguration(int level, Xml&) const;
    void updateConfiguration();

    virtual void focusInEvent(QFocusEvent*);
    virtual void keyPressEvent(QKeyEvent*); // p4.0.10 Tim.
    bool eventFilter(QObject *obj, QEvent *event);

	//-------------------------------------------------------------
	// Instrument Templates
	//-------------------------------------------------------------
	void writeInstrumentTemplates(int, Xml&) const;
	void readInstrumentTemplates(Xml&);
	void readInstrumentTemplates();

signals:
    void configChanged();
#ifdef LSCP_SUPPORT
	void channelInfoChanged(const LSCPChannelInfo&);
	void lscpStartListener();
	void lscpStopListener();
#endif
	
	//-------------------------------------------------------------
	// Instrument Templates
	//-------------------------------------------------------------

	void instrumentTemplateAdded(qint64 id);
	void instrumentTemplateRemoved(qint64 id);

	void viewReady();
	void songClearCalled();

protected:
	virtual void showEvent(QShowEvent*);

private slots:
    //void runPythonScript();
    void loadProject();
    void about();
    void aboutQt();
    void startHelpBrowser();
    void startHomepageBrowser();
    void startBugBrowser();
    void launchBrowser(QString &whereTo);
    void importMidi();
    void importPart();
    void exportMidi();

    void toggleTransport(bool);
    void toggleMarker(bool);
    void toggleBigTime(bool);
    void toggleMixer1(bool);
    void toggleMixer2(bool);

    void showToolbarOrchestra(bool);
    void showToolbarMixer(bool);
    void showToolbarComposerSettings(bool);
    void showToolbarSnap(bool);
    void showToolbarTransport(bool);
    void updateViewToolbarMenu();

    void configMidiFile();
    void configShortCuts();
    //void configAppearance();
    void startEditor(PartList*, int);
    void startMasterEditor();
    void startLMasterEditor();
    void startListEditor();
    void startListEditor(PartList*);
    void startDrumEditor();
    void startDrumEditor(PartList* /*pl*/, bool /*showDefaultCtrls*/ = false);
    void startEditor(Track*);
    void startPerformer();
    void startPerformer(PartList* /*pl*/, bool /*showDefaultCtrls*/ = false);
    void startWaveEditor();
    void startWaveEditor(PartList*);
    void startSongInfo(bool editable = true);

    void startMidiTransformer();
    void writeGlobalConfiguration() const;
    void startEditInstrument();
    void startClipList(bool);

    void openRecentMenu();
    void selectProject(QAction* act);
    void cmd(int);
    void clipboardChanged();
    void selectionChanged();
    void transpose();
    void modifyGateTime();
    void modifyVelocity();
    void crescendo();
    void thinOut();
    void eraseEvent();
    void noteShift();
    void moveClock();
    void copyMeasure();
    void eraseMeasure();
    void deleteMeasure();
    void createMeasure();
    void mixTrack();
    void startMidiInputPlugin(int);
    void hideMitPluginTranspose();
    void hideMidiInputTransform();
    void hideMidiFilterConfig();
    void hideMidiRemoteConfig();
#ifdef BUILD_EXPERIMENTAL
    void hideMidiRhythmGenerator();
#endif
    void globalCut();
    void globalInsert();
    void globalSplit();
    void copyRange();
    void cutEvents();
    void bounceToTrack();
    void resetMidiDevices();
    void initMidiDevices();
    void localOff();
    void switchMixerAutomation();
    void takeAutomationSnapshot();
    void clearAutomation();
    void bigtimeClosed();
    void mixer1Closed();
    void mixer2Closed();
    void markerClosed();

    void execDeliveredScript(int);
    void execUserScript(int);

	void performerClosed();
	void loadInitialProject();
	void lsStartupFailed();


public slots:
    void quitDoc(bool ext = false);
    bool save();
    bool saveAs();
    void configGlobalSettings(int tab = 0);
    void bounceToFile(AudioOutput* ao = 0);
    void closeEvent(QCloseEvent*e);
    void loadProjectFile(const QString&);
    void loadProjectFile(const QString&, bool songTemplate, bool loadAll);
    void toplevelDeleted(unsigned long tl);
    void loadTheme(const QString&);
    void loadStyleSheetFile(const QString&);
    bool seqRestart();
    void loadTemplate();
    void showBigtime(bool);
    void showMixer1(bool);
    void showMixer2(bool);
    void showMarker(bool);
    void importMidi(const QString &file);
    void setUsedTool(int);
    void showDidYouKnowDialog();
    void importWave(Track* track = NULL);
    void configMetronome();

    void routingPopupMenuAboutToHide();
    void configMidiAssign(int tab = -1);
	bool startServer();
	void stopServer();
#ifdef LSCP_SUPPORT
	void startLSCPClient();
	void stopLSCPClient();
	void restartLSCPSubscribe();
#endif
	void pipelineStateChanged(int);
	void connectDefaultSongPorts();
	void showUndoView();
    
    Tool getCurrentTool();
	void setRaster(int r)
	{
		m_rasterVal = r;
	}


public:
    OOMidi(int argc, char** argv);
    ~OOMidi();
    Composer* composer;
	//FIXME: Hack to make projects loaded from the commandline not start PR
	//I need to track where and what is cause it to crash
	bool firstrun;
    QRect configGeometryMain;
    bool importMidi(const QString name, bool merge);
    void kbAccel(int);
    void changeConfig(bool writeFlag);

    void seqStop();
    bool seqStart();
    void setHeartBeat();
    void importController(int, MidiPort*, int);
    //QWidget* mixerWindow();
    QWidget* mixer1Window();
    QWidget* mixer2Window();
    QWidget* transportWindow();
    QWidget* bigtimeWindow();
	AudioPortConfig* getRoutingDialog(bool);
    bool importWaveToTrack(QString& name, unsigned tick = 0, Track* track = NULL);
    void importPartToTrack(QString& filename, unsigned tick, Track* track);

    void showTransport(bool flag);

    // Special 'stay-open' menu for routes.
    PopupMenu* getRoutingPopupMenu();
    PopupMenu* prepareRoutingPopupMenu(Track* /*track*/, bool /*dst*/);
    void routingPopupMenuActivated(Track* /*track*/, int /*id*/);
    void updateRouteMenus(Track* /*track*/, QObject* /*master*/);

	QDockWidget* resourceDock() { return _resourceDock; }
	void addTransportToolbar();
    void setComposerAndSnapToolbars(QToolBar*, QToolBar*);
	bool saveRouteMapping(QString, QString note = "Untitled");
	bool loadRouteMapping(QString);
	bool updateRouteMapping(QString, QString);
	QString noteForRouteMapping(QString);
	QString* currentProject()
	{
		if(project.completeBaseName().endsWith("oom"))
			return new QString(project.filePath());
		else
			return new QString();
	}
	QDockWidget* mixerDock() { return m_mixerDock; }

	//-------------------------------------------------------------
	// Instrument Templates
	//-------------------------------------------------------------

	TrackViewList* instrumentTemplates()
	{
		return &m_instrumentTemplates;
	}

	TrackView* addInstrumentTemplate();
	TrackView* findInstrumentTemplateById(qint64 id) const;
    void insertInstrumentTemplate(TrackView*, int idx);
    void removeInstrumentTemplate(qint64);
	void addGlobalInput(QPair<int, QString> pinfo);
	void initGlobalInputPorts();

    int rasterStep(unsigned tick) const/*{{{*/
    {
        return AL::sigmap.rasterStep(tick, m_rasterVal);
    }

    unsigned rasterVal(unsigned v) const
    {
        return AL::sigmap.raster(v, m_rasterVal);
    }

    unsigned rasterVal1(unsigned v) const
    {
        return AL::sigmap.raster1(v, m_rasterVal);
    }

    unsigned rasterVal2(unsigned v) const
    {
        return AL::sigmap.raster2(v, m_rasterVal);
    }/*}}}*/
	int raster(){return m_rasterVal;}
#ifdef HAVE_LASH
    void lash_idle_cb();
#endif
};

extern void addProject(const QString& name);
#endif

