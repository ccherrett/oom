//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: strip.cpp,v 1.6.2.5 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QToolButton>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QColor>
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSpacerItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>


#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "icons.h"
#include "song.h"
#include "track.h"
#include "strip.h"
#include "meter.h"
#include "utils.h"
#include "ttoolbutton.h"
#include "TrackEffects.h"

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, Track* t)
: QFrame(parent)
{
	track = t;
	m_type = t->type();
	_curGridRow = 0;
	m_collapsed = false;
	hasRecord = true;
	hasAux = true;
	hasStereo = true;
	hasIRoute = true;
	hasORoute = true;
	setAttribute(Qt::WA_DeleteOnClose);
	layoutUi();

	//setBackgroundRole(QPalette::Mid);
	//setFrameStyle(Panel | Raised);
	//setLineWidth(2);
	setMaximumWidth(212);
	setMinimumHeight(300);
	// NOTE: Workaround for improper disabled button text colour (at least with Oxygen colours).
	// Just set the parent palette.
	QPalette pal(palette());
	pal.setColor(QPalette::Disabled, QPalette::ButtonText, pal.color(QPalette::Disabled, QPalette::WindowText));
	setPalette(pal);

	useSoloIconSet2 = false;

	meter[0] = 0;
	meter[1] = 0;
	setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding));

	//---------------------------------------------
	//    label
	//---------------------------------------------

	topRack = QPixmap(":/images/top_rack_midi.png");//":/images/top_rack.png");
	bottomRack = QPixmap(":/images/bottom_rack_midi.png");
	topRackLarge = QPixmap(":/images/top_rack_large.png");
	bottomRackLarge = QPixmap(":/images/bottom_rack_large.png");
	//Add your top image here
	toprack->setPixmap(topRackLarge);
	brack->setPixmap(bottomRackLarge);
	switch (track->type())/*{{{*/
	{
		case Track::AUDIO_OUTPUT:
			label->setObjectName("MixerAudioOutLabel");
			hasRecord = true;
			hasAux = false;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_BUSS:
			label->setObjectName("MixerAudioBussLabel");
			hasRecord = false;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_AUX:
			label->setObjectName("MixerAuxLabel");
			hasRecord = false;
			hasAux = false;
			hasStereo = true;
			hasIRoute = false;
			hasORoute = true;
			break;
		case Track::WAVE:
			label->setObjectName("MixerWaveLabel");
			hasRecord = true;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_INPUT:
			label->setObjectName("MixerAudioInLabel");
			hasRecord = false;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_SOFTSYNTH:
			label->setObjectName("MixerSynthLabel");
			m_btnAux->toggle(); //Collapse the box by default
			hasRecord = true;
			hasAux = true;
			hasStereo = false;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::MIDI:
		{
			label->setObjectName("MidiTrackLabel");
			//m_btnStereo->setIcon(QIcon(*mixer_blank_OffIcon));
			hasRecord = true;
			hasAux = true; //Used for controller knobs in midi
			hasStereo = true;
			hasIRoute = true;
			hasORoute = false;
		}
			break;
		case Track::DRUM:
		{
			label->setObjectName("MidiDrumTrackLabel");
			m_btnStereo->setIcon(*mixer_blank_OffIcon);
			hasRecord = true;
			hasAux = true; //Used for controller knobs in midi
			hasStereo = false;
			hasIRoute = true;
			hasORoute = false;
		}
			break;
	}/*}}}*/

	//if(hasAux)
	//	m_tabWidget->addTab(m_auxTab, QString(tr("Aux")));
	//if(!track->isMidiTrack())
	//	m_tabWidget->addTab(m_fxTab, QString(tr("FX")));
	
	//m_tabWidget->setCurrentIndex(track->mixerTab());
	//connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	//printf("Strip::Strip w:%d frw:%d layoutmarg:%d lx:%d ly:%d lw:%d lh:%d\n", STRIP_WIDTH, frameWidth(), layout->margin(), label->x(), label->y(), label->width(), label->height());

	label->setTextFormat(Qt::PlainText);

	label->setAlignment(Qt::AlignCenter);
	label->setWordWrap(false);
	label->setAutoFillBackground(true);
	label->setLineWidth(2);
	label->setFrameStyle(Sunken | StyledPanel);

	//label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));

	if(track->collapsed())
		m_btnAux->toggle();//setChecked(!track->collapsed());
	setLabelText();

}

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
{
}

