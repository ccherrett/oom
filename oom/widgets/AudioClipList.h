//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_AUDIO_CLIPLIST_
#define _OOM_AUDIO_CLIPLIST_

#include "ui_AudioClipListBase.h"
#include <QFrame>
#include <QStringList>
#include <QModelIndex>
#include <QModelIndexList>
#include <QFileSystemModel>
#include <QStandardItemModel>

class QListView;
class QMimeData;

//Model for the file list
class ClipListModel : public QStandardItemModel
{
public:
	ClipListModel(QObject* parent = 0);
	virtual QStringList mimeTypes() const;
	virtual QMimeData* mimeData(const QModelIndexList&) const;
};

//Model for the bookmark list
class BookmarkListModel : public QStandardItemModel
{
public:
	BookmarkListModel(QObject* parent = 0);
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int col, const QModelIndex &parent);
//	virtual Qt::ItemFlags flags(const QModelIndex&) const;
};

class AudioClipList : public QFrame, public Ui::AudioClipListBase {
	Q_OBJECT
	
	//QFileSystemModel *m_listModel;
	ClipListModel *m_listModel;
	BookmarkListModel *m_bookmarkModel;
	QStringList m_filters;
	QString m_currentPath;

	bool isSupported(const QString&);
	void loadBookmarks();
	void saveBookmarks();

private slots:
	void playClicked();
	void stopClicked();
	void homeClicked();
	void forwardClicked();
	void rewindClicked();
	//void addBookmarkClicked();
	void fileItemSelected(const QModelIndex&);
	void bookmarkItemSelected(const QModelIndex&);

//public slots:
public:
	AudioClipList(QWidget *parent = 0);
	~AudioClipList();
	void setDir(const QString& path);
};

#endif
