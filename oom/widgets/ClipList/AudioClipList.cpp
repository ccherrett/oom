//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "AudioClipList.h"
#include "BookmarkList.h"
#include "AudioPlayer.h"
#include "config.h"
#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "song.h"
#include "icons.h"
#include "track.h"
#include "traverso_shared/TConfig.h"
#include "slider.h"
#include "mixer/meter.h"
#include "CreateTrackDialog.h"

#include <fastlog.h>
#include <math.h>

#include <QDir>
#include <QUrl>
#include <QList>
#include <QFileInfo>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QtGui>


ClipListModel::ClipListModel(QObject* parent)
 : QStandardItemModel(parent)
{
}

QStringList ClipListModel::mimeTypes() const
{
	return QStringList(QString("text/uri-list"));
}

QMimeData *ClipListModel::mimeData(const QModelIndexList& indices) const
{
	QList<QUrl> urls;
	foreach(QModelIndex index, indices)
	{
		QStandardItem* item = itemFromIndex(index);
		if(item)
		{
			urls << QUrl::fromLocalFile(item->data().toString());
		}
	}
	QMimeData* mdata = new QMimeData();
	mdata->setUrls(urls);
	return mdata;
}

static AudioPlayer player;

AudioClipList::AudioClipList(QWidget *parent)
 : QFrame(parent)
{
	setupUi(this);
	
	m_filters << "wav" << "ogg" << "mpt";
	m_watcher = new QFileSystemWatcher(this);
	m_active = true;

	m_bookmarkModel = new BookmarkListModel(this);
	m_bookmarkList->setModel(m_bookmarkModel);
	m_bookmarkList->setAcceptDrops(true);
	//m_bookmarkList->viewport()->setAcceptDrops(true);
	m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
	m_bookmarkList->setContextMenuPolicy(Qt::CustomContextMenu);

	m_listModel = new ClipListModel(this);
	m_fileList->setModel(m_listModel);

	//btnHome->setIcon(QIcon(*startIconSet3));
	
	//btnRewind->setIcon(QIcon(*rewindIconSet3));
	
	//btnForward->setIcon(QIcon(*forwardIconSet3));
	
	btnStop->setIcon(QIcon(*stopIconSetLeft));
	//btnStop->setChecked(true);
	
	btnPlay->setIcon(QIcon(*playIconSetRight));

	//QColor sliderBgColor = g_trackColorList.value(3);
	QColor sliderBgColor = g_trackColorListSelected.value(3);
	m_slider = new Slider(this, "vol", Qt::Horizontal, Slider::None, Slider::BgSlot, sliderBgColor, false);
	m_slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_slider->setCursorHoming(true);
	m_slider->setRange(config.minSlider - 0.1, 10.0);
	m_slider->setFixedHeight(15);
	m_slider->setFont(config.fonts[1]);
	m_slider->setIgnoreWheel(true);
	playerLayout->insertWidget(2, m_slider);

	connect(btnPlay, SIGNAL(toggled(bool)), this, SLOT(playClicked(bool)));
	connect(btnStop, SIGNAL(toggled(bool)), this, SLOT(stopClicked(bool)));
	//connect(btnRewind, SIGNAL(clicked()), this, SLOT(rewindClicked()));
	//connect(btnForward, SIGNAL(clicked()), this, SLOT(forwardClicked()));
	//connect(btnHome, SIGNAL(clicked()), this, SLOT(homeClicked()));
	//connect(btnBookmark, SIGNAL(clicked()), this, SLOT(addBookmarkClicked()));

	connect(&player, SIGNAL(playbackStopped(bool)), this, SLOT(stopSlotCalled(bool)));
	connect(&player, SIGNAL(timeChanged(const QString&)), this, SLOT(updateTime(const QString&)));
	connect(&player, SIGNAL(nowPlaying(const QString&)), this, SLOT(updateNowPlaying(const QString&)));
	connect(&player, SIGNAL(readyForPlay()), this, SLOT(playNextFile()));
	connect(this, SIGNAL(stopPlayback()), &player, SLOT(stop()));
	connect(m_slider, SIGNAL(sliderMoved(double, int)), &player, SLOT(setVolume(double)));

	connect(m_fileList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(fileItemSelected(const QModelIndex&)));
	connect(m_fileList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(fileItemContextMenu(const QPoint&)));

	connect(m_bookmarkList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(bookmarkItemSelected(const QModelIndex&)));
	connect(m_bookmarkList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(bookmarkContextMenu(const QPoint&)));

	connect(m_watcher, SIGNAL(directoryChanged(const QString)), this, SLOT(refresh()));

	QList<int> sizes;
	QString str = tconfig().get_property("AudioClipList", "sizes", "30 250").toString();
	QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
	foreach (QString s, sl)
	{
		int val = s.toInt();
		sizes.append(val);
	}
	splitter->setSizes(sizes);
	double volume = tconfig().get_property("AudioClipList", "volume", fast_log10(0.60*20.0)).toDouble();
	//m_slider->setValue(fast_log10(0.60*20.0));
	m_slider->setValue(volume);
	player.setVolume(volume);
	loadBookmarks();
	//setDir(QDir::currentPath());
	setDir(oomProject);
	updateLabels();
}

