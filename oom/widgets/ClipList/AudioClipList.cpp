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
#include "song.h"
#include "icons.h"
#include "traverso_shared/TConfig.h"

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
	
	m_filters << "wav" << "ogg";

	m_bookmarkModel = new BookmarkListModel(this);
	m_bookmarkList->setModel(m_bookmarkModel);
	m_bookmarkList->setAcceptDrops(true);
	//m_bookmarkList->viewport()->setAcceptDrops(true);
	m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
	m_bookmarkList->setContextMenuPolicy(Qt::CustomContextMenu);

	m_listModel = new ClipListModel(this);
	m_fileList->setModel(m_listModel);

	btnHome->setIcon(QIcon(*startIconSet3));
	
	btnRewind->setIcon(QIcon(*rewindIconSet3));
	
	btnForward->setIcon(QIcon(*forwardIconSet3));
	
	btnStop->setIcon(QIcon(*stopIconSet3));
	//btnStop->setChecked(true);
	
	btnPlay->setIcon(QIcon(*playIconSetRight));

	connect(btnPlay, SIGNAL(toggled(bool)), this, SLOT(playClicked(bool)));
	connect(btnStop, SIGNAL(toggled(bool)), this, SLOT(stopClicked(bool)));
	connect(btnRewind, SIGNAL(clicked()), this, SLOT(rewindClicked()));
	connect(btnForward, SIGNAL(clicked()), this, SLOT(forwardClicked()));
	connect(btnHome, SIGNAL(clicked()), this, SLOT(homeClicked()));
	//connect(btnBookmark, SIGNAL(clicked()), this, SLOT(addBookmarkClicked()));
	connect(&player, SIGNAL(playbackStopped(bool)), this, SLOT(stopClicked(bool)));
	connect(&player, SIGNAL(timeChanged(const QString&)), this, SLOT(updateTime(const QString&)));
	connect(this, SIGNAL(stopPlayback()), &player, SLOT(stop()));

	connect(m_fileList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(fileItemSelected(const QModelIndex&)));
	connect(m_fileList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(fileItemContextMenu(const QPoint&)));

	connect(m_bookmarkList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(bookmarkItemSelected(const QModelIndex&)));
	connect(m_bookmarkList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(bookmarkContextMenu(const QPoint&)));

	QList<int> sizes;
	QString str = tconfig().get_property("AudioClipList", "sizes", "30 250").toString();
	QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
	foreach (QString s, sl)
	{
		int val = s.toInt();
		sizes.append(val);
	}
	splitter->setSizes(sizes);
	loadBookmarks();
	//setDir(QDir::currentPath());
	setDir(oomProject);
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
	saveBookmarks();
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
	QDir dir(path, "", QDir::DirsFirst);
	m_listModel->clear();
	if(dir.isReadable())
	{
		QString newDir = dir.canonicalPath();
		QString sep = dir.separator();
		QStringList entries = dir.entryList();
		foreach(QString entry, entries)
		{
			if(entry == ".")
				continue;
		//qDebug("Listing file: %s", entry.toUtf8().constData());
			QStandardItem* item = new QStandardItem(entry);
			QString fullPath = QString(newDir).append(sep).append(entry);
			item->setData(fullPath);
			bool skip = false;
			if(QFileInfo(fullPath).isDir())
			{//Set dir icon
				if(m_listModel->rowCount())
				{
					item->setIcon(QIcon(":/images/icons/clip-folder.png"));
				}
				else
				{
					item->setIcon(QIcon(":/images/icons/clip-folder-up.png"));
				}
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
		txtPath->setText(m_currentPath);
		//qDebug("Listed directory: %s", m_currentPath.toUtf8().constData());
	}
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
	timeLabel->setVisible(true);
	timeLabel->setText(time);
}

using namespace QtConcurrent;
static void doPlay(const QString& file)
{
	player.play(file);
}

void AudioClipList::playClicked(bool state)
{
	qDebug("AudioClipList::playClicked: state: %d", state);

	if(state)
	{
		btnStop->blockSignals(true);
		btnStop->setChecked(!state);
		btnStop->blockSignals(false);

		QModelIndex index = m_fileList->currentIndex();
		if(index.isValid())
		{
			QStandardItem* item = m_listModel->itemFromIndex(index);
			if(item)
			{
				QFileInfo info(item->data().toString());
				if(!info.isDir())
				{
					if(player.isPlaying())
						player.stop();
					QFuture<void> pl = run(doPlay, info.filePath());
				}
			}
		}
	}
}

void AudioClipList::stopClicked(bool state)
{
	qDebug("AudioClipList::stopClicked:state: %d", state);
	btnStop->blockSignals(true);
	btnStop->setChecked(true);
	btnStop->blockSignals(false);
	//TODO: Stop playback is playing
	if(state)
	{
		btnPlay->blockSignals(true);
		btnPlay->setChecked(!state);
		btnPlay->blockSignals(false);
		emit stopPlayback();//Make player stop
		timeLabel->setVisible(false);
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

