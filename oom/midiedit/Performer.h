//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: Performer.h,v 1.5.2.4 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PERFORMER_H__
#define __PERFORMER_H__

#include <QCloseEvent>
#include <QResizeEvent>
#include <QLabel>
#include <QKeyEvent>

#include <values.h>
#include "cobject.h"
#include "noteinfo.h"
#include "AbstractMidiEditor.h"
#include "mididev.h"
#include "toolbars/tools.h"
#include "event.h"
#ifdef LSCP_SUPPORT
#include "network/lsclient.h"
#endif

class MidiPart;
class TimeLabel;
class PitchLabel;
class PosLabel;
class QLabel;
class PerformerCanvas;
class MTScale;
class PCScale;
class SigScale;
class Track;
class QToolButton;
class QToolBar;
class QPushButton;
class CtrlEdit;
class Splitter;
class PartList;
class Xml;
class QuantConfig;
class ScrollScale;
class Part;
class SNode;
class QMenu;
class QAction;
class QWidget;
class QScrollBar;
class Conductor;
class QScrollArea;
class Piano;
class Patch;
class QDockWidget;
class QTabWidget;
class TrackListView;
class QSignalMapper;
class QHBoxLayout;
class QVBoxLayout;

//---------------------------------------------------------
//   Performer
//---------------------------------------------------------

class Performer : public AbstractMidiEditor
{
    Q_OBJECT

    Event selEvent;
    MidiPart* selPart;
    int selTick;

	QString m_currentInstrument;

    QMenu *menuEdit, *menuFunctions, *menuSelect, *menuConfig, *eventColor, *menuPlugins;
    Conductor *midiConductor;
    Track* selected;
    PCScale* pcbar;
	SigScale* m_sigRuler;
	Piano* piano;

	QSignalMapper* mapper;
	QSignalMapper* functionMapper;
	QSignalMapper* colorMapper;

    QAction* editCutAction;
    QAction* editCopyAction;
    QAction* editPasteAction;
    QAction* editDelEventsAction;

    QAction* selectAllAction;
    QAction* selectNoneAction;
    QAction* selectInvertAction;
    QAction* selectInsideLoopAction;
    QAction* selectOutsideLoopAction;
    QAction* selectPrevPartAction;
    QAction* selectNextPartAction;

    QAction* evColorBlueAction;
    QAction* evColorPitchAction;
    QAction* evColorVelAction;

    QAction* funcOverQuantAction;
    QAction* funcNoteOnQuantAction;
    QAction* funcNoteOnOffQuantAction;
    QAction* funcIterQuantAction;
    QAction* funcConfigQuantAction;
    QAction* funcGateTimeAction;
    QAction* funcModVelAction;
    QAction* funcCrescendoAction;
    QAction* funcTransposeAction;
    QAction* funcThinOutAction;
    QAction* funcEraseEventAction;
    QAction* funcNoteShiftAction;
    QAction* funcMoveClockAction;
    QAction* funcCopyMeasureAction;
    QAction* funcEraseMeasureAction;
    QAction* funcDelMeasureAction;
    QAction* funcCreateMeasureAction;
    QAction* funcSetFixedLenAction;
    QAction* funcDelOverlapsAction;

    QAction* funcLocateSelectionAction;
    QAction* funcSelectRightAction;
    QAction* funcSelectRightAddAction;
    QAction* funcSelectLeftAction;
    QAction* funcSelectLeftAddAction;
    QAction* funcIncreaseOctaveAction;
    QAction* funcDecreaseOctaveAction;
    QAction* funcIncreasePitchAction;
    QAction* funcDecreasePitchAction;
    QAction* funcIncreasePosAction;
    QAction* funcDecreasePosAction;
    QAction* funcIncreaseLenAction;
    QAction* funcDecreaseLenAction;
    QAction* funcGotoSelNoteAction;

    QAction* funcAddProgramChangeAction;
    QAction* funcDelProgramChangeAction;
    QAction* funcCopyProgramChangeAction;
    QAction* funcPasteProgramChangeAction;

    int tickOffset;
    int lenOffset;
    int pitchOffset;
    int veloOnOffset;
    int veloOffOffset;
    bool deltaMode;
    bool _stepQwerty;

