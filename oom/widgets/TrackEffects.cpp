//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include <fastlog.h>

#include "globaldefs.h"
#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "icons.h"
#include "song.h"
#include "track.h"
#include "utils.h"
#include "driver/audiodev.h"
#include "slider.h"
#include "knob.h"
#include "doublelabel.h"
#include "meter.h"
#include "EffectRack.h"
#include "node.h"
#include "popupmenu.h"
#include "midictrl.h"
#include "mididev.h"
#include "midiport.h"
#include "midimonitor.h"
#include "TrackEffects.h"

TrackEffects::TrackEffects(Track* t, QWidget* parent)
: QFrame(parent)
{
	m_track = t;
	if(t)
		m_trackId = t->id();

	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);

	m_inputRack = 0;
	m_rack = 0;

	m_mainVBoxLayout = new QVBoxLayout(this);
	m_mainVBoxLayout->setSpacing(0);
	m_mainVBoxLayout->setContentsMargins(0, 0, 0, 0);
	m_mainVBoxLayout->setObjectName(QString::fromUtf8("m_mainVBoxLayout"));

	setObjectName("EffectsAuxbox");
	layoutUi();

	hasAux = true;
	switch (m_track->type())/*{{{*/
	{
		case Track::AUDIO_OUTPUT:
			m_auxBase->setObjectName("MixerAudioOutAuxbox");
			hasAux = false;
			break;
		case Track::AUDIO_BUSS:
			m_auxBase->setObjectName("MixerAudioBussAuxbox");
			hasAux = true;
			break;
		case Track::AUDIO_AUX:
			m_auxBase->setObjectName("MixerAuxAuxbox");
			hasAux = false;
			break;
		case Track::WAVE:
			m_auxBase->setObjectName("MixerWaveAuxbox");
			hasAux = true;
			break;
		case Track::AUDIO_INPUT:
			m_auxBase->setObjectName("MixerAudioInAuxbox");
			hasAux = true;
			break;
		case Track::MIDI:
		case Track::DRUM:
		{
			m_auxBase->setObjectName("MidiTrackAuxbox");
			hasAux = true; //Used for controller knobs in midi
		}
			break;
		default:
		{
			hasAux = false;
		}
		break;
	}/*}}}*/

	if(hasAux)
	{
		m_tabWidget->addTab(m_auxTab, QString(tr("Aux")));
		setupAuxPanel();
	}
	m_tabWidget->addTab(m_fxTab, QString(tr("FX")));
	
	m_tabWidget->setCurrentIndex(m_track->mixerTab());
	connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

void TrackEffects::setupAuxPanel()/*{{{*/
{
	if(!hasAux)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			//QLabel* inputLabel = new QLabel(tr("Input Aux"));
			//m_auxBox->addWidget(inputLabel);
			populateAuxForTrack((AudioTrack*)in);
		}
	}
	else
	{//Populate my own aux send
		//QLabel* auxLabel = new QLabel(tr("Aux"));
		//m_auxBox->addWidget(auxLabel);
		populateAuxForTrack((AudioTrack*)m_track);
	}
}/*}}}*/

void TrackEffects::populateAuxForTrack(AudioTrack* t)/*{{{*/
{
	if (t && t->hasAuxSend())
	{
		AuxProxy *proxy = new AuxProxy(t);
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
				Knob* ak = addAuxKnob(proxy, iter.key(), at->name(), &al, &nl);
				proxy->auxIndexList[idx] = iter.key();
				proxy->auxKnobList[iter.key()] = ak;
				proxy->auxLabelList[iter.key()] = al;
				proxy->auxNameLabelList[iter.key()] = nl;
				ak->setId(idx);
				al->setId(idx);
				double val = fast_log10(t->auxSend(iter.key()))*20.0;
				ak->setValue(val);
				al->setValue(val);
				++idx;
			}
			++iter;
		}
		m_proxy.insert(t->id(), proxy);
	}
}/*}}}*/

