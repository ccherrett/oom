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

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, Track* t)
: QFrame(parent)
{
	track = t;
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
	setMaximumWidth(182);
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
			m_auxContainer->setObjectName("MixerAudioOutAuxbox");
			m_btnAux->toggle(); //Collapse the box by default
			//m_btnAux->setEnabled(false);
			hasRecord = true;
			hasAux = false;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_BUSS:
			label->setObjectName("MixerAudioBussLabel");
			m_auxContainer->setObjectName("MixerAudioBussAuxbox");
			hasRecord = false;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_AUX:
			label->setObjectName("MixerAuxLabel");
			m_auxContainer->setObjectName("MixerAuxAuxbox");
			m_btnAux->toggle(); //Collapse the box by default
			//m_btnAux->setEnabled(false);
			hasRecord = false;
			hasAux = false;
			hasStereo = true;
			hasIRoute = false;
			hasORoute = true;
			break;
		case Track::WAVE:
			label->setObjectName("MixerWaveLabel");
			m_auxContainer->setObjectName("MixerWaveAuxbox");
			hasRecord = true;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_INPUT:
			label->setObjectName("MixerAudioInLabel");
			m_auxContainer->setObjectName("MixerAudioInAuxbox");
			hasRecord = false;
			hasAux = true;
			hasStereo = true;
			hasIRoute = true;
			hasORoute = true;
			break;
		case Track::AUDIO_SOFTSYNTH:
			label->setObjectName("MixerSynthLabel");
			m_auxContainer->setObjectName("MixerSynthAuxbox");
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
			m_auxContainer->setObjectName("MidiTrackAuxbox");
			//topRack = QPixmap(":/images/top_rack_midi.png");
			//bottomRack = QPixmap(":/images/bottom_rack_midi.png");
			m_btnStereo->setVisible(false);
			hasRecord = true;
			hasAux = true; //Used for controller knobs in midi
			hasStereo = false;
			hasIRoute = true;
			hasORoute = false;
		}
			break;
		case Track::DRUM:
		{
			label->setObjectName("MidiDrumTrackLabel");
			m_auxContainer->setObjectName("MidiDrumTrackAuxbox");
			//topRack = QPixmap(":/images/top_rack_midi.png");
			//bottomRack = QPixmap(":/images/bottom_rack_midi.png");
			m_btnStereo->setVisible(false);
			hasRecord = true;
			hasAux = true; //Used for controller knobs in midi
			hasStereo = false;
			hasIRoute = true;
			hasORoute = false;
		}
			break;
	}/*}}}*/

	if(hasAux)
		m_tabWidget->addTab(auxTab, QString(tr("Aux")));
	if(!track->isMidiTrack())
		m_tabWidget->addTab(fxTab, QString(tr("FX")));
	//printf("Strip::Strip w:%d frw:%d layoutmarg:%d lx:%d ly:%d lw:%d lh:%d\n", STRIP_WIDTH, frameWidth(), layout->margin(), label->x(), label->y(), label->width(), label->height());

	label->setTextFormat(Qt::PlainText);

	label->setAlignment(Qt::AlignCenter);
	label->setWordWrap(false);
	label->setAutoFillBackground(true);
	label->setLineWidth(2);
	label->setFrameStyle(Sunken | StyledPanel);

	//label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));

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

	//m_rackBox = new QWidget(this);
	//m_rackBox->setObjectName(QString::fromUtf8("m_rackBox"));
	/*rackBox = new QVBoxLayout(m_rackBox);
	rackBox->setObjectName(QString::fromUtf8("rackBox"));
	rackBox->setSpacing(0);
	rackBox->setContentsMargins(0, 0, 0, 0);
	
	m_mainVBoxLayout->addWidget(m_rackBox);*/
	
	m_mainVBoxLayout->addItem(new QSpacerItem(0, 4));

	horizontalLayout = new QHBoxLayout();
	horizontalLayout->setContentsMargins(0, 0, 0, 0);
	horizontalLayout->setSpacing(0);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	m_mixerBox = new QWidget(this);
	m_mixerBox->setObjectName(QString::fromUtf8("m_mixerBox"));
	verticalLayout_5 = new QVBoxLayout(m_mixerBox);
	verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
	verticalLayout_5->setContentsMargins(0, 0, 0, 0);
	verticalLayout_5->setSpacing(0);
	horizontalLayout_2 = new QHBoxLayout();
