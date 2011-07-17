//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: pcanvas.h,v 1.11.2.4 2009/05/24 21:43:44 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PCANVAS_H__
#define __PCANVAS_H__

#include "song.h"
#include "canvas.h"
#include "trackautomationview.h"
#include <QHash>
#include <QIcon>
#include <QPixmap>
#include <QRect>
#include <QPainter>

class QDragMoveEvent;
class QDropEvent;
class QDragLeaveEvent;
class QMouseEvent;
class QKeyEvent;
class QEvent;
class QDragEnterEvent;

#define beats     4

//---------------------------------------------------------
//   NPart
//    ''visual'' Part
//    wraps Parts with additional information needed
//    for displaying
//---------------------------------------------------------

class NPart : public CItem
{
public:
    NPart(Part* e);

    const QString name() const
    {
        return part()->name();
    }

    void setName(const QString& s)
    {
        part()->setName(s);
    }

    Track* track() const
    {
        return part()->track();
    }
};

enum ControllerVals { doNothing, movingController, addNewController };
struct AutomationObject {
	CtrlVal *currentCtrlVal;
	CtrlList *currentCtrlList;
	Track *currentTrack;
	bool moveController;
	ControllerVals controllerState;
	QPoint mousePressPos;
};

class QLineEdit;
class MidiEditor;
class QMenu;
class Xml;
class CtrlVal;
class CtrlList;
class CurveNodeSelection;

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

class PartCanvas : public Canvas
{
    Q_OBJECT
    int* _raster;
    TrackList* tracks;

    Part* resizePart;
    QLineEdit* lineEditor;
    NPart* editPart;
    int curColorIndex;
    int trackOffset;
    bool editMode;
    bool unselectNodes;
	bool show_tip;

    AutomationObject automation;
    CurveNodeSelection* _curveNodeSelection;

	CItemList getSelectedItems();
    virtual void keyPress(QKeyEvent*);
    virtual void mousePress(QMouseEvent*);
    virtual void mouseMove(QMouseEvent* event);
    virtual void mouseRelease(const QPoint&);
    virtual void viewMouseDoubleClickEvent(QMouseEvent*);
    virtual void leaveEvent(QEvent*e);
    virtual void drawItem(QPainter&, const CItem*, const QRect&);
    virtual void drawMoving(QPainter&, const CItem*, const QRect&);
    virtual void updateSelection();
    virtual QPoint raster(const QPoint&) const;
    virtual int y2pitch(int y) const;
    virtual int pitch2y(int p) const;

    virtual void moveCanvasItems(CItemList&, int, int, DragType, int*);
    // Changed by T356.
    //virtual bool moveItem(CItem*, const QPoint&, DragType, int*);
    virtual bool moveItem(CItem*, const QPoint&, DragType);
    virtual CItem* newItem(const QPoint&, int);
    virtual void resizeItem(CItem*, bool);
    virtual void resizeItemLeft(CItem*, bool);
    virtual bool deleteItem(CItem*);
    virtual void startUndo(DragType);

    virtual void endUndo(DragType, int);
    virtual void startDrag(CItem*, DragType);
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dragMoveEvent(QDragMoveEvent*);
    virtual void dragLeaveEvent(QDragLeaveEvent*);
    virtual void viewDropEvent(QDropEvent*);

    virtual QMenu* genItemPopup(CItem*);
    virtual void itemPopup(CItem*, int, const QPoint&);

    void glueItem(CItem* item);
    void splitItem(CItem* item, const QPoint&);

    void copy(PartList*);
    void paste(bool clone = false, bool toTrack = true, bool doInsert = false);
    int pasteAt(const QString&, Track*, unsigned int, bool clone = false, bool toTrack = true);
    void movePartsTotheRight(unsigned int startTick, int length);
    //Part* readClone(Xml&, Track*, bool toTrack = true);
    void drawWavePart(QPainter&, const QRect&, WavePart*, const QRect&);
	void drawMidiPart(QPainter&, const QRect& rect, EventList* events, MidiTrack *mt, const QRect& r, int pTick, int from, int to, QColor c);
    Track* y2Track(int) const;
    void drawAudioTrack(QPainter& p, const QRect& r, AudioTrack* track);
    void drawAutomation(QPainter& p, const QRect& r, AudioTrack* track);
    void drawTopItem(QPainter& p, const QRect& rect);
	void drawTooltipText(QPainter& p, const QRect& rr, int height, double lazySelNodeVal, double lazySelNodePrevVal, int lazySelNodeFrame, bool paintAsDb, CtrlList*);

