//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cmath>
#include <math.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalMapper>
#include <QSizePolicy>
#include <QScrollArea>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWhatsThis>

#include "globals.h"
#include "gconfig.h"
#include "filedialog.h"
#include "slider.h"
#include "midictrl.h"
#include "plugin.h"
#include "plugingui.h"
#include "xml.h"
#include "icons.h"
#include "song.h"
#include "doublelabel.h"
#include "fastlog.h"
#include "checkbox.h"

#include "audio.h"
#include "al/dsp.h"

#include "config.h"

// TODO: We need to use .qrc files to use icons in WhatsThis bubbles. See Qt 
// Resource System in Qt documentation - ORCAN
//const char* presetOpenText = "<img source=\"fileopen\"> "
//      "Click this button to load a saved <em>preset</em>.";
const char* presetOpenText = "Click this button to load a saved <em>preset</em>.";
const char* presetSaveText = "Click this button to save curent parameter "
		"settings as a <em>preset</em>.  You will be prompted for a file name.";
const char* presetBypassText = "Click this button to bypass effect unit";

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(BasePlugin* p)
: QMainWindow(0)
{
	gw = 0;
	params = 0;
	plugin = p;
	setWindowTitle(plugin->name());
	setObjectName("PluginGui");

	QToolBar* tools = addToolBar(tr("File Buttons"));

	QAction* fileOpen = new QAction(QIcon(*openIconS), tr("Load Preset"), this);
	connect(fileOpen, SIGNAL(triggered()), this, SLOT(load()));
	tools->addAction(fileOpen);

	QAction* fileSave = new QAction(QIcon(*saveIconS), tr("Save Preset"), this);
	connect(fileSave, SIGNAL(triggered()), this, SLOT(save()));
	tools->addAction(fileSave);

	tools->addAction(QWhatsThis::createAction(this));

	onOff = new QAction(QIcon(*exitIconS), tr("bypass plugin"), this);
	onOff->setCheckable(true);
    onOff->setChecked(plugin->active());
	onOff->setToolTip(tr("bypass plugin"));
	connect(onOff, SIGNAL(toggled(bool)), SLOT(bypassToggled(bool)));
	tools->addAction(onOff);

	// TODO: We need to use .qrc files to use icons in WhatsThis bubbles. See Qt
	// Resource System in Qt documentation - ORCAN
	//Q3MimeSourceFactory::defaultFactory()->setPixmap(QString("fileopen"), *openIcon );
	fileOpen->setWhatsThis(tr(presetOpenText));
	onOff->setWhatsThis(tr(presetBypassText));
	fileSave->setWhatsThis(tr(presetSaveText));

    //mw = new QWidget(this);
    //setCentralWidget(mw);
    // p3.4.43
    view = new QScrollArea;
    view->setWidgetResizable(true);
    setCentralWidget(view);
    //view->setVScrollBarMode(QScrollView::AlwaysOff);

    mw = new QWidget;
    mw->setObjectName("PluginGuiBase");
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(2);

    mw->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    uint32_t n = plugin->getParameterCount();
    params = new GuiParam[n];

    //int style       = Slider::BgTrough | Slider::BgSlot;
    QFontMetrics fm = fontMetrics();
    int h = fm.height() + 4;

    for (uint32_t i = 0; i < n; i++)
    {
        QLabel* label = 0;
        ParameterPort* paramPort = plugin->getParameter(i);

//        double lower = paramPort->min;
//        double upper = paramPort->max;
//        double dlower = lower;
//        double dupper = upper;
//        double val = paramPort->value;
//        double dval = val;

        params[i].hints = paramPort->hints;

        if (paramPort->hints & PARAMETER_IS_TOGGLED)
        {
            CheckBox* cb = new CheckBox(mw, i, "param");
            cb->setId(i);
            cb->setText(QString(plugin->getParameterName(i)));
            cb->setChecked(paramPort->value > 0.5);
            cb->setFixedHeight(h);

            params[i].type = GuiParam::GUI_SWITCH;
            params[i].actuator = cb;
        }
        else
        {
            label = new QLabel(QString(plugin->getParameterName(i)), 0);
            params[i].type = GuiParam::GUI_SLIDER;
            params[i].label = new DoubleLabel(paramPort->value, paramPort->ranges.min, paramPort->ranges.max, this); // NOTE - parent was 0
            params[i].label->setFrame(true);
            params[i].label->setPrecision(2);
            params[i].label->setId(i);

            //params[i].label->setContentsMargins(2, 2, 2, 2);
            //params[i].label->setFixedHeight(h);

            Slider* s = new Slider(0, "param", Qt::Horizontal,
                                   Slider::None); //, style);

            s->setCursorHoming(true);
            s->setId(i);
            //s->setFixedHeight(h);
            //s->setRange(dlower, dupper); FIXME ???

            if (paramPort->hints & PARAMETER_IS_INTEGER)
                s->setStep(1.0);

            s->setValue(paramPort->value);

            params[i].actuator = s;
            params[i].label->setSlider((Slider*) params[i].actuator);
        }

        //params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
        params[i].actuator->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
        if (params[i].type == GuiParam::GUI_SLIDER)
        {
            //label->setFixedHeight(20);
            //label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
            label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            //params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
            params[i].label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
            grid->addWidget(label, i, 0);
            grid->addWidget(params[i].label, i, 1);
            grid->addWidget(params[i].actuator, i, 2);
        }
        else if (params[i].type == GuiParam::GUI_SWITCH)
        {
            //grid->addMultiCellWidget(params[i].actuator, i, i, 0, 2);
            grid->addWidget(params[i].actuator, i, 0, 1, 3);
        }
        if (params[i].type == GuiParam::GUI_SLIDER)
        {
            connect(params[i].actuator, SIGNAL(sliderMoved(double, int)), SLOT(sliderChanged(double, int)));
            connect(params[i].label, SIGNAL(valueChanged(double, int)), SLOT(labelChanged(double, int)));
            connect(params[i].actuator, SIGNAL(sliderPressed(int)), SLOT(ctrlPressed(int)));
            connect(params[i].actuator, SIGNAL(sliderReleased(int)), SLOT(ctrlReleased(int)));
            connect(params[i].actuator, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
        }
        else if (params[i].type == GuiParam::GUI_SWITCH)
        {
            connect(params[i].actuator, SIGNAL(checkboxPressed(int)), SLOT(ctrlPressed(int)));
            connect(params[i].actuator, SIGNAL(checkboxReleased(int)), SLOT(ctrlReleased(int)));
            connect(params[i].actuator, SIGNAL(checkboxRightClicked(const QPoint &, int)), SLOT(ctrlRightClicked(const QPoint &, int)));
        }
    }

    // p3.3.43
    resize(280, height());

    grid->setColumnStretch(2, 10);
    mw->setLayout(grid);
    view->setWidget(mw);

	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::~PluginGui()
{
	if (gw)
		delete[] gw;
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
//   ctrlPressed
//---------------------------------------------------------

void PluginGui::ctrlPressed(int param)/*{{{*/
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
        if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
			val = pow(10.0, val / 20.0);
        else if (params[param].hints & PARAMETER_IS_INTEGER)
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
}/*}}}*/

//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void PluginGui::ctrlReleased(int param)/*{{{*/
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
        if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
			val = pow(10.0, val / 20.0);
        else if (params[param].hints & PARAMETER_IS_INTEGER)
			val = rint(val);
		track->stopAutoRecord(id, val);
	}
	//else if(params[param].type == GuiParam::GUI_SWITCH)
	//{
	//double val = (double)((CheckBox*)params[param].actuator)->isChecked();
	// No concept of 'untouching' a checkbox. Remain 'touched' until stop.
	//plugin->track()->stopAutoRecord(genACnum(plugin->id(), param), val);
	//}
}/*}}}*/

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void PluginGui::ctrlRightClicked(const QPoint &p, int param)/*{{{*/
{
	int id = plugin->id();
	if (id != -1)
		//song->execAutomationCtlPopup((AudioTrack*)plugin->track(), p, genACnum(id, param));
		song->execAutomationCtlPopup(plugin->track(), p, genACnum(id, param));
}/*}}}*/

