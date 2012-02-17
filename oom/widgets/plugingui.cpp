//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//  (C) Copyright 2012 Filipe Coelho
//=========================================================

#include "plugingui.h"

#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QTimer>

#include <math.h>

#include "checkbox.h"
#include "doublelabel.h"
#include "slider.h"

#include "filedialog.h"
#include "globals.h"
#include "icons.h"
#include "plugin.h"
#include "song.h"

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(BasePlugin* p)
: QMainWindow(0),
  plugin(p)
{
    QString title;
    title += "OOStudio: ";
    title += plugin->name();
    if (plugin->track() && plugin->track()->name().isEmpty() == false)
    {
        title += " - ";
        title += plugin->track()->name();
    }

    setWindowTitle(title);
    setWindowIcon(*oomIcon);
    setObjectName("PluginGui");

    // menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // file menu --------------------------------------------
    QMenu* fileMenu = new QMenu(tr("File"), menuBar);

    QAction* fileOpen = new QAction(QIcon(*openIconS), tr("Load State..."), menuBar);
    connect(fileOpen, SIGNAL(triggered()), this, SLOT(load()));
    fileMenu->addAction(fileOpen);

    QAction* fileSave = new QAction(QIcon(*saveIcon), tr("Save State..."), menuBar);
    connect(fileSave, SIGNAL(triggered()), this, SLOT(save()));
    fileMenu->addAction(fileSave);

    fileMenu->addSeparator();

    QAction* fileReset = new QAction(tr("Reset to Default State"), menuBar);
    connect(fileReset, SIGNAL(triggered()), this, SLOT(reset()));
    fileMenu->addAction(fileReset);

    // plugin menu --------------------------------------------
    QMenu* pluginMenu = new QMenu(tr("Plugin"), menuBar);

    pluginBypass = new QAction(tr("Bypass"), menuBar);
    pluginBypass->setCheckable(true);
    pluginBypass->setChecked(plugin->active() == false);
    connect(pluginBypass, SIGNAL(triggered(bool)), SLOT(bypassToggled(bool)));
    pluginMenu->addAction(pluginBypass);

    pluginMenu->addSeparator();

    //QAction* pluginInfo = new QAction(tr("Show Information"), menuBar);
    //connect(pluginInfo, SIGNAL(triggered()), this, SLOT(showPluginInfo()));
    //pluginMenu->addAction(pluginInfo);

    // preset menu --------------------------------------------
    presetMenu = new QMenu(tr("Presets"), menuBar);
    connect(presetMenu, SIGNAL(aboutToShow()), SLOT(populatePresetMenu()));

    // show menu
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(pluginMenu);
    menuBar->addMenu(presetMenu);
    menuBar->show();

    mw = new QWidget;
    mw->setObjectName("PluginGuiBase");
    mw->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    view = new QScrollArea(this);
    view->setWidget(mw);
    view->setWidgetResizable(true);
    setCentralWidget(view);
    //view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QGridLayout* grid = new QGridLayout(mw);
    grid->setColumnStretch(2, 10);
    grid->setSpacing(2);

    uint32_t n = plugin->getParameterCount();
    if (n > 0)
        params = new GuiParam[n];
    else
        params = 0;

    int wHeight = fontMetrics().height() + 8;

    for (uint32_t i = 0; i < n; i++)
    {
        ParameterPort* paramPort = plugin->getParameterPort(i);
        if (! paramPort || paramPort->type != PARAMETER_INPUT)
        {
            params[i].type = GuiParam::GUI_NULL;
            continue;
        }

        params[i].hints = paramPort->hints;

        QLabel* label = 0;

        if (paramPort->hints & PARAMETER_IS_TOGGLED)
        {
            CheckBox* cb = new CheckBox(mw, i, "param");
            cb->setId(i);
            cb->setText(plugin->getParameterName(i));
            cb->setChecked(paramPort->value > 0.5);
            cb->setFixedHeight(wHeight);

            params[i].type  = GuiParam::GUI_SWITCH;
            params[i].label = 0;
            params[i].actuator = cb;
        }
        else
        {
            QString labelName = plugin->getParameterName(i);
            QString labelUnit = plugin->getParameterUnit(i);
            if (labelUnit.isEmpty() == false && labelUnit != "coef")
            {
                labelName += " (";
                labelName += labelUnit;
                labelName += ")";
            }
            label = new QLabel(labelName, 0);
            params[i].type  = GuiParam::GUI_SLIDER;
            params[i].label = new DoubleLabel(paramPort->value, paramPort->ranges.min, paramPort->ranges.max, this); // NOTE - parent was 0
            params[i].label->setFrame(true);
            params[i].label->setPrecision(2);
            params[i].label->setId(i);

            params[i].label->setContentsMargins(2, 2, 2, 2);
            params[i].label->setFixedHeight(wHeight);

            // make room for bigger values
            if (paramPort->ranges.min <= -10000 || paramPort->ranges.max > 10000)
                params[i].label->setMinimumWidth(70);
            else if (paramPort->ranges.min <= -1000 || paramPort->ranges.max > 1000)
                params[i].label->setMinimumWidth(60);
            else if (paramPort->ranges.min <= -100 || paramPort->ranges.max > 100)
                params[i].label->setMinimumWidth(55);
            else if (paramPort->ranges.min <= -10 || paramPort->ranges.max > 10)
                params[i].label->setMinimumWidth(50);

            Slider* s = new Slider(mw, "param", Qt::Horizontal, Slider::None, Slider::BgSlot, QColor(127, 125, 32));

            s->setCursorHoming(true);
            s->setId(i);
            s->setFixedHeight(wHeight);
            s->setRange(paramPort->ranges.min, paramPort->ranges.max, paramPort->ranges.step);
            s->setValue(paramPort->value);

            params[i].actuator = s;
            params[i].label->setSlider((Slider*) params[i].actuator);
        }

        //params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
        params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

        if (params[i].type == GuiParam::GUI_SWITCH)
        {
            grid->addWidget(params[i].actuator, i, 0, 1, 3);

            connect(params[i].actuator, SIGNAL(checkboxPressed(int)), SLOT(ctrlPressed(int)));
            connect(params[i].actuator, SIGNAL(checkboxReleased(int)), SLOT(ctrlReleased(int)));
            connect(params[i].actuator, SIGNAL(checkboxRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
        }
        else if (params[i].type == GuiParam::GUI_SLIDER)
        {
            label->setFixedHeight(20);
            //label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
            label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            ///params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
            params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(params[i].label, i, 1);
            grid->addWidget(params[i].actuator, i, 2);

            connect(params[i].actuator, SIGNAL(sliderMoved(double, int)), SLOT(sliderChanged(double, int)));
            connect(params[i].label, SIGNAL(valueChanged(double, int)), SLOT(labelChanged(double, int)));
            connect(params[i].actuator, SIGNAL(sliderPressed(int)), SLOT(ctrlPressed(int)));
            connect(params[i].actuator, SIGNAL(sliderReleased(int)), SLOT(ctrlReleased(int)));
            connect(params[i].actuator, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
        }
    }

    adjustSize();
    resize(350, height());

    connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::~PluginGui()
{
    if (params)
        delete[] params;
}


//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void PluginGui::heartBeat()
{
    updateControls();
}

//---------------------------------------------------------
//   setActive
//---------------------------------------------------------

void PluginGui::setActive(bool yesno)
{
    pluginBypass->blockSignals(true);
    pluginBypass->setChecked(!yesno);
    pluginBypass->blockSignals(false);
}

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PluginGui::updateValues()
{
    for (uint32_t i = 0; i < plugin->getParameterCount(); i++)
    {
        GuiParam* gp = &params[i];
        double value = plugin->getParameterValue(i);
        if (gp->type == GuiParam::GUI_SLIDER)
        {
//            double sv = lv;
//            if (params[i].hints & PARAMETER_IS_LOGARITHMIC)
//                sv = fast_log10(lv) * 20.0;
//            else if (params[i].hints & PARAMETER_IS_INTEGER)
//            {
//                sv = rint(lv);
//                lv = sv;
//            }
            gp->label->setValue(value);
            ((Slider*) (gp->actuator))->setValue(value);
        }
        else if (gp->type == GuiParam::GUI_SWITCH)
        {
            ((CheckBox*) (gp->actuator))->setChecked(value > 0.5);
        }
    }
}

//---------------------------------------------------------
//   setParameterValue
//---------------------------------------------------------

void PluginGui::setParameterValue(int index, double value)
{
    GuiParam* gp = &params[index];
    if (gp)
    {
        if (gp->type == GuiParam::GUI_SLIDER)
        {
            gp->label->setValue(value);
            ((Slider*) (gp->actuator))->setValue(value);
        }
        else if (gp->type == GuiParam::GUI_SWITCH)
        {
            ((CheckBox*) (gp->actuator))->setChecked(value > 0.5);
        }
    }
}

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void PluginGui::updateControls()
{
    if (!automation || !plugin->track() || plugin->id() == -1)
        return;

    AutomationType at = plugin->track()->automationType();
    if (at == AUTO_OFF)
        return;

    for (uint32_t i = 0; i < plugin->getParameterCount(); i++)
    {
        GuiParam* gp = &params[i];

        if (gp->type == GuiParam::GUI_SLIDER)
        {
            if (plugin->controllerEnabled(i) && plugin->controllerEnabled2(i))
            {
                double lv = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), i));
                double sv = lv;
                //if (params[i].hints & PARAMETER_IS_LOGARITHMIC)
                //    sv = fast_log10(lv) * 20.0;
                //else if (params[i].hints & PARAMETER_IS_INTEGER)
                //{
                //    sv = rint(lv);
                //    lv = sv;
                //}
                if (((Slider*) (gp->actuator))->value() != sv)
                {
                    gp->label->blockSignals(true);
                    ((Slider*) (gp->actuator))->blockSignals(true);
                    ((Slider*) (gp->actuator))->setValue(sv);
                    gp->label->setValue(lv);
                    ((Slider*) (gp->actuator))->blockSignals(false);
                    gp->label->blockSignals(false);
                }
            }

        }
        else if (gp->type == GuiParam::GUI_SWITCH)
        {
            if (plugin->controllerEnabled(i) && plugin->controllerEnabled2(i))
            {
                bool v = (int) plugin->track()->pluginCtrlVal(genACnum(plugin->id(), i));
                if (((CheckBox*) (gp->actuator))->isChecked() != v)
                {
                    //printf("PluginGui::updateControls switch\n");

                    ((CheckBox*) (gp->actuator))->blockSignals(true);
                    ((CheckBox*) (gp->actuator))->setChecked(v);
                    ((CheckBox*) (gp->actuator))->blockSignals(false);
                }
            }
        }
    }
}
//---------------------------------------------------------
//   load
//---------------------------------------------------------

void PluginGui::load()
{
    QString s("presets/plugins/");
    //s += plugin->plugin()->label();
    s += plugin->label();
    s += "/";

    QString fn = getOpenFileName(s, preset_file_pattern,
                                 this, tr("OOStudio: load preset"), 0);
    if (fn.isEmpty())
        return;
    bool popenFlag;
    FILE* f = fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
    if (f == 0)
        return;

    Xml xml(f);
    int mode = 0;
    for (;;)
    {
        Xml::Token token = xml.parse();
        QString tag = xml.s1();
        switch (token)
        {
        case Xml::Error:
        case Xml::End:
            return;
        case Xml::TagStart:
            if (mode == 0 && (tag == "oom" || tag == "muse"))
                mode = 1;
            else if (mode == 1 && tag == "plugin")
            {

                if (plugin->readConfiguration(xml, true))
                {
                    QMessageBox::critical(this, QString("OOStudio"),
                                          tr("Error reading preset. Might not be right type for this plugin"));
                    goto ende;
                }

                mode = 0;
            }
            else
                xml.unknown("PluginGui");
            break;
        case Xml::Attribut:
            break;
        case Xml::TagEnd:
            if (!mode && (tag == "oom" || tag == "muse"))
            {
                plugin->updateControllers();
                goto ende;
            }
        default:
            break;
        }
    }
ende:
    if (popenFlag)
        pclose(f);
    else
        fclose(f);
}

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void PluginGui::save()
{
    QString s("presets/plugins/");
    s += plugin->label();
    s += "/";

    //QString fn = getSaveFileName(s, preset_file_pattern, this,
    QString fn = getSaveFileName(s, preset_file_save_pattern, this,
                                 tr("OOStudio: save preset"));
    if (fn.isEmpty())
        return;
    bool popenFlag;
    FILE* f = fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, true);
    if (f == 0)
        return;
    Xml xml(f);
    xml.header();
    xml.tag(0, "oom version=\"1.0\"");
    plugin->writeConfiguration(1, xml);
    xml.tag(1, "/oom");

    if (popenFlag)
        pclose(f);
    else
        fclose(f);
}

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void PluginGui::reset()
{
    for (uint32_t i = 0; i < plugin->getParameterCount(); i++)
    {
        ParameterPort* paramPort = plugin->getParameterPort(i);
        if (! paramPort || paramPort->type != PARAMETER_INPUT)
            continue;

        double value = paramPort->ranges.def;
        plugin->setParameterValue(i, value);

        if (params[i].type == GuiParam::GUI_SLIDER)
        {
            if (params[i].hints & PARAMETER_IS_INTEGER)
                value = rint(value);
            ((DoubleLabel*) params[i].label)->setValue(value);
            ((Slider*) params[i].actuator)->setValue(value);
        }
        else if (params[i].type == GuiParam::GUI_SWITCH)
        {
            ((CheckBox*) params[i].actuator)->setChecked(value > 0.5);
        }
    }
}