void TrackEffects::setTrack(Track* t)
{
	//TODO: call an update for this track
	m_track = t;
	if(m_track)
		m_trackId = m_track->id();
}

void TrackEffects::layoutUi()/*{{{*/
{
	QSize iconSize = QSize(22, 20);
	
	m_tabWidget = new QTabWidget(this);/*{{{*/
	m_tabWidget->setObjectName(QString::fromUtf8("m_tabWidget"));
	m_tabWidget->setTabPosition(QTabWidget::North);
	m_tabWidget->setIconSize(QSize(0, 0));
	
	m_auxTab = new QWidget();
	m_auxTab->setObjectName(QString::fromUtf8("auxTab"));/*}}}*/
	
	m_auxTabLayout = new QVBoxLayout(m_auxTab);
	m_auxTabLayout->setSpacing(0);
	m_auxTabLayout->setContentsMargins(0, 0, 0, 0);
	m_auxTabLayout->setObjectName(QString::fromUtf8("auxTabLayout"));
	
	m_auxScroll = new QScrollArea(m_auxTab);
	m_auxScroll->setObjectName(QString::fromUtf8("m_auxScroll"));
	QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy2.setHorizontalStretch(1);
	sizePolicy2.setVerticalStretch(0);
	sizePolicy2.setHeightForWidth(m_auxScroll->sizePolicy().hasHeightForWidth());
	m_auxScroll->setSizePolicy(sizePolicy2);
	m_auxScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_auxScroll->setWidgetResizable(true);
	m_auxScroll->setAlignment(Qt::AlignCenter);
	m_auxScroll->setAutoFillBackground(true);
	m_auxScroll->setMinimumSize(QSize(95,0));
	
	m_auxBase = new QFrame(this);
	switch (m_track->type())/*{{{*/
	{
		case Track::AUDIO_OUTPUT:
			m_auxBase->setObjectName("MixerAudioOutAuxbox");
			break;
		case Track::AUDIO_BUSS:
			m_auxBase->setObjectName("MixerAudioBussAuxbox");
			break;
		case Track::AUDIO_AUX:
			m_auxBase->setObjectName("MixerAuxAuxbox");
			break;
		case Track::WAVE:
			m_auxBase->setObjectName("MixerWaveAuxbox");
			break;
		case Track::AUDIO_INPUT:
			m_auxBase->setObjectName("MixerAudioInAuxbox");
			break;
		case Track::AUDIO_SOFTSYNTH:
			m_auxBase->setObjectName("MixerSynthAuxbox");
			break;
		case Track::MIDI:
			m_auxBase->setObjectName("MidiTrackAuxbox");
			break;
		case Track::DRUM:
			m_auxBase->setObjectName("MidiDrumTrackAuxbox");
			break;
	}/*}}}*/
	QSizePolicy auxSizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
	auxSizePolicy2.setHorizontalStretch(1);
	auxSizePolicy2.setVerticalStretch(0);
	auxSizePolicy2.setHeightForWidth(m_auxBase->sizePolicy().hasHeightForWidth());
	
	m_auxBox = new QVBoxLayout(m_auxBase);
	m_auxBox->setObjectName(QString::fromUtf8("m_auxBox"));
	m_auxBox->setContentsMargins(0, -1, 0, -1);
	m_auxBox->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	m_auxScroll->setWidget(m_auxBase);
	

	m_auxTabLayout->addWidget(m_auxScroll);
	m_fxTab = new QWidget();
	m_fxTab->setObjectName(QString::fromUtf8("fxTab"));

	rackBox = new QVBoxLayout(m_fxTab);
	rackBox->setSpacing(0);
	rackBox->setContentsMargins(0, 0, 0, 0);
	rackBox->setObjectName(QString::fromUtf8("rackBox"));
	
	//m_tabWidget->addTab(m_auxTab, QString(tr("Aux")));
	//m_tabWidget->addTab(m_fxTab, QString(tr("FX")));

	//Populate effect rack box;
	Track *in = 0;/*{{{*/
	if(m_track->hasChildren())
	{
		QList<qint64> *chain = m_track->audioChain();
		for(int i = 0; i < chain->size(); i++)
		{
			in = song->findTrackByIdAndType(chain->at(i), Track::AUDIO_INPUT);
			if(in)
			{
				break;
			}
		}
	}
	else
		m_inputRack = 0;/*}}}*/

	if(m_track && m_track->isMidiTrack())
	{
		if(in)
		{
			m_inputRack = new EffectRack(this, (AudioTrack*)in);
			m_inputRack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			rackBox->addWidget(m_inputRack);
		}
		else
			m_inputRack = 0;
		m_rack = 0;
	}
	else if(m_track)
	{
		if(in)
		{
			m_inputRack = new EffectRack(this, (AudioTrack*)in);
			m_inputRack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			rackBox->addWidget(m_inputRack);
		}
		else
			m_inputRack = 0;
		m_rack = new EffectRack(this, (AudioTrack*)m_track);
		m_rack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		rackBox->addWidget(m_rack);
	}
	m_mainVBoxLayout->addWidget(m_tabWidget);

	
}/*}}}*/