//#ifndef Q_OS_MAC
	horizontalLayout_2->setSpacing(0);
//#endif
	horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
	horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
	m_vuContainer = new QWidget(m_mixerBox);
	m_vuContainer->setObjectName(QString::fromUtf8("m_vuBox"));
	QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(1);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_vuContainer->sizePolicy().hasHeightForWidth());
	m_vuContainer->setSizePolicy(sizePolicy);
	//m_vuContainer->setMinimumSize(QSize(55, 0));
	m_vuContainer->setMaximumSize(QSize(55, 2048));
	m_vuBox = new QHBoxLayout(m_vuContainer);
	m_vuBox->setObjectName(QString::fromUtf8("m_vuBox"));
	m_vuBox->setContentsMargins(3, 0, 0, 0);
	m_vuBox->setSpacing(0);
	
	horizontalLayout_2->addWidget(m_vuContainer);
	
	m_buttonStrip = new QWidget(m_mixerBox);
	m_buttonStrip->setObjectName(QString::fromUtf8("m_buttonStrip"));
	m_buttonStrip->setMaximumSize(QSize(24, 2048));
	QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(1);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(m_buttonStrip->sizePolicy().hasHeightForWidth());
	m_buttonStrip->setSizePolicy(sizePolicy1);
	verticalLayout_6 = new QVBoxLayout(m_buttonStrip);
	verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
	verticalLayout_6->setContentsMargins(0, 0, 0, 0);
	m_btnAux = new QToolButton(m_buttonStrip);
	m_btnAux->setObjectName(QString::fromUtf8("m_btnAux"));
	m_btnAux->setCheckable(true);
	m_btnAux->setChecked(true);
	m_btnAux->setIcon(*expandIcon);
	
	verticalLayout_6->addWidget(m_btnAux);
	
	m_btnStereo = new QToolButton(m_buttonStrip);
	m_btnStereo->setObjectName(QString::fromUtf8("m_btnStereo"));
	
	verticalLayout_6->addWidget(m_btnStereo);
	
	m_btnIRoute = new QToolButton(m_buttonStrip);
	m_btnIRoute->setObjectName(QString::fromUtf8("m_btnIRoute"));
	
	verticalLayout_6->addWidget(m_btnIRoute);
	
	m_btnORoute = new QToolButton(m_buttonStrip);
	m_btnORoute->setObjectName(QString::fromUtf8("m_btnORoute"));
	
	verticalLayout_6->addWidget(m_btnORoute);
	
	verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	
	verticalLayout_6->addItem(verticalSpacer);
	
	m_btnPower = new TransparentToolButton(m_buttonStrip);
	m_btnPower->setObjectName(QString::fromUtf8("m_btnPower"));
	
	verticalLayout_6->addWidget(m_btnPower);
	
	m_btnRecord = new TransparentToolButton(m_buttonStrip);
	m_btnRecord->setObjectName(QString::fromUtf8("m_btnRecord"));
	
	verticalLayout_6->addWidget(m_btnRecord);
	
	m_btnMute = new QToolButton(m_buttonStrip);
	m_btnMute->setObjectName(QString::fromUtf8("m_btnMute"));
	
	verticalLayout_6->addWidget(m_btnMute);
	
	m_btnSolo = new QToolButton(m_buttonStrip);
	m_btnSolo->setObjectName(QString::fromUtf8("m_btnSolo"));
	
	verticalLayout_6->addWidget(m_btnSolo);
	
	
	horizontalLayout_2->addWidget(m_buttonStrip);
	
	m_tabWidget = new QTabWidget(m_mixerBox);
	m_tabWidget->setObjectName(QString::fromUtf8("m_tabWidget"));
	m_tabWidget->setTabPosition(QTabWidget::North);
	m_tabWidget->setIconSize(QSize(0, 0));
	auxTab = new QWidget();
	auxTab->setObjectName(QString::fromUtf8("auxTab"));
	auxTabLayout = new QVBoxLayout(auxTab);
	auxTabLayout->setSpacing(0);
	auxTabLayout->setContentsMargins(0, 0, 0, 0);
	auxTabLayout->setObjectName(QString::fromUtf8("auxTabLayout"));
	
	verticalLayout_5->addLayout(horizontalLayout_2);
	
	m_panContainer = new QWidget(m_mixerBox);
	m_panContainer->setObjectName(QString::fromUtf8("m_panContainer"));
	m_panBox = new QVBoxLayout(m_panContainer);
	m_panBox->setObjectName(QString::fromUtf8("m_panBox"));
	m_panBox->setSpacing(0);
	m_panBox->setContentsMargins(2, 0, 0, 0);
	m_panBox->addItem(new QSpacerItem(0, 4));
	
	verticalLayout_5->addWidget(m_panContainer);
	
	m_autoBox = new QVBoxLayout();
	m_autoBox->setObjectName(QString::fromUtf8("m_autoBox"));
	m_autoBox->setSpacing(0);
	m_autoBox->setContentsMargins(0, 0, 0, -1);
	
	verticalLayout_5->addLayout(m_autoBox);
	
	
	horizontalLayout->addWidget(m_mixerBox);
	
	m_auxScroll = new QScrollArea(auxTab);
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
	
	m_auxContainer = new QFrame(this);
	switch (track->type())/*{{{*/
	{
		case Track::AUDIO_OUTPUT:
			m_auxContainer->setObjectName("MixerAudioOutAuxbox");
			break;
		case Track::AUDIO_BUSS:
			m_auxContainer->setObjectName("MixerAudioBussAuxbox");
			break;
		case Track::AUDIO_AUX:
			m_auxContainer->setObjectName("MixerAuxAuxbox");
			break;
		case Track::WAVE:
			m_auxContainer->setObjectName("MixerWaveAuxbox");
			break;
		case Track::AUDIO_INPUT:
			m_auxContainer->setObjectName("MixerAudioInAuxbox");
			break;
		case Track::AUDIO_SOFTSYNTH:
			m_auxContainer->setObjectName("MixerSynthAuxbox");
			break;
		case Track::MIDI:
			m_auxContainer->setObjectName("MidiTrackAuxbox");
			break;
		case Track::DRUM:
			m_auxContainer->setObjectName("MidiDrumTrackAuxbox");
			break;
	}/*}}}*/
	QSizePolicy auxSizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
	auxSizePolicy2.setHorizontalStretch(1);
	auxSizePolicy2.setVerticalStretch(0);
	auxSizePolicy2.setHeightForWidth(m_auxContainer->sizePolicy().hasHeightForWidth());
	//m_auxContainer->setObjectName("m_auxContainer");
	//m_auxContainer->setGeometry(QRect(0, 0, 80, 330));
	
	m_auxBox = new QVBoxLayout(m_auxContainer);
	m_auxBox->setObjectName(QString::fromUtf8("m_auxBox"));
	m_auxBox->setContentsMargins(0, -1, 0, -1);
	m_auxBox->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	m_auxScroll->setWidget(m_auxContainer);
	

	auxTabLayout->addWidget(m_auxScroll);
	fxTab = new QWidget();
	fxTab->setObjectName(QString::fromUtf8("fxTab"));
	rackBox = new QVBoxLayout(fxTab);
	rackBox->setSpacing(0);
	rackBox->setContentsMargins(0, 0, 0, 0);
	rackBox->setObjectName(QString::fromUtf8("rackBox"));
	
//	m_tabWidget->addTab(auxTab, QString(tr("Aux")));
//	m_tabWidget->addTab(fxTab, QString(tr("FX")));
	
	horizontalLayout->addWidget(m_tabWidget);
	
	horizontalLayout->setStretch(0, 2);
	
	m_mainVBoxLayout->addLayout(horizontalLayout);

	brack = new QLabel(this);
    brack->setObjectName(QString::fromUtf8("brack"));

	m_mainVBoxLayout->addWidget(brack);
	connect(m_btnAux, SIGNAL(toggled(bool)), this, SLOT(toggleAuxPanel(bool)));
}/*}}}*/

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
// Added by Tim. p3.3.9

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

void Strip::toggleAuxPanel(bool open)
{
	m_tabWidget->setVisible(open);
	m_collapsed = !open;
	if(open)
	{
		setMaximumWidth(182);
		toprack->setPixmap(topRackLarge);
		brack->setPixmap(bottomRackLarge);
	}
	else
	{
		setMaximumWidth(81);
		toprack->setPixmap(topRack);
		brack->setPixmap(bottomRack);
	}
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
	track->setAutomationType(AutomationType(t));
	song->update(SC_AUTOMATION);
}