//---------------------------------------------------------
//   bypassToggled
//---------------------------------------------------------

void PluginGui::bypassToggled(bool val)
{
    plugin->setActive(!val);
    song->update(SC_ROUTE);
}

//---------------------------------------------------------
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param)
{
    AutomationType at = AUTO_OFF;
    AudioTrack* track = plugin->track();
    if (track)
        at = track->automationType();

    if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        plugin->enableController(param, false);

    //if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
    //    val = pow(10.0, val / 20.0);
    //else
    if (params[param].hints & PARAMETER_IS_INTEGER)
        val = rint(val);

    if (plugin->getParameterValue(param) != val)
    {
        plugin->setParameterValue(param, val);
        ((DoubleLabel*) params[param].label)->setValue(val);
    }

    int id = plugin->id();
    if (id == -1)
        return;
    id = genACnum(id, param);

    // p3.3.43
    //audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);

    if (track)
    {
        // p3.3.43
        audio->msgSetPluginCtrlVal(track, id, val);
        track->recordAutomation(id, val);
    }
}

//---------------------------------------------------------
//   labelChanged
//---------------------------------------------------------

void PluginGui::labelChanged(double val, int param)/*{{{*/
{
    AutomationType at = AUTO_OFF;
    AudioTrack* track = plugin->track();
    if (track)
        at = track->automationType();

    if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        plugin->enableController(param, false);

    double dval = val;
    //if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
    //    dval = fast_log10(val) * 20.0;
    //else
    if (params[param].hints & PARAMETER_IS_INTEGER)
        dval = rint(val);

    if (plugin->getParameterValue(param) != val)
    {
        plugin->setParameterValue(param, val);
        ((Slider*) params[param].actuator)->setValue(dval);
    }

    int id = plugin->id();
    if (id == -1)
        return;

    id = genACnum(id, param);

    // p3.3.43
    //audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);

    if (track)
    {
        // p3.3.43
        audio->msgSetPluginCtrlVal(track, id, val);
        track->startAutoRecord(id, val);
    }
}/*}}}*/