void TrackEffects::tabChanged(int tab)
{
	if(m_track)
		m_track->setMixerTab(tab);
}

void TrackEffects::songChanged(int val)/*{{{*/
{
	if (val == SC_MIDI_CONTROLLER)
		return;

	foreach(AuxProxy* proxy, m_proxy)
	{
		proxy->songChanged(val);
	}
}/*}}}*/

Knob* TrackEffects::addAuxKnob(AuxProxy* proxy, qint64 id, QString name, DoubleLabel** dlabel, QLabel** nameLabel)/*{{{*/
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
	AuxPreCheckBox* chkPre = new AuxPreCheckBox("Pre", id, this);
	chkPre->setToolTip(tr("Make Aux Send Prefader"));
	chkPre->setChecked(proxy->track()->auxIsPrefader(id));

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
	connect(pl, SIGNAL(valueChanged(double, int)), proxy, SLOT(auxChanged(double, int)));
	// Not used yet. Switch if/when necessary.
	//connect(pl, SIGNAL(valueChanged(double, int)), SLOT(auxLabelChanged(double, int)));

	connect(knob, SIGNAL(sliderMoved(double, int)), proxy, SLOT(auxChanged(double, int)));
	connect(chkPre, SIGNAL(toggled(qint64, bool)), proxy, SLOT(auxPreToggled(qint64, bool)));
	return knob;
}/*}}}*/

void AuxProxy::songChanged(int val)/*{{{*/
{
	if (val == SC_MIDI_CONTROLLER)
		return;

	AudioTrack* src = (AudioTrack*)m_track;

	// Catch when label font, or configuration min slider and meter values change.
	if (val & SC_CONFIG)
	{
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
	}

	if (val & SC_TRACK_MODIFIED)
	{
		updateAuxNames();
	}
	if(!src)
		return;

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
}/*}}}*/

//---------------------------------------------------------
//   auxChanged
//---------------------------------------------------------

void AuxProxy::auxChanged(double val, int idx)/*{{{*/
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
		audio->msgSetAux((AudioTrack*) m_track, auxIndexList[idx], vol);
		song->update(SC_AUX);
	}
}/*}}}*/

void AuxProxy::auxPreToggled(qint64 idx, bool state)/*{{{*/
{
	((AudioTrack*)m_track)->setAuxPrefader(idx, state);
}/*}}}*/

//---------------------------------------------------------
//   auxLabelChanged
//---------------------------------------------------------

void AuxProxy::auxLabelChanged(double val, int idx)/*{{{*/
{
	if (auxIndexList.isEmpty() || !auxIndexList.contains(idx))
		return;
	if(!auxKnobList.isEmpty() && auxKnobList.contains(auxIndexList[idx]))
		auxKnobList[auxIndexList[idx]]->setValue(val);
}/*}}}*/


void AuxProxy::updateAuxNames()/*{{{*/
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
}/*}}}*/

