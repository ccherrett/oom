//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrledit.h,v 1.4.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include <QFrame>
#include <QPoint>

#include "ctrlcanvas.h"
#include "song.h"

class AbstractMidiEditor;
class QLabel;
class QMouseEvent;
class QResizeEvent;
class CtrlView;
class CtrlPanel;
class ResizeHandle;
class Xml;

#define CTRL_PANEL_FIXED_WIDTH 40
//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QFrame
{
    Q_OBJECT

    CtrlCanvas* canvas;
    CtrlPanel* panel;
	QWidget *vscale;
	ResizeHandle* handle; 

	bool m_collapsed;
	int m_collapsedHeight;
	int m_maxheight;
	int m_minheight;

protected:

	//virtual QSize sizeHint();
	//virtual QSize minimumSizeHint(){return QSize(400, 50);}


private slots:
    void destroyCalled();
	void collapsedCalled(bool);

public slots:
    void setTool(int tool);

    void setXPos(int val)
    {
        canvas->setXPos(val);
    }

    void setXMag(float val)
    {
        canvas->setXMag(val);
    }
    void setCanvasWidth(int w);
	void updateCanvas()
	{
		canvas->update();
	}
    void setPos(int idx, unsigned x, bool adjustScrollbar)
	{
		canvas->setPos(idx, x, adjustScrollbar);
	}
	void setMinHeight(int);
	void setMaxHeight(int);
	void updateHeight(int);

signals:
    void timeChanged(unsigned);
    void destroyedCtrl(CtrlEdit*);
    void enterCanvas();
    void yposChanged(int);
	void resizeEnded();
	void resizeStarted();
	void minHeightChanged(int);
	void maxHeightChanged(int);

public:
    CtrlEdit(QWidget*, AbstractMidiEditor* e, int xmag, int height = 80);
	virtual ~CtrlEdit(){}
    bool setType(QString);
	QString type();
	void setCollapsed(bool);
	bool collapsed() {return m_collapsed;}
	int expandedHeight() {return m_collapsedHeight;}
	void setExpandedHeight(int h) {m_collapsedHeight = h;}
	int minHeight(){return m_minheight;}
};

#endif

