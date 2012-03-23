//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: mstrip.cpp,v 1.9.2.13 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <fastlog.h>

#include <QLayout>
#include <QAction>
#include <QApplication>
//#include <QDialog>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
//#include <QPopupMenu>
#include <QCursor>
#include <QGridLayout>

#include <math.h>
#include "app.h"
#include "midi.h"
#include "midictrl.h"
#include "mstrip.h"
#include "midiport.h"
#include "globals.h"
#include "audio.h"
#include "song.h"
#include "slider.h"
#include "knob.h"
#include "combobox.h"
#include "meter.h"
#include "track.h"
#include "doublelabel.h"
#include "EffectRack.h"
#include "node.h"
#include "AudioMixer.h"
#include "icons.h"
#include "gconfig.h"
#include "ttoolbutton.h"
#include "utils.h"
#include "popupmenu.h"
#include "midimonitor.h"

enum
{
	KNOB_PAN, KNOB_VAR_SEND, KNOB_REV_SEND, KNOB_CHO_SEND
};

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MidiTrack* t)
: Strip(parent, t)
{
	inHeartBeat = true;

	// Clear so the meters don't start off by showing stale values.
	t->setActivity(0);
	t->setLastActivity(0);

	volume = CTRL_VAL_UNKNOWN;
	pan = CTRL_VAL_UNKNOWN;
	variSend = CTRL_VAL_UNKNOWN;
	chorusSend = CTRL_VAL_UNKNOWN;
	reverbSend = CTRL_VAL_UNKNOWN;

	addKnob(KNOB_VAR_SEND, tr("VariationSend"), tr("Variation"), SLOT(setVariSend(double)), false);
	addKnob(KNOB_REV_SEND, tr("ReverbSend"), tr("Reverb"), SLOT(setReverbSend(double)), false);
	addKnob(KNOB_CHO_SEND, tr("ChorusSend"), tr("Chorus"), SLOT(setChorusSend(double)), false);

	//---------------------------------------------------
	//    slider, label, meter
	//---------------------------------------------------

	MidiPort* mp = &midiPorts[t->outPort()];
	MidiController* mc = mp->midiController(CTRL_VOLUME);
	int chan = t->outChannel();
	int mn = mc->minVal();
	int mx = mc->maxVal();
	bool usePixmap = false;
	QColor sliderBgColor = g_trackColorListSelected.value(t->type());/*{{{*/
    switch(vuColorStrip)
    {
        case 0:
            sliderBgColor = g_trackColorListSelected.value(t->type());
        break;
        case 1:
            //if(width() != m_width)
            //    m_scaledPixmap_w = m_pixmap_w->scaled(width(), 1, Qt::IgnoreAspectRatio);
            //m_width = width();
            //myPen.setBrush(m_scaledPixmap_w);
            //myPen.setBrush(m_trackColor);
            sliderBgColor = QColor(0,0,0);
			usePixmap = true;
        break;
        case 2:
            sliderBgColor = QColor(0,166,172);
            //myPen.setBrush(QColor(0,166,172));//solid blue
        break;
        case 3:
            sliderBgColor = QColor(131,131,131);
            //myPen.setBrush(QColor(131,131,131));//solid grey
        break;
        default:
            sliderBgColor = g_trackColorListSelected.value(t->type());
            //myPen.setBrush(m_trackColor);
        break;
    }/*}}}*/
	m_sliderBg = sliderBgColor;

	slider = new Slider(this, "vol", Qt::Vertical, Slider::None, Slider::BgSlot, sliderBgColor, usePixmap);
	slider->setCursorHoming(true);
	slider->setRange(double(mn), double(mx), 1.0);
	slider->setFixedWidth(20);
	slider->setFont(config.fonts[1]);
	slider->setId(CTRL_VOLUME);

	meter[0] = new Meter(this, Track::MIDI, Meter::LinMeter, Qt::Vertical);
	meter[0]->setRange(0, 127.0);
	meter[0]->setFixedWidth(15);
	connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));

	sliderGrid = new QGridLayout();
	sliderGrid->setRowStretch(0, 100);
	sliderGrid->addWidget(slider, 0, 0, Qt::AlignRight);
	sliderGrid->addWidget(meter[0], 0, 1, Qt::AlignLeft);
	m_vuBox->addLayout(sliderGrid);

	sl = new DoubleLabel(0.0, -98.0, 0.0, this);
	sl->setFont(config.fonts[1]);
	sl->setBackgroundRole(QPalette::Mid);
	sl->setSpecialText(tr("off"));
	sl->setSuffix(tr("dB"));
	sl->setToolTip(tr("double click on/off"));
	sl->setFrame(true);
	sl->setPrecision(0);
	sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
	// Set the label's slider 'buddy'.
	sl->setSlider(slider);

	double dlv;
	int v = mp->hwCtrlState(chan, CTRL_VOLUME);
	if (v == CTRL_VAL_UNKNOWN)
	{
		int lastv = mp->lastValidHWCtrlState(chan, CTRL_VOLUME);
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			if (mc->initVal() == CTRL_VAL_UNKNOWN)
				v = 0;
			else
				v = mc->initVal();
		}
		else
			v = lastv - mc->bias();
		dlv = sl->off() - 1.0;
	}
	else
	{
		if (v == 0)
			dlv = sl->minValue() - 0.5 * (sl->minValue() - sl->off());
		else
		{
			dlv = -fast_log10(float(127 * 127) / float(v * v))*20.0;
			if (dlv > sl->maxValue())
				dlv = sl->maxValue();
		}
		// Auto bias...
		v -= mc->bias();
	}
	slider->setValue(double(v));
	sl->setValue(dlv);


	//      connect(sl, SIGNAL(valueChanged(double,int)), slider, SLOT(setValue(double)));
	//      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
	connect(slider, SIGNAL(sliderMoved(double, int)), SLOT(setVolume(double)));
	connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
	connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
	connect(sl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked(int)));

	m_panBox->addWidget(sl);

	//---------------------------------------------------
	//    pan, balance
	//---------------------------------------------------

	addKnob(KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(setPan(double)), true);

	updateControls();

	//---------------------------------------------------
	//    mute, solo
	//    or
	//    m_btnRecord, mixdownfile
	//---------------------------------------------------

	m_btnRecord->setCheckable(true);
	m_btnRecord->setToolTip(tr("record"));
	m_btnRecord->setObjectName("btnRecord");
	m_btnRecord->setChecked(track->recordFlag());
	connect(m_btnRecord, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));

	m_btnMute->setCheckable(true);
	m_btnMute->setToolTip(tr("mute"));
	m_btnMute->setObjectName("btnMute");
	m_btnMute->setChecked(track->mute());
	connect(m_btnMute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

	/*if ((bool)t->internalSolo())
	{
		m_btnSolo->setIcon(*soloIconSet2);
		m_btnSolo->setIconSize(soloIconOn->size());
		useSoloIconSet2 = true;
	}
	else
	{
		m_btnSolo->setIcon(*soloIconSet1);
		m_btnSolo->setIconSize(soloblksqIconOn->size());
		useSoloIconSet2 = false;
	}*/

	m_btnSolo->setToolTip(tr("solo mode"));
	m_btnSolo->setCheckable(true);
	m_btnSolo->setObjectName("btnSolo");
	m_btnSolo->setChecked(t->solo());
	connect(m_btnSolo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));

	m_btnPower->setCheckable(true);
	m_btnPower->setToolTip(tr("off"));
	m_btnPower->setObjectName("btnExit");
	m_btnPower->setChecked(t->off());
	connect(m_btnPower, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

	//---------------------------------------------------
	//    routing
	//---------------------------------------------------

	m_btnIRoute->setCheckable(false);
	m_btnIRoute->setToolTip(tr("input routing"));
	m_btnIRoute->setObjectName("btnIns");
	connect(m_btnIRoute, SIGNAL(pressed()), SLOT(iRoutePressed()));
	
	m_btnORoute->setCheckable(false);
	m_btnORoute->setObjectName("btnOuts");
	// TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
	//m_btnORoute->setEnabled(false);
	//m_btnORoute->setToolTip(tr("output routing"));
	m_btnIRoute->setToolTip("");
	m_btnORoute->setIcon(QIcon(*mixer_blank_OffIcon));
	//connect(m_btnORoute, SIGNAL(pressed()), SLOT(oRoutePressed()));

	//---------------------------------------------------
	//    automation mode
	//---------------------------------------------------

	autoType = new ComboBox(this);
	autoType->setFont(config.fonts[1]);
	//autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));

    autoType->insertItem(tr("Off"), AUTO_OFF);
	autoType->insertItem(tr("Read"), AUTO_READ);
	autoType->insertItem(tr("Touch"), AUTO_TOUCH);
	autoType->insertItem(tr("Write"), AUTO_WRITE);
    
    if (track && track->wantsAutomation())
    {
        AudioTrack* atrack = ((MidiTrack*)track)->getAutomationTrack();
        if (atrack)
            autoType->setCurrentItem(atrack->automationType());
        else
            autoType->setCurrentItem(AUTO_OFF);
    }
    else
    {
        autoType->setCurrentItem(AUTO_OFF);
        autoType->setText("");
        autoType->setEnabled(false);
    }

	QSizePolicy autoSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	autoSizePolicy.setHorizontalStretch(1);
	autoSizePolicy.setVerticalStretch(0);
	autoSizePolicy.setHeightForWidth(autoType->sizePolicy().hasHeightForWidth());
	autoType->setSizePolicy(autoSizePolicy);
    autoType->setAlignment(Qt::AlignCenter);
    autoType->setMaximumSize(QSize(65,20));
    autoType->setToolTip(tr("automation type"));
	connect(autoType, SIGNAL(activated(int, int)), SLOT(setAutomationType(int, int)));
	m_autoBox->addWidget(autoType);

	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
	inHeartBeat = false;
}


