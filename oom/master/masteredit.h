//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: masteredit.h,v 1.3.2.2 2009/04/01 01:37:11 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MASTER_EDIT_H__
#define __MASTER_EDIT_H__

#include "AbstractMidiEditor.h"
#include "noteinfo.h"
#include "cobject.h"

namespace Awl
{
    class SigEdit;
};
using Awl::SigEdit;

class QCloseEvent;
class QShowEvent;
class QKeyEvent;
class QToolBar;
class QToolButton;
class QComboBox;

class Master;
class ScrollScale;
class MTScale;
class SigScale;
class HitScale;
class TScale;
class TempoEdit;
class PosLabel;
class TempoLabel;
class EditToolBar;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

class MasterEdit : public AbstractMidiEditor
{
    Q_OBJECT

    Master* canvas;
    ScrollScale* hscroll;
    ScrollScale* vscroll;
    MTScale* time1;
    MTScale* time2;
    SigScale* sign;
    TScale* tscale;

    TempoEdit* curTempo;
    SigEdit* curSig;
    QComboBox* rasterLabel;
    PosLabel* cursorPos;
    TempoLabel* tempo;
    EditToolBar* editbar;

    static int _rasterInit;

private slots:
    void _setRaster(int);
    void posChanged(int, unsigned, bool);
    void setTime(unsigned);
    void setTempo(int);

public slots:
    void songChanged(int);

signals:
    void deleted(unsigned long);
    void keyPressExt(QKeyEvent*);

public:
    MasterEdit();
    ~MasterEdit();
    virtual void readStatus(Xml&);
    virtual void writeStatus(int, Xml&) const;
    static void readConfiguration(Xml&);
    static void writeConfiguration(int, Xml&);

protected:
	virtual void showEvent(QShowEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void closeEvent(QCloseEvent*);

};

#endif

