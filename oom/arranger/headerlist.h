//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: tlist.h,v 1.8.2.5 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HEADERLIST_H__
#define __HEADERLIST_H__

#include "track.h"

#include <QScrollArea>
#include <QList>

class QKeyEvent;
class QLineEdit;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QScrollBar;
class QWidget;
class QWheelEvent;
class QVBoxLayout;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class TrackHeader;
class QSpacerItem;
class Track;
class Xml;

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

class HeaderList : public QFrame
{
    Q_OBJECT

    int ypos;

    QPixmap bgPixmap; // background Pixmap
    bool resizeFlag; // true if resize cursor is shown

	QWidget* m_viewPort;
	QList<TrackHeader*> m_headers;
	QList<TrackHeader*> m_dirtyheaders;
	bool wantCleanup;

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

    //virtual void resizeEvent(QResizeEvent*);
    Track* y2Track(int) const;
    void classesPopupMenu(Track*, int x, int y);
    TrackList getRecEnabledTracks();

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dropEvent(QDropEvent*);

private slots:
    void songChanged(int flags);
	void updateSelection(Track*, bool);

signals:
    ///void selectionChanged();
    void selectionChanged(Track*);
    void keyPressExt(QKeyEvent*);
    void redirectWheelEvent(QWheelEvent*);
	void trackInserted(int);
	void updateHeader(int);

public slots:
    void tracklistChanged();
    void selectTrack(Track*);
    void selectTrackAbove();
    void selectTrackBelow();
    void moveSelection(int n);
    void moveSelectedTrack(int n);
	void updateTrackList();
	void renameTrack(Track*);

public:
    HeaderList(QWidget* parent, const char* name);
	bool isEditing();
};

#endif

