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

class TrackViewEditor : public QDialog, public Ui::TrackViewEditorBase 
{
	Q_OBJECT
	TrackViewList* _viewList;
	TrackView* _selected;
	bool _editing;
	bool _addmode;
	QStandardItemModel *m_model;
	QItemSelectionModel *m_selmodel;

	QStringList _trackTypes;
	QPushButton* btnAdd;
	QPushButton* btnRemove;
	QPushButton* btnOk;
	QPushButton* btnCancel;
	QPushButton* btnApply;

private slots:
	void cmbViewSelected(int);
	void cmbTypeSelected(int);
	void btnAddTrack(bool);
	void btnRemoveTrack(bool);
	void btnNewClicked(bool);
	void btnOkClicked(bool);
	void btnApplyClicked(bool);
	void btnCancelClicked(bool);
	void btnCopyClicked(bool);
	void btnDeleteClicked(bool);
	void btnDownClicked(bool);
	void btnUpClicked(bool);
	void txtNameEdited(QString);
	void chkRecordChecked(bool);
	void reset();
	void updateTableHeader();
	void settingsChanged(QStandardItem*);
    QList<int> getSelectedRows();

public:
	TrackViewEditor(QWidget*, TrackViewList* = 0);
	void setSelected(TrackView*);
	TrackView* selectedView( ) { return _selected; }
	void setTypes(QStringList);
	void setViews(TrackViewList*);
	QStringList trackTypes(){return _trackTypes;}
	TrackViewList* views(){return _viewList;}


private:
	QStringList buildViewList();
};

#endif
