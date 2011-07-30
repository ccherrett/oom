//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#ifndef __PIANO_H__
#define __PIANO_H__

#include "view.h"
#include <QList>

class QEvent;
class QMouseEvent;
class QPainter;
class QPixmap;
class QWheelEvent;
class AbstractMidiEditor;

#define KH  13

//---------------------------------------------------------
//   Piano
//---------------------------------------------------------

class Piano : public View
{
    int curPitch;
    QPixmap* c_keys[10];
    QPixmap* mk1;
    QPixmap* mk2;
    QPixmap* mk3;
    QPixmap* mk4;
    QPixmap* mk1_l;
    QPixmap* mk2_l;
    QPixmap* mk3_l;
    QPixmap* mk4_l;
    QPixmap* mk1_n;
    QPixmap* mk2_n;
    QPixmap* mk3_n;
    QPixmap* mk4_n;
    QPixmap* mk5_n;
    QPixmap* mk6_n;
    QPixmap* mk5_sn;
    QPixmap* mk6_sn;
    QPixmap* mk1_s;
    QPixmap* mk2_s;
    QPixmap* mk3_s;
    QPixmap* mk4_s;
    QPixmap* mk5_s;
    QPixmap* mk6_s;
    QPixmap* mk1_lswitch;
    QPixmap* mk2_lswitch;
    QPixmap* mk3_lswitch;
    QPixmap* mk4_lswitch;
    int keyDown;
    bool shift;
    int button;
	QList<int> enabled;
	QList<int> keyswitch;
	bool m_menu;

	AbstractMidiEditor *m_editor;

    Q_OBJECT
    int y2pitch(int) const;
    int pitch2y(int) const;
    void viewMouseMoveEvent(QMouseEvent* event);
	void setEditor(AbstractMidiEditor *e)
	{
		m_editor = e;
	}
	AbstractMidiEditor* editor()
	{
		return m_editor;
	}
    virtual void leaveEvent(QEvent*e);

    virtual void viewMousePressEvent(QMouseEvent* event);
    virtual void viewMouseReleaseEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent* e);

protected:
    virtual void draw(QPainter&, const QRect&);

signals:
    void pitchChanged(int);
    void keyPressed(int, int, bool);
    void keyReleased(int, bool);
    void redirectWheelEvent(QWheelEvent*);

public slots:
    void setPitch(int);

public:
    Piano(QWidget*, int, AbstractMidiEditor*);
	void setMIDIKeyBindings(QList<int>, QList<int>);
};

#endif

