//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include <fastlog.h>
#include <math.h>

#include "gconfig.h"
#include "globals.h"
#include "config.h"
#include "track.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "plugin.h"
#include "knob.h"
#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "shortcuts.h"
#include "scrollscale.h"
#include "xml.h"
#include "midi.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
#include "midictrl.h"
#include "comment.h"
#include "node.h"
#include "instruments/minstrument.h"
#include "Composer.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "menulist.h"
#include "midimonitor.h"
#include "ComposerCanvas.h"
#include "trackheader.h"
#include "slider.h"
#include "mixer/meter.h"
#include "CreateTrackDialog.h"
#include "AutomationMenu.h"
#include "TrackInstrumentMenu.h"
#include "TrackEffects.h"

static QString styletemplate = "QLineEdit { border-width:1px; border-radius: 0px; border-image: url(:/images/frame.png) 4; border-top-color: #1f1f22; border-bottom-color: #505050; color: #%1; background-color: #%2; font-family: fixed-width; font-weight: bold; font-size: 15px; padding-left: 15px; }";
static QString trackHeaderStyle = "QFrame#TrackHeader { border-bottom: 1px solid #888888; border-right: 1px solid #888888; border-left: 1px solid #888888; background-color: #2e2e2e; }";
static QString trackHeaderStyleSelected = "QFrame#TrackHeader { border-bottom: 1px solid #888888; border-right: 1px solid #888888; border-left: 1px solid #888888; background-color: #171717; }";
static QString lineStyleTemplate = "QFrame { border: 0px; background-color: %1; }";

static const int TRACK_HEIGHT_3 = 100;
static const int TRACK_HEIGHT_4 = 180;
static const int TRACK_HEIGHT_5 = 320;
static const int TRACK_HEIGHT_6 = 640;