//---------------------------------------------------------
//   sliderChanged
//---------------------------------------------------------

void PluginGui::sliderChanged(double val, int param)/*{{{*/
{
	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		plugin->enableController(param, false);

    if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
		val = pow(10.0, val / 20.0);
    else if (params[param].hints & PARAMETER_IS_INTEGER)
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
}/*}}}*/

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
    if (params[param].hints & PARAMETER_IS_LOGARITHMIC)
		dval = fast_log10(val) * 20.0;
    else if (params[param].hints & PARAMETER_IS_INTEGER)
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
//   load
//---------------------------------------------------------

void PluginGui::load()/*{{{*/
{
	QString s("presets/plugins/");
	//s += plugin->plugin()->label();
    s += plugin->label();
	s += "/";

	QString fn = getOpenFileName(s, preset_file_pattern,
			this, tr("OOMidi: load preset"), 0);
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
						QMessageBox::critical(this, QString("OOMidi"),
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
}/*}}}*/

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void PluginGui::save()/*{{{*/
{
	QString s("presets/plugins/");
	//s += plugin->plugin()->label();
    s += plugin->label();
	s += "/";

	//QString fn = getSaveFileName(s, preset_file_pattern, this,
	QString fn = getSaveFileName(s, preset_file_save_pattern, this,
			tr("OOMidi: save preset"));
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
}/*}}}*/

//---------------------------------------------------------
//   bypassToggled
//---------------------------------------------------------

void PluginGui::bypassToggled(bool val)/*{{{*/
{
    plugin->setActive(val);
	song->update(SC_ROUTE);
}/*}}}*/

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void PluginGui::setOn(bool val)/*{{{*/
{
	onOff->blockSignals(true);
	onOff->setChecked(val);
	onOff->blockSignals(false);
}/*}}}*/

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PluginGui::updateValues()/*{{{*/
{
	if (params)
    {
        for (uint32_t i = 0; i < plugin->getParameterCount(); ++i)
		{
			GuiParam* gp = &params[i];
			if (gp->type == GuiParam::GUI_SLIDER)
			{
                double lv = plugin->getParameterValue(i);
				double sv = lv;
                if (params[i].hints & PARAMETER_IS_LOGARITHMIC)
					sv = fast_log10(lv) * 20.0;
                else if (params[i].hints & PARAMETER_IS_INTEGER)
				{
					sv = rint(lv);
					lv = sv;
				}
				gp->label->setValue(lv);
				((Slider*) (gp->actuator))->setValue(sv);
			}
			else if (gp->type == GuiParam::GUI_SWITCH)
			{
                ((CheckBox*) (gp->actuator))->setChecked(int(plugin->getParameterValue(i)));
			}
		}
	}
	else if (gw)
	{
		for (int i = 0; i < nobj; ++i)
		{
			QWidget* widget = gw[i].widget;
			int type = gw[i].type;
			int param = gw[i].param;
            double val = plugin->getParameterValue(param);
			switch (type)
			{
				case GuiWidgets::SLIDER:
					((Slider*) widget)->setValue(val);
					break;
				case GuiWidgets::DOUBLE_LABEL:
					((DoubleLabel*) widget)->setValue(val);
					break;
				case GuiWidgets::QCHECKBOX:
					((QCheckBox*) widget)->setChecked(int(val));
					break;
				case GuiWidgets::QCOMBOBOX:
					((QComboBox*) widget)->setCurrentIndex(int(val));
					break;
			}
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void PluginGui::updateControls()/*{{{*/
{
    if (!automation || !plugin->track() || plugin->id() == -1)
		return;
	AutomationType at = plugin->track()->automationType();
	if (at == AUTO_OFF)
		return;
	if (params)
	{
        for (uint32_t i = 0; i < plugin->getParameterCount(); ++i)
		{
			GuiParam* gp = &params[i];
			if (gp->type == GuiParam::GUI_SLIDER)
			{
				if (plugin->controllerEnabled(i) && plugin->controllerEnabled2(i))
				{
					double lv = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), i));
					double sv = lv;
                    if (params[i].hints & PARAMETER_IS_LOGARITHMIC)
						sv = fast_log10(lv) * 20.0;
                    else if (params[i].hints & PARAMETER_IS_INTEGER)
					{
						sv = rint(lv);
						lv = sv;
					}
					if (((Slider*) (gp->actuator))->value() != sv)
					{
						//printf("PluginGui::updateControls slider\n");

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
	else if (gw)
	{
		for (int i = 0; i < nobj; ++i)
		{
			QWidget* widget = gw[i].widget;
			int type = gw[i].type;
			int param = gw[i].param;
			switch (type)
			{
				case GuiWidgets::SLIDER:
					if (plugin->controllerEnabled(param) && plugin->controllerEnabled2(param))
					{
						double v = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
						if (((Slider*) widget)->value() != v)
						{
							//printf("PluginGui::updateControls slider\n");

							((Slider*) widget)->blockSignals(true);
							((Slider*) widget)->setValue(v);
							((Slider*) widget)->blockSignals(false);
						}
					}
					break;
				case GuiWidgets::DOUBLE_LABEL:
					if (plugin->controllerEnabled(param) && plugin->controllerEnabled2(param))
					{
						double v = plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
						if (((DoubleLabel*) widget)->value() != v)
						{
							//printf("PluginGui::updateControls label\n");

							((DoubleLabel*) widget)->blockSignals(true);
							((DoubleLabel*) widget)->setValue(v);
							((DoubleLabel*) widget)->blockSignals(false);
						}
					}
					break;
				case GuiWidgets::QCHECKBOX:
					if (plugin->controllerEnabled(param) && plugin->controllerEnabled2(param))
					{
						bool b = (bool) plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
						if (((QCheckBox*) widget)->isChecked() != b)
						{
							//printf("PluginGui::updateControls checkbox\n");

							((QCheckBox*) widget)->blockSignals(true);
							((QCheckBox*) widget)->setChecked(b);
							((QCheckBox*) widget)->blockSignals(false);
						}
					}
					break;
				case GuiWidgets::QCOMBOBOX:
					if (plugin->controllerEnabled(param) && plugin->controllerEnabled2(param))
					{
						int n = (int) plugin->track()->pluginCtrlVal(genACnum(plugin->id(), param));
						if (((QComboBox*) widget)->currentIndex() != n)
						{
							//printf("PluginGui::updateControls combobox\n");

							((QComboBox*) widget)->blockSignals(true);
							((QComboBox*) widget)->setCurrentIndex(n);
							((QComboBox*) widget)->blockSignals(false);
						}
					}
					break;
			}
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   guiParamChanged
//---------------------------------------------------------

void PluginGui::guiParamChanged(int idx)/*{{{*/
{
	QWidget* w = gw[idx].widget;
	int param = gw[idx].param;
	int type = gw[idx].type;

	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		plugin->enableController(param, false);

	double val = 0.0;
	switch (type)
	{
		case GuiWidgets::SLIDER:
			val = ((Slider*) w)->value();
			break;
		case GuiWidgets::DOUBLE_LABEL:
			val = ((DoubleLabel*) w)->value();
			break;
		case GuiWidgets::QCHECKBOX:
			val = double(((QCheckBox*) w)->isChecked());
			break;
		case GuiWidgets::QCOMBOBOX:
			val = double(((QComboBox*) w)->currentIndex());
			break;
	}

	for (int i = 0; i < nobj; ++i)
	{
		QWidget* widget = gw[i].widget;
		if (widget == w || param != gw[i].param)
			continue;
		int type = gw[i].type;
		switch (type)
		{
			case GuiWidgets::SLIDER:
				((Slider*) widget)->setValue(val);
				break;
			case GuiWidgets::DOUBLE_LABEL:
				((DoubleLabel*) widget)->setValue(val);
				break;
			case GuiWidgets::QCHECKBOX:
				((QCheckBox*) widget)->setChecked(int(val));
				break;
			case GuiWidgets::QCOMBOBOX:
				((QComboBox*) widget)->setCurrentIndex(int(val));
				break;
		}
	}

	int id = plugin->id();
	if (track && id != -1)
	{
		id = genACnum(id, param);

		// p3.3.43
		//audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);

		//if(track)
		//{
		// p3.3.43
		audio->msgSetPluginCtrlVal(track, id, val);

		switch (type)
		{
			case GuiWidgets::DOUBLE_LABEL:
			case GuiWidgets::QCHECKBOX:
				track->startAutoRecord(id, val);
				break;
			default:
				track->recordAutomation(id, val);
				break;
		}
		//}
	}
    plugin->setParameterValue(param, val);
}/*}}}*/

//---------------------------------------------------------
//   guiParamPressed
//---------------------------------------------------------

void PluginGui::guiParamPressed(int idx)/*{{{*/
{
	int param = gw[idx].param;

	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	if (at != AUTO_OFF)
		plugin->enableController(param, false);

	int id = plugin->id();
	if (!track || id == -1)
		return;

	id = genACnum(id, param);

	// NOTE: For this to be of any use, the freeverb gui 2142.ui
	//  would have to be used, and changed to use CheckBox and ComboBox
	//  instead of QCheckBox and QComboBox, since both of those would
	//  need customization (Ex. QCheckBox doesn't check on click).
	/*
	switch(type) {
		  case GuiWidgets::QCHECKBOX:
				  double val = (double)((CheckBox*)w)->isChecked();
				  track->startAutoRecord(id, val);
				break;
		  case GuiWidgets::QCOMBOBOX:
				  double val = (double)((ComboBox*)w)->currentIndex();
				  track->startAutoRecord(id, val);
				break;
		  }
	 */
}/*}}}*/

//---------------------------------------------------------
//   guiParamReleased
//---------------------------------------------------------

void PluginGui::guiParamReleased(int idx)/*{{{*/
{
	int param = gw[idx].param;
	int type = gw[idx].type;

	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	// Special for switch - don't enable controller until transport stopped.
	if (at != AUTO_WRITE && (type != GuiWidgets::QCHECKBOX
			|| !audio->isPlaying()
			|| at != AUTO_TOUCH))
		plugin->enableController(param, true);

	int id = plugin->id();

	if (!track || id == -1)
		return;

	id = genACnum(id, param);

	// NOTE: For this to be of any use, the freeverb gui 2142.ui
	//  would have to be used, and changed to use CheckBox and ComboBox
	//  instead of QCheckBox and QComboBox, since both of those would
	//  need customization (Ex. QCheckBox doesn't check on click).
	/*
	switch(type) {
		  case GuiWidgets::QCHECKBOX:
				  double val = (double)((CheckBox*)w)->isChecked();
				  track->stopAutoRecord(id, param);
				break;
		  case GuiWidgets::QCOMBOBOX:
				  double val = (double)((ComboBox*)w)->currentIndex();
				  track->stopAutoRecord(id, param);
				break;
		  }
	 */
}/*}}}*/

//---------------------------------------------------------
//   guiSliderPressed
//---------------------------------------------------------

void PluginGui::guiSliderPressed(int idx)/*{{{*/
{
	int param = gw[idx].param;
	QWidget *w = gw[idx].widget;

	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	int id = plugin->id();

	if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
		plugin->enableController(param, false);

	if (!track || id == -1)
		return;

	id = genACnum(id, param);

	double val = ((Slider*) w)->value();
    plugin->setParameterValue(param, val);

	//audio->msgSetPluginCtrlVal(((PluginI*)plugin), id, val);
	// p3.3.43
	audio->msgSetPluginCtrlVal(track, id, val);

	track->startAutoRecord(id, val);

	// Needed so that paging a slider updates a label or other buddy control.
	for (int i = 0; i < nobj; ++i)
	{
		QWidget* widget = gw[i].widget;
		if (widget == w || param != gw[i].param)
			continue;
		int type = gw[i].type;
		switch (type)
		{
			case GuiWidgets::SLIDER:
				((Slider*) widget)->setValue(val);
				break;
			case GuiWidgets::DOUBLE_LABEL:
				((DoubleLabel*) widget)->setValue(val);
				break;
			case GuiWidgets::QCHECKBOX:
				((QCheckBox*) widget)->setChecked(int(val));
				break;
			case GuiWidgets::QCOMBOBOX:
				((QComboBox*) widget)->setCurrentIndex(int(val));
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   guiSliderReleased
//---------------------------------------------------------

void PluginGui::guiSliderReleased(int idx)/*{{{*/
{
	int param = gw[idx].param;
	QWidget *w = gw[idx].widget;

	AutomationType at = AUTO_OFF;
	AudioTrack* track = plugin->track();
	if (track)
		at = track->automationType();

	if (at != AUTO_WRITE || (!audio->isPlaying() && at == AUTO_TOUCH))
		plugin->enableController(param, true);

	int id = plugin->id();

	if (!track || id == -1)
		return;

	id = genACnum(id, param);

	double val = ((Slider*) w)->value();
	track->stopAutoRecord(id, val);
}/*}}}*/

//---------------------------------------------------------
//   guiSliderRightClicked
//---------------------------------------------------------

void PluginGui::guiSliderRightClicked(const QPoint &p, int idx)/*{{{*/
{
	int param = gw[idx].param;
	int id = plugin->id();
	if (id != -1)
		//song->execAutomationCtlPopup((AudioTrack*)plugin->track(), p, genACnum(id, param));
		song->execAutomationCtlPopup(plugin->track(), p, genACnum(id, param));
}/*}}}*/
