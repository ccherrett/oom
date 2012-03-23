//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#include <list>
#include <cmath>

#include <QApplication>
#include <QtGui>

#include "app.h"
#include "icons.h"
#include "AudioMixer.h"
#include "song.h"
#include "shortcuts.h"

#include "astrip.h"
//#include "mstrip.h"
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

AudioMixer::AudioMixer(const QString& title, QWidget* parent)
: QMainWindow(parent)
{
	setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding)); 
	setWindowTitle(title);
	setWindowIcon(*oomIcon);

	/*QWidget *dock = new QWidget(this);
	QVBoxLayout* dockBox = new QVBoxLayout(dock);
	dockBox->setContentMargins(0,0,0,0);
	dockBox->setSpacing(2);*/

	QDockWidget* m_mixerDock = new QDockWidget(tr("Mixer Views"), this);
	m_mixerDock->setAllowedAreas(Qt::LeftDockWidgetArea);
	m_mixerDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	addDockWidget(Qt::LeftDockWidgetArea, m_mixerDock);
	m_mixerView = new MixerView(this);
	QWidget* dockWidget = new QWidget(this);
	QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
	dockLayout->setContentsMargins(0,0,0,0);
	QLabel* logoLabel = new QLabel(dockWidget);
	logoLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	logoLabel->setPixmap(QPixmap(":/images/icons/oomidi_icon_the_mixer.png"));
	dockLayout->addWidget(logoLabel);
	dockLayout->addWidget(m_mixerView);
	m_mixerDock->setWidget(dockWidget);
	
	m_btnAux = new QPushButton(m_mixerDock);
	m_btnAux->setToolTip(tr("Show/hide Effects Rack"));
	m_btnAux->setShortcut(shortcuts[SHRT_TOGGLE_RACK].key);
	m_btnAux->setMaximumSize(QSize(20,16));
	m_btnAux->setObjectName("m_btnAux");
	m_btnAux->setCheckable(true);
	m_btnAux->setChecked(true);
	m_btnAux->setIcon(*expandIconSet3);
	m_btnAux->setIconSize(QSize(22,18));

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

AudioMixer::~AudioMixer()
{
	tconfig().set_property(objectName(), "geometry", geometry());
	tconfig().set_property(objectName(), "rows", m_cmbRows->currentIndex());
    tconfig().save();
}

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void AudioMixer::clear()
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		delete *si;
	}
	m_dockList.clear();
}

void AudioMixer::trackListChanged(TrackList* list)
{
	m_tracklist = list;
	updateMixer(m_cmbRows->currentIndex());
}

//---------------------------------------------------------
//   updateMixer
//---------------------------------------------------------

void AudioMixer::updateMixer(int index)/*{{{*/
{
	clear();
	int rows = m_cmbRows->itemData(index).toInt();
	for(int i = 1; i < rows+1; ++i)
	{
		MixerDock *mixerRow;
		if(i == 1)
			mixerRow = new MixerDock(MASTER, this);
		else
			mixerRow = new MixerDock(PANE, this);
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
			MixerDock *mixerRow = new MixerDock(MASTER, this);
			mixerRow->setObjectName("MixerDock");
			m_dockList.push_back(mixerRow);
			m_splitter->addWidget(mixerRow);
			TrackList* list = new TrackList();
			bool found = false;
			for(iTrack it = m_tracklist->begin(); it != m_tracklist->end(); ++it)
			{
				list->push_back(*it);
				if((*it)->name() == "Master")
				{
					found = true;
				}
			}
			if(!found)
			{
				Track* master = song->findTrack("Master");
				if(master)
				{
					list->push_back(master);
				}
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
			int last = rows-1;
			for (int i = 0; i < rows; ++i)
			{
				TrackList* list = new TrackList();
				if(i == 0)
				{
					Track* master = song->findTrack("Master");
					if(master)
					{
						list->push_back(master);
					}
					for(int c = 1; it != m_tracklist->end() && c <= rowCount; ++it, ++c)
					{
						list->push_back(*it);
					}
				}
				else if(i != last)
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
	if(rows > 1)/*{{{*/
	{
		int size = geometry().height() / rows;
		QList<int> sizelist;
		for(int i = 0; i < rows; ++i)
		{
			sizelist.append(size);
		}
		m_splitter->setSizes(sizelist);
	}/*}}}*/
}/*}}}*/

void AudioMixer::resizeEvent(QResizeEvent* event)/*{{{*/
{
	int rows = m_cmbRows->itemData(m_cmbRows->currentIndex()).toInt();
	if(rows > 1)
	{
		int size = event->size().height() / rows;
		QList<int> sizelist;
		for(int i = 0; i < rows; ++i)
		{
			sizelist.append(size);
		}
		m_splitter->setSizes(sizelist);
	}
}/*}}}*/

void AudioMixer::getRowCount(int trackCount, int rows, int& rowcount, int& remainder)/*{{{*/
{
	int q;
	q = trackCount / rows;
	rowcount = q;
	remainder = trackCount - (rows * q);
}/*}}}*/

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void AudioMixer::configChanged()
{
	songChanged(SC_CONFIG);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioMixer::songChanged(int flags)
{
	//printf("AudioMixer::songChanged\n");
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		(*si)->songChanged(flags);
	}
}

void AudioMixer::toggleAuxRack(bool toggle)
{
	DockList::iterator si = m_dockList.begin();
	for (; si != m_dockList.end(); ++si)
	{
		(*si)->toggleAuxRack(toggle);
	}
}

void AudioMixer::showEvent(QShowEvent* e)
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

void AudioMixer::closeEvent(QCloseEvent* e)
{
	tconfig().set_property(objectName(), "geometry", geometry());
	tconfig().set_property(objectName(), "rows", m_cmbRows->currentIndex());
    tconfig().save();
	emit closed();
	e->accept();
}

void AudioMixer::hideEvent(QHideEvent* e)
{
	if(!e->spontaneous())
	{
		tconfig().set_property(objectName(), "rows", m_cmbRows->currentIndex());
		if(song && !song->invalid)
			disconnect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	}
}