TrackHeader::TrackHeader(Track* t, QWidget* parent)
: QFrame(parent)
{
	setupUi(this);
	m_track = 0;
	m_effects = 0;
	m_pan = 0;
	m_slider = 0;
	m_tracktype = 0;
	m_channels = 2;
	setupStyles();
	resizeFlag = false;
	mode = NORMAL;
	inHeartBeat = true;
	m_editing = false;
	m_midiDetect = false;
	m_processEvents = true;
	m_meterVisible = true;
	m_sliderVisible = true;
	m_toolsVisible = false;
	m_nopopulate = false;
	panVal = 0.0;
	volume = 0.0;
	setObjectName("TrackHeader");
	setFrameStyle(QFrame::StyledPanel|QFrame::Raised);
	m_buttonHBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	m_buttonVBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	m_panBox->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	setAcceptDrops(false);
	setMouseTracking(true);
	
	m_trackName->installEventFilter(this);
	m_trackName->setAcceptDrops(false);
	m_btnSolo->setAcceptDrops(false);
	m_btnSolo->setIcon(*solo_trackIconSet3);
	m_btnRecord->setAcceptDrops(false);
	m_btnRecord->setIcon(*record_trackIconSet3);
	m_btnMute->setAcceptDrops(false);
	m_btnMute->setIcon(*mute_trackIconSet3);
	m_btnInstrument->setAcceptDrops(false);
	m_btnAutomation->setAcceptDrops(false);
	m_btnReminder1->setAcceptDrops(false);
	m_btnReminder2->setAcceptDrops(false);
	m_btnReminder3->setAcceptDrops(false);
	m_btnReminder1->setIcon(*reminder1IconSet3);
	m_btnReminder2->setIcon(*reminder2IconSet3);
	m_btnReminder3->setIcon(*reminder3IconSet3);

	setTrack(t);

	connect(m_trackName, SIGNAL(editingFinished()), this, SLOT(updateTrackName()));
	connect(m_trackName, SIGNAL(returnPressed()), this, SLOT(updateTrackName()));
	connect(m_trackName, SIGNAL(textEdited(QString)), this, SLOT(setEditing()));
	connect(m_btnRecord, SIGNAL(toggled(bool)), this, SLOT(toggleRecord(bool)));
	connect(m_btnMute, SIGNAL(toggled(bool)), this, SLOT(toggleMute(bool)));
	connect(m_btnSolo, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
	connect(m_btnReminder1, SIGNAL(toggled(bool)), this, SLOT(toggleReminder1(bool)));
	connect(m_btnReminder2, SIGNAL(toggled(bool)), this, SLOT(toggleReminder2(bool)));
	connect(m_btnReminder3, SIGNAL(toggled(bool)), this, SLOT(toggleReminder3(bool)));
	connect(m_btnAutomation, SIGNAL(clicked()), this, SLOT(generateAutomationMenu()));
	connect(m_btnInstrument, SIGNAL(clicked()), this, SLOT(generateInstrumentMenu()));
	//Let header list control this for now
	//connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	connect(song, SIGNAL(playChanged(bool)), this, SLOT(resetPeaksOnPlay(bool)));
	connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
	inHeartBeat = false;
}

TrackHeader::~TrackHeader()
{
	m_processEvents = false;
}

//Public member functions

void TrackHeader::stopProcessing()/*{{{*/
{
	m_processEvents = false;
}/*}}}*/

void TrackHeader::startProcessing()/*{{{*/
{
	m_processEvents = true;
}/*}}}*/

bool TrackHeader::isSelected()/*{{{*/
{
	if(!m_track)
		return false;
	return m_track->selected();
}/*}}}*/

void TrackHeader::setTrack(Track* track)/*{{{*/
{
	m_processEvents = false;
	if(m_slider)
		delete m_slider;
	Meter* m;
	while(!meter.isEmpty() && (m = meter.takeAt(0)) != 0)
	{
		//printf("Removing meter\n");
		m->hide();
		delete m;
	}
	if(m_effects)
	{
		m_effects->hide();
		delete m_effects;
	}
	m_track = track;
	if(!m_track || !track)
		return;
	Track::TrackType type = m_track->type();
	m_tracktype = (int)type;
	initPan();
	initVolume();
	
	m_effects = new TrackEffects(m_track, this);
	m_effects->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	m_effects->installEventFilter(this);
	m_controlsBox->addWidget(m_effects);
	connect(song, SIGNAL(songChanged(int)), m_effects, SLOT(songChanged(int)));

	m_trackName->setText(m_track->name());
	m_trackName->setReadOnly(true);

	if(m_pan)
		m_pan->setAcceptDrops(false);
	setSelected(m_track->selected(), true);
	if(m_track->height() < MIN_TRACKHEIGHT)
	{
		setFixedHeight(MIN_TRACKHEIGHT);
		m_track->setHeight(MIN_TRACKHEIGHT);
	}
	else
	{
		setFixedHeight(m_track->height());
	}
    if(m_track->isMidiTrack())
	{
		MidiPort *mp = oomMidiPorts.value(((MidiTrack*)m_track)->outPortId());
		m_btnInstrument->setIcon(*instrument_trackIconSet3);
		if(mp)
			m_btnInstrument->setToolTip(QString(tr("Change Instrument: ")).append(mp->instrument()->iname()));
		else
			m_btnInstrument->setToolTip(tr("Change Instrument"));
		//if(m_track->wantsAutomation())
		//{
			m_btnAutomation->setIcon(*automation_trackIconSet3);
			m_btnAutomation->setToolTip(tr("Toggle Automation"));
		/*}
		else
		{
			m_btnAutomation->setIcon(QIcon(*input_indicator_OffIcon));
			m_btnAutomation->setToolTip("");
		}*/
	}
	else
	{
		m_btnInstrument->setIcon(*input_indicator_OffIcon);
		m_btnInstrument->setToolTip("");

		m_btnAutomation->setIcon(*automation_trackIconSet3);
		m_btnAutomation->setToolTip(tr("Toggle Automation"));
	}
	
	if(type == Track::AUDIO_BUSS || type == Track::AUDIO_INPUT || type == Track::AUDIO_AUX)
	{
		m_btnRecord->setIcon(QIcon(*input_indicator_OffIcon));
		m_btnRecord->setToolTip("");
	}
	else
	{
		m_btnRecord->setIcon(*record_trackIconSet3);
		m_btnRecord->setToolTip(tr("Record"));
	}

	m_btnRecord->blockSignals(true);
	m_btnRecord->setChecked(m_track->recordFlag());
	m_btnRecord->blockSignals(false);

	m_btnMute->blockSignals(true);
	m_btnMute->setChecked(m_track->mute());
	m_btnMute->blockSignals(false);

	m_btnSolo->blockSignals(true);
	m_btnSolo->setChecked(m_track->solo());
	m_btnSolo->blockSignals(false);

	m_btnReminder1->blockSignals(true);
	m_btnReminder1->setChecked(m_track->getReminder1());
	m_btnReminder1->blockSignals(false);
	
	m_btnReminder2->blockSignals(true);
	m_btnReminder2->setChecked(m_track->getReminder2());
	m_btnReminder2->blockSignals(false);

	m_btnReminder3->blockSignals(true);
	m_btnReminder3->setChecked(m_track->getReminder3());
	m_btnReminder3->blockSignals(false);

	m_trackName->blockSignals(true);
	m_trackName->setText(m_track->name());
	m_trackName->blockSignals(false);
	setSelected(m_track->selected());
	//setProperty("selected", m_track->selected());
	if(m_track->height() < MIN_TRACKHEIGHT)
	{
		setFixedHeight(MIN_TRACKHEIGHT);
		m_track->setHeight(MIN_TRACKHEIGHT);
	}
	else
	{
		setFixedHeight(m_track->height());
	}
	m_meterVisible = m_track->height() >= MIN_TRACKHEIGHT_VU;
	m_sliderVisible = m_track->height() >= MIN_TRACKHEIGHT_SLIDER;
	m_toolsVisible = (m_track->height() >= MIN_TRACKHEIGHT_TOOLS);
	foreach(Meter* m, meter)
		m->setVisible(m_meterVisible);
	if(m_slider)
		m_slider->setVisible(m_sliderVisible);
	if(m_pan)
		m_pan->setVisible(m_sliderVisible);
	if(m_effects)
	{
		m_effects->setVisible(m_toolsVisible);
		m_controlsBox->setEnabled(m_toolsVisible);
	}
	m_processEvents = true;
	//songChanged(-1);
}/*}}}*/

//Public slots

void TrackHeader::setSelected(bool sel, bool force)/*{{{*/
{
	bool oldsel = m_selected;
	if(!m_track)
	{
		m_selected = false;
	}
	else
	{
		m_selected = sel;
		if(m_track->selected() != sel)
			m_track->setSelected(sel);
	}
	if((m_selected != oldsel) || force)
	{
		if(m_selected)
		{
			bool usePixmap = false;
			QColor sliderBgColor = g_trackColorListSelected.value(m_track->type());/*{{{*/
   		    switch(vuColorStrip)
   		    {
   		        case 0:
   		            sliderBgColor = g_trackColorListSelected.value(m_track->type());
   		        break;
   		        case 1:
   		            //if(width() != m_width)
   		            //    m_scaledPixmap_w = m_pixmap_w->scaled(width(), 1, Qt::IgnoreAspectRatio);
   		            //m_width = width();
   		            //myPen.setBrush(m_scaledPixmap_w);
   		            //myPen.setBrush(m_trackColor);
   		            sliderBgColor = g_trackColorListSelected.value(m_track->type());
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
   		            sliderBgColor = g_trackColorListSelected.value(m_track->type());
   		            //myPen.setBrush(m_trackColor);
   		        break;
   		    }/*}}}*/
			//m_trackName->setFont();
			m_trackName->setStyleSheet(m_selectedStyle[m_tracktype]);
			setStyleSheet(trackHeaderStyleSelected);
			//m_slider->setSliderBackground(g_trackColorListSelected.value(m_track->type()));
			if(usePixmap)
				m_slider->setUsePixmap();
			else
				m_slider->setSliderBackground(sliderBgColor);
			m_colorLine->setStyleSheet(lineStyleTemplate.arg(g_trackColorListLine.value(m_track->type()).name()));
		}
		else
		{
			m_trackName->setStyleSheet(m_style[m_tracktype]);
			setStyleSheet(trackHeaderStyle);
			m_slider->setSliderBackground(g_trackColorList.value(m_track->type()));
			m_colorLine->setStyleSheet(lineStyleTemplate.arg(g_trackColorListLine.value(m_track->type()).name()));
			//m_strip->setStyleSheet("QFrame {background-color: blue;}");
		}
	}
}/*}}}*/

void TrackHeader::songChanged(int flags)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	
	if(!m_track->isMidiTrack())
	{
		if (flags & SC_CHANNELS)
		{
			//printf("TrackHeader::songChanged SC_CHANNELS\n");
			updateChannels();
		}

		if (flags & SC_CONFIG)
		{
			//printf("TrackHeader::songChanged SC_CONFIG\n");
			if(m_slider)
				m_slider->setRange(config.minSlider - 0.1, 10.0);

			for (int c = 0; c < ((AudioTrack*)m_track)->channels(); ++c)
			{
				if(!meter.isEmpty() && c > meter.size())
					meter.at(c)->setRange(config.minMeter, 10.0);
			}
		}
	}
	else
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			if (flags & SC_CHANNELS)
			{
				//printf("TrackHeader::songChanged SC_CHANNELS\n");
				updateChannels();
			}

			if (flags & SC_CONFIG)
			{
				//printf("TrackHeader::songChanged SC_CONFIG\n");
				if(m_slider)
					m_slider->setRange(config.minSlider - 0.1, 10.0);

				for (int c = 0; c < ((AudioTrack*)in)->channels(); ++c)
				{
					if(!meter.isEmpty() && c > meter.size())
						meter.at(c)->setRange(config.minMeter, 10.0);
				}
			}
		}
	}

	if (flags & SC_TRACK_MODIFIED)/*{{{*/
	{
		//printf("TrackHeader::songChanged SC_TRACK_MODIFIED updating all\n");
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(m_track->recordFlag());
		m_btnRecord->blockSignals(false);

		m_btnMute->blockSignals(true);
		m_btnMute->setChecked(m_track->mute());
		m_btnMute->blockSignals(false);

		m_btnSolo->blockSignals(true);
		m_btnSolo->setChecked(m_track->solo());
		m_btnSolo->blockSignals(false);

		m_btnReminder1->blockSignals(true);
		m_btnReminder1->setChecked(m_track->getReminder1());
		m_btnReminder1->blockSignals(false);
		
		m_btnReminder2->blockSignals(true);
		m_btnReminder2->setChecked(m_track->getReminder2());
		m_btnReminder2->blockSignals(false);

		m_btnReminder3->blockSignals(true);
		m_btnReminder3->setChecked(m_track->getReminder3());
		m_btnReminder3->blockSignals(false);

		m_trackName->blockSignals(true);
		m_trackName->setText(m_track->name());
		m_trackName->blockSignals(false);
		
		setSelected(m_track->selected());
		
		if(m_track->height() < MIN_TRACKHEIGHT)
		{
			setFixedHeight(MIN_TRACKHEIGHT);
			m_track->setHeight(MIN_TRACKHEIGHT);
		}
		else
		{
			setFixedHeight(m_track->height());
		}
		return;
	}/*}}}*/
	if (flags & SC_MUTE)
	{
		//printf("TrackHeader::songChanged SC_MUTE\n");
		m_btnMute->blockSignals(true);
		m_btnMute->setChecked(m_track->mute());
		m_btnMute->blockSignals(false);
	}
	if (flags & SC_SOLO)
	{
		//printf("TrackHeader::songChanged SC_SOLO\n");
		m_btnSolo->blockSignals(true);
		m_btnSolo->setChecked(m_track->solo());
		m_btnSolo->blockSignals(false);
	}
	if (flags & SC_RECFLAG)
	{
		//printf("TrackHeader::songChanged SC_RECFLAG\n");
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(m_track->recordFlag());
		m_btnRecord->blockSignals(false);
	}
	if (flags & SC_MIDI_TRACK_PROP)
	{
		//printf("TrackHeader::songChanged SC_MIDI_TRACK_PROP\n");
	}
	if (flags & SC_SELECTION)
	{
		//printf("TrackHeader::songChanged SC_SELECTION\n");
		setSelected(m_track->selected());
	}
}/*}}}*/

