//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrlpanel.h,v 1.2.2.5 2009/06/10 00:34:59 terminator356 Exp $
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_PANEL_H__
#define __CTRL_PANEL_H__

#include <QWidget>

class MidiController;

class QMenu;
class QPushButton;
class QString;

class AbstractMidiEditor;
class Knob;
class DoubleLabel;
class MidiPort;
class MidiTrack;
class QComboBox;

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

class CtrlPanel : public QWidget
{
    Q_OBJECT

    ///QMenu* pop;
    QPushButton* selCtrl;
    AbstractMidiEditor* editor;

    MidiTrack* _track;
    MidiController* _ctrl;
    int _dnum;
    bool inHeartBeat;
    Knob* _knob;
    DoubleLabel* _dl;
    int _val;
	QComboBox* _cmbMode;


signals:
    void destroyPanel();
    void controllerChanged(int);

private slots:
    void ctrlChanged(double val);
    void labelDoubleClicked();
    void ctrlRightClicked(const QPoint& p, int id);
    //void ctrlReleased(int id);

protected slots:
    virtual void heartBeat();

public slots:
    void setHeight(int);
    void ctrlPopup();

public:
    CtrlPanel(QWidget*, AbstractMidiEditor*, const char* name = 0);
	virtual ~CtrlPanel(){}
    void setHWController(MidiTrack* t, MidiController* ctrl);
    bool ctrlSetTypeByName(QString);
};
#endif