AudioClipList::~AudioClipList()
{
	QList<int> sizes = splitter->sizes();
	QStringList out;
	foreach(int s, sizes)
	{
		out << QString::number(s);
	}
	tconfig().set_property("AudioClipList", "sizes", out.join(" "));
	tconfig().set_property("AudioClipList", "volume", m_slider->value());
	saveBookmarks();
	tconfig().save();
	player.stop();
}

void AudioClipList::saveBookmarks()
{
	QStringList out;
	for(int i = 0; i < m_bookmarkModel->rowCount(); ++i)
	{
		QStandardItem* item = m_bookmarkModel->item(i);
		if(item)
		{
			out << item->data().toString();
		}
	}
	if(out.size())
		tconfig().set_property("AudioClipList", "bookmarks", out.join(","));
}

void AudioClipList::loadBookmarks()
{
	m_bookmarkModel->clear();
	QString defDir = QDir::homePath();
	QString str = tconfig().get_property("AudioClipList", "bookmarks", defDir).toString();
	QStringList sl = str.split(QString(","), QString::SkipEmptyParts);
	foreach (QString s, sl)
	{
		addBookmark(s);
	}
}

void AudioClipList::addBookmark(const QString &dir)
{
	QFileInfo f(dir);
	QList<QStandardItem*> items = m_bookmarkModel->findItems(f.fileName());
	if(f.isDir() && items.isEmpty())
	{
		QStandardItem *item = new QStandardItem();
		item->setText(f.fileName());
		item->setData(dir);
		item->setDropEnabled(true);
		item->setIcon(QIcon(":/images/icons/clip-folder-bookmark.png"));
		m_bookmarkModel->appendRow(item);
	}
}

void AudioClipList::setDir(const QString &path)
{
	QDir dir(path, "*", QDir::DirsFirst|QDir::LocaleAware, QDir::AllEntries | QDir::Readable|QDir::NoDotAndDotDot);
	m_listModel->clear();
	if(dir.isReadable())
	{
		QString newDir = dir.canonicalPath();
		if(!m_currentPath.isEmpty())
			m_watcher->removePath(m_currentPath);
		QString sep = dir.separator();
		QStringList entries = dir.entryList();
		
		QStandardItem* up = new QStandardItem("..");
		QString fullPath = QString(newDir).append(sep).append("..");
		up->setData(fullPath);
		up->setIcon(QIcon(":/images/icons/clip-folder-up.png"));
		m_listModel->appendRow(up);

		foreach(QString entry, entries)
		{
			//qDebug("Listing file: %s", entry.toUtf8().constData());
			
			QStandardItem* item = new QStandardItem(entry);
			fullPath = QString(newDir).append(sep).append(entry);
			item->setData(fullPath);
			bool skip = false;
			if(QFileInfo(fullPath).isDir())
			{//Set dir icon
				item->setIcon(QIcon(":/images/icons/clip-folder.png"));
			}
			else
			{//Set file icon
				item->setIcon(QIcon(":/images/icons/clip-file-audio.png"));
				skip = !isSupported(QFileInfo(fullPath).suffix());
			}
			if(!skip)
				m_listModel->appendRow(item);
		}
		m_currentPath = newDir;
		m_watcher->addPath(m_currentPath);
		txtPath->setText(m_currentPath);
		//qDebug("Listed directory: %s", m_currentPath.toUtf8().constData());
	}
}

void AudioClipList::refresh()
{
	if(!m_active)
		return;
	if(m_currentPath.isEmpty())
		setDir(oomProject);
	else
	{
		QFileInfo i(m_currentPath);
		if(i.exists())
			setDir(m_currentPath);
		else
			setDir(oomProject);
	}
	loadBookmarks();
}

bool AudioClipList::isSupported(const QString& suffix)
{
	return m_filters.contains(suffix);
}