//Private slots
void TrackHeader::heartBeat()/*{{{*/
{
	if(!m_track || inHeartBeat || !m_processEvents)
		return;
	if(song->invalid)
		return;
	inHeartBeat = true;
	if(m_track->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)m_track;
		
		Track *in = m_track->inputTrack();
		if(in)
		{
			if(m_meterVisible)
			{
				for (int ch = 0; ch < ((AudioTrack*)in)->channels(); ++ch)
				{
					if (!meter.isEmpty() && ch < meter.size())
					{
						meter.at(ch)->setVal(((AudioTrack*)in)->meter(ch), ((AudioTrack*)in)->peak(ch), false);
					}
				}
			}
		}
		/*int act = track->activity();
		double dact = double(act) * (m_slider->value() / 127.0);

		if ((int) dact > track->lastActivity())
			track->setLastActivity((int) dact);

		foreach(Meter* m, meter)
			m->setVal(dact, track->lastActivity(), false);

		// Gives reasonable decay with gui update set to 20/sec.
		if (act)
			track->setActivity((int) ((double) act * 0.8));*/
		
		//int outChannel = track->outChannel();
		int outPort = track->outPort();

		MidiPort* mp = &midiPorts[outPort];

		// Check for detection of midi general activity on chosen channels...
		if(m_track->recordFlag())
		{
			int mpt = 0;
			RouteList* rl = track->inRoutes();

			ciRoute r = rl->begin();
			for (; r != rl->end(); ++r)
			{
				if (!r->isValid() || (r->type != Route::MIDI_PORT_ROUTE))
					continue;

				// NOTE: TODO: Code for channelless events like sysex,
				// ** IF we end up using the 'special channel 17' method.
				if (r->channel == -1 || r->channel == 0) 
					continue;

				// No port assigned to the device?
				mpt = r->midiPort;
				if (mpt < 0 || mpt >= MIDI_PORTS)
					continue;

				if (midiPorts[mpt].syncInfo().actDetectBits() & r->channel)
				{
					if (!m_midiDetect)
					{
						m_midiDetect = true;
						m_btnInstrument->setIcon(QIcon(*instrument_track_ActiveIcon));
						//m_btnAutomation->setIcon(QIcon(*input_indicator_OnIcon));
					}
					break;
				}
			}
			// No activity detected?
			if (r == rl->end())
			{
				if (m_midiDetect)
				{
					m_midiDetect = false;
					m_btnInstrument->setIcon(*instrument_trackIconSet3);
                    /*if (m_track->wantsAutomation())
                        m_btnAutomation->setIcon(*automation_trackIconSet3);
                    else
                        m_btnAutomation->setIcon(QIcon(*input_indicator_OffIcon));*/
				}
			}
		}
		else
		{
			m_btnInstrument->setIcon(*instrument_trackIconSet3);
            /*if (m_track->wantsAutomation())
                m_btnAutomation->setIcon(*automation_trackIconSet3);
            else
                m_btnAutomation->setIcon(QIcon(*input_indicator_OffIcon));*/
		}
		if(mp)
			m_btnInstrument->setToolTip(QString(tr("Change Instrument: ")).append(mp->instrument()->iname()));
		else
			m_btnInstrument->setToolTip(tr("Change Instrument"));
	}
	else
	{
		if(m_meterVisible)
		{
			for (int ch = 0; ch < ((AudioTrack*)m_track)->channels(); ++ch)
			{
				if (!meter.isEmpty() && ch < meter.size())
				{
					meter.at(ch)->setVal(((AudioTrack*)m_track)->meter(ch), ((AudioTrack*)m_track)->peak(ch), false);
				}
			}
		}
	}

	updateVolume();
	updatePan();
	inHeartBeat = false;
}/*}}}*/

void TrackHeader::generatePopupMenu()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	TrackList selectedTracksList = song->getSelectedTracks();
	bool multipleSelectedTracks = false;
	if (selectedTracksList.size() > 1)
	{
		multipleSelectedTracks = true;
	}

	QMenu* p = new QMenu;
	//Part Color menu
	QMenu* colorPopup = p->addMenu(tr("Default Part Color"));/*{{{*/

	QMenu* colorSub;
	for (int i = 0; i < NUM_PARTCOLORS; ++i)
	{
		QString colorname(config.partColorNames[i]);
		if(colorname.contains("menu:", Qt::CaseSensitive))
		{
			colorSub = colorPopup->addMenu(colorname.replace("menu:", ""));
		}
		else
		{
			if(m_track->getDefaultPartColor() == i)
			{
				colorname = QString(config.partColorNames[i]);
				colorPopup->setIcon(ComposerCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true));
				colorPopup->setTitle(colorSub->title()+": "+colorname);
	
				colorname = QString("* "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(ComposerCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true), colorname);
				act_color->setData(20 + i);
			}
			else
			{
				colorname = QString("     "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(ComposerCanvas::colorRect(config.partColors[i], config.partWaveColors[i], 80, 80), colorname);
				act_color->setData(20 + i);
			}
		}
	}/*}}}*/
	if(m_track && m_track->type() == Track::WAVE)
		p->addAction(tr("Import Audio File"))->setData(1);

	//Add Track menu
	QAction* beforeTrack = p->addAction(tr("Add Track Before"));
	beforeTrack->setData(10000);
	QAction* afterTrack = p->addAction(tr("Add Track After"));
	afterTrack->setData(12000);

	if(m_track->id() != song->masterId() && m_track->id() != song->oomVerbId())
	{
		p->addAction(QIcon(*midi_edit_instrumentIcon), tr("Rename Track"))->setData(15);
		p->addSeparator();
		p->addAction(QIcon(*automation_clear_dataIcon), tr("Delete Track"))->setData(0);
		p->addSeparator();
	}
	
	QAction* selectAllAction = p->addAction(tr("Select All Tracks"));
	selectAllAction->setData(4);
	selectAllAction->setShortcut(shortcuts[SHRT_SEL_ALL_TRACK].key);

	QString currentTrackHeightString = tr("Track Height") + ": ";
	if (m_track->height() == DEFAULT_TRACKHEIGHT)
	{
		currentTrackHeightString += tr("Default");
	}
	if (m_track->height() == MIN_TRACKHEIGHT)
	{
		currentTrackHeightString += tr("Compact");
	}
	if (m_track->height() == TRACK_HEIGHT_3)
	{
		currentTrackHeightString += tr("3");
	}
	if (m_track->height() == TRACK_HEIGHT_4)
	{
		currentTrackHeightString += tr("4");
	}
	if (m_track->height() == TRACK_HEIGHT_5)
	{
		currentTrackHeightString += tr("5");
	}
	if (m_track->height() == TRACK_HEIGHT_6)
	{
		currentTrackHeightString += tr("6");
	}
	if (m_track->height() == oom->composer->getCanvas()->height())
	{
		currentTrackHeightString += tr("Full Screen");
	}

	QMenu* trackHeightsMenu = p->addMenu(currentTrackHeightString);

	trackHeightsMenu->addAction(tr("Compact"))->setData(7);
	trackHeightsMenu->addAction(tr("Default"))->setData(6);
	trackHeightsMenu->addAction("3")->setData(8);
	trackHeightsMenu->addAction("4")->setData(9);
	trackHeightsMenu->addAction("5")->setData(10);
	trackHeightsMenu->addAction("6")->setData(11);
	trackHeightsMenu->addAction(tr("Full Screen"))->setData(12);
	if (selectedTracksList.size() > 1)
	{
		trackHeightsMenu->addAction(tr("Fit Selection in View"))->setData(13);
	}

	if(m_track->isMidiTrack() && !multipleSelectedTracks)
	{
		int oPort = ((MidiTrack*) m_track)->outPort();
		MidiPort* port = &midiPorts[oPort];

        if (port->device() && port->device()->isSynthPlugin())
        {
            SynthPluginDevice* synth = (SynthPluginDevice*)port->device();
            
            QAction* mact = p->addAction(tr("Show Gui"));
            mact->setCheckable(true);
            mact->setChecked(synth->guiVisible());
            mact->setData(3);

            QAction* mactn = p->addAction(tr("Show Native Gui"));
            mactn->setCheckable(true);
            mactn->setEnabled(synth->hasNativeGui());
            mactn->setChecked(synth->nativeGuiVisible());
            mactn->setData(16);
			mactn->setShortcut(shortcuts[SHRT_SHOW_PLUGIN_GUI].key);
        }
	}

	QAction* act = p->exec(QCursor::pos());
	if (act)
	{
		int n = act->data().toInt();
		switch (n)
		{
			case 0: // delete track
			{
				/*QString msg(tr("You are about to delete \n%1 \nAre you sure this is what you want?"));
				if(QMessageBox::question(this, 
					tr("Delete Track"),
					msg.arg(multipleSelectedTracks ? "all selected tracks" : m_track->name()),
					QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
				{*/
					if (multipleSelectedTracks)
					{
						trackManager->removeSelectedTracks();
					/*	song->startUndo();
						audio->msgRemoveTracks();
						song->endUndo(SC_TRACK_REMOVED);
						song->updateSoloStates();*/
					}
					else
					{
						if(m_track->id() == song->masterId() || m_track->id() == song->oomVerbId())
							break;
						trackManager->removeTrack(m_track->id());
						//song->removeTrack(m_track);
						//audio->msgUpdateSoloStates();
					}
				//}
			}
			break;

			case 1: // Import audio file
			{
				oom->importWave(m_track);
			}
			break;
			case 2:
			{
			}
			break;
			case 3:
			{
				int oPort = ((MidiTrack*) m_track)->outPort();
				MidiPort* port = &midiPorts[oPort];
				bool show = !port->guiVisible();
				audio->msgShowInstrumentGui(port->instrument(), show);
			}
			break;
			case 4:
			{
				TrackList* tl = song->visibletracks();
				TrackList selectedTracks = song->getSelectedTracks();
				bool select = true;
				if (selectedTracks.size() == tl->size())
				{
					select = false;
				}

				for (iTrack t = tl->begin(); t != tl->end(); ++t)
				{
					(*t)->setSelected(select);
				}
				song->update(SC_SELECTION);
			}
			break;
			case 5:
			{
			}
			case 6:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, DEFAULT_TRACKHEIGHT);
				}
				else
				{
					m_track->setHeight(DEFAULT_TRACKHEIGHT);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 7:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, MIN_TRACKHEIGHT);
				}
				else
				{
					m_track->setHeight(MIN_TRACKHEIGHT);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 8:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, TRACK_HEIGHT_3);
				}
				else
				{
					m_track->setHeight(TRACK_HEIGHT_3);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 9:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, TRACK_HEIGHT_4);
				}
				else
				{
					m_track->setHeight(TRACK_HEIGHT_4);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 10:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, TRACK_HEIGHT_5);
				}
				else
				{
					m_track->setHeight(TRACK_HEIGHT_5);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 11:
			{
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, TRACK_HEIGHT_6);
				}
				else
				{
					m_track->setHeight(TRACK_HEIGHT_6);
					song->update(SC_TRACK_MODIFIED);
				}
				emit trackHeightChanged();
				break;
			}
			case 12:
			{
				int canvasHeight = oom->composer->getCanvas()->height();

				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, canvasHeight);
					Track* firstSelectedTrack = *selectedTracksList.begin();
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(firstSelectedTrack));

				}
				else
				{
					m_track->setHeight(canvasHeight);
					song->update(SC_TRACK_MODIFIED);
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(m_track));
				}
				emit trackHeightChanged();
				break;
			}
			case 13:
			{
				int canvasHeight = oom->composer->getCanvas()->height();
				if (multipleSelectedTracks)
				{
					song->setTrackHeights(selectedTracksList, canvasHeight / selectedTracksList.size());
					Track* firstSelectedTrack = *selectedTracksList.begin();
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(firstSelectedTrack));

				}
				else
				{
					m_track->setHeight(canvasHeight);
					song->update(SC_TRACK_MODIFIED);
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(m_track));
				}
				emit trackHeightChanged();
				break;
			}
			case 15:
			{
				if(m_track->name() != "Master")
				{
					m_trackName->setReadOnly(false);
					m_trackName->setFocus();
					setEditing(true);
				}
				//if(!multipleSelectedTracks)
				//{
				//	renameTrack(m_track);
				//}
				break;
			}
            case 16:
            {
                int oPort = ((MidiTrack*) m_track)->outPort();
                MidiPort* port = &midiPorts[oPort];
                SynthPluginDevice* synth = (SynthPluginDevice*)port->device();
                bool show = !synth->nativeGuiVisible();
                audio->msgShowInstrumentNativeGui(port->instrument(), show);
            }   
            break;
			case 20 ... NUM_PARTCOLORS + 20:
			{
				int curColorIndex = n - 20;
				m_track->setDefaultPartColor(curColorIndex);
				break;
			}
			case 10000:
			{//Insert before
				int mypos = song->tracks()->index(m_track);
				CreateTrackDialog *ctdialog = new CreateTrackDialog(Track::MIDI, --mypos, this);
				connect(ctdialog, SIGNAL(trackAdded(qint64)), this, SLOT(newTrackAdded(qint64)));
				ctdialog->exec();
			
				break;
			}
			case 12000:
			{//Insert after
				int mypos = song->tracks()->index(m_track);
				CreateTrackDialog *ctdialog = new CreateTrackDialog(Track::MIDI, ++mypos, this);
				connect(ctdialog, SIGNAL(trackAdded(qint64)), this, SLOT(newTrackAdded(qint64)));
				ctdialog->exec();
				break;
			}
			default:
				printf("action %d\n", n);
			break;
		}
	}
	delete trackHeightsMenu;
	delete p;
}/*}}}*/

