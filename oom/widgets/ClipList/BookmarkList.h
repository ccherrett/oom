//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_BOOKMARK_LIST_
#define _OOM_BOOKMARK_LIST_

#include <QListView>
#include <QStandardItemModel>
#include <QModelIndex>

class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMimeData;


//Model for the bookmark list
class BookmarkListModel : public QStandardItemModel
{
	Q_OBJECT

signals:
	void bookmarkAdded();

public:
	BookmarkListModel(QObject* parent = 0);
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int col, const QModelIndex &parent);
};

//Subclass for the bookmark list
class BookmarkList : public QListView
{
protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dropEvent(QDropEvent *event);

public:
	BookmarkList(QWidget* parent = 0);
};

#endif
