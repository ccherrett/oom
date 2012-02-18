//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef __TRACKVIEWEDITOR_H__
#define __TRACKVIEWEDITOR_H__

#include "ui_trackvieweditorbase.h"
#include <QList>
#include <QHash>
#include <QStringList>
#include <QObject>
#include "trackview.h"
#include "track.h"

class TrackView;
class Track;
class QDialog;
class QPushButton;
class QStandardItemModel;
class QStandardItem;
class QItemSelectionModel;
class QSortFilterProxyModel;
class VirtualTrack;


enum TrackSourceType { EXISTING = 0, VIRTUAL};

class TrackViewEditor : public QDialog, public Ui::TrackViewEditorBase 
{
	Q_OBJECT
	qint64 _selected;
	bool _editing;
	bool _addmode;
	bool m_templateMode;
	QStandardItemModel *m_model;
	QItemSelectionModel *m_selmodel;
	QStandardItemModel *m_allmodel;
	QItemSelectionModel *m_allselmodel;
	QSortFilterProxyModel* m_filterModel;

	QPushButton* btnOk;
	QPushButton* btnCancel;
	QPushButton* btnApply;

	QHash<qint64, VirtualTrack*> m_vtrackList;

	void buildViewList();
	void populateTypes();
	void setType(int);
    QList<int> getSelectedRows();

private slots:
	void cmbViewSelected(int);
	void cmbTypeSelected(int);
	void btnAddTrack();
	void btnAddVirtualTrack();
	void btnRemoveTrack();
	void btnNewClicked(bool);
	void btnOkClicked(bool);
	void btnApplyClicked(bool);
	void btnCancelClicked(bool);
	void btnCopyClicked();
	void btnDeleteClicked(bool);
	void btnDownClicked(bool);
	void btnUpClicked(bool);
	void txtNameEdited(QString);
	void txtCommentChanged();
	void chkRecordChecked(bool);
	void reset();
	void updateTableHeader();
	void settingsChanged(QStandardItem*);
	void populateTrackList();

public slots:
	void setMode(int moded); //0 == song local mode (old trackview mode), 1 == global template mode

public:
	TrackViewEditor(QWidget*, bool temp = false);
};

#endif