void AudioClipList::fileItemContextMenu(const QPoint& pos)
{
	QModelIndex index = m_fileList->indexAt(pos);
	if(index.isValid())
	{
		QStandardItem* item = m_listModel->itemFromIndex(index);
		if(item)
		{
			QFileInfo f(item->data().toString());
			QMenu *m = new QMenu(QString(tr("ClipList Menu")), this);
			if(f.isDir())
			{
				//Not for the ..
				if(item->row())
				{
					QAction *add = m->addAction(QIcon(":/images/icons/clip-folder-bookmark.png"), QString(tr("Add To Bookmarks")));
					add->setData(1);
				}
			}
			else
			{
				QAction* add = m->addAction(QIcon(":/images/icons/clip-file-audio.png"), tr("Add to Selected Track"));
				add->setData(2);
				QAction* asnew = m->addAction(QIcon(":/images/icons/clip-file-audio.png"), tr("Add to New Track"));
				asnew->setData(3);
			}
			QAction* ref = m->addAction(QIcon(":/images/icons/clip-folder-refresh.png"), tr("Refresh"));
			ref->setData(4);
			QAction* act = m->exec(QCursor::pos());
			if(act)
			{
				int data = act->data().toInt();
				switch(data)
				{
					case 1:
					{
						addBookmark(item->data().toString());
						saveBookmarks();
					}
					break;
					case 2:
					{
						QList<qint64> selectedTracks = song->selectedTracks();
						if(selectedTracks.size() == 1)
						{//FIXME:If its greater we could present the user with a list to choose from
							Track* track = song->findTrackById(selectedTracks.at(0));
							QString text(f.filePath());
							if(track)
							{
								if (track->type() == Track::WAVE &&
										(f.suffix().endsWith("wav", Qt::CaseInsensitive) ||
										(f.suffix().endsWith("ogg", Qt::CaseInsensitive))))
								{
									oom->importWaveToTrack(text, song->cpos(), track);
								}
								else if ((track->isMidiTrack() || track->type() == Track::WAVE) && f.suffix().endsWith("mpt", Qt::CaseInsensitive))
								{//Who saves a wave part as anything but a wave file?
									oom->importPartToTrack(text, song->cpos(), track);
								}
							}
						}
					}
					break;
					case 3:
					{
						Track *track = 0;

						Track::TrackType t = Track::MIDI;
						if(f.suffix().endsWith("wav", Qt::CaseInsensitive) || f.suffix().endsWith("ogg", Qt::CaseInsensitive))
							t = Track::WAVE;
						VirtualTrack* vt;
						CreateTrackDialog *ctdialog = new CreateTrackDialog(&vt, t, -1, this);
						ctdialog->lockType(true);
						if(ctdialog->exec() && vt)
						{
							TrackManager* tman = new TrackManager();
							qint64 nid = tman->addTrack(vt);
							track = song->findTrackById(nid);
						}
						if(track)
						{
							QString text(f.filePath());
							if(track->type() == Track::WAVE)
								oom->importWaveToTrack(text, song->cpos(), track);
							else
								oom->importPartToTrack(text, song->cpos(), track);
						}
					}
					break;
					case 4:
						setDir(m_currentPath);
					break;
					default:
						qDebug("Not yet implemented!!");
					break;
				}
			}
		}
	}
}

void AudioClipList::bookmarkContextMenu(const QPoint& pos)
{
	QModelIndex index = m_bookmarkList->indexAt(pos);
	if(index.isValid() && index.row())//Dont remove the Home bookmark
	{
		QStandardItem* item = m_bookmarkModel->itemFromIndex(index);
		if(item)
		{
			QMenu *m = new QMenu(QString(tr("Bookmarks Menu")), this);
			QAction *del = m->addAction(QIcon(":/images/icons/clip-folder-bookmark.png"), QString(tr("Remove ")).append(item->text()));
			del->setData(1);
			QAction* act = m->exec(QCursor::pos());
			if(act)
			{
				int data = act->data().toInt();
				if(data == 1)
				{
					m_bookmarkModel->removeRow(item->row());
				}
			}
		}
	}
}

void AudioClipList::fileItemSelected(const QModelIndex& index)
{
	if(index.isValid())
	{
		QStandardItem *item = m_listModel->itemFromIndex(index);
		if(item && QFileInfo(item->data().toString()).isDir())
		{
			setDir(item->data().toString());
		}
		else
		{
			playClicked(true);
		}
	}
}

void AudioClipList::bookmarkItemSelected(const QModelIndex& index)
{
	if(index.isValid())
	{
		QStandardItem *item = m_bookmarkModel->itemFromIndex(index);
		if(item && QFileInfo(item->data().toString()).isDir())
		{
			setDir(item->data().toString());
		}
	}
}

void AudioClipList::updateTime(const QString& time)
{
	//timeLabel->setVisible(true);
	timeLabel->setText(time);
	if(btnStop->isChecked())
	{
		btnStop->blockSignals(true);
		btnStop->setChecked(false);
		btnStop->blockSignals(false);
	}
	if(!btnPlay->isChecked())
	{
		btnPlay->blockSignals(true);
		btnPlay->setChecked(true);
		btnPlay->blockSignals(false);
	}
}