void TrackHeader::newTrackAdded(qint64 id)
{
	Track* t = song->findTrackById(id);
	if(t)
	{
		emit selectionChanged(t);
		emit trackInserted(t->type());
		song->updateTrackViews();
	}
}

void TrackHeader::generateAutomationMenu()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;

	QMenu* p = new QMenu(this);
	//p->setTearOffEnabled(true);
	AutomationMenu *amenu = new AutomationMenu(p, m_track);

	p->addAction(amenu);
	p->exec(QCursor::pos());

	delete p;
}/*}}}*/

void TrackHeader::generateInstrumentMenu()/*{{{*/
{
	if(!m_track || !m_track->isMidiTrack())
		return;

	QMenu* p = new QMenu(this);
	//p->setTearOffEnabled(true);
	TrackInstrumentMenu *imenu = new TrackInstrumentMenu(p, m_track);
	connect(imenu, SIGNAL(instrumentSelected(qint64, const QString&, int)), this, SLOT(instrumentChangeRequested(qint64, const QString&, int)));

	p->addAction(imenu);
	p->exec(QCursor::pos());

	delete p;
	updateSelection(false);
	//emit selectionChanged(m_track);
}/*}}}*/

void TrackHeader::instrumentChangeRequested(qint64 id, const QString& instrument, int type)
{
	trackManager->setTrackInstrument(id, instrument, type);
}

void TrackHeader::toggleRecord(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if (!m_track->isMidiTrack())
	{
		if (m_track->type() == Track::AUDIO_OUTPUT)
		{
			if (state && !m_track->recordFlag())
			{
				oom->bounceToFile((AudioOutput*) m_track);
			}
			audio->msgSetRecord((AudioOutput*) m_track, state);
			if (!((AudioOutput*) m_track)->recFile())
			{
				state = false;
			}
			else
			{
				m_btnRecord->blockSignals(true);
				m_btnRecord->setChecked(state);
				m_btnRecord->blockSignals(false);
				return;
			}
		}
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(state);
		m_btnRecord->blockSignals(false);
		song->setRecordFlag(m_track, state);
	}
	else
	{
		m_btnRecord->blockSignals(true);
		m_btnRecord->setChecked(state);
		m_btnRecord->blockSignals(false);
		song->setRecordFlag(m_track, state);
	}
	/*
	else if (button == Qt::RightButton)
	{
		// enable or disable ALL tracks of this type
		if (!m_track->isMidiTrack() && valid)
		{
			if (m_track->type() == Track::AUDIO_OUTPUT)
			{
				return;
			}
			WaveTrackList* wtl = song->waves();

			foreach(WaveTrack *wt, *wtl)
			{
				song->setRecordFlag(wt, val);
			}
		}
		else if(valid)
		{
			MidiTrackList* mtl = song->midis();

			foreach(MidiTrack *mt, *mtl)
			{
				song->setRecordFlag(mt, val);
			}
		}
		else
		{
			updateSelection(m_track, shift);
		}
	}
	*/
}/*}}}*/

void TrackHeader::toggleMute(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setMute(state);
	m_btnMute->blockSignals(true);
	m_btnMute->setChecked(state);
	m_btnMute->blockSignals(false);
	song->update(SC_MUTE);
}/*}}}*/

void TrackHeader::toggleSolo(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	audio->msgSetSolo(m_track, state);
	m_btnSolo->blockSignals(true);
	m_btnSolo->setChecked(state);
	m_btnSolo->blockSignals(false);
	song->update(SC_SOLO);
}/*}}}*/

void TrackHeader::toggleOffState(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setOff(state);
}/*}}}*/

void TrackHeader::toggleReminder1(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder1(state);
}/*}}}*/

void TrackHeader::toggleReminder2(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder2(state);
}/*}}}*/

void TrackHeader::toggleReminder3(bool state)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	m_track->setReminder3(state);
}/*}}}*/