    void checkAutomation(Track * t, const QPoint& pointer, bool addNewCtrl);
    void selectAutomation(Track * t, const QPoint& pointer);
    void processAutomationMovements(QMouseEvent *event);    
    void addNewAutomation(QMouseEvent *event);    
	double getControlValue(CtrlList*, double);

protected:
    virtual void drawCanvas(QPainter&, const QRect&);

signals:
    void timeChanged(unsigned);
    void tracklistChanged();
    void dclickPart(Track*);
    void selectionChanged();
    void dropSongFile(const QString&);
    void dropMidiFile(const QString&);
    void setUsedTool(int);
    void trackChanged(Track*);
    void selectTrackAbove();
    void selectTrackBelow();
	void renameTrack(Track*);
	void moveSelectedTracks(int);

    void startEditor(PartList*, int);

//private slots:

public:

    enum
    {
        CMD_CUT_PART, CMD_COPY_PART, CMD_PASTE_PART, CMD_PASTE_CLONE_PART, CMD_PASTE_PART_TO_TRACK, CMD_PASTE_CLONE_PART_TO_TRACK,
	CMD_INSERT_PART, CMD_INSERT_EMPTYMEAS
    };

    PartCanvas(int* raster, QWidget* parent, int, int);
    void partsChanged();
    void cmd(int);
    void controllerChanged(Track *t);
    int track2Y(Track*) const;
	bool isEditing() { return editMode; }
	CItem* addPartAtCursor(Track*);
    virtual void newItem(CItem*, bool);
	static QIcon colorRect(const QColor& color, const QColor& color2, int width, int height, bool selected = false)//{{{
	{
		QPainter painter;
		QPixmap image(width, height);
		painter.begin(&image);
		painter.setBrush(selected ? color2 : color);
		QRect rectangle(0, 0, width, height);
		painter.drawRect(rectangle);
		painter.setPen(selected ? color : color2);
		painter.drawLine(0,(height/2)-1,width,(height/2)-1);
		painter.drawLine(0,(height/2),width,(height/2));
		painter.drawLine(0,(height/2)+1,width,(height/2)+1);
	
		painter.drawLine((width/2)-12,(height/2)+15,(width/2)-12,(height/2)-15);
		painter.drawLine((width/2)-13,(height/2)+15,(width/2)-13,(height/2)-15);
		painter.drawLine((width/2)-14,(height/2)+15,(width/2)-14,(height/2)-15);
		painter.drawLine((width/2)-18,(height/2)+5,(width/2)-18,(height/2)-5);
		painter.drawLine((width/2)-19,(height/2)+5,(width/2)-19,(height/2)-5);
		painter.drawLine((width/2)-20,(height/2)+10,(width/2)-20,(height/2)-10);
		painter.drawLine((width/2)-23,(height/2)+20,(width/2)-23,(height/2)-20);
		painter.drawLine((width/2)-24,(height/2)+10,(width/2)-24,(height/2)-10);
		painter.drawLine((width/2)-25,(height/2)+5,(width/2)-25,(height/2)-5);
	
		painter.drawLine((width/2)-5,(height/2)+15,(width/2)-5,(height/2)-15);
		painter.drawLine((width/2)-6,(height/2)+15,(width/2)-6,(height/2)-15);
		painter.drawLine((width/2)-7,(height/2)+15,(width/2)-7,(height/2)-15);
		painter.drawLine((width/2)-8,(height/2)+5,(width/2)-8,(height/2)-5);
		painter.drawLine((width/2)-9,(height/2)+5,(width/2)-9,(height/2)-5);
		
		painter.drawLine((width/2)+12,(height/2)+15,(width/2)+12,(height/2)-15);
		painter.drawLine((width/2)+13,(height/2)+15,(width/2)+13,(height/2)-15);
		painter.drawLine((width/2)+14,(height/2)+15,(width/2)+14,(height/2)-15);
		painter.drawLine((width/2)+18,(height/2)+5,(width/2)+18,(height/2)-5);
		painter.drawLine((width/2)+19,(height/2)+5,(width/2)+19,(height/2)-5);
		painter.drawLine((width/2)+20,(height/2)+10,(width/2)+20,(height/2)-10);
		painter.drawLine((width/2)+23,(height/2)+30,(width/2)+23,(height/2)-30);
		painter.drawLine((width/2)+24,(height/2)+20,(width/2)+24,(height/2)-20);
		painter.drawLine((width/2)+25,(height/2)+10,(width/2)+25,(height/2)-10);
		
		painter.end();
		QIcon icon(image);
		return icon;
	}//}}}

public slots:

    void returnPressed();
    void redirKeypress(QKeyEvent* e)
    {
        keyPress(e);
    }
	void trackViewChanged();
};
#endif
