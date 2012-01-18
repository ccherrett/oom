//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams
//=========================================================

#ifndef _LSCP_IMPORT_
#define _LSCP_IMPORT_
#include "ui_importbase.h"
#include <QFutureWatcher>

class QDialog;
class QStandardItem;
class QStandardItemModel;
class LSClient;
class QCloseEvent;
class QShowEvent;
class MidiInstrument;
class LSCPImport : public QDialog, public Ui::LSCPInstrumentBase
{
	Q_OBJECT

public:
	LSCPImport(QWidget* parent);

private slots:
	//void btnConnectClicked(bool);
	void btnListClicked(bool);
	void btnImportClicked(bool);
	void btnCloseClicked(bool);
	void btnSaveClicked(bool);
	void btnSelectAllClicked(bool);
	void portValueChanged(int);
	void retryValueChanged(int);
	void timeoutValueChanged(int);
	void hostValueChanged();
	void appendInstrument(int);
	void bankAsNumberChecked(bool);

signals:
	void instrumentsImported();

private:
	QStandardItemModel* _mapModel;
	//void redirLookup(int);
	LSClient* m_import_client;
	QFutureWatcher<MidiInstrument*> *m_futureWatcher;
	void updateTableHeader(bool list = false);

protected:
	virtual void closeEvent(QCloseEvent*);
	virtual void showEvent(QShowEvent*);
friend class LSClient;
};
#endif
