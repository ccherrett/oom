//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: editinstrument.h,v 1.1.1.1.2.4 2009/05/31 05:12:12 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDITINSTRUMENT_H__
#define __EDITINSTRUMENT_H__

#include "ui_editinstrumentbase.h"
#include "minstrument.h"
#include "midictrl.h"
#include "config.h"
#ifdef LSCP_SUPPORT
#include "importinstruments.h"
#endif

class QDialog;
class QMenu;
class QCloseEvent;
class Knob;
class DoubleLabel;

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public QMainWindow, public Ui::EditInstrumentBase
{
    Q_OBJECT

    MidiInstrument workingInstrument;
    QListWidgetItem* oldMidiInstrument;
    QTreeWidgetItem* oldPatchItem;
    
	Knob* m_panKnob;
	DoubleLabel* m_panLabel;
	Knob* m_auxKnob;
	DoubleLabel* m_auxLabel;
	bool m_loading;

	void closeEvent(QCloseEvent*);
	int checkDirty(MidiInstrument*, bool isClose = false);
    bool fileSave(MidiInstrument*, const QString&);
    void saveAs();
    void updateInstrument(MidiInstrument*);
    void updatePatch(MidiInstrument*, Patch*);
    void updatePatchGroup(MidiInstrument*, PatchGroup*);
    void changeInstrument();
    QTreeWidgetItem* addControllerToView(MidiController* mctrl);
    QString getPatchItemText(int);
    void enableDefaultControls(bool, bool);
    void setDefaultPatchName(int);
    int getDefaultPatchNumber();
    void setDefaultPatchNumbers(int);
    void setDefaultPatchControls(int);
    QString getPatchName(int);
    void deleteInstrument(QListWidgetItem*);
		
#ifdef LSCP_SUPPORT
	LSCPImport* import;
#endif
    ///QMenu* patchpopup;

private slots:
    virtual void fileNew();
    virtual void fileOpen();
    virtual void fileSave();
    virtual void fileSaveAs();
    virtual void fileExit();
    virtual void helpWhatsThis();
    void instrumentChanged();
    void tabChanged(QWidget*);
    void patchChanged();
    void controllerChanged();
    //void instrumentNameChanged(const QString&);
    void instrumentNameReturn();
    void patchNameReturn();
    void deletePatchClicked();
    void newPatchClicked();
    void newGroupClicked();
    void patchButtonClicked();
    void defPatchChanged(int);
    //void newCategoryClicked();
    void deleteControllerClicked();
    void newControllerClicked();
    void addControllerClicked();
    void ctrlTypeChanged(int);
    //void ctrlNameChanged(const QString&);
    void ctrlNameReturn();
    void ctrlHNumChanged(int);
    void ctrlLNumChanged(int);
    void ctrlMinChanged(int);
    void ctrlMaxChanged(int);
    void ctrlDefaultChanged(int);
	void populateInstruments();
    //void sysexChanged();
    //void deleteSysexClicked();
    //void newSysexClicked();
    void ctrlNullParamHChanged(int);
    void ctrlNullParamLChanged(int);
	void autoLoadChecked(bool);
	void volumeChanged(double);
	void engineChanged(int);
	void loadmodeChanged(int);
	void patchFilenameChanged();
	void browseFilenameClicked();
	void panChanged(double);
	void auxChanged(double);
#ifdef LSCP_SUPPORT
	void btnImportClicked(bool);
#endif

public:
    EditInstrument(QWidget* parent = 0, Qt::WFlags fl = Qt::Window);
};

#endif

