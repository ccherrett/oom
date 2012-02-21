//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: PerformerCanvas.h,v 1.5.2.6 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PERFORMERCANVAS_H__
#define __PERFORMERCANVAS_H__

#include "EventCanvas.h"
#include "Performer.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>

#define KH        13

//---------------------------------------------------------
//   NEvent
//    ''visual'' Note Event
//---------------------------------------------------------

class NEvent : public CItem
{
public:
    NEvent(Event& e, Part* p, int y);
};

class ScrollScale;
class Performer;
class QRect;

//---------------------------------------------------------
//   PerformerCanvas
//---------------------------------------------------------

class PerformerCanvas : public EventCanvas
{
    Q_OBJECT

    int cmdRange;
    int colorMode;
    int playedPitch;
    int _octaveQwerty;
	bool m_globalKey;
    bool _playMoveEvent;

    QMap<QString, int> _qwertyToMidiMap;
    void bindQwertyKeyToMidiValue(const char* key, int note);

    virtual void viewMouseDoubleClickEvent(QMouseEvent*);
    virtual void drawItem(QPainter&, const CItem*, const QRect&);
    void drawTopItem(QPainter &p, const QRect &rect);
    virtual void drawOverlay(QPainter& p, const QRect&);
    virtual void drawMoving(QPainter&, const CItem*, const QRect&);
    virtual void moveCanvasItems(CItemList&, int, int, DragType, int*);
    // Changed by T356.
    //virtual bool moveItem(CItem*, const QPoint&, DragType, int*);
    virtual bool moveItem(CItem*, const QPoint&, DragType);
    virtual CItem* newItem(const QPoint&, int);
    virtual void resizeItem(CItem*, bool noSnap);
    virtual void resizeItemLeft(CItem*, QPoint, bool){}
    virtual void newItem(CItem*, bool noSnap);
    virtual bool deleteItem(CItem*);
    virtual void startDrag(CItem* item, bool copymode);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent*);
    virtual void dragLeaveEvent(QDragLeaveEvent*);
    virtual void selectLasso(bool toggle);
    virtual void addItem(Part*, Event&);

    int y2pitch(int) const;
    int pitch2y(int) const;
	void processKeySwitches(Part*, int, int);
    virtual void drawCanvas(QPainter&, const QRect&);
    void quantize(int, int, bool);
    void copy();
    void paste();
    virtual void itemPressed(const CItem*);
    virtual void itemReleased(const CItem*, const QPoint&);
    virtual void itemMoved(const CItem*, const QPoint&);
    virtual void curPartChanged();
    virtual void resizeEvent(QResizeEvent*);
	void stepInputNote(Part*, unsigned, int, int);


private slots:
    void midiNote(int pitch, int velo);

signals:
    void quantChanged(int);
    void rasterChanged(int);
    void newWidth(int);
	void partChanged(Part*);

public slots:
    void pianoCmd(int);
    void pianoPressed(int pitch, int velocity, bool shift);
    void pianoReleased(int pitch, bool);

    void createQWertyToMidiBindings();
    void setOctaveQwerty(int octave);
	void setGlobalKey(bool k) { m_globalKey = k; }
	void recordArmAll();
	void globalTransposeClicked(bool);
	void toggleComments(bool);

public:

    enum
    {
	CMD_CUT, CMD_COPY, CMD_PASTE, CMD_DEL,
	CMD_OVER_QUANTIZE, CMD_ON_QUANTIZE, CMD_ONOFF_QUANTIZE,
	CMD_ITERATIVE_QUANTIZE,
	CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
	CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PREV_PART, CMD_SELECT_NEXT_PART,
	CMD_MODIFY_GATE_TIME, CMD_MODIFY_VELOCITY,
	CMD_CRESCENDO, CMD_TRANSPOSE, CMD_THIN_OUT, CMD_ERASE_EVENT,
	CMD_NOTE_SHIFT, CMD_MOVE_CLOCK, CMD_COPY_MEASURE,
	CMD_ERASE_MEASURE, CMD_DELETE_MEASURE, CMD_CREATE_MEASURE,
	CMD_FIXED_LEN, CMD_DELETE_OVERLAPS, CMD_ADD_PROGRAM, CMD_DELETE_PROGRAM, CMD_COPY_PROGRAM
    };

    PerformerCanvas(AbstractMidiEditor*, QWidget*, int, int);
    void cmd(int, int, int, bool, int);
    int stepInputQwerty(QKeyEvent* event);

    void setColorMode(int mode)
    {
		colorMode = mode;
		redraw();
    }
    virtual void modifySelected(NoteInfo::ValType type, int delta, bool strict = false);
	virtual bool isEventSelected(Event);
	void doModify(NoteInfo::ValType type, int delta, CItem*, bool);
};
#endif

