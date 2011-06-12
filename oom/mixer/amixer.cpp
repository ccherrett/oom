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
	routingId = menuView->addAction(tr("Audio Connections Manager"), this, SLOT(toggleAudioPortConfig()));
	routingId->setCheckable(true);

	menuView->addSeparator();

	QActionGroup* actionItems = new QActionGroup(this);
	actionItems->setExclusive(false);

	menuView->addActions(actionItems->actions());

	/*QWidget *dock = new QWidget(this);
	QVBoxLayout* dockBox = new QVBoxLayout(dock);
	dockBox->setContentMargins(0,0,0,0);
	dockBox->setSpacing(2);*/

	QDockWidget* m_mixerDock = new QDockWidget(tr("Mixer Views"), this);
	m_mixerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::LeftDockWidgetArea, m_mixerDock);
	m_mixerView = new MixerView(this);
	m_mixerDock->setWidget(m_mixerView);
	
	m_btnAux = new QPushButton(m_mixerDock);
	m_btnAux->setToolTip(tr("Show/hide Effects Rack"));
	m_btnAux->setShortcut(shortcuts[SHRT_TOGGLE_RACK].key);
	m_btnAux->setMaximumSize(QSize(22,18));
	m_btnAux->setObjectName("m_btnAux");
	m_btnAux->setCheckable(true);
	m_btnAux->setChecked(true);
	m_btnAux->setIcon(*expandIcon);

	m_cmbRows = new QComboBox(m_mixerDock);
	for(int i = 1; i < 6; i++)
	{
		m_cmbRows->addItem(QString::number(i), i);
	}
	m_cmbRows->setCurrentIndex(1);
	m_mixerView->addButton(m_btnAux);
	m_mixerView->addButton(m_cmbRows);
	m_mixerView->addButton(new QLabel(tr("Rows ")));

	m_view = new QScrollArea();
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setCentralWidget(m_view);

	m_splitter = new QSplitter(Qt::Vertical, m_view);
	m_splitter->setHandleWidth(4);
	m_splitter->setChildrenCollapsible(true);
	m_view->setWidget(m_splitter);
	m_view->setWidgetResizable(true);

	/*layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);*/

	//connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));
	connect(m_cmbRows, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMixer(int)));
	connect(m_btnAux, SIGNAL(toggled(bool)), this, SLOT(toggleAuxRack(bool)));
	connect(m_mixerView, SIGNAL(trackListChanged(TrackList*)), this, SLOT(trackListChanged(TrackList*)));
}

AudioMixerApp::~AudioMixerApp()
{
	tconfig().set_property(objectName(), "geometry", geometry());
	tconfig().set_property(objectName(), "rows", m_cmbRows->currentIndex());
    tconfig().save();
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void AudioMixerApp::clear()
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		delete *si;
	}
	m_dockList.clear();
	oldAuxsSize = -1;
}

void AudioMixerApp::trackListChanged(TrackList* list)
{
	m_tracklist = list;
	updateMixer(m_cmbRows->currentIndex());
}

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void AudioMixerApp::updateMixer(int index)/*{{{*/
{
	clear();
	int rows = m_cmbRows->itemData(index).toInt();
	for(int i = 1; i < rows+1; ++i)
	{
		MixerDock *mixerRow = new MixerDock(PANE, this);
		mixerRow->setObjectName("MixerDock");
		m_dockList.push_back(mixerRow);
		m_splitter->addWidget(mixerRow);
	}

	if(m_tracklist->size())
	{
		int count = m_tracklist->size();
		int rowCount,rem,lastrow = 0;
		getRowCount(count, rows, rowCount, rem);
		if(!rowCount)
		{
			//printf("getRowcount return zero\n");
			clear();
			MixerDock *mixerRow = new MixerDock(PANE, this);
			mixerRow->setObjectName("MixerDock");
			m_dockList.push_back(mixerRow);
			m_splitter->addWidget(mixerRow);
			TrackList* list = new TrackList();
			for(iTrack it = m_tracklist->begin(); it != m_tracklist->end(); ++it)
			{
				list->push_back(*it);
			}
			mixerRow->setTracklist(list);

			m_cmbRows->blockSignals(true);
			m_cmbRows->setCurrentIndex(0);
			m_cmbRows->blockSignals(false);
		}
		else
		{
			if(rem && rem >= rows)
			{//split the remainder among the rows
				int add,addrem;
				getRowCount(rem, rows, add, addrem);
				rowCount = rowCount+add;
				lastrow = rowCount+addrem;
			}
			else
			{
				lastrow = rowCount+rem;
			}
			DockList::iterator si = m_dockList.begin();
			
			iTrack it = m_tracklist->begin();
			//printf("Tracklist size; %d\n", (int)m_tracklist->size());
			for (int i = 0; i < rows; ++i)
			{
				TrackList* list = new TrackList();
				if(i != (rows-1))
				{
					for(int c = 1; it != m_tracklist->end() && c <= rowCount; ++it, ++c)
					{
						list->push_back(*it);
					}
				}
				else
				{
					for(int c = 1; it != m_tracklist->end() && c <= lastrow; ++it, ++c)
					{
						list->push_back(*it);
					}
				}
				if(si != m_dockList.end())
					(*si)->setTracklist(list);
				++si;
			}
		}
	}
	else
	{
		//printf("TrackList is empty\n");
		DockList::iterator si = m_dockList.begin();
		for(; si != m_dockList.end(); ++si)
		{
			
			TrackList* list = new TrackList();
			(*si)->setTracklist(list);
		}
	}
}/*}}}*/

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

void AudioMixerApp::showEvent(QShowEvent* e)
{
	QRect geometry = tconfig().get_property(objectName(), "geometry", QRect(0,0,600, 600)).toRect();
	setGeometry(geometry);
	if(!e->spontaneous())
	{
		int rows = tconfig().get_property(objectName(), "rows", 1).toInt();
		m_cmbRows->blockSignals(true);
		m_cmbRows->setCurrentIndex(rows);
		m_cmbRows->blockSignals(false);
		connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
		m_mixerView->updateTrackList();
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

void AudioMixerApp::hideEvent(QHideEvent* e)
{
	if(!e->spontaneous())
	{
		if(song && !song->invalid)
			disconnect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	}
}

void AudioMixerApp::toggleAuxRack(bool toggle)
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		(*si)->toggleAuxRack(toggle);
	}
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