void TrackHeader::updateTrackName()/*{{{*/
{
	if(!m_track || !m_processEvents)
	{
		m_trackName->setReadOnly(true);
		return;
	}
	QString name = m_trackName->text();
	if(name.isEmpty())
	{
		//m_trackName->blockSignals(true);
		m_trackName->undo();//setText(m_track->name());
		//m_trackName->blockSignals(false);
		setEditing(false);
		m_trackName->setReadOnly(true);
		return;
	}
	if (name != m_track->name())
	{
		TrackList* tl = song->tracks();
		for (iTrack i = tl->begin(); i != tl->end(); ++i)
		{
			if ((*i)->name() == name)
			{
				QMessageBox::critical(this,
						tr("OOMidi: bad trackname"),
						tr("please choose a unique track name"),
						QMessageBox::Ok,
						Qt::NoButton,
						Qt::NoButton);
				//m_trackName->blockSignals(true);
				//m_trackName->setText(m_track->name());
				//m_trackName->blockSignals(false);
				m_trackName->undo();//setText(m_track->name());
				setEditing(false);
				m_trackName->setReadOnly(true);
				return;
			}
		}
		Track* track = m_track->clone(false);
		m_track->setName(name);
		audio->msgChangeTrack(track, m_track);
	}
	m_trackName->setReadOnly(true);
	setEditing(false);
}/*}}}*/

void TrackHeader::updateVolume()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		/*int channel = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		ciMidiCtrlValList icl;

		MidiController* ctrl = mp->midiController(CTRL_VOLUME);
		int nvolume = mp->hwCtrlState(channel, CTRL_VOLUME);
		if (nvolume == CTRL_VAL_UNKNOWN)
		{
			volume = CTRL_VAL_UNKNOWN;
			nvolume = mp->lastValidHWCtrlState(channel, CTRL_VOLUME);
			if (nvolume != CTRL_VAL_UNKNOWN)
			{
				nvolume -= ctrl->bias();
				if (double(nvolume) != m_slider->value())
				{
					m_slider->setValue(double(nvolume));
				}
			}
		}
		else
		{
			nvolume -= ctrl->bias();
			if (nvolume != volume)
			{
				m_slider->setValue(double(nvolume));
				volume = nvolume;
			}
		}*/
		Track *in = m_track->inputTrack();
		if(in)
		{
			double vol = ((AudioTrack*) in)->volume();/*{{{*/
			if (vol != volume)
			{
				m_slider->blockSignals(true);
				double val = fast_log10(vol) * 20.0;
				m_slider->setValue(val);
				m_slider->blockSignals(false);
				volume = vol;
				if(((AudioTrack*) in)->volFromAutomation())
				{
					midiMonitor->msgSendAudioOutputEvent((Track*)in, CTRL_VOLUME, vol);
				}
			}/*}}}*/
		}
	}
	else
	{
		double vol = ((AudioTrack*) m_track)->volume();/*{{{*/
		if (vol != volume)
		{
			m_slider->blockSignals(true);
			double val = fast_log10(vol) * 20.0;
			m_slider->setValue(val);
			m_slider->blockSignals(false);
			volume = vol;
			if(((AudioTrack*) m_track)->volFromAutomation())
			{
				midiMonitor->msgSendAudioOutputEvent((Track*)m_track, CTRL_VOLUME, vol);
			}
		}/*}}}*/
	}
}/*}}}*/

void TrackHeader::volumeChanged(double val)/*{{{*/
{
	if(!m_track || inHeartBeat)
		return;
	if(m_track->isMidiTrack())
	{
		/*int num = CTRL_VOLUME;
		int  mval = lrint(val);

		MidiTrack* t = (MidiTrack*) m_track;
		int port = t->outPort();

		int chan = t->outChannel();
		MidiPort* mp = &midiPorts[port];
		MidiController* mctl = mp->midiController(num);
		if ((mval < mctl->minVal()) || (mval > mctl->maxVal()))
		{
			if (mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
				audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
		}
		else
		{
			mval += mctl->bias();

			int tick = song->cpos();

			MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, num, mval, m_track);
			ev.setEventSource(AudioSource);

			audio->msgPlayMidiEvent(&ev);
			midiMonitor->msgSendMidiOutputEvent(m_track, num, mval);
		}
		song->update(SC_MIDI_CONTROLLER);*/
		Track *in = m_track->inputTrack();
		if(in)
		{
			AutomationType at = ((AudioTrack*) in)->automationType();/*{{{*/
			if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
				((AudioTrack*)in)->enableVolumeController(false);
	
			double vol;
			if (val <= config.minSlider)
			{
				vol = 0.0;
				val -= 1.0; 
			}
			else
				vol = pow(10.0, val / 20.0);
			volume = vol;
			audio->msgSetVolume((AudioTrack*) in, vol);
			((AudioTrack*) in)->recordAutomation(AC_VOLUME, vol);
			song->update(SC_TRACK_MODIFIED);/*}}}*/
		}
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();/*{{{*/
		if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
			((AudioTrack*)m_track)->enableVolumeController(false);
	
		double vol;
		if (val <= config.minSlider)
		{
			vol = 0.0;
			val -= 1.0; 
		}
		else
			vol = pow(10.0, val / 20.0);
		volume = vol;
		audio->msgSetVolume((AudioTrack*) m_track, vol);
		((AudioTrack*) m_track)->recordAutomation(AC_VOLUME, vol);
		song->update(SC_TRACK_MODIFIED);/*}}}*/
	}
}/*}}}*/

void TrackHeader::volumePressed()/*{{{*/
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			AutomationType at = ((AudioTrack*) in)->automationType();/*{{{*/
			if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
				in->enableVolumeController(false);

			double val = m_slider->value();
			double vol;
			if (val <= config.minSlider)
			{
				vol = 0.0;
			}
			else
				vol = pow(10.0, val / 20.0);
			volume = vol;
			audio->msgSetVolume((AudioTrack*) in, volume);
			((AudioTrack*) in)->startAutoRecord(AC_VOLUME, volume);/*}}}*/
		}
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();/*{{{*/
		if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
			m_track->enableVolumeController(false);

		double val = m_slider->value();
		double vol;
		if (val <= config.minSlider)
		{
			vol = 0.0;
		}
		else
			vol = pow(10.0, val / 20.0);
		volume = vol;
		audio->msgSetVolume((AudioTrack*) m_track, volume);
		((AudioTrack*) m_track)->startAutoRecord(AC_VOLUME, volume);/*}}}*/
	}

}/*}}}*/

void TrackHeader::volumeReleased()/*{{{*/
{
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			if (((AudioTrack*)in)->automationType() != AUTO_WRITE)
				((AudioTrack*)in)->enableVolumeController(true);
			((AudioTrack*) in)->stopAutoRecord(AC_VOLUME, volume);
		}
	}
	else
	{
		if (((AudioTrack*)m_track)->automationType() != AUTO_WRITE)
			((AudioTrack*)m_track)->enableVolumeController(true);
		((AudioTrack*) m_track)->stopAutoRecord(AC_VOLUME, volume);
	}

}/*}}}*/

void TrackHeader::volumeRightClicked(const QPoint &p, int ctrl)/*{{{*/
{
	Q_UNUSED(ctrl);
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		//song->execMidiAutomationCtlPopup((MidiTrack*) m_track, 0, p, ctrl);
		Track *in = m_track->inputTrack();
		if(in)
		{
			song->execAutomationCtlPopup((AudioTrack*) in, p, AC_VOLUME);
		}
	}
	else
	{
		song->execAutomationCtlPopup((AudioTrack*) m_track, p, AC_VOLUME);
	}

}/*}}}*/

