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
#include <QStandardItemModel>

class QListView;
class QMimeData;
class QPoint;

//Model for the file list
class ClipListModel : public QStandardItemModel
{
public:
	ClipListModel(QObject* parent = 0);
	virtual QStringList mimeTypes() const;
	virtual QMimeData* mimeData(const QModelIndexList&) const;
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
	void addBookmark(const QString&);

private slots:
	void playClicked();
	void stopClicked();
	void homeClicked();
	void forwardClicked();
	void rewindClicked();
	//void addBookmarkClicked();
	void fileItemSelected(const QModelIndex&);
	void fileItemContextMenu(const QPoint&);
	void bookmarkItemSelected(const QModelIndex&);
	void bookmarkContextMenu(const QPoint&);

//public slots:
public:
	AudioClipList(QWidget *parent = 0);
	~AudioClipList();
	void setDir(const QString& path);
};

#endif
