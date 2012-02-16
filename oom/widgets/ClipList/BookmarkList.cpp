//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "BookmarkList.h"
#include "icons.h"
#include <QtGui>

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
	if(!data->hasUrls())/*{{{*/
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
		emit bookmarkAdded();
	}/*}}}*/
	return true;
}

/*Qt::ItemFlags BookmarkListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags def = QStandardItemModel::flags(index);
	return Qt::ItemIsDropEnabled | def;
}*/

BookmarkList::BookmarkList(QWidget* parent)
 : QListView(parent)
{
}

void BookmarkList::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void BookmarkList::dragMoveEvent(QDragMoveEvent *event)
{
	event->acceptProposedAction();
}

void BookmarkList::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void BookmarkList::dropEvent(QDropEvent *event)
{
	const QMimeData* data = event->mimeData();
	if(data->hasUrls())
	{
		//TODO: Add insert to the dropped row, except row 0
		QStandardItemModel* mod = (QStandardItemModel*)model();
		QString path = data->urls()[0].path(); 
		QFileInfo f(path);
		if(mod)
		{
			QList<QStandardItem*> items = mod->findItems(f.fileName());
			if(f.isDir() && items.isEmpty())
			{
				QStandardItem *it = new QStandardItem();
				it->setText(f.fileName());
				it->setData(path);
				it->setIcon(QIcon(":/images/icons/clip-folder-bookmark.png"));
				it->setDropEnabled(true);
				mod->appendRow(it);
			}
		}
	}
	event->acceptProposedAction();
}