void Strip::layoutUi()/*{{{*/
{
	//if (MixerStripBase->objectName().isEmpty())
	//	MixerStripBase->setObjectName(QString::fromUtf8("MixerStripBase"));
	//resize(213, 441);
	QSize iconSize = QSize(22, 20);
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);
	
	m_mainVBoxLayout = new QVBoxLayout(this);
	m_mainVBoxLayout->setSpacing(0);
	m_mainVBoxLayout->setContentsMargins(0, 0, 0, 0);
	m_mainVBoxLayout->setObjectName(QString::fromUtf8("m_mainVBoxLayout"));

	toprack = new QLabel(this);
	toprack->setObjectName(QString::fromUtf8("toprack"));

	m_mainVBoxLayout->addWidget(toprack);

	label = new QLabel(this);
	label->setObjectName(QString::fromUtf8("label"));
	
	m_mainVBoxLayout->addWidget(label);
	
	//m_mainVBoxLayout->addItem(new QSpacerItem(0, 4));

	m_mainVBoxLayout->addItem(new QSpacerItem(0, 4));

	horizontalLayout = new QHBoxLayout();
	horizontalLayout->setContentsMargins(0, 0, 0, 0);
	horizontalLayout->setSpacing(0);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	
	m_mixerBase = new QWidget(this);
	m_mixerBase->setObjectName(QString::fromUtf8("m_mixerBase"));
	
	m_mixerBox = new QVBoxLayout(m_mixerBase);
	m_mixerBox->setObjectName(QString::fromUtf8("m_mixerBox"));
	m_mixerBox->setContentsMargins(0, 0, 0, 0);
	m_mixerBox->setSpacing(0);
	
	horizontalLayout_2 = new QHBoxLayout();
//#ifndef Q_OS_MAC
	horizontalLayout_2->setSpacing(0);
