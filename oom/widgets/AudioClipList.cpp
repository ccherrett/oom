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
#include "traverso_shared/TConfig.h"

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

BookmarkListModel::BookmarkListModel(QObject *parent)
 : QStandardItemModel(parent)
{
}

bool BookmarkListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int col, const QModelIndex &parent)
{
	Q_UNUSED(parent);
	if(action == Qt::IgnoreAction)
		return true;

	//Make sure we have urls as created by the fileList	
	if(!data->hasUrls())
	{
		qDebug("Drop is not a URL");
		return false;
	}

	//Single column for now
	if(col > 0)
		return false;
	
	int irow = row;
	if(irow == 0)
		irow = 1;

	QString path = data->urls()[0].path(); 
	QFileInfo f(path);
	QList<QStandardItem*> items = findItems(f.fileName());
	if(f.isDir() && items.isEmpty())
	{
		QStandardItem *it = new QStandardItem();
		it->setText(f.fileName());
		it->setData(path);
		it->setIcon(QIcon(":/images/icons/clip-folder-bookmark.png"));
		it->setDropEnabled(true);
		if(row)
			insertRow(irow, it);
		else
			appendRow(it);
	}
	return true;
}

/*Qt::ItemFlags BookmarkListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags def = QStandardItemModel::flags(index);
	return Qt::ItemIsDropEnabled | def;
}*/

AudioClipList::AudioClipList(QWidget *parent)
 : QFrame(parent)
{
	setupUi(this);
	
	m_filters << "wav" << "ogg";

	m_bookmarkModel = new BookmarkListModel(this);
	m_bookmarkList->setModel(m_bookmarkModel);
	m_bookmarkList->setAcceptDrops(true);
	m_bookmarkList->viewport()->setAcceptDrops(true);

	m_listModel = new ClipListModel(this);
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
	//connect(btnBookmark, SIGNAL(clicked()), this, SLOT(addBookmarkClicked()));

	connect(m_fileList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(fileItemSelected(const QModelIndex&)));
	connect(m_bookmarkList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(bookmarkItemSelected(const QModelIndex&)));

	QList<int> sizes;
	sizes << 30 << 250;
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
		QFileInfo f(s);
		QStandardItem *item = new QStandardItem();
		item->setText(f.fileName());
		item->setData(s);
		item->setDropEnabled(true);
		item->setIcon(QIcon(":/images/icons/clip-folder-bookmark.png"));
		m_bookmarkModel->appendRow(item);
	}
}

void AudioClipList::setDir(const QString &path)
{
	QDir dir(path, ""/*m_filters*/, QDir::DirsFirst);
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
		qDebug("Listed directory: %s", m_currentPath.toUtf8().constData());
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
	if(index.isValid())
	{
		QStandardItem *item = m_bookmarkModel->itemFromIndex(index);
		if(item && QFileInfo(item->data().toString()).isDir())
		{
			setDir(item->data().toString());
		}
	}
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

/*void AudioClipList::addBookmarkClicked()
{
	qDebug("AudioClipList::addBookmarkClicked");
}*/

