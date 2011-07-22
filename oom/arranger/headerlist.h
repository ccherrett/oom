//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: tlist.h,v 1.8.2.5 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HEADERLIST_H__
#define __HEADERLIST_H__

#include "track.h"

#include <QWidget>

class QKeyEvent;
class QLineEdit;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QScrollBar;
class QWheelEvent;
class QVBoxLayout;

class ScrollScale;
class Track;
class Xml;

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

class HeaderList : public QWidget
{
    Q_OBJECT

    int ypos;

    QPixmap bgPixmap; // background Pixmap
    bool resizeFlag; // true if resize cursor is shown

    QScrollBar* _scroll;
	Track* editAutomation;

    int startY;
    int curY;
    int sTrack;
    int dragHeight;
    int dragYoff;
	QVBoxLayout* m_layout;

    enum
    {
        NORMAL, START_DRAG, DRAG, RESIZE
    } mode;

    virtual void mousePressEvent(QMouseEvent* event);
    //virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void paintEvent(QPaintEvent*);

    void adjustScrollbar();
    virtual void resizeEvent(QResizeEvent*);
    Track* y2Track(int) const;
    void classesPopupMenu(Track*, int x, int y);
    TrackList getRecEnabledTracks();

private slots:
    void songChanged(int flags);
	void updateSelection(Track*, bool);

signals:
    ///void selectionChanged();
    void selectionChanged(Track*);
    void keyPressExt(QKeyEvent*);
    void redirectWheelEvent(QWheelEvent*);
	void trackInserted(int);

public slots:
    void tracklistChanged();
    void setYPos(int);
    void redraw();
    void selectTrack(Track*);
    void selectTrackAbove();
    void selectTrackBelow();
    void moveSelection(int n);
    void moveSelectedTrack(int n);

public:
    HeaderList(QWidget* parent, const char* name);

    void setScroll(QScrollBar* s)
    {
        _scroll = s;
    }
};

#endif