//#endif
	horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
	horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));

	m_vuBase = new QWidget(m_mixerBase);
	m_vuBase->setObjectName(QString::fromUtf8("m_vuBox"));
	QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(1);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_vuBase->sizePolicy().hasHeightForWidth());
	m_vuBase->setSizePolicy(sizePolicy);
	//m_vuBase->setMinimumSize(QSize(55, 0));
	m_vuBase->setMaximumSize(QSize(55, 2048));

	m_vuBox = new QHBoxLayout(m_vuBase);
	m_vuBox->setObjectName(QString::fromUtf8("m_vuBox"));
	m_vuBox->setContentsMargins(3, 0, 0, 0);
	m_vuBox->setSpacing(0);
	
	horizontalLayout_2->addWidget(m_vuBase);
	
	m_buttonBase = new QWidget(m_mixerBase);
	m_buttonBase->setObjectName(QString::fromUtf8("m_buttonBase"));
	m_buttonBase->setMaximumSize(QSize(24, 2048));
	QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(1);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(m_buttonBase->sizePolicy().hasHeightForWidth());
	m_buttonBase->setSizePolicy(sizePolicy1);

	m_buttonBox = new QVBoxLayout(m_buttonBase);
	m_buttonBox->setObjectName(QString::fromUtf8("m_buttonBox"));
	m_buttonBox->setContentsMargins(0, 0, 0, 0);
	m_buttonBox->setSpacing(0);
	
	m_buttonBox->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

	m_btnAux = new QToolButton(m_buttonBase);
	m_btnAux->setObjectName("m_btnAux");
	m_btnAux->setCheckable(true);
	m_btnAux->setToolTip(tr("Toggle Mixer Rack"));
	//m_btnAux->setChecked(true);
	m_btnAux->setIcon(*mixer_resizeIconSet3);
	m_btnAux->setIconSize(iconSize);
	m_btnAux->setFixedSize(iconSize);
	m_btnAux->setAutoRaise(true);
	//m_btnAux->setIcon(*expandIcon);
	
	m_buttonBox->addWidget(m_btnAux);
	
	m_btnStereo = new QToolButton(m_buttonBase);
	m_btnStereo->setObjectName(QString::fromUtf8("m_btnStereo"));
	m_btnStereo->setIcon(*mixer_stereoIconSet3);
	m_btnStereo->setIconSize(iconSize);
	m_btnStereo->setFixedSize(iconSize);
	m_btnStereo->setAutoRaise(true);
	
	m_buttonBox->addWidget(m_btnStereo);
	
	m_btnIRoute = new QToolButton(m_buttonBase);
	m_btnIRoute->setObjectName(QString::fromUtf8("m_btnIRoute"));
	m_btnIRoute->setIcon(*mixer_inputIconSet3);
	m_btnIRoute->setIconSize(iconSize);
	m_btnIRoute->setFixedSize(iconSize);
	m_btnIRoute->setAutoRaise(true);
	
	m_buttonBox->addWidget(m_btnIRoute);
	
	m_btnORoute = new QToolButton(m_buttonBase);
	m_btnORoute->setObjectName(QString::fromUtf8("m_btnORoute"));
	m_btnORoute->setIcon(*mixer_outputIconSet3);
	m_btnORoute->setIconSize(iconSize);
	m_btnORoute->setFixedSize(iconSize);
	m_btnORoute->setAutoRaise(true);
	
	m_buttonBox->addWidget(m_btnORoute);
	
	verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	
	//m_buttonBox->addItem(verticalSpacer);
	
	m_btnPower = new TransparentToolButton(m_buttonBase);
	m_btnPower->setObjectName(QString::fromUtf8("m_btnPower"));
	m_btnPower->setIcon(*mixer_powerIconSet3);
	m_btnPower->setIconSize(iconSize);
	m_btnPower->setFixedSize(iconSize);
	m_btnPower->setAutoRaise(true);
	
	m_buttonBox->addWidget(m_btnPower);
	
	m_btnRecord = new TransparentToolButton(m_buttonBase);
	m_btnRecord->setObjectName(QString::fromUtf8("m_btnRecord"));
	m_btnRecord->setIcon(*mixer_recordIconSet3);
	m_btnRecord->setIconSize(iconSize);
	m_btnRecord->setFixedSize(iconSize);
	m_btnRecord->setAutoRaise(true);
	
	
	m_btnMute = new QToolButton(m_buttonBase);
	m_btnMute->setObjectName(QString::fromUtf8("m_btnMute"));
	m_btnMute->setIcon(*mixer_muteIconSet3);
	m_btnMute->setIconSize(iconSize);
	m_btnMute->setFixedSize(iconSize);
	m_btnMute->setAutoRaise(true);
	
	m_btnSolo = new QToolButton(m_buttonBase);
	m_btnSolo->setObjectName(QString::fromUtf8("m_btnSolo"));
	m_btnSolo->setIcon(*mixer_soloIconSet3);
	m_btnSolo->setIconSize(iconSize);
	m_btnSolo->setFixedSize(iconSize);
	m_btnSolo->setAutoRaise(true);
	
	m_buttonBox->addWidget(m_btnRecord);
	m_buttonBox->addWidget(m_btnMute);
	m_buttonBox->addWidget(m_btnSolo);
	
	
	horizontalLayout_2->addWidget(m_buttonBase);
	
	//m_tabWidget = new QTabWidget(m_mixerBase);
	//m_tabWidget->setObjectName(QString::fromUtf8("m_tabWidget"));
	//m_tabWidget->setTabPosition(QTabWidget::North);
	//m_tabWidget->setIconSize(QSize(0, 0));
	
	//m_auxTab = new QWidget();
	//m_auxTab->setObjectName(QString::fromUtf8("auxTab"));
	
	//m_auxTabLayout = new QVBoxLayout(m_auxTab);
	//m_auxTabLayout->setSpacing(0);
	//m_auxTabLayout->setContentsMargins(0, 0, 0, 0);
	//m_auxTabLayout->setObjectName(QString::fromUtf8("auxTabLayout"));
	
	m_mixerBox->addLayout(horizontalLayout_2);
	
	m_panBase = new QWidget(m_mixerBase);
	m_panBase->setObjectName(QString::fromUtf8("m_panBase"));
	
	m_panBox = new QVBoxLayout(m_panBase);
	m_panBox->setObjectName(QString::fromUtf8("m_panBox"));
	m_panBox->setSpacing(0);
	m_panBox->setContentsMargins(2, 0, 0, 0);
	m_panBox->addItem(new QSpacerItem(0, 4));
	
	m_mixerBox->addWidget(m_panBase);
	
	m_autoBox = new QVBoxLayout();
	m_autoBox->setObjectName(QString::fromUtf8("m_autoBox"));
	m_autoBox->setSpacing(0);
	m_autoBox->setContentsMargins(0, 0, 0, -1);
	
	m_mixerBox->addLayout(m_autoBox);
	
	
	horizontalLayout->addWidget(m_mixerBase);
	
	//m_auxScroll = new QScrollArea(m_auxTab);
	//m_auxScroll->setObjectName(QString::fromUtf8("m_auxScroll"));
	//QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
	///sizePolicy2.setHorizontalStretch(1);
	//sizePolicy2.setVerticalStretch(0);
	//sizePolicy2.setHeightForWidth(m_auxScroll->sizePolicy().hasHeightForWidth());
	//m_auxScroll->setSizePolicy(sizePolicy2);
	//m_auxScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//m_auxScroll->setWidgetResizable(true);
	//m_auxScroll->setAlignment(Qt::AlignCenter);
	//m_auxScroll->setAutoFillBackground(true);
	//m_auxScroll->setMinimumSize(QSize(95,0));
	
	//m_auxBase = new QFrame(this);
	/*switch (track->type())//{{{
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
	}//}}}*/
	//QSizePolicy auxSizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//auxSizePolicy2.setHorizontalStretch(1);
	//auxSizePolicy2.setVerticalStretch(0);
	//auxSizePolicy2.setHeightForWidth(m_auxBase->sizePolicy().hasHeightForWidth());
	
	//m_auxBox = new QVBoxLayout(m_auxBase);
	//m_auxBox->setObjectName(QString::fromUtf8("m_auxBox"));
	//m_auxBox->setContentsMargins(0, -1, 0, -1);
	//m_auxBox->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	//m_auxScroll->setWidget(m_auxBase);
	

	//m_auxTabLayout->addWidget(m_auxScroll);
	//m_fxTab = new QWidget();
	//m_fxTab->setObjectName(QString::fromUtf8("fxTab"));
	//rackBox = new QVBoxLayout(m_fxTab);
	//rackBox->setSpacing(0);
	//rackBox->setContentsMargins(0, 0, 0, 0);
	//rackBox->setObjectName(QString::fromUtf8("rackBox"));
	