void TrackHeader::updatePan()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		/*int channel = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		//MidiCtrlValListList* mc = mp->controller();
		ciMidiCtrlValList icl;

		MidiController* ctrl = mp->midiController(CTRL_PANPOT);
		int npan = mp->hwCtrlState(channel, CTRL_PANPOT);
		if (npan == CTRL_VAL_UNKNOWN)
		{
			panVal = CTRL_VAL_UNKNOWN;
			npan = mp->lastValidHWCtrlState(channel, CTRL_PANPOT);
			if (npan != CTRL_VAL_UNKNOWN)
			{
				npan -= ctrl->bias();
				if (double(npan) != m_pan->value())
				{
					m_pan->setValue(double(npan));
				}
			}
		}
		else
		{
			npan -= ctrl->bias();
			if (npan != panVal)
			{
				m_pan->setValue(double(npan));
				panVal = npan;
			}
		}*/
		Track *in = m_track->inputTrack();
		if(in)
		{
			double v = ((AudioTrack*) in)->pan();
			if (v != panVal)
			{
				m_pan->blockSignals(true);
				m_pan->setValue(v);
				m_pan->blockSignals(false);
				panVal = v;
				if(((AudioTrack*) in)->panFromAutomation())
				{
					midiMonitor->msgSendAudioOutputEvent((Track*)in, CTRL_PANPOT, v);
				}
			}
		}
	}
	else
	{
		double v = ((AudioTrack*) m_track)->pan();
		if (v != panVal)
		{
			m_pan->blockSignals(true);
			m_pan->setValue(v);
			m_pan->blockSignals(false);
			panVal = v;
			if(((AudioTrack*) m_track)->panFromAutomation())
			{
				midiMonitor->msgSendAudioOutputEvent((Track*)m_track, CTRL_PANPOT, v);
			}
		}
	}
	if(m_pan && panVal != CTRL_VAL_UNKNOWN)
	{
		QString val = QString::number(panVal);
		m_pan->setToolTip(val+" Panorama");
	}
	else if(m_pan)
		m_pan->setToolTip("Panorama");
}/*}}}*/

void TrackHeader::panChanged(double val)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		/*int ctrl = CTRL_PANPOT;
		int ival =  lrint(val);
		MidiTrack* t = (MidiTrack*) m_track;
		int port = t->outPort();

		int chan = t->outChannel();
		MidiPort* mp = &midiPorts[port];
		MidiController* mctl = mp->midiController(ctrl);
		if ((ival < mctl->minVal()) || (ival > mctl->maxVal()))
		{
			if (mp->hwCtrlState(chan, ctrl) != CTRL_VAL_UNKNOWN)
				audio->msgSetHwCtrlState(mp, chan, ctrl, CTRL_VAL_UNKNOWN);
			panVal = 0.0;
		}
		else
		{
			val += mctl->bias();

			int tick = song->cpos();

			MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, ctrl, val, m_track);
			ev.setEventSource(AudioSource);

			audio->msgPlayMidiEvent(&ev);
			midiMonitor->msgSendMidiOutputEvent(m_track, ctrl, val);
			panVal = ival;
		}
		song->update(SC_MIDI_CONTROLLER);*/
		Track *in = m_track->inputTrack();
		if(in)
		{
			AutomationType at = ((AudioTrack*) in)->automationType();
			if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
				in->enablePanController(false);

			panVal = val;
			audio->msgSetPan(((AudioTrack*) in), val);
			((AudioTrack*) in)->recordAutomation(AC_PAN, val);
		}
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
			m_track->enablePanController(false);

		panVal = val;
		audio->msgSetPan(((AudioTrack*) m_track), val);
		((AudioTrack*) m_track)->recordAutomation(AC_PAN, val);
	}
	QString span = QString::number(panVal);
	span.append(tr(" Panorama"));

	QPoint cursorPos = QCursor::pos();
	QToolTip::showText(cursorPos, span, this, QRect(cursorPos.x(), cursorPos.y(), 2, 2));
}/*}}}*/

void TrackHeader::panPressed()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			AutomationType at = ((AudioTrack*) in)->automationType();
			if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
				in->enablePanController(false);

			panVal = m_pan->value();
			audio->msgSetPan(((AudioTrack*) in), panVal);
			((AudioTrack*) in)->startAutoRecord(AC_PAN, panVal);
		}
	}
	else
	{
		AutomationType at = ((AudioTrack*) m_track)->automationType();
		if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
			m_track->enablePanController(false);

		panVal = m_pan->value();
		audio->msgSetPan(((AudioTrack*) m_track), panVal);
		((AudioTrack*) m_track)->startAutoRecord(AC_PAN, panVal);
	}
}/*}}}*/

void TrackHeader::panReleased()/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
		{
			if (in->automationType() != AUTO_WRITE)
				in->enablePanController(true);
			((AudioTrack*) in)->stopAutoRecord(AC_PAN, panVal);
		}
	}
	else
	{
		if (m_track->automationType() != AUTO_WRITE)
			m_track->enablePanController(true);
		((AudioTrack*) m_track)->stopAutoRecord(AC_PAN, panVal);
	}
}/*}}}*/

void TrackHeader::panRightClicked(const QPoint &p)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
			song->execAutomationCtlPopup((AudioTrack*) in, p, AC_PAN);
	}
	else
		song->execAutomationCtlPopup((AudioTrack*) m_track, p, AC_PAN);
}/*}}}*/

void TrackHeader::resetPeaks(bool)/*{{{*/
{
	if(!m_track)
		return;
		
	m_track->resetPeaks();
	if(m_track->isMidiTrack())
	{
		Track *in = m_track->inputTrack();
		if(in)
			in->resetPeaks();
	}
}/*}}}*/

void TrackHeader::resetPeaksOnPlay(bool play)/*{{{*/
{
	if(!m_track)
		return;
	if(play)	
		resetPeaks(play);
}/*}}}*/

//Private member functions

bool TrackHeader::eventFilter(QObject *obj, QEvent *event)/*{{{*/
{
	if(!m_processEvents)
		return true;
	/*if (event->type() & (QEvent::MouseButtonPress | QEvent::MouseMove | QEvent::MouseButtonRelease))
	{
		bool alltype = false;
		bool isname = false;
		if(obj == m_trackName)
		{
			isname = true;
			QLineEdit* tname = static_cast<QLineEdit*>(obj);
			if(tname && tname->isReadOnly())
			{
				alltype = true;
			}
		}
		if(alltype && isname)
		{
			QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
			mousePressEvent(mEvent);
			mode = NORMAL;
			return true;
			//return QObject::eventFilter(obj, event);
		}
	}
	*/
	if (event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
		if(mEvent && mEvent->button() == Qt::LeftButton )
		{
			mousePressEvent(mEvent);
			mode = NORMAL;
		}
		else if(mEvent && mEvent->button() == Qt::RightButton && obj == m_trackName)
		{
			mousePressEvent(mEvent);
			mode = NORMAL;
		}
	}
	if(event->type() == QEvent::KeyPress)
	{
		QKeyEvent* kEvent = static_cast<QKeyEvent*>(event);
		if((kEvent->key() == Qt::Key_Return || kEvent->key() == Qt::Key_Enter) && m_editing)
		{
			updateTrackName();
		}
	}
	// standard event processing
	return QObject::eventFilter(obj, event);

}/*}}}*/

void TrackHeader::updateChannels()/*{{{*/
{
	if(m_track && m_track->isMidiTrack() && m_processEvents)
	{
		Track *in = m_track->inputTrack();
		AudioTrack* t = (AudioTrack*)in;/*{{{*/
		int c = t->channels();
		//printf("TrackHeader::updateChannels(%d) chanels: %d\n", c, m_channels);

		if (c > m_channels)
		{
			//printf("Going stereo\n");
			int size = 3+m_channels;
			for (int cc = m_channels; cc < c; ++size, ++cc)
			{
				Meter* metercc = new Meter(this, m_track->type(), Meter::DBMeter, Qt::Horizontal);
				metercc->setRange(config.minMeter, 10.0);
				metercc->setFixedHeight(5);
				metercc->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
				connect(metercc, SIGNAL(mousePress(bool)), this, SLOT(resetPeaks(bool)));
				connect(metercc, SIGNAL(mousePress(bool)), this, SLOT(updateSelection(bool)));
				meter.append(metercc);
				m_buttonVBox->addWidget(metercc);
				metercc->show();
			}
		}
		else if (c < m_channels)
		{
			//printf("Going mono\n");
			for (int cc = m_channels - 1; cc >= c; --cc)
			{
				if(!meter.isEmpty() && cc < meter.size())
					delete meter.takeAt(cc);
			}
		}
		m_channels = c;/*}}}*/
	}
	else if(m_track && !m_track->isMidiTrack() && m_processEvents)
	{
		AudioTrack* t = (AudioTrack*) m_track;/*{{{*/
		int c = t->channels();
		//printf("TrackHeader::updateChannels(%d) chanels: %d\n", c, m_channels);

		if (c > m_channels)
		{
			//printf("Going stereo\n");
			int size = 3+m_channels;
			for (int cc = m_channels; cc < c; ++size, ++cc)
			{
				Meter* metercc = new Meter(this, m_track->type(), Meter::DBMeter, Qt::Horizontal);
				metercc->setRange(config.minMeter, 10.0);
				metercc->setFixedHeight(5);
				metercc->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
				connect(metercc, SIGNAL(mousePress(bool)), this, SLOT(resetPeaks(bool)));
				connect(metercc, SIGNAL(mousePress(bool)), this, SLOT(updateSelection(bool)));
				meter.append(metercc);
				m_buttonVBox->addWidget(metercc);
				metercc->show();
			}
		}
		else if (c < m_channels)
		{
			//printf("Going mono\n");
			for (int cc = m_channels - 1; cc >= c; --cc)
			{
				if(!meter.isEmpty() && cc < meter.size())
					delete meter.takeAt(cc);
			}
		}
		m_channels = c;/*}}}*/
	}
}/*}}}*/

