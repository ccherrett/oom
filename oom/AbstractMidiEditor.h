//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: AbstractMidiEditor.h,v 1.3.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ABSTRACTMIDIEDITOR_H__
#define __ABSTRACTMIDIEDITOR_H__

///#include "sig.h"
#include "al/sig.h"
#include "cobject.h"
#include <QList>

class QGridLayout;
class QWidget;

class PartList;
class Xml;
class EventCanvas;
class ScrollScale;
class CtrlEdit;
class MTScale;
class Part;
class WavePart;
class Event;

//---------------------------------------------------------
//   AbstractMidiEditor
//---------------------------------------------------------

class AbstractMidiEditor : public TopWin
{
    Q_OBJECT

    PartList* _pl;
    std::list<int> _parts;
    int _curDrumInstrument; // currently selected instrument if drum
    // editor
protected:
    ScrollScale* hscroll;
    ScrollScale* vscroll;
    MTScale* time;
    EventCanvas* canvas;

    QList<CtrlEdit*> ctrlEditList;
    int _quant, _raster;
    QGridLayout* mainGrid;
    QWidget* mainw;
    virtual void readStatus(Xml&);
    virtual void writeStatus(int, Xml&) const;
    void writePartList(int, Xml&) const;
    void genPartlist();

public slots:
    void songChanged(int type);
    void setCurDrumInstrument(int instr);

    virtual void updateHScrollRange()
    {
    };
signals:
    void curDrumInstrumentChanged(int);

public:
    AbstractMidiEditor(int, int, PartList*,
            QWidget* parent = 0, const char* name = 0);
    ~AbstractMidiEditor();

	bool hasPart(int sn);
	void addPart(Part* p);
	void addParts(PartList* p);
	void removePart(int sn);
	void removeParts(PartList*);
    int quantVal(int v) const;
    ///int rasterStep(unsigned tick) const   { return sigmap.rasterStep(tick, _raster); }
    ///unsigned rasterVal(unsigned v)  const { return sigmap.raster(v, _raster);  }
    ///unsigned rasterVal1(unsigned v) const { return sigmap.raster1(v, _raster); }
    ///unsigned rasterVal2(unsigned v) const { return sigmap.raster2(v, _raster); }

    int rasterStep(unsigned tick) const
    {
        return AL::sigmap.rasterStep(tick, _raster);
    }

    unsigned rasterVal(unsigned v) const
    {
        return AL::sigmap.raster(v, _raster);
    }

    unsigned rasterVal1(unsigned v) const
    {
        return AL::sigmap.raster1(v, _raster);
    }

    unsigned rasterVal2(unsigned v) const
    {
        return AL::sigmap.raster2(v, _raster);
    }

    int quant() const
    {
        return _quant;
    }

    void setQuant(int val)
    {
        _quant = val;
    }

    int raster() const
    {
        return _raster;
    }

    void setRaster(int val)
    {
        _raster = val;
    }

    PartList* parts()
    {
        return _pl;
    }

    int curDrumInstrument() const
    {
        return _curDrumInstrument;
    }
    Part* curCanvasPart();
	bool isEventSelected(Event e);
	QList<Event> getSelectedEvents();
    WavePart* curWavePart();
    virtual void setCurCanvasPart(Part*);
	virtual bool isGlobalEdit(){ return false; }
	virtual void updateCanvas() {}
};

#endif