//	m_tabWidget->addTab(m_auxTab, QString(tr("Aux")));
//	m_tabWidget->addTab(m_fxTab, QString(tr("FX")));
	//horizontalLayout->addWidget(m_tabWidget);
	
	m_effectsRack = new TrackEffects(track, this);
	m_effectsRack->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	connect(song, SIGNAL(songChanged(int)), m_effectsRack, SLOT(songChanged(int)));
	horizontalLayout->addWidget(m_effectsRack);
	
	horizontalLayout->setStretch(0, 2);
	
	m_mainVBoxLayout->addLayout(horizontalLayout);

	brack = new QLabel(this);
    brack->setObjectName(QString::fromUtf8("brack"));

	m_mainVBoxLayout->addWidget(brack);
	connect(m_btnAux, SIGNAL(toggled(bool)), this, SLOT(toggleAuxPanel(bool)));
}/*}}}*/

/*void Strip::tabChanged(int tab)
{
	if(track)
		track->setMixerTab(tab);
}*/

/**
 * @param t Track object.
 * NOTE: The track being passed in MUST be of the same type as the current 
 * track. This function is only currently used to manage Master
 */
bool Strip::setTrack(Track* t)
{
	Track::TrackType type = (Track::TrackType)m_type;
	if(t && t->type() == type)
	{
		track = t;
		m_type = t->type();

		if(m_effectsRack)
		{
			m_effectsRack->hide();
			delete m_effectsRack;
		}
		m_effectsRack = new TrackEffects(track, this);
		m_effectsRack->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		m_effectsRack->setVisible(!track->collapsed());
		connect(song, SIGNAL(songChanged(int)), m_effectsRack, SLOT(songChanged(int)));
		horizontalLayout->addWidget(m_effectsRack);
	
		trackChanged();
		return true;
	}
	return false;
}

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Strip::setRecordFlag(bool flag)
{
	if (hasRecord)
	{
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(flag);
		m_btnRecord->blockSignals(false);
	}
}

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Strip::resetPeaks()
{
	track->resetPeaks();
}

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void Strip::recordToggled(bool val)
{
	if (track->type() == Track::AUDIO_OUTPUT)
	{
		if (val && track->recordFlag() == false)
		{
			oom->bounceToFile((AudioOutput*) track);
		}
		audio->msgSetRecord((AudioOutput*) track, val);
		if (!((AudioOutput*) track)->recFile())
			m_btnRecord->setChecked(false);
		return;
	}
	song->setRecordFlag(track, val);
}
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Strip::heartBeat()
{
}