MidiStrip::~MidiStrip()
{
}


//---------------------------------------------------------
//   addKnob
//---------------------------------------------------------

void MidiStrip::addKnob(int idx, const QString& tt, const QString& label,
		const char* slot, bool enabled)
{
	int ctl = CTRL_PANPOT, mn, mx, v;
	int chan = ((MidiTrack*) track)->outChannel();
	QString img;
	img = QString(":images/knob_audio_new.png");
	/*(Track::TrackType type_tmp = ((MidiTrack*) track)->type();
	switch (type_tmp)
	{
		case Track::MIDI:
		case Track::DRUM:
			img = QString(":images/knob_audio_new.png");
		break;
		case Track::WAVE:
			img = QString(":images/knob_input_new.png");
		break;
		case Track::AUDIO_OUTPUT:
			img = QString(":images/knob_output_new.png");
		break;
		case Track::AUDIO_INPUT:
			img = QString(":images/knob_midi_new.png");
		break;
		case Track::AUDIO_BUSS:
			img = QString(":images/knob_buss_new.png");
		break;
		case Track::AUDIO_AUX:
			img = QString(":images/knob_aux_new.png");
		break;
		case Track::AUDIO_SOFTSYNTH:
			img = QString(":images/knob_audio_new.png");
		break;
		default:
			img = QString(":images/knob_aux.png");
		break;
	}*/
	switch (idx)
	{
		case KNOB_VAR_SEND:
			ctl = CTRL_VARIATION_SEND;
			img = QString(":images/knob_aux.png");
			break;
		case KNOB_REV_SEND:
			ctl = CTRL_REVERB_SEND;
			img = QString(":images/knob_aux.png");
			break;
		case KNOB_CHO_SEND:
			ctl = CTRL_CHORUS_SEND;
			img = QString(":images/knob_aux.png");
			break;
	}
	MidiPort* mp = &midiPorts[((MidiTrack*) track)->outPort()];
	MidiController* mc = mp->midiController(ctl);
	mn = mc->minVal();
	mx = mc->maxVal();

	Knob* knob = new Knob(this);
	knob->setRange(double(mn), double(mx), 1.0);
	knob->setId(ctl);
	knob->setKnobImage(img);

	controller[idx].knob = knob;
	knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
	knob->setBackgroundRole(QPalette::Mid);
	knob->setToolTip(tt);
	knob->setEnabled(enabled);

	DoubleLabel* dl = new DoubleLabel(0.0, double(mn), double(mx), this);
	dl->setId(idx);
	dl->setSpecialText(tr("off"));
	dl->setToolTip(tr("double click on/off"));
	controller[idx].dl = dl;
	dl->setFont(config.fonts[1]);
	dl->setBackgroundRole(QPalette::Mid);
	dl->setFrame(true);
	dl->setPrecision(0);
	dl->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	dl->setAlignment(Qt::AlignCenter);
	dl->setEnabled(enabled);

	double dlv;
	v = mp->hwCtrlState(chan, ctl);
	if (v == CTRL_VAL_UNKNOWN)
	{
		int lastv = mp->lastValidHWCtrlState(chan, ctl);
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			if (mc->initVal() == CTRL_VAL_UNKNOWN)
				v = 0;
			else
				v = mc->initVal();
		}
		else
			v = lastv - mc->bias();
		dlv = dl->off() - 1.0;
	}
	else
	{
		// Auto bias...
		v -= mc->bias();
		dlv = double(v);
	}

	knob->setValue(double(v));
	dl->setValue(dlv);

	QLabel* lb = new QLabel(label, this);
	controller[idx].lb = lb;
	lb->setFont(config.fonts[1]);
	lb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	lb->setAlignment(Qt::AlignCenter);
	lb->setEnabled(enabled);

	QHBoxLayout *container = new QHBoxLayout();
	container->setContentsMargins(0, 0, 0, 0);
	container->setAlignment(Qt::AlignHCenter|Qt::AlignCenter);
	container->setSpacing(0);
	QVBoxLayout *labelBox = new QVBoxLayout();
	labelBox->setContentsMargins(0, 0, 0, 0);
	labelBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	labelBox->setSpacing(0);
	labelBox->addWidget(lb);
	if(idx == KNOB_PAN)
	{ //Pan
		labelBox->addWidget(dl);
		container->addLayout(labelBox);
		container->addWidget(knob);
		m_panBox->addLayout(container);
	}
	else
	{ //Controller
		container->addItem(new QSpacerItem(19, 0));
		container->addWidget(dl);
		container->addWidget(knob);
		labelBox->addLayout(container);
		m_auxBox->addLayout(labelBox);
	}

	connect(knob, SIGNAL(sliderMoved(double, int)), slot);
	connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
	connect(dl, SIGNAL(valueChanged(double, int)), slot);
	connect(dl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked(int)));
}

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void MidiStrip::updateOffState()
{
	bool val = !track->off();
	slider->setEnabled(val);
	sl->setEnabled(val);
	controller[KNOB_PAN].knob->setEnabled(val);
	controller[KNOB_PAN].dl->setEnabled(val);
	label->setEnabled(val);

	if (hasRecord)
		m_btnRecord->setEnabled(val);
	//if (solo)
		m_btnSolo->setEnabled(val);
	//if (mute)
		m_btnMute->setEnabled(val);
	if (autoType)
    {
		autoType->setEnabled(val);
        autoType->setVisible(track->wantsAutomation());
    }
	if (hasIRoute)
		m_btnIRoute->setEnabled(val);
	// TODO: Disabled for now.
	if (hasORoute)
	      m_btnORoute->setEnabled(val);
	m_btnPower->blockSignals(true);
	m_btnPower->setChecked(track->off());
	m_btnPower->blockSignals(false);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(int val)
{
	if (m_btnMute && (val & SC_MUTE))
	{ // mute && off
		m_btnMute->blockSignals(true);
		m_btnMute->setChecked(track->isMute());
		updateOffState();
		m_btnMute->blockSignals(false);
	}
	if (m_btnSolo && (val & SC_SOLO))
	{
		/*if ((bool)track->internalSolo())
		{
			if (!useSoloIconSet2)
			{
				m_btnSolo->setIcon(*soloIconSet2);
				m_btnSolo->setIconSize(soloIconOn->size());
				useSoloIconSet2 = true;
			}
		}
		else if (useSoloIconSet2)
		{
			m_btnSolo->setIcon(*soloIconSet1);
			m_btnSolo->setIconSize(soloblksqIconOn->size());
			useSoloIconSet2 = false;
		}*/
		m_btnSolo->blockSignals(true);
		m_btnSolo->setChecked(track->solo());
		m_btnSolo->blockSignals(false);
	}

	if (val & SC_RECFLAG)
		setRecordFlag(track->recordFlag());
	if (val & SC_TRACK_MODIFIED)
	{
		setLabelText();
		// Added by Tim. p3.3.9
		setLabelFont();

	}
	// Added by Tim. p3.3.9

	// Catch when label font changes.
	if (val & SC_CONFIG)
	{
		// Set the strip label's font.
		//label->setFont(config.fonts[1]);
		setLabelFont();
	}

	// p3.3.47 Update the routing popup menu if anything relevant changes.
	//if(gRoutingPopupMenuMaster == this && track && (val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG)))
	if (val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG)) // p3.3.50
		// Use this handy shared routine.
		//oom->updateRouteMenus(track);
		oom->updateRouteMenus(track, this); // p3.3.50
}