void TrackHeader::initPan()/*{{{*/
{
	if(!m_track)
		return;
	QString img(":images/knob_midi_new.png");
	Track::TrackType type = m_track->type();
	switch (type)
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
	}
	bool update = false;
	if(m_pan)
	{
		update = true;
		delete m_pan;
	}
	if(m_track->isMidiTrack())
	{
		/*int ctl = CTRL_PANPOT, mn, mx, v;
		int chan = ((MidiTrack*) m_track)->outChannel();
		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
		MidiController* mc = mp->midiController(ctl);
		mn = mc->minVal();
		mx = mc->maxVal();

		m_pan = new Knob(this);
		m_pan->setRange(double(mn), double(mx), 1.0);
		m_pan->setId(ctl);
		m_pan->setKnobImage(img);

		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		m_pan->setBackgroundRole(QPalette::Mid);
		m_pan->setToolTip("Panorama");
		m_pan->setEnabled(true);
		m_pan->setIgnoreWheel(true);

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
		}
		else
		{
			// Auto bias...
			v -= mc->bias();
		}

		m_pan->setValue(double(v));

		if(!update)
		{
			m_panLayout->addItem(new QSpacerItem(30, 9, QSizePolicy::Fixed, QSizePolicy::Fixed));
			m_panLayout->addWidget(m_pan);
			m_panLayout->addItem(new QSpacerItem(30, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
		}
		else
		{
			m_panLayout->insertWidget(1, m_pan);
		}
		m_pan->show();

		connect(m_pan, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
		//connect(m_pan, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
		*/
		m_pan = new Knob(this);/*{{{*/
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
		m_pan->setRange(-1.0, +1.0);
		m_pan->setToolTip(tr("Panorama"));
		m_pan->setKnobImage(img);
		m_pan->setIgnoreWheel(true);
		m_pan->setBackgroundRole(QPalette::Mid);
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		if(!update)
		{
			m_panLayout->addItem(new QSpacerItem(30, 9, QSizePolicy::Fixed, QSizePolicy::Fixed));
			m_panLayout->addWidget(m_pan);
			m_panLayout->addItem(new QSpacerItem(30, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
		}
		else
		{
			m_panLayout->insertWidget(1, m_pan);
		}
		m_pan->show();
		Track *in = m_track->inputTrack();
		if(in)
		{
			double v = ((AudioTrack*) in)->pan();
			m_pan->blockSignals(true);
			m_pan->setValue(v);
			m_pan->blockSignals(false);
			panVal = v;
			if(((AudioTrack*) in)->panFromAutomation())
			{
				midiMonitor->msgSendAudioOutputEvent((Track*)in, CTRL_PANPOT, v);
			}
		}
		connect(m_pan, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
		connect(m_pan, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
		connect(m_pan, SIGNAL(sliderReleased(int)), SLOT(panReleased()));/*}}}*/
	}
	else
	{
		m_pan = new Knob(this);/*{{{*/
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
		m_pan->setRange(-1.0, +1.0);
		m_pan->setToolTip(tr("Panorama"));
		m_pan->setKnobImage(img);
		m_pan->setIgnoreWheel(true);
		m_pan->setBackgroundRole(QPalette::Mid);
		m_pan->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		if(!update)
		{
			m_panLayout->addItem(new QSpacerItem(30, 9, QSizePolicy::Fixed, QSizePolicy::Fixed));
			m_panLayout->addWidget(m_pan);
			m_panLayout->addItem(new QSpacerItem(30, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
		}
		else
		{
			m_panLayout->insertWidget(1, m_pan);
		}
		m_pan->show();
		connect(m_pan, SIGNAL(sliderMoved(double, int)), SLOT(panChanged(double)));
		connect(m_pan, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
		connect(m_pan, SIGNAL(sliderReleased(int)), SLOT(panReleased()));/*}}}*/
	}
}/*}}}*/

void TrackHeader::initVolume()/*{{{*/
{
	bool usePixmap = false;
	QColor sliderBgColor = g_trackColorList.value(m_track->type());/*{{{*/
   	switch(vuColorStrip)
   	{
   	    case 0:
   	        sliderBgColor = g_trackColorList.value(m_track->type());
   	    break;
   	    case 1:
   	        //if(width() != m_width)
   	        //    m_scaledPixmap_w = m_pixmap_w->scaled(width(), 1, Qt::IgnoreAspectRatio);
   	        //m_width = width();
   	        //myPen.setBrush(m_scaledPixmap_w);
   	        //myPen.setBrush(m_trackColor);
   	        sliderBgColor = g_trackColorList.value(m_track->type());
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
   	        sliderBgColor = g_trackColorList.value(m_track->type());
   	        //myPen.setBrush(m_trackColor);
   	    break;
   	}/*}}}*/
	if(!m_track)
		return;
	if(m_track->isMidiTrack())
	{
		/*MidiTrack* track = (MidiTrack*)m_track;
		MidiPort* mp = &midiPorts[track->outPort()];
		MidiController* mc = mp->midiController(CTRL_VOLUME);
		//int chan = track->outChannel();
		int mn = mc->minVal();
		int mx = mc->maxVal();

		m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, sliderBgColor, usePixmap);
		m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m_slider->setCursorHoming(true);
		m_slider->setRange(double(mn), double(mx), 1.0);
		m_slider->setFixedHeight(15);
		m_slider->setFont(config.fonts[1]);
		m_slider->setId(CTRL_VOLUME);
		m_slider->setIgnoreWheel(true);
		m_buttonVBox->addWidget(m_slider);
		connect(m_slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
		connect(m_slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &, int)));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(updateSelection()));

		Meter* m = new Meter(this, m_track->type(), Meter::LinMeter, Qt::Horizontal);
		m->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m->setRange(0, 127.0);
		m->setFixedHeight(5);
		meter.append(m);
		//m->setFixedWidth(150);
		m_buttonVBox->addWidget(m);
		connect(m, SIGNAL(mousePress(bool)), this, SLOT(resetPeaks(bool)));
		connect(m, SIGNAL(mousePress(bool)), this, SLOT(updateSelection(bool)));*/
		Track *in = m_track->inputTrack();
		int channels = ((AudioTrack*)in)->channels();/*{{{*/
		m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, sliderBgColor, usePixmap);
		m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m_slider->setCursorHoming(true);
		m_slider->setRange(config.minSlider - 0.1, 10.0);
		m_slider->setFixedHeight(15);
		m_slider->setFont(config.fonts[1]);
		m_slider->setValue(fast_log10(((AudioTrack*)in)->volume())*20.0);
		m_slider->setIgnoreWheel(true);
		m_buttonVBox->addWidget(m_slider);

		connect(m_slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(updateSelection()));
		connect(m_slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
		connect(m_slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));

		for (int i = 0; i < channels; ++i)/*{{{*/
		{
			Meter* m = new Meter(this, m_track->type(), Meter::DBMeter, Qt::Horizontal);
			m->setRange(config.minMeter, 10.0);
			m->setFixedHeight(5);
			//m->setFixedWidth(150);
			m->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
			connect(m, SIGNAL(mousePress(bool)), this, SLOT(resetPeaks(bool)));
			connect(m, SIGNAL(mousePress(bool)), this, SLOT(updateSelection(bool)));
			//connect(m, SIGNAL(meterClipped()), this, SLOT(playbackClipped()));
			meter.append(m);
			m_buttonVBox->addWidget(m);
		}/*}}}*/
		m_channels = channels;/*}}}*/
	}
	else
	{
		int channels = ((AudioTrack*)m_track)->channels();/*{{{*/
		m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, sliderBgColor, usePixmap);
		m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
		m_slider->setCursorHoming(true);
		m_slider->setRange(config.minSlider - 0.1, 10.0);
		m_slider->setFixedHeight(15);
		m_slider->setFont(config.fonts[1]);
		m_slider->setValue(fast_log10(((AudioTrack*)m_track)->volume())*20.0);
		m_slider->setIgnoreWheel(true);
		m_buttonVBox->addWidget(m_slider);

		connect(m_slider, SIGNAL(sliderMoved(double, int)), SLOT(volumeChanged(double)));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
		connect(m_slider, SIGNAL(sliderPressed(int)), SLOT(updateSelection()));
		connect(m_slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
		connect(m_slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));

		for (int i = 0; i < channels; ++i)/*{{{*/
		{
			Meter* m = new Meter(this, m_track->type(), Meter::DBMeter, Qt::Horizontal);
			m->setRange(config.minMeter, 10.0);
			m->setFixedHeight(5);
			//m->setFixedWidth(150);
			m->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
			connect(m, SIGNAL(mousePress(bool)), this, SLOT(resetPeaks(bool)));
			connect(m, SIGNAL(mousePress(bool)), this, SLOT(updateSelection(bool)));
			//connect(m, SIGNAL(meterClipped()), this, SLOT(playbackClipped()));
			meter.append(m);
			m_buttonVBox->addWidget(m);
		}/*}}}*/
		m_channels = channels;/*}}}*/
	}
	//printf("TrackHeader::initVolume - button row item count: %d\n", m_buttonVBox->count());
}/*}}}*/

void TrackHeader::setupStyles()/*{{{*/
{
	m_style.insert(Track::MIDI, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::WAVE, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::AUDIO_OUTPUT, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::AUDIO_INPUT, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::AUDIO_BUSS, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::AUDIO_AUX, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	m_style.insert(Track::AUDIO_SOFTSYNTH, styletemplate.arg(QString("939393"), QString("1a1a1a")));
	
	m_selectedStyle.insert(Track::AUDIO_INPUT, styletemplate.arg(QString("e18fff"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::MIDI, styletemplate.arg(QString("01e6ee"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::AUDIO_OUTPUT, styletemplate.arg(QString("fc7676"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::WAVE, styletemplate.arg(QString("81f476"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::AUDIO_BUSS, styletemplate.arg(QString("fca424"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::AUDIO_AUX, styletemplate.arg(QString("ecf276"), QString("0a0a0a")));
	m_selectedStyle.insert(Track::AUDIO_SOFTSYNTH, styletemplate.arg(QString("01e6ee"), QString("0a0a0a")));
}/*}}}*/

void TrackHeader::updateSelection(bool shift)/*{{{*/
{
	if(!m_track)
		return;
	//printf("TrackHeader::updateSelection - shift; %d\n", shift);
	if (!shift)
	{
		song->deselectTracks();
		if(song->hasSelectedParts)
			song->deselectAllParts();
		setSelected(true);

		//record enable track if expected
		int recd = 0;
		TrackList* tracks = song->visibletracks();
		Track* recTrack = 0;
		for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
		{
			if ((*t)->recordFlag())
			{
				if(!recTrack)
					recTrack = *t;
				recd++;
			}
		}
		if (recd == 1 && config.moveArmedCheckBox)
		{ 
			//one rec enabled track, move rec enabled with selection
			song->setRecordFlag(recTrack, false);
			song->setRecordFlag(m_track, true);
		}
	}
	else
	{
		song->deselectAllParts();
		setSelected(true);
	}
	song->update(SC_SELECTION | SC_RECFLAG);
}/*}}}*/

//Protected events
//We overwrite these from QWidget to implement our own functionality

void TrackHeader::mousePressEvent(QMouseEvent* ev) //{{{
{
	if(!m_track || !m_processEvents)
		return;
	int button = ev->button();
	bool shift = ((QInputEvent*) ev)->modifiers() & Qt::ShiftModifier;

	if (button == Qt::LeftButton)
	{
		if (resizeFlag)
		{
			startY = ev->y();
			mode = RESIZE;
			return;
		}
		else
		{
			m_startPos = ev->pos();
			updateSelection(shift);
			if(m_track->name() != "Master")
				mode = START_DRAG;
		}
	}
	else if(button == Qt::RightButton)
	{
		generatePopupMenu();
	}
}/*}}}*/

void TrackHeader::mouseMoveEvent(QMouseEvent* ev)/*{{{*/
{
	if(!m_track || !m_processEvents)
		return;
	bool shift = ((QInputEvent*) ev)->modifiers() & Qt::ShiftModifier;
	if(shift)
	{
		resizeFlag = false;
		setCursor(QCursor(Qt::ArrowCursor));
		return;
	}
	if ((((QInputEvent*) ev)->modifiers() | ev->buttons()) == 0)
	{
		QRect geo = geometry();
		QRect hotBox(0, m_track->height() - 2, width(), 2);
		//printf("HotBox: x: %d, y: %d, event pos x: %d, y: %d, geo bottom: %d\n", hotBox.x(), hotBox.y(), ev->x(), ev->y(), geo.bottom());
		if (hotBox.contains(ev->pos()))
		{
			//printf("Hit hotspot\n");
			if (!resizeFlag)
			{
				resizeFlag = true;
				setCursor(QCursor(Qt::SplitVCursor));
			}
		}
		else
		{
			resizeFlag = false;
			setCursor(QCursor(Qt::ArrowCursor));
		}
		return;
	}
	curY = ev->y();
	int delta = curY - startY;
	switch (mode)
	{
		case START_DRAG:
		{
			if ((ev->pos() - m_startPos).manhattanLength() < QApplication::startDragDistance())
				return;

			m_editing = true;
			mode = DRAG;
			QPoint hotSpot = ev->pos();
			int index = song->visibletracks()->index(m_track);
			
			QByteArray itemData;
			QDataStream dataStream(&itemData, QIODevice::WriteOnly);
			dataStream << m_track->name() << index << QPoint(hotSpot);
			
			QMimeData *mimeData = new QMimeData;
			mimeData->setData("oomidi/x-trackinfo", itemData);
			mimeData->setText(m_track->name());
			
			QDrag *drag = new QDrag(this);
			drag->setMimeData(mimeData);
			drag->setPixmap(g_trackDragImageList.value(m_track->type()));
			drag->setHotSpot(QPoint(80,20));//hotSpot);
			drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction);
		}
		break;
		case NORMAL:
		case DRAG:
		break;
		case RESIZE:
		{
			if (m_track)
			{
				int h = m_track->height() + delta;
				startY = curY;
				if (h < MIN_TRACKHEIGHT)
					h = MIN_TRACKHEIGHT;
				m_track->setHeight(h);
				song->update(SC_TRACK_MODIFIED);
			}
		}
		break;
	}
}/*}}}*/

void TrackHeader::mouseReleaseEvent(QMouseEvent*)/*{{{*/
{
	if(mode == RESIZE)
		emit trackHeightChanged();
	mode = NORMAL;
	setCursor(QCursor(Qt::ArrowCursor));
	m_editing = false;
	resizeFlag = false;
}/*}}}*/

void TrackHeader::resizeEvent(QResizeEvent* event)/*{{{*/
{
	//We will trap this to disappear widgets like vu's and volume slider
	//on the track header. For now we'll just pass it up the chain
	QSize size = event->size();
	if(m_track)
	{
		m_meterVisible = size.height() >= MIN_TRACKHEIGHT_VU;
		m_sliderVisible = size.height() >= MIN_TRACKHEIGHT_SLIDER;
		m_toolsVisible = (size.height() >= MIN_TRACKHEIGHT_TOOLS);
		foreach(Meter* m, meter)
			m->setVisible(m_meterVisible);
		/*if(m_track->isMidiTrack())
		{
		}
		else
		{
			for (int ch = 0; ch < ((AudioTrack*)m_track)->channels(); ++ch)
			{
				if (meter[ch])
				{
					meter[ch]->setVisible(m_meterVisible);
				}
			}
		}*/
		if(m_slider)
			m_slider->setVisible(m_sliderVisible);
		if(m_pan)
			m_pan->setVisible(m_sliderVisible);
		if(m_effects)
		{
			m_effects->setVisible(m_toolsVisible);
			m_controlsBox->setEnabled(m_toolsVisible);
		}
		/*if(m_sliderVisible)
		{
			m_colorLine->setStyleSheet(lineStyleTemplate.arg("#2e2e2e"));
		}
		else
		{*/
	//		m_colorLine->setStyleSheet(lineStyleTemplate.arg(g_trackColorList.value(m_track->type()).name()));
		//}
	}
	//m_autoTable->update();
	//tabWidget->update();
	//QFrame::resizeEvent(event);
}/*}}}*/

