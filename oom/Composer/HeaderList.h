//===============================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//===============================================================

#ifndef __HEADERLIST_H__
#define __HEADERLIST_H__

#include "track.h"
#include <QFrame>
#include <QList>

class QKeyEvent;
class QMouseEvent;
class QResizeEvent;
class QWidget;
class QWheelEvent;
class QVBoxLayout;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class TrackHeader;
class Track;

class HeaderList : public QFrame
{
    Q_OBJECT

	QVBoxLayout* m_layout;
    int ypos;

	QList<TrackHeader*> m_headers;
	QList<TrackHeader*> m_dirtyheaders;
	bool wantCleanup;
	bool m_lockupdate;

    int startY;
    int sTrack;

    Track* y2Track(int) const;
    TrackList getRecEnabledTracks();

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
    //virtual void resizeEvent(QResizeEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dropEvent(QDropEvent*);

private slots:
    void songChanged(int flags);
	void updateSelection(Track*, bool);
	void composerViewChanged();
	void newTrackAdded(qint64);

signals:
    void selectionChanged(Track*);
    void keyPressExt(QKeyEvent*);
    void redirectWheelEvent(QWheelEvent*);
	void trackInserted(int);
	void updateHeader(int);
	void trackHeightChanged();

public slots:
    void tracklistChanged();
    void selectTrack(Track*);
    void selectTrackAbove();
    void selectTrackBelow();
    void moveSelection(int n);
    void moveSelectedTrack(int n);
	void updateTrackList(bool viewupdate = false);
	void renameTrack(Track*);
	void clear();

public:
    HeaderList(QWidget* parent, const char* name);
	bool isEditing();
};

#endif