//---------------------------------------------------------
//   controlRightClicked
//---------------------------------------------------------

void MidiStrip::controlRightClicked(const QPoint &p, int id)
{
	song->execMidiAutomationCtlPopup((MidiTrack*) track, 0, p, id);
}

//---------------------------------------------------------
//   labelDoubleClicked
//---------------------------------------------------------

void MidiStrip::labelDoubleClicked(int idx)
{
	//int mn, mx, v;
	//int num = CTRL_VOLUME;
	int num;
	switch (idx)
	{
		case KNOB_PAN:
			num = CTRL_PANPOT;
			break;
		case KNOB_VAR_SEND:
			num = CTRL_VARIATION_SEND;
			break;
		case KNOB_REV_SEND:
			num = CTRL_REVERB_SEND;
			break;
		case KNOB_CHO_SEND:
			num = CTRL_CHORUS_SEND;
			break;
			//case -1:
		default:
			num = CTRL_VOLUME;
			break;
	}
	int outport = ((MidiTrack*) track)->outPort();
	int chan = ((MidiTrack*) track)->outChannel();
	MidiPort* mp = &midiPorts[outport];
	MidiController* mc = mp->midiController(num);

	int lastv = mp->lastValidHWCtrlState(chan, num);
	int curv = mp->hwCtrlState(chan, num);

	if (curv == CTRL_VAL_UNKNOWN)
	{
		// If no value has ever been set yet, use the current knob value
		//  (or the controller's initial value?) to 'turn on' the controller.
		if (lastv == CTRL_VAL_UNKNOWN)
		{
			//int kiv = _ctrl->initVal());
			int kiv;
			if (idx == -1)
				kiv = lrint(slider->value());
			else
				kiv = lrint(controller[idx].knob->value());
			if (kiv < mc->minVal())
				kiv = mc->minVal();
			if (kiv > mc->maxVal())
				kiv = mc->maxVal();
			kiv += mc->bias();

			//MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, num, kiv);
			MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, num, kiv, track);
			audio->msgPlayMidiEvent(&ev);
		}
		else
		{
			//MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, num, lastv);
			MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, num, lastv, track);
			audio->msgPlayMidiEvent(&ev);
		}
	}
	else
	{
		if (mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
	}
	song->update(SC_MIDI_CONTROLLER);
}


