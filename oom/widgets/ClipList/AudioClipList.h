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
class QResizeEvent;
class AudioPlayer;
class Slider;
class QFileSystemWatcher;


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
	
	ClipListModel *m_listModel;
	BookmarkListModel *m_bookmarkModel;
	Slider* m_slider;
	Slider* m_seekSlider;
	QStringList m_filters;
	QString m_currentPath;
	QString m_currentSong;
	QList<QString> m_playlist;
	QFileSystemWatcher* m_watcher;
	bool m_active;

	bool isSupported(const QString&);
	void addBookmark(const QString&);
	void updateLabels();

protected:
	virtual void resizeEvent(QResizeEvent*);

signals:
	void stopPlayback();

private slots:
	void playClicked(bool);
	void stopClicked(bool);
	void homeClicked();
	void forwardClicked();
	void rewindClicked();
	void saveBookmarks();

	void fileItemSelected(const QModelIndex&);
	void fileItemContextMenu(const QPoint&);
	void bookmarkItemSelected(const QModelIndex&);
	void bookmarkContextMenu(const QPoint&);
	
	void updateTime(const QString&);
	void updateSlider(int);

	void updateNowPlaying(const QString&, int samples);
	void playNextFile();
	void stopSlotCalled(bool); 
	void offlineSeek(int);

public slots:
	void refresh();
	void loadBookmarks();
	void setActive(bool v)
	{
		m_active = v;
	}

public:
	AudioClipList(QWidget *parent = 0);
	~AudioClipList();
	void setDir(const QString& path);
	bool active()
	{
		return m_active;
	}
};

#endif
