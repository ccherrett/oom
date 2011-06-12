//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: amixer.cpp,v 1.49.2.5 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <list>
#include <cmath>

#include <QApplication>
#include <QtGui>

#include "app.h"
#include "icons.h"
#include "amixer.h"
#include "song.h"
#include "shortcuts.h"

#include "astrip.h"
#include "mstrip.h"
#include "apconfig.h"

#include "gconfig.h"
#include "xml.h"
#include "traverso_shared/TConfig.h"
#include "widgets/mixerview.h"
#include "widgets/mixerdock.h"

extern QActionGroup* populateAddTrack(QMenu* addTrack);

#define __WIDTH_COMPENSATION 4

//---------------------------------------------------------
//   AudioMixer
//---------------------------------------------------------

AudioMixerApp::AudioMixerApp(const QString& title, QWidget* parent)
: QMainWindow(parent)
{
	oldAuxsSize = 0;
	routingDialog = 0;
	setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding)); 
	setWindowTitle(title);
	setWindowIcon(*oomIcon);

	QMenu* menuView = menuBar()->addMenu(tr("&View"));
	routingId = menuView->addAction(tr("Routing"), this, SLOT(toggleAudioPortConfig()));
	routingId->setCheckable(true);

	menuView->addSeparator();

	QActionGroup* actionItems = new QActionGroup(this);
	actionItems->setExclusive(false);

	//toggleShowEffectsRackAction = new QAction(tr("Show/hide Effects Rack"), actionItems);
	//toggleShowEffectsRackAction->setShortcut(shortcuts[SHRT_TOGGLE_RACK].key);
	//toggleShowEffectsRackAction->setCheckable(true);

	//connect(toggleShowEffectsRackAction, SIGNAL(triggered(bool)), SLOT(toggleShowEffectsRack(bool)));

	menuView->addActions(actionItems->actions());

	/*QWidget *dock = new QWidget(this);
	QVBoxLayout* dockBox = new QVBoxLayout(dock);
	dockBox->setContentMargins(0,0,0,0);
	dockBox->setSpacing(2);*/

	QDockWidget* m_mixerDock = new QDockWidget(tr("Mixer Views"), this);
	//m_mixerDock->setTitleBarWidget(faketitle);
	m_mixerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::LeftDockWidgetArea, m_mixerDock);
	MixerView *m_mixerView = new MixerView(this);
	m_mixerDock->setWidget(m_mixerView);
	
	m_cmbRows = new QComboBox(m_mixerDock);
	for(int i = 1; i < 6; i++)
	{
		m_cmbRows->addItem(QString::number(i), i);
	}
	m_mixerView->addButton(m_cmbRows);
	m_mixerView->addButton(new QLabel(tr("Rows ")));

	m_view = new QScrollArea();
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setCentralWidget(m_view);

	m_splitter = new QSplitter(Qt::Vertical, m_view);
	m_view->setWidget(m_splitter);
	m_view->setWidgetResizable(true);

	/*layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);*/

	//connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	//connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));
	connect(m_cmbRows, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMixer(int)));
	//m_cmbRows->setCurrentIndex(1);
}

AudioMixerApp::~AudioMixerApp()
{
	tconfig().set_property(objectName(), "geometry", geometry());
    tconfig().save();
}

void AudioMixerApp::showEvent(QShowEvent*)
{
	QRect geometry = tconfig().get_property(objectName(), "geometry", QRect(0,0,600, 600)).toRect();
	setGeometry(geometry);
	int rows = tconfig().get_property(objectName(), "rows", 1).toInt();
	//m_cmbRows->setCurrentIndex(rows);
	//song->update(); // calls update mixer
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void AudioMixerApp::clear()
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		//m_splitter->removeWidget(*si);
		delete *si;
	}
	m_dockList.clear();
	oldAuxsSize = -1;
}

void AudioMixerApp::trackListChanged(TrackList* list)
{
	m_tracklist = list;
	//updateMixer(m_cmbRows->currentIndex());
}

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void AudioMixerApp::updateMixer(int index)
{
	m_dockList.clear();
	int rows = m_cmbRows->itemData(index).toInt();
	for(int i = 1; rows+1; ++i)
	{
		MixerDock *mixerRow = new MixerDock(this);
		mixerRow->setObjectName("MixerDock");
		m_dockList.push_back(mixerRow);
		m_splitter->addWidget(mixerRow);
	}

	if(m_tracklist->size())
	{
		int count = m_tracklist->size()-1;
		int rowCount,rem;
		getRowCount(count, rows, rowCount, rem);
	}
}

void AudioMixerApp::getRowCount(int trackCount, int rows, int& rowcount, int& remainder)
{
	int q;
	q = trackCount / rows;
	rowcount = q;
	remainder = trackCount - (rows * q);
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixerApp::configChanged()
{
	songChanged(SC_CONFIG);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioMixerApp::songChanged(int flags)
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		(*si)->songChanged(flags);
	}
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void AudioMixerApp::closeEvent(QCloseEvent* e)
{
	tconfig().set_property(objectName(), "geometry", geometry());
	tconfig().set_property(objectName(), "rows", m_cmbRows->currentIndex());
    tconfig().save();
	emit closed();
	e->accept();
}

void AudioMixerApp::hideEvent(QHideEvent*)
{
}

void AudioMixerApp::toggleShowEffectsRack(bool)
{
/*
	StripList::iterator si = stripList.begin();
	for (; si != stripList.end(); ++si)
	{
		Strip* audioStrip = qobject_cast<Strip*>(*si);
		if (audioStrip)
		{
			audioStrip->toggleAuxPanel(toggle);
		}

	}
	*/
}

//---------------------------------------------------------
//   toggleAudioPortConfig
//---------------------------------------------------------

void AudioMixerApp::toggleAudioPortConfig()
{
	showAudioPortConfig(routingId->isChecked());
}

//---------------------------------------------------------
//   showAudioPortConfig
//---------------------------------------------------------

void AudioMixerApp::showAudioPortConfig(bool on)
{
	if (on && routingDialog == 0)
	{
		routingDialog = oom->getRoutingDialog(true);
		connect(routingDialog, SIGNAL(closed()), SLOT(routingDialogClosed()));
	}
	if (routingDialog)
    {
        routingDialog->show();
    }
	routingId->setChecked(on);
}

