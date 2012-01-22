//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <fastlog.h>

#include <QLayout>
#include <QApplication>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
//#include <QPopupMenu>
#include <QCursor>
#include <QPainter>
#include <QString>
#include <QPoint>
#include <QEvent>
#include <QWidget>
#include <QVariant>
#include <QAction>
#include <QGridLayout>

#include "app.h"
#include "globals.h"
#include "apconfig.h"
#include "audio.h"
#include "driver/audiodev.h"
#include "song.h"
#include "slider.h"
#include "knob.h"
#include "combobox.h"
#include "meter.h"
#include "astrip.h"
#include "track.h"
#include "synth.h"
//#include "route.h"
#include "doublelabel.h"
#include "rack.h"
#include "node.h"
#include "AudioMixer.h"
#include "icons.h"
#include "gconfig.h"
#include "ttoolbutton.h"
#include "menutitleitem.h"
#include "popupmenu.h"
#include "utils.h"
#include "midictrl.h"
#include "mididev.h"
#include "midiport.h"
#include "midimonitor.h"

//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(QWidget* parent, AudioTrack* at)/*{{{*/
: Strip(parent, at)
{

	volume = -1.0;
	panVal = 0;

	AudioTrack* t = (AudioTrack*) track;
	channel = at->channels();
	///setMinimumWidth(STRIP_WIDTH);

	int ch = 0;
	for (; ch < channel; ++ch)
		meter[ch] = new Meter(this, track->type());
		//meter[ch] = new Meter(this);
	for (; ch < MAX_CHANNELS; ++ch)
		meter[ch] = 0;

	//---------------------------------------------------
	//    plugin rack
	//---------------------------------------------------

	rack = new EffectRack(this, t);
	rack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	rackBox->addWidget(rack);

	//---------------------------------------------------
	//    mono/stereo  pre/post
	//---------------------------------------------------

	m_btnStereo->setCheckable(true);
	m_btnStereo->setObjectName("btnStereo");
	m_btnStereo->setToolTip(tr("1/2 channel"));
	m_btnStereo->setChecked(channel == 2);
	connect(m_btnStereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));

	// disable mono/stereo for Synthesizer-Plugins
	if (t->type() == Track::AUDIO_SOFTSYNTH)
		m_btnStereo->setEnabled(false);

	//pre = new QToolButton();/*{{{*/
	//pre->setFont(config.fonts[1]);
	//pre->setCheckable(true);
	//pre->setText(tr("Pre"));
	//QIcon preSet;
	//preSet.addPixmap(*preIcon, QIcon::Normal, QIcon::Off);
	//preSet.addPixmap(*preIconOn, QIcon::Normal, QIcon::On);
	//preSet.addPixmap(*muteIcon, QIcon::Active, QIcon::On);
	//pre->setIcon(preSet);
	//pre->setObjectName("btnPre");
	//pre->setIconSize(preIcon->size());
	//pre->setToolTip(tr("pre fader - post fader"));
	//pre->setChecked(t->prefader());
	//pre->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
	//connect(pre, SIGNAL(clicked(bool)), SLOT(preToggled(bool)));
	//pre->setAttribute(Qt::WA_Hover);/*}}}*/


        // FIXME
        // It seems the prefader send doens't send to anywhere or to the
        // same output as the track is routing too.
        // It also overloads 'some output' when toggling the mono/stereo button
        // when Pre send is turned on during playback.

	//---------------------------------------------------
	//    aux send
	//---------------------------------------------------

	//int auxsSize = song->auxs()->size();
	if (t->hasAuxSend())
	{
		int idx = 0;
		QHash<qint64, AuxInfo>::const_iterator iter = t->auxSends()->constBegin();
		while (iter != t->auxSends()->constEnd())
		{
			Track* at = song->findTrackByIdAndType(iter.key(), Track::AUDIO_AUX);
			if(at)
			{
				//qDebug("Adding AUX to strip: Name: %s, tid: %lld, auxId: %lld", at->name().toUtf8().constData(), at->id(), iter.key());
				DoubleLabel* al;
				QLabel* nl;
				Knob* ak = addAuxKnob(iter.key(), at->name(), &al, &nl);
				auxIndexList[idx] = iter.key();
				auxKnobList[iter.key()] = ak;
				auxLabelList[iter.key()] = al;
				auxNameLabelList[iter.key()] = nl;
				ak->setId(idx);
				al->setId(idx);
				double val = fast_log10(t->auxSend(iter.key()))*20.0;
				ak->setValue(val);
				al->setValue(val);
				++idx;
			}
			++iter;
		}
	}

	//---------------------------------------------------
	//    slider, label, meter
	//---------------------------------------------------

	sliderGrid = new QGridLayout();
	sliderGrid->setRowStretch(0, 100);
	sliderGrid->setContentsMargins(0, 0, 8, 0);
	sliderGrid->setSpacing(0);
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

	m_sliderBg = sliderBgColor;

	slider = new Slider(this, "vol", Qt::Vertical, Slider::None, Slider::BgSlot, sliderBgColor, usePixmap);
	slider->setCursorHoming(true);
	slider->setRange(config.minSlider - 0.1, 10.0);
	slider->setFixedWidth(20);
	slider->setFont(config.fonts[1]);
	slider->setValue(fast_log10(t->volume())*20.0);

	m_vuBox->addWidget(slider);
	//sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);

	for (int i = 0; i < channel; ++i)
	{
		//meter[i]->setRange(config.minSlider, 10.0);
		meter[i]->setRange(config.minMeter, 10.0);
		meter[i]->setFixedWidth(15);
		connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
		connect(meter[i], SIGNAL(meterClipped()), this, SLOT(playbackClipped()));
		//sliderGrid->addWidget(meter[i], 0, i + 1); // , Qt::AlignHCenter);
		m_vuBox->addWidget(meter[i]);
		sliderGrid->setColumnStretch(i, 50);
	}
	//m_vuBox->addLayout(sliderGrid);

	sl = new DoubleLabel(0.0, config.minSlider, 10.0, this);
	sl->setSlider(slider);
	sl->setFont(config.fonts[1]);
	sl->setBackgroundRole(QPalette::Mid);
	sl->setSuffix(tr("dB"));
	sl->setFrame(true);
	sl->setPrecision(0);
	sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
	sl->setValue(fast_log10(t->volume()) * 20.0);
	slDefaultStyle = sl->styleSheet();

	connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
	//connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volumeChanged(double)));
	connect(slider, SIGNAL(valueChanged(double, int)), sl, SLOT(setValue(double)));
	connect(slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
	connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
	connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
	connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));
	
	m_panBox->addWidget(sl);

	//---------------------------------------------------
	//    pan, balance
	//---------------------------------------------------

	pan = addKnob(tr("Pan"), &panl);
	pan->setValue(t->pan());

	//---------------------------------------------------
	//    mute, solo, record
	//---------------------------------------------------

	if (track->canRecord())
	{
		m_btnRecord->setCheckable(true);
		m_btnRecord->setToolTip(tr("record"));
		m_btnRecord->setObjectName("btnRecord");
		m_btnRecord->setChecked(t->recordFlag());
		connect(m_btnRecord, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
	}
	else
	{
		m_btnRecord->setCheckable(false);
		m_btnRecord->setIcon(QIcon(*mixer_blank_OffIcon));
		m_btnRecord->setObjectName("btnRecord");
	}
	
	Track::TrackType type = t->type();

	m_btnMute->setCheckable(true);
	m_btnMute->setToolTip(tr("mute"));
	m_btnMute->setObjectName("btnMute");
	m_btnMute->setChecked(t->mute());
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

	m_btnSolo->setCheckable(true);
	m_btnSolo->setChecked(t->solo());
	m_btnSolo->setObjectName("btnSolo");
	connect(m_btnSolo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
	if (type == Track::AUDIO_OUTPUT)
	{
		m_btnRecord->setToolTip(tr("record downmix"));
		m_btnSolo->setToolTip(tr("solo mode"));
	}
	else
	{
		//m_btnSolo->setToolTip(tr("pre fader listening"));
		m_btnSolo->setToolTip(tr("solo mode"));
	}

	m_btnPower->setObjectName("btnExit");
	m_btnPower->setCheckable(true);
	m_btnPower->setToolTip(tr("off"));
	m_btnPower->setChecked(t->off());
	connect(m_btnPower, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

	//---------------------------------------------------
	//    routing
	//---------------------------------------------------

	if (hasIRoute)
	{
		m_btnIRoute->setObjectName("btnIns");
		m_btnIRoute->setCheckable(false);
		m_btnIRoute->setToolTip(tr("input routing"));
		connect(m_btnIRoute, SIGNAL(pressed()), SLOT(iRoutePressed()));
	}
	else
	{
		//m_btnIRoute->setEnabled(false);
		m_btnIRoute->setIcon(QIcon(*mixer_blank_OffIcon));
		m_btnIRoute->setToolTip("");
	}

	m_btnORoute->setObjectName("btnOuts");
	m_btnORoute->setCheckable(false);
	m_btnORoute->setToolTip(tr("output routing"));
	connect(m_btnORoute, SIGNAL(pressed()), SLOT(oRoutePressed()));

	//---------------------------------------------------
	//    automation type
	//---------------------------------------------------

	autoType = new ComboBox(this);
	autoType->setFont(config.fonts[1]);
	//autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
	QSizePolicy autoSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	autoSizePolicy.setHorizontalStretch(1);
	autoSizePolicy.setVerticalStretch(0);
	autoSizePolicy.setHeightForWidth(autoType->sizePolicy().hasHeightForWidth());
	autoType->setSizePolicy(autoSizePolicy);
	autoType->setAlignment(Qt::AlignCenter);
	autoType->setMaximumSize(QSize(65,20));

	autoType->insertItem(tr("Off"), AUTO_OFF);
	autoType->insertItem(tr("Read"), AUTO_READ);
	autoType->insertItem(tr("Touch"), AUTO_TOUCH);
	autoType->insertItem(tr("Write"), AUTO_WRITE);
	autoType->setCurrentItem(t->automationType());

	if (t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
	{
		QPalette palette;
		palette.setColor(autoType->backgroundRole(), QColor(Qt::red));
		autoType->setPalette(palette);
	}
	else
	{
		QPalette palette;
		palette.setColor(autoType->backgroundRole(), qApp->palette().color(QPalette::Active, QPalette::Background));
		autoType->setPalette(palette);
	}
	autoType->setToolTip(tr("automation type"));
	connect(autoType, SIGNAL(activated(int, int)), SLOT(setAutomationType(int, int)));
	m_autoBox->addWidget(autoType);

	m_btnPower->blockSignals(true);
	updateOffState(); // init state
	m_btnPower->blockSignals(false);
	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
}/*}}}*/

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
{
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void AudioStrip::heartBeat()
{
	if(song->invalid)
		return;
	for (int ch = 0; ch < track->channels(); ++ch)
	{
		if (meter[ch])
		{
			//int meterVal = track->meter(ch);
			//int peak  = track->peak(ch);
			//meter[ch]->setVal(meterVal, peak, false);
			meter[ch]->setVal(track->meter(ch), track->peak(ch), false);
		}
	}
	Strip::heartBeat();
	updateVolume();
	updatePan();
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
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioStrip::configChanged()
{
	songChanged(SC_CONFIG);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(int val)/*{{{*/
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (val == SC_MIDI_CONTROLLER)
		return;

	AudioTrack* src = (AudioTrack*) track;

	// Do channels before config...
	if (val & SC_CHANNELS)
		updateChannels();

	// Update the routing popup menu if anything relevant changed.
	if (val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))
	{
		oom->updateRouteMenus(track, this);
	}

	// Catch when label font, or configuration min slider and meter values change.
	if (val & SC_CONFIG)
	{
		// Set the strip label's font.
		setLabelFont();

		// Adjust minimum volume slider and label values.
		slider->setRange(config.minSlider - 0.1, 10.0);
		sl->setRange(config.minSlider, 10.0);

		// Adjust minimum aux knob and label values.
		
		QHashIterator<int, qint64> iter(auxIndexList);
		while(iter.hasNext())
		{
			iter.next();
			if(auxKnobList[iter.value()])
			{
				auxKnobList[iter.value()]->blockSignals(true);
				auxLabelList[iter.value()]->blockSignals(true);
				auxKnobList[iter.value()]->setRange(config.minSlider - 0.1, 10.0);
				auxLabelList[iter.value()]->setRange(config.minSlider, 10.1);
				auxKnobList[iter.value()]->blockSignals(false);
				auxLabelList[iter.value()]->blockSignals(false);
			}
		}

		// Adjust minimum meter values.
		for (int c = 0; c < channel; ++c)
			meter[c]->setRange(config.minMeter, 10.0);
	}

	if (m_btnMute && (val & SC_MUTE))
	{ // m_btnMute && m_btnPower
		m_btnMute->blockSignals(true);
		m_btnMute->setChecked(src->mute());
		m_btnMute->blockSignals(false);
		updateOffState();
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
		setLabelFont();
		//Update aux labels in case aux track was renamed
		updateAuxNames();
	}
	if (val & SC_ROUTE)
	{
		/*if (pre)
		{
			pre->blockSignals(true);
			pre->setChecked(src->prefader());
			pre->blockSignals(false);
		}*/
	}
	if (val & SC_AUX)
	{
		QHashIterator<int, qint64> iter(auxIndexList);
		while(iter.hasNext())
		{
			iter.next();
			if(auxKnobList[iter.value()])
			{
				double val = fast_log10(src->auxSend(iter.value())) * 20.0;
				auxKnobList[iter.value()]->blockSignals(true);
				auxLabelList[iter.value()]->blockSignals(true);
				auxKnobList[iter.value()]->setValue(val);
				auxLabelList[iter.value()]->setValue(val);
				auxKnobList[iter.value()]->blockSignals(false);
				auxLabelList[iter.value()]->blockSignals(false);
			}
		}
	}
	if (autoType && (val & SC_AUTOMATION))
	{
		autoType->blockSignals(true);
		autoType->setCurrentItem(track->automationType());
		if (track->automationType() == AUTO_TOUCH || track->automationType() == AUTO_WRITE)
		{
			QPalette palette;
			palette.setColor(autoType->backgroundRole(), QColor(Qt::red));
			autoType->setPalette(palette);
		}
		else
		{
			QPalette palette;
			palette.setColor(autoType->backgroundRole(), qApp->palette().color(QPalette::Active, QPalette::Background));
			autoType->setPalette(palette);
		}

		autoType->blockSignals(false);
	}
}/*}}}*/

//---------------------------------------------------------
//   updateVolume
//---------------------------------------------------------

void AudioStrip::updateVolume()/*{{{*/
{
	double vol = ((AudioTrack*) track)->volume();
	if (vol != volume)
	{
		//printf("AudioStrip::updateVolume setting slider and label\n");

		slider->blockSignals(true);
		sl->blockSignals(true);
		double val = fast_log10(vol) * 20.0;
		slider->setValue(val);
		sl->setValue(val);
		sl->blockSignals(false);
		slider->blockSignals(false);
		volume = vol;
		if(((AudioTrack*) track)->volFromAutomation())
		{
			//printf("AudioStrip::updateVolume via automation\n");
			midiMonitor->msgSendAudioOutputEvent((Track*)track, CTRL_VOLUME, vol);
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   updatePan
//---------------------------------------------------------

void AudioStrip::updatePan()/*{{{*/
{
	double v = ((AudioTrack*) track)->pan();
	if (v != panVal)
	{
		//printf("AudioStrip::updatePan setting slider and label\n");

		pan->blockSignals(true);
		panl->blockSignals(true);
		pan->setValue(v);
		panl->setValue(v);
		panl->blockSignals(false);
		pan->blockSignals(false);
		panVal = v;
		if(((AudioTrack*) track)->panFromAutomation())
		{
			midiMonitor->msgSendAudioOutputEvent((Track*)track, CTRL_PANPOT, v);
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void AudioStrip::offToggled(bool val)
{
	track->setOff(val);
	song->update(SC_MUTE);
}

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void AudioStrip::updateOffState()/*{{{*/
{
	bool val = !track->off();
	slider->setEnabled(val);
	sl->setEnabled(val);
	pan->setEnabled(val);
	panl->setEnabled(val);
	if (track->type() != Track::AUDIO_SOFTSYNTH)
		m_btnStereo->setEnabled(val);
	label->setEnabled(val);

	QHashIterator<int, qint64> iter(auxIndexList);
	while(iter.hasNext())
	{
		iter.next();
		if(auxKnobList[iter.value()])
		{
			auxKnobList[iter.value()]->setEnabled(val);
			auxLabelList[iter.value()]->setEnabled(val);
		}
	}

	//if (pre)
	//	pre->setEnabled(val);
	if (hasRecord)
		m_btnRecord->setEnabled(val);
	//if (m_btnSolo)
		m_btnSolo->setEnabled(val);
	//if (m_btnMute)
		m_btnMute->setEnabled(val);
	if (autoType)
		autoType->setEnabled(val);
	if (hasIRoute)
		m_btnIRoute->setEnabled(val);
	if (hasORoute)
		m_btnORoute->setEnabled(val);
	m_btnPower->blockSignals(true);
	m_btnPower->setChecked(track->off());
	m_btnPower->blockSignals(false);
}/*}}}*/

//---------------------------------------------------------
//   preToggled
//---------------------------------------------------------

void AudioStrip::preToggled(bool val)
{
	audio->msgSetPrefader((AudioTrack*) track, val);
	resetPeaks();
	song->update(SC_ROUTE);
}

//---------------------------------------------------------
//   stereoToggled
//---------------------------------------------------------

void AudioStrip::stereoToggled(bool val)
{
	int oc = track->channels();
	int nc = val ? 2 : 1;
	//      m_btnStereo->setIcon(nc == 2 ? *stereoIcon : *monoIcon);
	if (oc == nc)
		return;
	audio->msgSetChannels((AudioTrack*) track, nc);
	song->update(SC_CHANNELS);
}

//---------------------------------------------------------
//   auxChanged
//---------------------------------------------------------

void AudioStrip::auxChanged(double val, int idx)
{
	double vol;
	if (val <= config.minSlider)
	{
		vol = 0.0;
		val -= 1.0; // display special value "off"
	}
	else
		vol = pow(10.0, val / 20.0);
	if(!auxIndexList.isEmpty() && auxIndexList.contains(idx))
	{
		audio->msgSetAux((AudioTrack*) track, auxIndexList[idx], vol);
		song->update(SC_AUX);
	}
}

void AudioStrip::auxPreToggled(qint64 idx, bool state)
{
	((AudioTrack*)track)->setAuxPrefader(idx, state);
}

//---------------------------------------------------------
//   auxLabelChanged
//---------------------------------------------------------

void AudioStrip::auxLabelChanged(double val, int idx)
{
	if (auxIndexList.isEmpty() || !auxIndexList.contains(idx))
		return;
	if(!auxKnobList.isEmpty() && auxKnobList.contains(auxIndexList[idx]))
		auxKnobList[auxIndexList[idx]]->setValue(val);
}

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(double val)
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		track->enableVolumeController(false);

	//printf("AudioStrip::volumeChanged(%g) \n", val);
	double vol;
	if (val <= config.minSlider)
	{
		vol = 0.0;
		val -= 1.0; // display special value "off"
	}
	else
		vol = pow(10.0, val / 20.0);
	volume = vol;
	audio->msgSetVolume((AudioTrack*) track, vol);
	((AudioTrack*) track)->recordAutomation(AC_VOLUME, vol);
	song->update(SC_TRACK_MODIFIED);
	//double vv = (vol + 60)/0.5546875;
	//printf("AudioStrip::volumeChanged(%g) - val: %g - midiNum: %d whacky: %d\n", vol, dbToTrackVol(val), dbToMidi(val), dbToMidi(trackVolToDb(vol)));
}

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed()
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
		track->enableVolumeController(false);

	double val = slider->value();
	double vol;
	if (val <= config.minSlider)
	{
		vol = 0.0;
		//val -= 1.0; // display special value "off"
	}
	else
		vol = pow(10.0, val / 20.0);
	volume = vol;
	audio->msgSetVolume((AudioTrack*) track, volume);
	((AudioTrack*) track)->startAutoRecord(AC_VOLUME, volume);
}

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased()
{
	if (track->automationType() != AUTO_WRITE)
		track->enableVolumeController(true);

	((AudioTrack*) track)->stopAutoRecord(AC_VOLUME, volume);
}

//---------------------------------------------------------
//   volumeRightClicked
//---------------------------------------------------------

void AudioStrip::volumeRightClicked(const QPoint &p)
{
	song->execAutomationCtlPopup((AudioTrack*) track, p, AC_VOLUME);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void AudioStrip::volLabelChanged(double val)
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		track->enableVolumeController(false);

	double vol;
	if (val <= config.minSlider)
	{
		vol = 0.0;
		val -= 1.0; // display special value "off"
	}
	else
		vol = pow(10.0, val / 20.0);
	volume = vol;
	slider->setValue(val);
	audio->msgSetVolume((AudioTrack*) track, vol);
	((AudioTrack*) track)->startAutoRecord(AC_VOLUME, vol);
}

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AudioStrip::panChanged(double val)
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		track->enablePanController(false);

	panVal = val;
	audio->msgSetPan(((AudioTrack*) track), val);
	((AudioTrack*) track)->recordAutomation(AC_PAN, val);
	//printf("AudioStrip::panChanged(%d) midiToTrackPan(%g)\n", trackPanToMidi(val), midiToTrackPan(trackPanToMidi(val)));
}

//---------------------------------------------------------
//   panPressed
//---------------------------------------------------------

void AudioStrip::panPressed()
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
		track->enablePanController(false);

	panVal = pan->value();
	audio->msgSetPan(((AudioTrack*) track), panVal);
	((AudioTrack*) track)->startAutoRecord(AC_PAN, panVal);
}

//---------------------------------------------------------
//   panReleased
//---------------------------------------------------------

void AudioStrip::panReleased()
{
	if (track->automationType() != AUTO_WRITE)
		track->enablePanController(true);
	((AudioTrack*) track)->stopAutoRecord(AC_PAN, panVal);
}

//---------------------------------------------------------
//   panRightClicked
//---------------------------------------------------------

void AudioStrip::panRightClicked(const QPoint &p)
{
	song->execAutomationCtlPopup((AudioTrack*) track, p, AC_PAN);
}

//---------------------------------------------------------
//   panLabelChanged
//---------------------------------------------------------

void AudioStrip::panLabelChanged(double val)
{
	AutomationType at = ((AudioTrack*) track)->automationType();
	if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
		track->enablePanController(false);

	panVal = val;
	pan->setValue(val);
	audio->msgSetPan((AudioTrack*) track, val);
	((AudioTrack*) track)->startAutoRecord(AC_PAN, val);
}

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------

void AudioStrip::updateChannels()/*{{{*/
{
	AudioTrack* t = (AudioTrack*) track;
	int c = t->channels();
	//printf("AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);

	if (c > channel)
	{
		for (int cc = channel; cc < c; ++cc)
		{
			//meter[cc] = new Meter(this);
			//meter[cc] = new Meter(this, track->type(), Meter::LinMeter, Qt::Vertical);
			meter[cc] = new Meter(this, track->type());
			//meter[cc]->setRange(config.minSlider, 10.0);
			meter[cc]->setRange(config.minMeter, 10.0);
			meter[cc]->setFixedWidth(15);
			connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
			m_vuBox->addWidget(meter[cc]);
			//sliderGrid->addWidget(meter[cc], 0, cc + 1, Qt::AlignHCenter);
			//sliderGrid->setColumnStretch(cc, 50);
			meter[cc]->show();
		}
	}
	else if (c < channel)
	{
		for (int cc = channel - 1; cc >= c; --cc)
		{
			delete meter[cc];
			meter[cc] = 0;
		}
	}
	channel = c;
	m_btnStereo->blockSignals(true);
	m_btnStereo->setChecked(channel == 2);
	m_btnStereo->blockSignals(false);
}/*}}}*/

//---------------------------------------------------------
//   addKnob
//    type = 0 - panorama
//           1 - aux send
//---------------------------------------------------------

Knob* AudioStrip::addKnob(QString name, DoubleLabel** dlabel)/*{{{*/
{
	Knob* knob = new Knob(this);
	knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
	knob->setRange(-1.0, +1.0);
	knob->setToolTip(tr("panorama"));
	Track::TrackType type_tmp = track->type();
	switch (type_tmp)
	{
		case Track::MIDI:
		case Track::DRUM:
			knob->setKnobImage(":images/knob_audio_new.png");
		break;
		case Track::WAVE:
			knob->setKnobImage(":images/knob_input_new.png");
		break;
		case Track::AUDIO_OUTPUT:
			knob->setKnobImage(":images/knob_output_new.png");
		break;
		case Track::AUDIO_INPUT:
			knob->setKnobImage(":images/knob_midi_new.png");
		break;
		case Track::AUDIO_BUSS:
			knob->setKnobImage(":images/knob_buss_new.png");
		break;
		case Track::AUDIO_AUX:
			knob->setKnobImage(":images/knob_aux_new.png");
		break;
		case Track::AUDIO_SOFTSYNTH:
			knob->setKnobImage(":images/knob_audio_new.png");
		break;
		default:
			knob->setKnobImage(":images/knob_aux_new.png");
		break;
	}

	knob->setBackgroundRole(QPalette::Mid);

	DoubleLabel* pl = new DoubleLabel(0, -1.0, +1.0, this);

	if (dlabel)
		*dlabel = pl;
	pl->setSlider(knob);
	pl->setFont(config.fonts[1]);
	pl->setBackgroundRole(QPalette::Mid);
	pl->setFrame(true);
	pl->setAlignment(Qt::AlignCenter);
	pl->setPrecision(2);
	pl->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	QString label = name;//tr("Pan");

	QLabel* plb = new QLabel(label, this);
	plb->setFont(config.fonts[1]);
	plb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	plb->setAlignment(Qt::AlignCenter);


	QHBoxLayout *container = new QHBoxLayout();
	container->setContentsMargins(0, 0, 0, 0);
	container->setSpacing(0);
	container->setAlignment(Qt::AlignHCenter|Qt::AlignCenter);
	QVBoxLayout *labelBox = new QVBoxLayout();
	labelBox->setContentsMargins(0, 0, 0, 0);
	labelBox->setSpacing(0);
	labelBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	labelBox->addWidget(plb);
	labelBox->addWidget(pl);
	container->addLayout(labelBox);
	container->addWidget(knob);
	m_panBox->addLayout(container);
	connect(knob, SIGNAL(valueChanged(double, int)), pl, SLOT(setValue(double)));
	//connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panChanged(double)));

	connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
	connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
	connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
	connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
	connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(panRightClicked(const QPoint &)));
	return knob;
}/*}}}*/

Knob* AudioStrip::addAuxKnob(qint64 id, QString name, DoubleLabel** dlabel, QLabel** nameLabel)/*{{{*/
{
	Knob* knob = new Knob(this);
	knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));

	knob->setRange(config.minSlider - 0.1, 10.0);
	knob->setKnobImage(":images/knob_aux_new.png");
	knob->setToolTip(tr("aux send level"));
	knob->setBackgroundRole(QPalette::Mid);

	DoubleLabel* pl = new DoubleLabel(0.0, config.minSlider, 10.1, this);

	if (dlabel)
		*dlabel = pl;
	pl->setSlider(knob);
	pl->setFont(config.fonts[1]);
	pl->setBackgroundRole(QPalette::Mid);
	pl->setFrame(true);
	pl->setAlignment(Qt::AlignCenter);
	pl->setPrecision(0);
	pl->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	
	QString label;
	label = name;
	if (name.length() > 17)
		label = name.mid(0, 16).append("..");
	AuxCheckBox* chkPre = new AuxCheckBox("Pre", id, this);
	chkPre->setToolTip(tr("Make Aux Send Prefader"));
	chkPre->setChecked(((AudioTrack*)track)->auxIsPrefader(id));

	QLabel* plb = new QLabel(label, this);
	if(nameLabel)
		*nameLabel = plb;
	plb->setFont(config.fonts[1]);
	plb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	plb->setAlignment(Qt::AlignCenter);


	QHBoxLayout *container = new QHBoxLayout();
	container->setContentsMargins(0, 0, 0, 0);
	container->setSpacing(0);
	container->setAlignment(Qt::AlignHCenter|Qt::AlignCenter);
	QVBoxLayout *labelBox = new QVBoxLayout();
	labelBox->setContentsMargins(0, 0, 0, 0);
	labelBox->setSpacing(0);
	labelBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	labelBox->addWidget(plb);

	plb->setToolTip(name);
	container->addItem(new QSpacerItem(7, 0));
	container->addWidget(pl);
	container->addWidget(knob);
	container->addWidget(chkPre);
	container->addItem(new QSpacerItem(7, 0));
	labelBox->addLayout(container);
	m_auxBox->addLayout(labelBox);
	connect(knob, SIGNAL(valueChanged(double, int)), pl, SLOT(setValue(double)));
	//connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panChanged(double)));

	//knob->setId(id);
	//pl->setId(id);

	connect(pl, SIGNAL(valueChanged(double, int)), knob, SLOT(setValue(double)));
	connect(pl, SIGNAL(valueChanged(double, int)), SLOT(auxChanged(double, int)));
	// Not used yet. Switch if/when necessary.
	//connect(pl, SIGNAL(valueChanged(double, int)), SLOT(auxLabelChanged(double, int)));

	connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(auxChanged(double, int)));
	connect(chkPre, SIGNAL(toggled(qint64, bool)), SLOT(auxPreToggled(qint64, bool)));
	return knob;
}/*}}}*/

void AudioStrip::updateAuxNames()
{
	QHashIterator<qint64, QLabel*> iter(auxNameLabelList);
	while(iter.hasNext())
	{
		iter.next();
		Track* t = song->findTrackByIdAndType(iter.key(), Track::AUDIO_AUX);
		if(t)
		{
			QString label = t->name();
			if (label.length() > 17)
				label = label.mid(0, 16).append("..");
			if(iter.value()->text() != label)
			{
				iter.value()->setText(label);
			}
		}
	}
}

void AudioStrip::trackChanged()
{
	rack->setTrack((AudioTrack*)track);
	songChanged(-1);
}

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
{
	//if(track->isMidiTrack() || (track->type() == Track::AUDIO_AUX) || (track->type() == Track::AUDIO_SOFTSYNTH))
	if (!track || track->isMidiTrack() || track->type() == Track::AUDIO_AUX)
	{
		gRoutingPopupMenuMaster = 0;
		return;
	}
	AudioPortConfig* aconf = oom->getRoutingDialog(true);
	if(aconf)
		aconf->setSelected((AudioTrack*)track);
}

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void AudioStrip::routingPopupMenuActivated(QAction* act)
{
	if (!track || gRoutingPopupMenuMaster != this || track->isMidiTrack())
		return;

	PopupMenu* pup = oom->getRoutingPopupMenu();

	if (pup->actions().isEmpty())
		return;

	AudioTrack* t = (AudioTrack*) track;
	RouteList* rl = gIsOutRoutingPopupMenu ? t->outRoutes() : t->inRoutes();

	int n = act->data().toInt();
	if (n == -1)
		return;

	if (gIsOutRoutingPopupMenu)
	{
		if (track->type() == Track::AUDIO_OUTPUT)
		{

			int chan = n & 0xf;

			Route srcRoute(t, chan);
			Route dstRoute(act->text(), true, -1, Route::JACK_ROUTE);
			dstRoute.channel = chan;

			// check if route src->dst exists:
			iRoute irl = rl->begin();
			for (; irl != rl->end(); ++irl)
			{
				if (*irl == dstRoute)
					break;
			}
			if (irl != rl->end())
			{
				// disconnect if route exists
				audio->msgRemoveRoute(srcRoute, dstRoute);
			}
			else
			{
				// connect if route does not exist
				audio->msgAddRoute(srcRoute, dstRoute);
			}
			audio->msgUpdateSoloStates();
			song->update(SC_ROUTE);
			return;
		}

		iRouteMenuMap imm = gRoutingMenuMap.find(n);
		if (imm == gRoutingMenuMap.end())
			return;

		Route srcRoute(t, imm->second.channel, imm->second.channels);
		srcRoute.remoteChannel = imm->second.remoteChannel;

		Route &dstRoute = imm->second;

		// check if route src->dst exists:
		iRoute irl = rl->begin();
		for (; irl != rl->end(); ++irl)
		{
			if (*irl == dstRoute)
				break;
		}
		if (irl != rl->end())
		{
			// disconnect if route exists
			audio->msgRemoveRoute(srcRoute, dstRoute);
		}
		else
		{
			// connect if route does not exist
			audio->msgAddRoute(srcRoute, dstRoute);
		}
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
	else
	{
		if (track->type() == Track::AUDIO_INPUT)
		{
			int chan = n & 0xf;

			Route srcRoute(act->text(), false, -1, Route::JACK_ROUTE);
			Route dstRoute(t, chan);

			srcRoute.channel = chan;

			iRoute irl = rl->begin();
			for (; irl != rl->end(); ++irl)
			{
				if (*irl == srcRoute)
					break;
			}
			if (irl != rl->end())
				// disconnect
				audio->msgRemoveRoute(srcRoute, dstRoute);
			else
				// connect
				audio->msgAddRoute(srcRoute, dstRoute);

			audio->msgUpdateSoloStates();
			song->update(SC_ROUTE);
			return;
		}

		iRouteMenuMap imm = gRoutingMenuMap.find(n);
		if (imm == gRoutingMenuMap.end())
			return;

		Route &srcRoute = imm->second;

		Route dstRoute(t, imm->second.channel, imm->second.channels);
		dstRoute.remoteChannel = imm->second.remoteChannel;

		iRoute irl = rl->begin();
		for (; irl != rl->end(); ++irl)
		{
			if (*irl == srcRoute)
				break;
		}
		if (irl != rl->end())
		{
			// disconnect
			audio->msgRemoveRoute(srcRoute, dstRoute);
		}
		else
		{
			// connect
			audio->msgAddRoute(srcRoute, dstRoute);
		}
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
	if (!track || track->isMidiTrack())
	{
		gRoutingPopupMenuMaster = 0;
		return;
	}
	AudioPortConfig* apconfig = oom->getRoutingDialog(true);
	if(apconfig)
		apconfig->setSelected((AudioTrack*)track);
}

void AudioStrip::playbackClipped()
{
	sl->setStyleSheet("DoubleLabel { padding-left: 2px; border: 1px solid #9d9d9d; border-image: none; background-color: black; color: #ba0000; font-weight: normal;}");
}

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void AudioStrip::resetPeaks()
{
	track->resetPeaks();
	sl->setStyleSheet(slDefaultStyle);
}

void AudioStrip::toggleShowEffectsRack(bool open)
{
	toggleAuxPanel(open);
	/*if (rack->isVisible())
	{
		rack->hide();
	}
	else
	{
		rack->show();
	}*/
}

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(const QString& ss, QWidget* parent)
: QWidgetAction(parent)
{
	s = ss;
	// Don't allow to click on it.
	setEnabled(false);
	// Just to be safe, set to -1 instead of default 0.
	setData(-1);
}

QWidget* MenuTitleItem::createWidget(QWidget *parent)
{
	QLabel* l = new QLabel(s, parent);
	l->setAlignment(Qt::AlignCenter);
	return l;
}

