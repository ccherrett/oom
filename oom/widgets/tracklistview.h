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
class QToolButton;
class QCheckBox;
class QPoint;
class QStringList;
class QShowEvent;

class Part;
class PartList;
class Track;
class AbstractMidiEditor;

class TrackListView : public QFrame
{
	Q_OBJECT

	QTableView* m_table;
	QStandardItemModel* m_model;
	QItemSelectionModel* m_selmodel;
	QVBoxLayout* m_layout;
	QHBoxLayout* m_buttonBox;
	AbstractMidiEditor* m_editor;
	QList<QString> m_selected;
	QToolButton* m_btnRefresh;;
	QCheckBox* m_chkWorkingView;
	QCheckBox* m_chkSnapToPart;
	int m_displayRole;
	int m_selectedIndex;
	QStringList m_headers;
	QPoint scrollPos;
	
	void updatePartSelection(Part*);


private slots:
	void songChanged(int);
	void displayRoleChanged(int);
	void contextPopupMenu(QPoint);
	void selectionChanged(const QModelIndex, const QModelIndex);
	void updateCheck(PartList*, bool);
	void updateCheck();
	void snapToPartChanged(int);

protected:
	virtual void showEvent(QShowEvent*);

public slots:
	void toggleTrackPart(QStandardItem*);
	void populateTable();

public:
	TrackListView(AbstractMidiEditor* e, QWidget* parent = 0);
	static void movePlaybackToPart(Part*);
	virtual ~TrackListView();
};

#endif
