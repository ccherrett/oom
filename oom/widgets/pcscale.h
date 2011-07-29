//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: mtscale.h,v 1.3 2004/04/27 22:27:06 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PCSCALE_H__
#define __PCSCALE_H__

#include "view.h"
#include "Performer.h"
#include "midictrl.h"
//#include "audio.h"

enum ProgramChangeVals { doNothing, movingController, selectedController };
struct ProgramChangeObject {
	Event event;
	Part *part;
	bool valid;
	QPoint mousePressPos;
	unsigned starttick;
	ProgramChangeVals state;
	inline bool operator==(ProgramChangeObject pco)
	{
		return pco.part == part && pco.event == event && pco.state == state;
	}
};
//---------------------------------------------------------
//   PCScale
//    program change scale for midi track
//---------------------------------------------------------

class PCScale : public View
{
    Q_OBJECT
    Performer* currentEditor;
    int* raster;
    unsigned pos[4];
    int button;
    bool barLocator;
    bool waveMode;
	int m_clickpos;
    //Audio* audio;
	ProgramChangeObject _pc;
	QList<ProgramChangeObject> selectionList;


private slots:
    void songChanged(int);
	void deleteProgramChangeClicked(bool);
	void changeProgramChangeClicked(int, QString);

protected:
    virtual void pdraw(QPainter&, const QRect&);
    virtual void viewMousePressEvent(QMouseEvent* event);
    virtual void viewMouseMoveEvent(QMouseEvent* event);
    virtual void viewMouseReleaseEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent*e);

signals:
    void selectInstrument();
    void addProgramChange(Part*, unsigned);
	void drawSelectedProgram(int, bool);

public slots:
    void setPos(int, unsigned, bool);
    void updateProgram();
	void copySelected(bool);
	void moveSelected(int);
	bool selectProgramChange(int x);
	void deleteProgramChange(Event);
    //void setAudio(Audio*);

public:
    PCScale(int* raster, QWidget* parent, Performer* editor, int xscale, bool f = false);

    void setBarLocator(bool f)
    {
	barLocator = f;
    }
    void setEditor(Performer*);

    Performer* getEditor()
    {
	return currentEditor;
    }
};
#endif

