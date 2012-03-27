//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Andrew Williams & Christopher Cherrett
//=========================================================

#ifndef __OOM_TEMPO_CANVAS_H__
#define __OOM_TEMPO_CANVAS_H__

#include "view.h"
#include "toolbars/tools.h"

class QMouseEvent;
class QPainter;
class QPoint;
class QRect;

class ScrollScale;

//---------------------------------------------------------
//   TempoCanvas
//---------------------------------------------------------

class TempoCanvas : public View
{
    Q_OBJECT

    enum DragMode
    {
        DRAG_OFF, DRAG_NEW, DRAG_MOVE_START, DRAG_MOVE,
        DRAG_DELETE, DRAG_COPY_START, DRAG_COPY,
        DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO
    };
    unsigned pos[4];
    QPoint start;
    QPoint lastPos;
    Tool tool;
    DragMode drag;
    bool m_drawLineMode;
	bool m_drawToolTip;
    int line1x;
    int line1y;
    int line2x;
    int line2y;
	int m_tempoStart;
	int m_tempoEnd;


    void draw(QPainter&, const QRect&);
    void newVal(int x1, int x2, int y);
	void newValRamp(int x1, int y1, int x2, int y2);
    bool deleteVal1(unsigned int x1, unsigned int x2);
    void deleteVal(int x1, int x2);
	int computeTempo(int y, int height);
	int tempoToPixel(int tempo, int height);

protected:
    virtual void pdraw(QPainter&, const QRect&);
    virtual void viewMouseReleaseEvent(QMouseEvent*);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    //virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent*e);
	virtual void enterEvent(QEvent*);
    virtual void drawOverlay(QPainter&, const QRect&);

signals:
    void followEvent(int);
    void xposChanged(int);
    void yposChanged(int);
    void timeChanged(unsigned);
    void tempoChanged(int);

public slots:
    void setPos(int, unsigned, bool adjustScrollbar);
    void setTool(int t);
	void setStartTempo(int tempo)
	{
		m_tempoStart = tempo;
		redraw();
	}
	void setEndTempo(int tempo)
	{
		m_tempoEnd = tempo;
		redraw();
	}

public:
    TempoCanvas(QWidget* parent, int xmag);
};

#endif

