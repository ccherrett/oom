//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_TRACKLISTVIEW_H_
#define _OOM_TRACKLISTVIEW_H_
#include <QFrame>
#include <QList>
#include <QModelIndex>
#define PartRole Qt::UserRole+2
#define TrackRole Qt::UserRole+3
#define TrackNameRole Qt::UserRole+4
#define TickRole Qt::UserRole+5

class QTableView;
class QStandardItem;
class QStandardItemModel;
class QItemSelectionModel;
class QString;
class QVBoxLayout;
class QHBoxLayout;
class QButtonGroup;
class QCheckBox;
class QPoint;
class QStringList;

class Part;
class PartList;
class Track;
class MidiEditor;

class TrackListView : public QFrame
{
	Q_OBJECT

	QTableView* m_table;
	QStandardItemModel* m_model;
	QItemSelectionModel* m_selmodel;
	QVBoxLayout* m_layout;
	QHBoxLayout* m_buttonBox;
	MidiEditor* m_editor;
	QList<QString> m_selected;
	QButtonGroup* m_buttons;
	QCheckBox* m_chkWorkingView;
	QCheckBox* m_chkSnapToPart;
	int m_displayRole;
	int m_selectedIndex;
	QStringList m_headers;
	
	void updatePartSelection(Part*);


private slots:
	void songChanged(int);
	void displayRoleChanged(int);
	void contextPopupMenu(QPoint);
	void selectionChanged(const QModelIndex, const QModelIndex);
	void updateCheck(PartList*, bool);
	void updateCheck();
	void snapToPartChanged(int);

public slots:
	void toggleTrackPart(QStandardItem*);

public:
	TrackListView(MidiEditor* e, QWidget* parent = 0);
	static void movePlaybackToPart(Part*);
	virtual ~TrackListView();
};

#endif