//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void Strip::setLabelFont()
{
	return;
	// Use the new font #6 I created just for these labels (so far).
	// Set the label's font.
	label->setFont(config.fonts[6]);
	// Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
	autoAdjustFontSize(label, label->text(), false, true, config.fonts[6].pointSize(), 5);
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void Strip::setLabelText()
{
	QString trackName = track->name();
	if(m_collapsed)
	{
		if (track->name().length() > 8)
			trackName = track->name().mid(0, 7) + "..";
	}

	label->setText(trackName);
	label->setToolTip(track->name());
}

void Strip::toggleAuxPanel(bool collapse)
{
	m_effectsRack->setVisible(!collapse);
	//m_tabWidget->setVisible(!collapse);
	m_collapsed = collapse;
	if(!collapse)
	{
		setMaximumWidth(212);
		toprack->setPixmap(topRackLarge);
		brack->setPixmap(bottomRackLarge);
	}
	else
	{
		setMaximumWidth(81);
		toprack->setPixmap(topRack);
		brack->setPixmap(bottomRack);
	}
	track->setCollapsed(m_collapsed);

	m_btnAux->blockSignals(true);
	m_btnAux->setChecked(collapse);
	m_btnAux->blockSignals(false);
	setLabelText();
}

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void Strip::muteToggled(bool val)
{
	track->setMute(val);
	song->update(SC_MUTE);
}

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void Strip::soloToggled(bool val)
{
	audio->msgSetSolo(track, val);
	song->update(SC_SOLO);
}

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void Strip::setAutomationType(int t, int)
{
    if (track->isMidiTrack())
    {
		Track* input = track->inputTrack();
		if(input)
		{
        	input->setAutomationType(AutomationType(t));
		}
        if (track->wantsAutomation())
        {
            AudioTrack* atrack = ((MidiTrack*)track)->getAutomationTrack();
            if (atrack)
                atrack->setAutomationType(AutomationType(t));
        }
    }
    else
        track->setAutomationType(AutomationType(t));

	song->update(SC_AUTOMATION);
}

