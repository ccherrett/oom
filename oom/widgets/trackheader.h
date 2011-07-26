//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_TRACKHEADER_H_
#define _OOM_TRACKHEADER_H_
#include "ui_trackheaderbase.h"
#include "globaldefs.h"
#include <QHash>

static const int MIN_TRACKHEIGHT = 78;
static const int MIN_TRACKHEIGHT_SLIDER = 78;
static const int MIN_TRACKHEIGHT_VU = 98;

class Track;
class Knob;
class QAction;
class QSize;
class QMouseEvent;
class QResizeEvent;
class QPoint;
class Slider;
class Meter;

class TrackHeader : public QFrame, public Ui::TrackHeaderBase
{
	Q_OBJECT
	Q_PROPERTY(bool selected READ isSelected WRITE setSelected)

	Track* m_track;
	Knob* m_pan;
	Slider* m_slider;
	bool resizeFlag;
	bool m_selected;
    bool m_midiDetect;
	double panVal;
	double volume;
	QPoint m_startPos;
    int startY;
    int curY;
	int m_tracktype;
	int m_channels;
	bool inHeartBeat;
	bool m_editing;
	bool m_processEvents;
	bool m_meterVisible;
	bool m_sliderVisible;
	QHash<int, QString> m_selectedStyle;
	QHash<int, QString> m_style;
    Meter* meter[MAX_CHANNELS];
	void initPan();
	void initVolume();
	void setupStyles();
    bool eventFilter(QObject *obj, QEvent *event);
	void updateChannels();

private slots:
	void heartBeat();
	void generateAutomationMenu();
	void toggleRecord(bool);
	void toggleMute(bool);
	void toggleSolo(bool);
	void toggleOffState(bool);
	void toggleReminder1(bool);
	void toggleReminder2(bool);
	void toggleReminder3(bool);
	void updateTrackName();
	void generatePopupMenu();
	void panChanged(double);
	void panPressed();
	void panReleased();
	void panRightClicked(const QPoint &p);
	void updatePan();
	void updateVolume();
    void volumeChanged(double);
    void volumePressed();
    void volumeReleased();
    void volumeRightClicked(const QPoint &, int ctl = 0);
	void setEditing(bool edit = true)
	{
		m_editing = edit;
	}

public slots:
	void songChanged(int);
	void stopProcessing();
	void startProcessing();
	void setSelected(bool);

protected:
    enum
    {
        NORMAL, START_DRAG, DRAG, RESIZE
    } mode;

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void resizeEvent(QResizeEvent*);

signals:
    void selectionChanged(Track*);
	void trackInserted(int);

public:
	TrackHeader(Track* track, QWidget* parent = 0);
	virtual ~TrackHeader();
	bool isSelected();
	bool isEditing()
	{
		return m_editing;
	}
};

#endif
