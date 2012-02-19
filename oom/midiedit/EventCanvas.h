//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id:$
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#ifndef __EVENTCANVAS_H__
#define __EVENTCANVAS_H__

#include "canvas.h"
#include "noteinfo.h"
#include <QEvent>
#include <QKeyEvent>
#include <QList>

class MidiPart;
class MidiTrack;
class AbstractMidiEditor;
class Part;
class QMimeData;
class QDrag;
class QString;
class QDropEvent;

struct PartToChange
{
    Part* npart;
    int xdiff;
};
typedef std::map<Part*, PartToChange> PartsToChangeMap;
typedef std::map<Part*, PartToChange>::iterator iPartToChange;

enum { LOCATORS_TO_SELECTION, SEL_RIGHT, SEL_RIGHT_ADD, SEL_LEFT, SEL_LEFT_ADD, 
						INC_PITCH_OCTAVE, DEC_PITCH_OCTAVE, INC_PITCH, DEC_PITCH, INC_POS, DEC_POS,
						INCREASE_LEN, DECREASE_LEN, GOTO_SEL_NOTE, MIDI_PANIC };

//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

class EventCanvas : public Canvas
{
    Q_OBJECT

	QList<CItem*> m_tempPlayItems;
    virtual void leaveEvent(QEvent*e);
    virtual void enterEvent(QEvent*e);

    virtual void startUndo(DragType);

    virtual void endUndo(DragType, int flags = 0);
    virtual void mouseMove(QMouseEvent* event);

protected:
    bool _playEvents;
    AbstractMidiEditor* editor;
    unsigned start_tick, end_tick;
    int curVelo;
    bool _steprec;
    bool _midiin;
	bool m_showcomments;

    void updateSelection();
    CItem* getRightMostSelected();
    CItem* getLeftMostSelected();
    virtual void addItem(Part*, Event&) = 0;
    // Added by T356.
    virtual QPoint raster(const QPoint&) const;
    virtual void viewMousePressEvent(QMouseEvent* event);
	virtual void populateMultiSelect(CItem*);
    virtual QMenu* genItemPopup(CItem*);
    virtual void itemPopup(CItem*, int, const QPoint&);
    virtual void mouseRelease(const QPoint&);

private slots:

	void playReleaseForItem();

public slots:

    void redrawGrid()
    {
        redraw();
    }

    void setSteprec(bool f)
    {
        _steprec = f;
    }

    void setMidiin(bool f)
    {
        _midiin = f;
    }
	void toggleComments(bool){}
	void actionCommand(int);

signals:
    void pitchChanged(int); // current cursor position
    void timeChanged(unsigned);
    void selectionChanged(int, Event&, Part*);
    void enterCanvas();

public:
    EventCanvas(AbstractMidiEditor*, QWidget*, int, int, const char* name = 0);
    MidiTrack* track() const;

    unsigned start() const
    {
        return start_tick;
    }

    unsigned end() const
    {
        return end_tick;
    }

    bool midiin() const
    {
        return _midiin;
    }

    bool steprec() const
    {
        return _steprec;
    }
    QString getCaption() const;
    void songChanged(int);

    void range(int* s, int* e) const
    {
        *s = start_tick;
        *e = end_tick;
    }

    void playEvents(bool flag)
    {
        _playEvents = flag;
    }
    void selectAtTick(unsigned int tick);
    //QDrag* getTextDrag(QWidget* parent);
    QMimeData* getTextDrag();
    void pasteAt(const QString& pt, int pos);
    void viewDropEvent(QDropEvent* event);

    virtual void modifySelected(NoteInfo::ValType, int, bool strict = false)
    {
		if(strict)
			return;
    }
    virtual void keyPress(QKeyEvent*);
	virtual bool showComments() { return m_showcomments; }
};

#endif

