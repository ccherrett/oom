//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//=========================================================

#ifndef __CONDUCTOR_H__
#define __CONDUCTOR_H__

#include "ui_ConductorBase.h"
#include "pctablemodel.h"
#include "pctable.h"
#include <QToolBar>

#define ConductorPatchIdRole Qt::UserRole+2

class QToolButton;
class QComboBox;

class Track;
class Part;

class QTableView;
class QStandardItem;
class QStandardItemModel;
class QModelIndex;
class QItemSelection;
class QItemSelectionModel;
class Patch;

//---------------------------------------------------------
//   Conductor
//---------------------------------------------------------

class Conductor : public QFrame/*QWidget*/, public Ui::ConductorBase {
    Q_OBJECT

    Track* selected;
    bool _midiDetect;
    int program, pan, volume, hbank, lbank;
    int _progRowNum, _selectedIndex;
    ProgramChangeTable* tableView;
    QList<int>* _matrix;
    ProgramChangeTableModel* _tableModel;
    QItemSelectionModel* _selModel;
    QItemSelectionModel* _patchSelModel;
    QStandardItemModel* _patchModel;
    bool editing;
    bool _useMatrix;
    bool _autoExapand;
    bool _resetProgram;
    bool m_globalState;
    Part* m_eventPart;
    virtual void showEvent(QShowEvent *);

private slots:
    void iOutputChannelChanged(int);
    void iOutputPortChanged(int);
    void iProgHBankChanged();
    void iProgLBankChanged();
    void iProgramChanged();
    void iProgramDoubleClicked();
    void iLautstChanged(int);
    void iLautstDoubleClicked();
    void iTranspChanged(int);
    void iAnschlChanged(int);
    void iVerzChanged(int);
    void iLenChanged(int);
    void iKomprChanged(int);
    void iPanChanged(int);
    void iPanDoubleClicked();
    void recordClicked();
    void volRecClicked();
    void panRecClicked();
    void recEchoToggled(bool);
    void inRoutesPressed();
    void outRoutesPressed();
    void routingPopupMenuActivated(QAction*);
    //void routingPopupViewActivated(const QModelIndex&);
    void toggleAdvanced(int);
    void matrixItemChanged(QStandardItem*);
    void matrixSelectionChanged(QItemSelection, QItemSelection);
    void deleteSelectedPatches(bool);
    void movePatchUp(bool);
    void movePatchDown(bool);
    void patchSequenceInserted(QModelIndex, int, int);
    void patchSequenceRemoved(QModelIndex, int, int);
    void patchSequenceClicked(QModelIndex);
    void clonePatchSequence();
    void patchDoubleClicked(QModelIndex);
    void patchClicked(QModelIndex);
    void patchSelectionChanged(QItemSelection, QItemSelection);
    void editorPartChanged(Part*);
    void transposeStateChanged(bool);
	void toggleSynthGui(bool);
	void instrumentChangeRequested(qint64, const QString&, int);
	void generateInstrumentMenu();

protected slots:
    virtual void heartBeat();

public slots:
    void setTrack(Track*);
    void configChanged();
    void instrPopup();
    void progRecClicked();
    void progRecClicked(Track*);
    void songChanged(int);
    void insertMatrixEvent(Part*, unsigned);
    void updateSize();
    void populateMatrix();
    void populatePatches();
    void updateTableHeader();
    void rebuildMatrix();
    void addSelectedPatch();
    void previewSelectedPatch();
    void updateCommentState(bool, bool = true);

    void setGlobalState(bool state) {
        m_globalState = state;
    }

signals:
    void outputPortChanged(int);
    void updateCurrentPatch(QString);
    void patchChanged(Patch*);
    void globalTransposeClicked(bool);
    void toggleComments(bool);

public:
    Conductor(QWidget*, Track* = 0);
    ~Conductor();

    Track* track() const {
	return selected;
    }
    QTableView* getView() const
    {
	    return tableView;
    }
    QTreeView* getPatchListview() const
    {
	    return patchList;
    }
    void updateConductor(int);
    void setSolo(bool val);
};



#endif

