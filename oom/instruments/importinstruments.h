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

class QDialog;
class QStandardItem;
class QStandardItemModel;
class LSClient;
class QCloseEvent;
class LSCPImport : public QDialog, public Ui::LSCPInstrumentBase
{
	Q_OBJECT
	void updateTableHeader(bool list = false);
public:
	LSCPImport(QWidget* parent);
private slots:
	void btnConnectClicked(bool);
	void btnListClicked(bool);
	void btnImportClicked(bool);
	void btnCloseClicked(bool);
	void btnSaveClicked(bool);
signals:
	void instrumentsImported();
private:
	QStandardItemModel* _mapModel;
	LSClient* _client;
protected:
	virtual void closeEvent(QCloseEvent*);
};
#endif