void AudioClipList::updateNowPlaying(const QString& val)
{
	QStringList values = val.split("@--,--@");
	if(values.size())
	{	
		QFileInfo info(values[0]);
		QFont font = songLabel->font();
		int w = songLabel->width();
		QFontMetrics fm(font);
		songLabel->setText(fm.elidedText(info.fileName(), Qt::ElideRight, w));
		//songLabel->setText(info.fileName());
		songLabel->setToolTip(info.filePath());
		lengthLabel->setText(values[1]);
	}
	else
	{
		songLabel->setText(tr("Unknown"));
		songLabel->setToolTip("");
		lengthLabel->setText("00:00:00");
	}
}

void AudioClipList::updateLabels()
{
	if(m_currentSong.isEmpty())
	{
		songLabel->setText(tr("<None>"));
		songLabel->setToolTip("");
		lengthLabel->setText("00:00:00");
	}
	else
	{
		QFileInfo info(m_currentSong);
		QFont font = songLabel->font();
		int w = songLabel->width();
		QFontMetrics fm(font);
		songLabel->setText(fm.elidedText(info.fileName(), Qt::ElideRight, w));
	}
	timeLabel->setText("00:00:00");
}

void AudioClipList::resizeEvent(QResizeEvent* event)
{
	QFrame::resizeEvent(event);
	if(!m_currentSong.isEmpty())
		updateLabels();
}

using namespace QtConcurrent;
static void doPlay(const QString& file)
{
	player.play(file);
}

void AudioClipList::playNextFile()
{
	if(m_playlist.size())
	{
		m_currentSong = m_playlist.takeFirst();
		QFuture<void> pl = run(doPlay, m_currentSong);
		btnPlay->blockSignals(true);
		btnPlay->setChecked(true);
		btnPlay->blockSignals(false);
		btnStop->blockSignals(true);
		btnStop->setChecked(false);
		btnStop->blockSignals(false);
	}
}

void AudioClipList::playClicked(bool state)
{
	//qDebug("AudioClipList::playClicked: state: %d", state);

	if(state)
	{
		bool error_free = false;
		QModelIndex index = m_fileList->currentIndex();
		if(index.isValid())
		{
			QStandardItem* item = m_listModel->itemFromIndex(index);
			if(item)
			{
				QFileInfo info(item->data().toString());
				if(!info.isDir() && !info.suffix().endsWith("mpt"))
				{
					m_playlist.append(info.filePath());
					if(player.isPlaying())
						player.stop();
					else
						player.check();
					error_free = true;
				}
				else if(!m_currentSong.isEmpty())
				{
					m_playlist.append(info.filePath());
					if(player.isPlaying())
						player.stop();
					else
						player.check();
					error_free = true;
				}
			}
			else if(!m_currentSong.isEmpty())
			{
				m_playlist.append(m_currentSong);
				if(player.isPlaying())
					player.stop();
				else
					player.check();
				error_free = true;
			}
		}
		else if(!m_currentSong.isEmpty())
		{
			m_playlist.append(m_currentSong);
			if(player.isPlaying())
				player.stop();
			else
				player.check();
			error_free = true;
		}
		else
		{
			btnPlay->blockSignals(true);
			btnPlay->setChecked(false);
			btnPlay->blockSignals(false);
		}
		if(error_free)
		{
			btnStop->blockSignals(true);
			btnStop->setChecked(!state);
			btnStop->blockSignals(false);
		}
	}
	else
	{
		btnStop->setChecked(!state);
	}
}

void AudioClipList::stopSlotCalled(bool)
{
	btnStop->blockSignals(true);
	btnStop->setChecked(true);
	btnStop->blockSignals(false);

	btnPlay->blockSignals(true);
	btnPlay->setChecked(false);
	btnPlay->blockSignals(false);
}

void AudioClipList::stopClicked(bool state)
{
	if(state)
	{
		btnPlay->blockSignals(true);
		btnPlay->setChecked(!state);
		btnPlay->blockSignals(false);
		m_playlist.clear();
		emit stopPlayback();//Make player stop
		updateLabels();
	}
}

void AudioClipList::homeClicked()
{
	qDebug("AudioClipList::homeClicked");
}

void AudioClipList::rewindClicked()
{
	qDebug("AudioClipList::rewindClicked");
}

void AudioClipList::forwardClicked()
{
	qDebug("AudioClipList::forwardClicked");
}

/*void AudioClipList::addBookmarkClicked()
{
	qDebug("AudioClipList::addBookmarkClicked");
}*/
