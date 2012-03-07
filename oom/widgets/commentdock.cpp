//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: comment.cpp,v 1.2 2004/02/08 18:30:00 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "commentdock.h"
#include "song.h"
#include "track.h"
#include "traverso_shared/TConfig.h"

#include <QWidget>

//---------------------------------------------------------
//   Comment
//---------------------------------------------------------

CommentDock::CommentDock(QWidget* parent, Track* t)
: QWidget(parent)
{
	setupUi(this);
	m_track = t;
	setObjectName("commentDockSplitter");
	//connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
	connect(songComment, SIGNAL(textChanged()), this, SLOT(songCommentChanged()));
	updateComments();
    commentDockSplitter->setChildrenCollapsible(false);
	QByteArray str = tconfig().get_property("CommentDock", "settings", "").toByteArray();
	commentDockSplitter->restoreState(str);
	/*
	QString str = tconfig().get_property("CommentDock", "sizes", "30 250").toString();
	QList<int> sizes;
	QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
	foreach (QString s, sl)
	{
		int val = s.toInt();
		sizes.append(val);
	}
	commentDockSplitter->setSizes(sizes);
	*/
}

CommentDock::~CommentDock()
{
	//QList<int> sizes = commentDockSplitter->sizes();
	//QStringList out;
	/*foreach(int s, sizes)
	{
		out << QString::number(s);
	}*/
	//out << QString::number(songComment->height()) << QString::number(textentry->height());
	//tconfig().set_property("CommentDock", "sizes", out.join(" "));
	tconfig().set_property("CommentDock", "settings", commentDockSplitter->saveState());
	tconfig().save();
}

//---------------------------------------------------------
//   textChanged
//---------------------------------------------------------

void CommentDock::textChanged()
{
	//printf("CommentDock::textChanged()\n");
	if(!m_track)
		return;
	//setText(textentry->toPlainText());
	QString tcomment = textentry->toPlainText();
	if(tcomment != m_track->comment())
	{
		m_track->setComment(tcomment);
		song->dirty = true;
	}
	//song->update(SC_TRACK_MODIFIED);
}

void CommentDock::songCommentChanged()
{
	//printf("CommentDock::songCommentChanged()\n");
	song->setSongInfo(songComment->toPlainText());
}

//---------------------------------------------------------
//   TrackComment
//---------------------------------------------------------

void CommentDock::updateComments()
{
	songComment->blockSignals(true);
	songComment->setText(song->getSongInfo());
	songComment->blockSignals(false);
	if(m_track)
	{
		textentry->blockSignals(true);
		textentry->setText(m_track->comment());
		textentry->blockSignals(false);
		textentry->moveCursor(QTextCursor::End);
		label1->setText(tr("Track Comments:"));
		label2->setText(m_track->name());
	}
	else
	{
		textentry->blockSignals(true);
		textentry->setText("");
		textentry->blockSignals(false);
		label2->setText(tr("Select Track"));
	}
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void CommentDock::songChanged(int flags)
{
	if (flags & !(SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED))
		return;
	printf("CommentDock::songChanged() after flag check\n");
	// check if track still exists:
	TrackList* tl = song->tracks();
	iTrack it;
	for (it = tl->begin(); it != tl->end(); ++it)
	{
		if (m_track == *it)
			break;
	}
	if (it == tl->end())
	{
		//close();
		return;
	}
	updateComments();
	/*
	label2->setText(m_track->name());
	if (m_track->comment() != textentry->toPlainText())
	{
		//disconnect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
		textentry->blockSignals(true);
		textentry->setText(m_track->comment());
		textentry->blockSignals(false);
		textentry->moveCursor(QTextCursor::End);
		//connect(textentry, SIGNAL(textChanged()), this, SLOT(textChanged()));
	}
	songInfoText->blockSignals(true);
	songInfoText->setText(song->getSongInfo());
	songInfoText->blockSignals(true);
	*/
}

