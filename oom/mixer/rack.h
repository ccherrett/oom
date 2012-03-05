//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: rack.h,v 1.5.2.3 2006/09/24 19:32:31 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RACK_H__
#define __RACK_H__

#include <QListWidget>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMouseEvent;

class AudioTrack;
class Xml;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListWidget
{
    Q_OBJECT

    AudioTrack* track;

    void startDrag(int idx);
    void initPlugin(Xml xml, int idx);
    QPoint dragPos;
    void savePreset(int idx);
    void choosePlugin(QListWidgetItem* item, bool replace = false);

private slots:
    void menuRequested(QListWidgetItem*);
    void doubleClicked(QListWidgetItem*);
    void songChanged(int);
    void segmentSizeChanged(int);
    void updateContents();

protected:
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;

public:
    EffectRack(QWidget*, AudioTrack* t);
    virtual ~EffectRack();

    AudioTrack* getTrack()
    {
        return track;
    }
	void setTrack(AudioTrack* t)
	{
		track = t;
		songChanged(-1);
	}

    QPoint getDragPos()
    {
        return dragPos;
    }
};

#endif

