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
class LSCPImport : public QDialog, public Ui::LSCPInstrumentBase
{
	Q_OBJECT
public:
	LSCPImport(QWidget* parent);
private slots:
	void btnConnectClicked(bool);
	void btnRefreshClicked(bool);
	void btnImportClicked(bool);
	void btnCloseClicked(bool);
private:
	QStandardItemModel* _mapModel;
};
#endif
