//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//=========================================================

#ifndef __MTRACKINFO_H__
#define __MTRACKINFO_H__

#include "ui_mtrackinfobase.h"
#include "pctablemodel.h"
#include "pctable.h"
#include <QToolBar>

class QToolButton;
class QTableWidget;
class QComboBox;

class Track;
class Part;
class GridCombo;

class QTableView;
class QStandardItem;
class QStandardItemModel;
class QModelIndex;
class QItemSelection;
class QItemSelectionModel;
class Patch;

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public QFrame/*QWidget*/, public Ui::MidiTrackInfoBase {
    Q_OBJECT

	//ToolBar1 merge
    GridCombo* quantLabel;
    QTableWidget* qlist;
    GridCombo* rasterLabel;
    QTableWidget* rlist;
	//end merge

    Track* selected;
    bool _midiDetect;
    int program, pan, volume;
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
    //tb1
	void _rasterChanged(int);
    void _quantChanged(int);
	//end

protected slots:
    virtual void heartBeat();

public slots:
    void setTrack(Track*);
    void configChanged();
    void instrPopup();
    void progRecClicked();
    void songChanged(int);
    void insertMatrixEvent(Part*, unsigned);
    void updateSize();
    void populateMatrix();
	void populatePatches();
    void updateTableHeader();
    void rebuildMatrix();
	void addSelectedPatch();
	void previewSelectedPatch();
	//tb1
    void setRaster(int);
    void setQuant(int);
	//end

signals:
    void outputPortChanged(int);
	void updateCurrentPatch(QString);
	void patchChanged(Patch*);
	//tb1
    void rasterChanged(int);
    void quantChanged(int);
    void soloChanged(bool);
    void toChanged(int);
	//end

public:
    MidiTrackInfo(QWidget*, Track* = 0, int rast = 96, int quant = 96);
	~MidiTrackInfo();

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
    void setLabelText();
    void setLabelFont();
    void updateTrackInfo(int);
    void setSolo(bool val);
};



#endif

