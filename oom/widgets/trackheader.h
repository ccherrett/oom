//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_TRACKHEADER_H_
#define _OOM_TRACKHEADER_H_
#include "ui_trackheaderbase.h"

static const int MIN_TRACKHEIGHT = 58;

class Track;
class Knob;
class QAction;
class QSize;
class QMouseEvent;

class TrackHeader : public QFrame, public Ui::TrackHeaderBase
{
	Q_OBJECT
	Q_PROPERTY(bool selected READ isSelected WRITE setSelected)

	Track* m_track;
	Knob* m_pan;
	bool resizeFlag;
	bool m_selected;
    bool m_midiDetect;
	double panVal;
	double volume;
    int startY;
    int curY;
	bool inHeartBeat;
	bool m_editing;
	void initPan();

private slots:
	void heartBeat();
	void songChanged(int);
	void generateAutomationMenu();
	void toggleRecord(bool);
	void toggleMute(bool);
	void toggleSolo(bool);
	void toggleOffState(bool);
	void toggleReminder1(bool);
	void toggleReminder2(bool);
	void toggleReminder3(bool);
	void setSelected(bool);
	void updateTrackName();
	void generatePopupMenu();
	void panChanged(double);
	void panPressed();
	void panReleased();
	void panRightClicked(const QPoint &p);
	void updatePan();
	void updateVolume();
	void setEditing(bool edit = true)
	{
		m_editing = edit;
	}

public slots:

protected:
    enum
    {
        NORMAL, START_DRAG, DRAG, RESIZE
    } mode;

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);

public:
	TrackHeader(Track* track, QWidget* parent = 0);
	virtual ~TrackHeader(){}
	bool isSelected();
	bool isEditing()
	{
		return m_editing;
	}
};

#endif
