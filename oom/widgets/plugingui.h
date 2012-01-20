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

class QAction;
class BasePlugin;
class DoubleLabel;
class QScrollArea;

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam {
    enum { GUI_NULL, GUI_SLIDER, GUI_SWITCH };
    int type;
    int hints;
    DoubleLabel* label;
    QWidget* actuator; // Slider or Toggle Button (SWITCH)
};

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------
class PluginGui : public QMainWindow
{
    Q_OBJECT

public:
    PluginGui(BasePlugin*);
    ~PluginGui();

    void setActive(bool);
    void setParameterValue(int index, double value);
    void updateValues();

protected slots:
    void heartBeat();

private:
    BasePlugin* const plugin;

    QWidget* mw; // main widget
    QScrollArea* view;

    QAction* pluginBypass;
    QMenu* presetMenu;
    GuiParam* params;

    void updateControls();

private slots:
    void load();
    void save();
    void reset();
    void bypassToggled(bool);
    void sliderChanged(double, int);
    void labelChanged(double, int);
    void ctrlPressed(int);
    void ctrlReleased(int);
    void ctrlRightClicked(const QPoint &, int);
    void populatePresetMenu();
    void programSelected();
};

#endif