    NoteInfo* info;
    QToolButton* alpha;
    //QToolButton* srec;
    QToolButton* midiin;
    QToolButton* solo;
    PosLabel* posLabel;
    PitchLabel* pitchLabel;
	QLabel *patchLabel;

    QFrame* canvasFrame;
	QVBoxLayout *m_layout;

    QToolButton* speaker;
    QToolButton* m_globalKey;
    QToolButton* m_globalArm;
    QToolButton* m_mutePart;
	QToolButton* m_btnNext;
	QToolButton* m_btnPrev;
	QToolButton* m_btnAddProgram;
	QToolButton* m_btnDeleteProgram;

	QAction* m_muteAction;
	QAction* m_soloAction;
	QAction* m_globalKeyAction;
	QAction* m_globalArmAction;
	QAction* m_stepAction;
	QAction* m_speakerAction;
    QToolBar* cursorBar;
    QToolBar* editToolbar;
    EditToolBar* editTools;

    int colorMode;

    static int _quantInit, _rasterInit;
    static int _widthInit, _heightInit;

    static int _quantStrengthInit;
    static int _quantLimitInit;
    static bool _quantLenInit;
    static int _toInit;
    static int colorModeInit;

    int _quantStrength;
    int _quantLimit;
    int _to;
    bool _quantLen;
    QuantConfig* quantConfig;
    bool _playEvents;

    QScrollArea* infoScroll;
	QDockWidget* m_prDock;
	QTabWidget* m_tabs;
	TrackListView* m_trackListView;

    void initShortcuts();
	void initFunctions();
    void setEventColorMode(int);
    QWidget* genToolbar(QWidget* parent);
    virtual void closeEvent(QCloseEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void showEvent(QShowEvent *);
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
	void toggleMuteCurrentPart(bool);
    void toggleMultiPartSelection(bool);
    void toggleEpicEdit(bool);
    void setSelection(int, Event&, Part*);
    void noteinfoChanged(NoteInfo::ValType, int);
    void soloChanged(bool flag);
    void setRaster(int);
    void setQuant(int);
    void configQuant();
    void ctrlPopup();

    void setQuantStrength(int val)
    {
        _quantStrength = val;
    }

    void setQuantLimit(int val)
    {
        _quantLimit = val;
    }

    void setQuantLen(bool val)
    {
        _quantLen = val;
    }
    void cmd(int);
    void setSteprec(bool);

    void setTo(int val)
    {
        _to = val;
    }
    void eventColorModeChanged(int);
    void clipboardChanged(); // enable/disable "Paste"
    void selectionChanged(); // enable/disable "Copy" & "Paste"
    void setSpeaker(bool);
    void setTime(unsigned);
    void setTimeFromSig(unsigned);
    void follow(int pos);
    void songChanged1(int);
    void configChanged();
    void updateConductor();
	void dockAreaChanged(Qt::DockWidgetArea);
	void selectPrevPart();
	void selectNextPart();
	void checkPartLengthForRecord(bool);
#ifdef LSCP_SUPPORT
	void setKeyBindings(LSCPChannelInfo);
#endif
	void toggleCollapseAllControllers(bool);
	void updateControllerForInstrument(qint64);
	void saveInstrumentControllerState();

signals:
    void deleted();
	void showComments(bool);
	void showComments(bool, bool);
	void controllerMaxHeightChanged(int);

public slots:
    virtual void updateHScrollRange();
    void execDeliveredScript(int id);
    void execUserScript(int id);
    CtrlEdit* addCtrl(QString name = "Velocity", bool collapsed = false, int height = 80);
    void removeCtrl(CtrlEdit* ctrl);
	void setKeyBindings(Patch*);
	void updateControllerSizes();

public:
    Performer(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
    ~Performer();
    //void setSolo(bool val);
	bool isCurrentPatch(int, int, int);
	bool isGlobalEdit();

    virtual void setCurCanvasPart(Part*);
	virtual void updateCanvas();
    virtual void readStatus(Xml&);
    virtual void writeStatus(int, Xml&) const;
    static void readConfiguration(Xml&);
    static void writeConfiguration(int, Xml&);
};

#endif

