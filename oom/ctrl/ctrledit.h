//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrledit.h,v 1.4.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include <QWidget>

#include "ctrlcanvas.h"
#include "song.h"

class AbstractMidiEditor;
class QLabel;
class CtrlView;
class CtrlPanel;
class Xml;

#define CTRL_PANEL_FIXED_WIDTH 40
//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QWidget
{
    Q_OBJECT

    CtrlCanvas* canvas;
    CtrlPanel* panel;
	QWidget *vscale;

	bool m_collapsed;

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
signals:
    void timeChanged(unsigned);
    void destroyedCtrl(CtrlEdit*);
    void enterCanvas();
    void yposChanged(int);

public:
    CtrlEdit(QWidget*, AbstractMidiEditor* e, int xmag, bool expand = false, const char* name = 0);
	virtual ~CtrlEdit(){}
    void readStatus(Xml&);
    void writeStatus(int, Xml&);
    bool setType(QString);
	QString type();
	void setCollapsed(bool);
	bool collapsed() {return m_collapsed;}
};

#endif

