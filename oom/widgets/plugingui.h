//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _PLUGINGUI_H_
#define _PLUGINGUI_H_

#include <QMainWindow>
#include <QUiLoader>

class QAbstractButton;
class QComboBox;
class QRadioButton;
class QScrollArea;
class QToolButton;
class PluginIBase;

class Xml;
class Slider;
class DoubleLabel;
class AudioTrack;
class MidiController;
class PluginGui;

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam/*{{{*/
{
    enum
    {
        GUI_SLIDER, GUI_SWITCH
    };
    int type;
    int hint;

    DoubleLabel* label;
    QWidget* actuator; // Slider or Toggle Button (SWITCH)
};/*}}}*/

//---------------------------------------------------------
//   GuiWidgets
//---------------------------------------------------------

struct GuiWidgets/*{{{*/
{
    enum
    {
        SLIDER, DOUBLE_LABEL, QCHECKBOX, QCOMBOBOX
    };
    QWidget* widget;
    int type;
    int param;
};/*}}}*/


//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------
class PluginGui : public QMainWindow/*{{{*/
{
    Q_OBJECT

    PluginIBase* plugin; // plugin instance

    GuiParam* params;
    int nobj; // number of widgets in gw
    GuiWidgets* gw;

    QAction* onOff;
    QWidget* mw; // main widget
    QScrollArea* view;

    virtual void updateControls();

private slots:
    virtual void load();
    virtual void save();
    virtual void bypassToggled(bool);
    virtual void sliderChanged(double, int);
    virtual void labelChanged(double, int);
    virtual void guiParamChanged(int);
    virtual void ctrlPressed(int);
    virtual void ctrlReleased(int);
    virtual void guiParamPressed(int);
    virtual void guiParamReleased(int);
    virtual void guiSliderPressed(int);
    virtual void guiSliderReleased(int);
    virtual void ctrlRightClicked(const QPoint &, int);
    virtual void guiSliderRightClicked(const QPoint &, int);

protected slots:
    virtual void heartBeat();

public:
    PluginGui(PluginIBase*);

    ~PluginGui();
    void setOn(bool);
    virtual void updateValues();
};/*}}}*/

#endif
