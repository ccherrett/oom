//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: comment.h,v 1.2 2004/02/08 18:30:00 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMMENTDOCK_H__
#define __COMMENTDOCK_H__

#include "ui_commentdockbase.h"

class Xml;
class Track;
class QWidget;

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

class CommentDock : public QWidget, public Ui::CommentDockBase
{
    Q_OBJECT

private:
    Track* m_track;

private slots:
    void textChanged();
	void songCommentChanged();
    void songChanged(int);

public:
    CommentDock(QWidget* parent = 0, Track* t = 0);
    ~CommentDock();
	void setTrack(Track* t) 
	{ 
		m_track = t; 
		updateComments();
	}
	Track* track() { return m_track; }
	void updateComments();
};

#endif