//---------------------------------------------------------
//   ctrlPressed
//---------------------------------------------------------

void PluginGui::ctrlPressed(int param)
{
    AutomationType at = AUTO_OFF;
    AudioTrack* track = plugin->track();
    if (track)
        at = track->automationType();

    if (at != AUTO_OFF)
        plugin->enableController(param, false);

    int id = plugin->id();

    if (id == -1)
        return;

    id = genACnum(id, param);

    if (params[param].type == GuiParam::GUI_SLIDER)
    {
        double val = ((Slider*) params[param].actuator)->value();
        //if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
        //    val = pow(10.0, val / 20.0);
        //else
        if (params[param].hints & PARAMETER_IS_INTEGER)
            val = rint(val);
        plugin->setParameterValue(param, val);
        ((DoubleLabel*) params[param].label)->setValue(val);

        // p3.3.43
        //audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);

        if (track)
        {
            // p3.3.43
            audio->msgSetPluginCtrlVal(track, id, val);
            track->startAutoRecord(id, val);
        }
    }
    else if (params[param].type == GuiParam::GUI_SWITCH)
    {
        double val = (double) ((CheckBox*) params[param].actuator)->isChecked();
        plugin->setParameterValue(param, val);

        // p3.3.43
        //audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);

        if (track)
        {
            // p3.3.43
            audio->msgSetPluginCtrlVal(track, id, val);
            track->startAutoRecord(id, val);
        }
    }
}

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::ctrlReleased(int param)
{
    AutomationType at = AUTO_OFF;
    AudioTrack* track = plugin->track();
    if (track)
        at = track->automationType();

    // Special for switch - don't enable controller until transport stopped.
    if (at != AUTO_WRITE && ((params[param].type != GuiParam::GUI_SWITCH
                              || !audio->isPlaying()
                              || at != AUTO_TOUCH) || (!audio->isPlaying() && at == AUTO_TOUCH)))
        plugin->enableController(param, true);

    int id = plugin->id();
    if (!track || id == -1)
        return;
    id = genACnum(id, param);

    if (params[param].type == GuiParam::GUI_SLIDER)
    {
        double val = ((Slider*) params[param].actuator)->value();
        //if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
        //    val = pow(10.0, val / 20.0);
        //else
        if (params[param].hints & PARAMETER_IS_INTEGER)
            val = rint(val);
        track->stopAutoRecord(id, val);
    }
    //else if(params[param].type == GuiParam::GUI_SWITCH)
    //{
    //double val = (double)((CheckBox*)params[param].actuator)->isChecked();
    // No concept of 'untouching' a checkbox. Remain 'touched' until stop.
    //plugin->track()->stopAutoRecord(genACnum(plugin->id(), param), val);
    //}
}

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void PluginGui::ctrlRightClicked(const QPoint &p, int param)
{
    int id = plugin->id();
    if (id != -1)
        song->execAutomationCtlPopup(plugin->track(), p, genACnum(id, param));
}

//---------------------------------------------------------
//   populatePresetMenu
//---------------------------------------------------------

void PluginGui::populatePresetMenu()
{
    presetMenu->clear();

    if (plugin && plugin->getProgramCount() > 0)
    {
        for (uint32_t i = 0; i < plugin->getProgramCount(); i++)
        {
            QAction* preset = new QAction(plugin->getProgramName(i), menuBar());
            preset->setData(i);
            connect(preset, SIGNAL(triggered()), SLOT(programSelected()));
            presetMenu->addAction(preset);
        }
    }
    else
    {
        QAction* presetNone = new QAction(tr("(none)"), menuBar());
        presetNone->setEnabled(false);
        presetMenu->addAction(presetNone);
    }
}

void PluginGui::programSelected()
{
    if (!plugin)
        return;

    bool ok;
    unsigned int program = ((QAction*)sender())->data().toInt(&ok);

    if (ok)
    {
        plugin->setProgram(program);
        updateValues();
    }
}
