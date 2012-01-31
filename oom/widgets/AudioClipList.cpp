//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "AudioClipList.h"
#include "config.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "icons.h"

#include <QDir>
#include <QUrl>
#include <QList>
#include <QFileInfo>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMimeData>


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

AudioClipList::AudioClipList(QWidget *parent)
 : QFrame(parent)
{
	setupUi(this);
	m_bookmarkModel = new QStandardItemModel(this);
	m_bookmarkList->setModel(m_bookmarkModel);
	//m_listModel = new QFileSystemModel(this);
	m_listModel = new ClipListModel(this);
	//m_listModel->setFilter(QDir::AllDirs|QDir::Files|QDir::Readable);
	//QStringList fTypes;
	m_filters << "wav" << "ogg";
	//m_listModel->setNameFilters(fTypes);
	//m_listModel->setRootPath(oomProject);
	m_fileList->setModel(m_listModel);

	btnHome->setIcon(QIcon(*startIconSet3));
	
	btnRewind->setIcon(QIcon(*rewindIconSet3));
	
	btnForward->setIcon(QIcon(*forwardIconSet3));
	
	btnStop->setIcon(QIcon(*stopIconSet3));
	
	btnPlay->setIcon(QIcon(*playIconSetRight));

	connect(btnPlay, SIGNAL(clicked()), this, SLOT(playClicked()));
	connect(btnStop, SIGNAL(clicked()), this, SLOT(stopClicked()));
	connect(btnRewind, SIGNAL(clicked()), this, SLOT(rewindClicked()));
	connect(btnForward, SIGNAL(clicked()), this, SLOT(forwardClicked()));
	connect(btnHome, SIGNAL(clicked()), this, SLOT(homeClicked()));
	connect(btnBookmark, SIGNAL(clicked()), this, SLOT(addBookmarkClicked()));

	connect(m_fileList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(fileItemSelected(const QModelIndex&)));
	connect(m_bookmarkList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(bookmarkItemSelected(const QModelIndex&)));

	QList<int> sizes;
	sizes << 30 << 250;
	splitter->setSizes(sizes);
	//setDir(QDir::currentPath());
	setDir(oomProject);
}

void AudioClipList::setDir(const QString &path)
{
	QDir dir(path, ""/*m_filters*/, QDir::DirsFirst);
	qDebug("Changing to directory: %s", path.toUtf8().constData());
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
					item->setIcon(QIcon(":/images/icons/clip-folder.png"));
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
	}
}

bool AudioClipList::isSupported(const QString& suffix)
{
	return m_filters.contains(suffix);
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
	Q_UNUSED(index);
}

void AudioClipList::playClicked()
{
	qDebug("AudioClipList::playClicked");
}

void AudioClipList::stopClicked()
{
	qDebug("AudioClipList::stopClicked");
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

void AudioClipList::addBookmarkClicked()
{
	qDebug("AudioClipList::addBookmarkClicked");
}