//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void MidiStrip::offToggled(bool val)
{
	track->setOff(val);
	song->update(SC_MUTE);
}

/*
//---------------------------------------------------------
//   routeClicked
//---------------------------------------------------------

void MidiStrip::routeClicked()
	  {
	  }
 */

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiStrip::heartBeat()
{
	inHeartBeat = true;

	int act = track->activity();
	double dact = double(act) * (slider->value() / 127.0);

	if ((int) dact > track->lastActivity())
		track->setLastActivity((int) dact);

	if (meter[0])
		//meter[0]->setVal(int(double(act) * (slider->value() / 127.0)), 0, false);
		meter[0]->setVal(dact, track->lastActivity(), false);

	// Gives reasonable decay with gui update set to 20/sec.
	if (act)
		track->setActivity((int) ((double) act * 0.8));

	Strip::heartBeat();
	updateControls();
	bool usePixmap = false;
	QColor sliderBgColor = g_trackColorListSelected.value(track->type());/*{{{*/
    switch(vuColorStrip)
    {
        case 0:
            sliderBgColor = g_trackColorListSelected.value(track->type());
        break;
        case 1:
            //if(width() != m_width)
            //    m_scaledPixmap_w = m_pixmap_w->scaled(width(), 1, Qt::IgnoreAspectRatio);
            //m_width = width();
            //myPen.setBrush(m_scaledPixmap_w);
            //myPen.setBrush(m_trackColor);
            sliderBgColor = QColor(0,0,0);
			usePixmap = true;
        break;
        case 2:
            sliderBgColor = QColor(0,166,172);
            //myPen.setBrush(QColor(0,166,172));//solid blue
        break;
        case 3:
            sliderBgColor = QColor(131,131,131);
            //myPen.setBrush(QColor(131,131,131));//solid grey
        break;
        default:
            sliderBgColor = g_trackColorListSelected.value(track->type());
            //myPen.setBrush(m_trackColor);
        break;
    }/*}}}*/
	if(sliderBgColor.name() != m_sliderBg.name())
	{//color changed update the slider
		if(slider)
		{
			if(usePixmap)
				slider->setUsePixmap();
			else
				slider->setSliderBackground(sliderBgColor);
		}
		m_sliderBg = sliderBgColor;
	}

	inHeartBeat = false;
}

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void MidiStrip::updateControls()
{
	bool en;
	int channel = ((MidiTrack*) track)->outChannel();
	MidiPort* mp = &midiPorts[((MidiTrack*) track)->outPort()];
	MidiCtrlValListList* mc = mp->controller();
	ciMidiCtrlValList icl;

	MidiController* ctrl = mp->midiController(CTRL_VOLUME);
	int nvolume = mp->hwCtrlState(channel, CTRL_VOLUME);
	if (nvolume == CTRL_VAL_UNKNOWN)
	{
		//if(nvolume != volume)
		//{
		// DoubleLabel ignores the value if already set...
		sl->setValue(sl->off() - 1.0);
		//volume = nvolume;
		//}
		volume = CTRL_VAL_UNKNOWN;
		nvolume = mp->lastValidHWCtrlState(channel, CTRL_VOLUME);
		//if(nvolume != volume)
		if (nvolume != CTRL_VAL_UNKNOWN)
		{
			nvolume -= ctrl->bias();
			//slider->blockSignals(true);
			if (double(nvolume) != slider->value())
			{
				//printf("MidiStrip::updateControls setting volume slider\n");

				slider->setValue(double(nvolume));
			}
		}
	}
	else
	{
		int ivol = nvolume;
		nvolume -= ctrl->bias();
		if (nvolume != volume)
		{
			//printf("MidiStrip::updateControls setting volume slider\n");

			//slider->blockSignals(true);
			slider->setValue(double(nvolume));
			//sl->setValue(double(nvolume));
			if (ivol == 0)
			{
				//printf("MidiStrip::updateControls setting volume slider label\n");

				sl->setValue(sl->minValue() - 0.5 * (sl->minValue() - sl->off()));
			}
			else
			{
				double v = -fast_log10(float(127 * 127) / float(ivol * ivol))*20.0;
				if (v > sl->maxValue())
				{
					//printf("MidiStrip::updateControls setting volume slider label\n");

					sl->setValue(sl->maxValue());
				}
				else
				{
					//printf("MidiStrip::updateControls setting volume slider label\n");

					sl->setValue(v);
				}
			}
			//slider->blockSignals(false);
			volume = nvolume;
		}
	}


	KNOB* gcon = &controller[KNOB_PAN];
	ctrl = mp->midiController(CTRL_PANPOT);
	int npan = mp->hwCtrlState(channel, CTRL_PANPOT);
	if (npan == CTRL_VAL_UNKNOWN)
	{
		// DoubleLabel ignores the value if already set...
		//if(npan != pan)
		//{
		gcon->dl->setValue(gcon->dl->off() - 1.0);
		//pan = npan;
		//}
		pan = CTRL_VAL_UNKNOWN;
		npan = mp->lastValidHWCtrlState(channel, CTRL_PANPOT);
		if (npan != CTRL_VAL_UNKNOWN)
		{
			npan -= ctrl->bias();
			if (double(npan) != gcon->knob->value())
			{
				//printf("MidiStrip::updateControls setting pan knob\n");

				gcon->knob->setValue(double(npan));
			}
		}
	}
	else
	{
		npan -= ctrl->bias();
		if (npan != pan)
		{
			//printf("MidiStrip::updateControls setting pan label and knob\n");

			//controller[KNOB_PAN].knob->blockSignals(true);
			gcon->knob->setValue(double(npan));
			gcon->dl->setValue(double(npan));
			//controller[KNOB_PAN].knob->blockSignals(false);
			pan = npan;
		}
	}


	icl = mc->find(channel, CTRL_VARIATION_SEND);
	en = icl != mc->end();

	gcon = &controller[KNOB_VAR_SEND];
	if (gcon->knob->isEnabled() != en)
		gcon->knob->setEnabled(en);
	if (gcon->lb->isEnabled() != en)
		gcon->lb->setEnabled(en);
	if (gcon->dl->isEnabled() != en)
		gcon->dl->setEnabled(en);

	if (en)
	{
		ctrl = mp->midiController(CTRL_VARIATION_SEND);
		int nvariSend = icl->second->hwVal();
		if (nvariSend == CTRL_VAL_UNKNOWN)
		{
			// DoubleLabel ignores the value if already set...
			//if(nvariSend != variSend)
			//{
			gcon->dl->setValue(gcon->dl->off() - 1.0);
			//variSend = nvariSend;
			//}
			variSend = CTRL_VAL_UNKNOWN;
			nvariSend = mp->lastValidHWCtrlState(channel, CTRL_VARIATION_SEND);
			if (nvariSend != CTRL_VAL_UNKNOWN)
			{
				nvariSend -= ctrl->bias();
				if (double(nvariSend) != gcon->knob->value())
				{
					gcon->knob->setValue(double(nvariSend));
				}
			}
		}
		else
		{
			nvariSend -= ctrl->bias();
			if (nvariSend != variSend)
			{
				//controller[KNOB_VAR_SEND].knob->blockSignals(true);
				gcon->knob->setValue(double(nvariSend));
				gcon->dl->setValue(double(nvariSend));
				//controller[KNOB_VAR_SEND].knob->blockSignals(false);
				variSend = nvariSend;
			}
		}
	}

	icl = mc->find(channel, CTRL_REVERB_SEND);
	en = icl != mc->end();

	gcon = &controller[KNOB_REV_SEND];
	if (gcon->knob->isEnabled() != en)
		gcon->knob->setEnabled(en);
	if (gcon->lb->isEnabled() != en)
		gcon->lb->setEnabled(en);
	if (gcon->dl->isEnabled() != en)
		gcon->dl->setEnabled(en);

	if (en)
	{
		ctrl = mp->midiController(CTRL_REVERB_SEND);
		int nreverbSend = icl->second->hwVal();
		if (nreverbSend == CTRL_VAL_UNKNOWN)
		{
			// DoubleLabel ignores the value if already set...
			//if(nreverbSend != reverbSend)
			//{
			gcon->dl->setValue(gcon->dl->off() - 1.0);
			//reverbSend = nreverbSend;
			//}
			reverbSend = CTRL_VAL_UNKNOWN;
			nreverbSend = mp->lastValidHWCtrlState(channel, CTRL_REVERB_SEND);
			if (nreverbSend != CTRL_VAL_UNKNOWN)
			{
				nreverbSend -= ctrl->bias();
				if (double(nreverbSend) != gcon->knob->value())
				{
					gcon->knob->setValue(double(nreverbSend));
				}
			}
		}
		else
		{
			nreverbSend -= ctrl->bias();
			if (nreverbSend != reverbSend)
			{
				//controller[KNOB_REV_SEND].knob->blockSignals(true);
				gcon->knob->setValue(double(nreverbSend));
				gcon->dl->setValue(double(nreverbSend));
				//controller[KNOB_REV_SEND].knob->blockSignals(false);
				reverbSend = nreverbSend;
			}
		}
	}

	icl = mc->find(channel, CTRL_CHORUS_SEND);
	en = icl != mc->end();

	gcon = &controller[KNOB_CHO_SEND];
	if (gcon->knob->isEnabled() != en)
		gcon->knob->setEnabled(en);
	if (gcon->lb->isEnabled() != en)
		gcon->lb->setEnabled(en);
	if (gcon->dl->isEnabled() != en)
		gcon->dl->setEnabled(en);

	if (en)
	{
		ctrl = mp->midiController(CTRL_CHORUS_SEND);
		int nchorusSend = icl->second->hwVal();
		if (nchorusSend == CTRL_VAL_UNKNOWN)
		{
			// DoubleLabel ignores the value if already set...
			//if(nchorusSend != chorusSend)
			//{
			gcon->dl->setValue(gcon->dl->off() - 1.0);
			//chorusSend = nchorusSend;
			//}
			chorusSend = CTRL_VAL_UNKNOWN;
			nchorusSend = mp->lastValidHWCtrlState(channel, CTRL_CHORUS_SEND);
			if (nchorusSend != CTRL_VAL_UNKNOWN)
			{
				nchorusSend -= ctrl->bias();
				if (double(nchorusSend) != gcon->knob->value())
				{
					gcon->knob->setValue(double(nchorusSend));
				}
			}
		}
		else
		{
			nchorusSend -= ctrl->bias();
			if (nchorusSend != chorusSend)
			{
				gcon->knob->setValue(double(nchorusSend));
				gcon->dl->setValue(double(nchorusSend));
				chorusSend = nchorusSend;
			}
		}
	}
}
//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiStrip::ctrlChanged(int num, int val)
{
	if (inHeartBeat)
		return;

	MidiTrack* t = (MidiTrack*) track;
	int port = t->outPort();

	int chan = t->outChannel();
	MidiPort* mp = &midiPorts[port];
	MidiController* mctl = mp->midiController(num);
	if ((val < mctl->minVal()) || (val > mctl->maxVal()))
	{
		if (mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
	}
	else
	{
		val += mctl->bias();

		int tick = song->cpos();

		MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, num, val, track);

		audio->msgPlayMidiEvent(&ev);
		midiMonitor->msgSendMidiOutputEvent((Track*)track, num, val);
	}
	song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
{
	val = sqrt(float(127 * 127) / pow(10.0, -val / 20.0));

	ctrlChanged(CTRL_VOLUME, lrint(val));

}

//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val)
{

	//printf("MidiStrip::setVolume(%g)\n", midiToDb(lrint(val)));
	ctrlChanged(CTRL_VOLUME, lrint(val));
}

//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void MidiStrip::setPan(double val)
{
	//printf("MidiStrip::setPan(%g)\n", midiToDb(lrint(val)));
	ctrlChanged(CTRL_PANPOT, lrint(val));
}

//---------------------------------------------------------
//   setVariSend
//---------------------------------------------------------

void MidiStrip::setVariSend(double val)
{
	ctrlChanged(CTRL_VARIATION_SEND, lrint(val));
}

//---------------------------------------------------------
//   setChorusSend
//---------------------------------------------------------

void MidiStrip::setChorusSend(double val)
{
	ctrlChanged(CTRL_CHORUS_SEND, lrint(val));
}

//---------------------------------------------------------
//   setReverbSend
//---------------------------------------------------------

void MidiStrip::setReverbSend(double val)
{
	ctrlChanged(CTRL_REVERB_SEND, lrint(val));
}

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void MidiStrip::routingPopupMenuActivated(QAction* act)
{
	if (gRoutingPopupMenuMaster != this || !track || !track->isMidiTrack())
		return;

	oom->routingPopupMenuActivated(track, act->data().toInt());
}

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void MidiStrip::iRoutePressed()
{
	if (!track || !track->isMidiTrack())
		return;

	PopupMenu* pup = oom->prepareRoutingPopupMenu(track, false);
	if (!pup)
		return;

	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	pup->popup(QCursor::pos());
	m_btnIRoute->setDown(false);
}

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void MidiStrip::oRoutePressed()
{
	if (!track || !track->isMidiTrack())
		return;

	PopupMenu* pup = oom->prepareRoutingPopupMenu(track, true);
	if (!pup)
		return;

	gRoutingPopupMenuMaster = this;
	connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
	connect(pup, SIGNAL(aboutToHide()), oom, SLOT(routingPopupMenuAboutToHide()));
	pup->popup(QCursor::pos());
	m_btnORoute->setDown(false);
}


