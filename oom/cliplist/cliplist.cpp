//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: cliplist.cpp,v 1.6.2.3 2008/08/18 00:15:24 terminator356 Exp $

//  (C) Copyright 2011 Remon Sijrier
//      january 31: Actually populate the list with file information
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <QCloseEvent>

#include "cliplist.h"
#include "song.h"
#include "globals.h"
#include "wave.h"
#include "xml.h"
#include "ui_cliplisteditorbase.h"


extern int mtcType;

enum
{
	COL_NAME = 0, COL_REFS, COL_POS, COL_LEN
};

//---------------------------------------------------------
//   ClipItem
//---------------------------------------------------------

class ClipItem : public QTreeWidgetItem
{
	SndFileR _wf;

public:
	ClipItem(QTreeWidget*, const SndFileR&);

	SndFileR* wf()
	{
		return &_wf;
	}

        virtual QString text(int) const;
};

ClipItem::ClipItem(QTreeWidget* parent, const SndFileR& w)
: QTreeWidgetItem(parent), _wf(w)
{
}

//---------------------------------------------------------
//   frames to hours minutes seconds
//---------------------------------------------------------

static QString frames_to_hms(const uint frames, uint rate)
{
        qint64 remainder;
        int hours, mins, secs;


        hours = (int) (frames / (3600 * rate));
        remainder = qint64(frames - (hours * (3600 * rate)));
        mins = (int) (remainder / (60 * rate));
        remainder -= mins * (60 * rate);
        secs = (int) (remainder / rate);
        return QString().sprintf("%02d:%02d:%02d", hours, mins, secs);
}


//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString ClipItem::text(int col) const
{
	QString s("");
	switch (col)
	{
		case COL_NAME:
			s = _wf.name();
			break;
		case COL_POS:
                        break;
		case COL_LEN:
                        s = frames_to_hms(_wf.samples(), _wf.samplerate());
			break;
		case COL_REFS:
			s.setNum(_wf.getRefCount());
			break;
	}
	return s;
}

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

ClipListEdit::ClipListEdit(QWidget* parent)
: TopWin(parent, "cliplist", Qt::Window)
{
	//setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("OOMidi: Clip List Editor"));

	editor = new ClipListEditorBaseWidget;
	setCentralWidget(editor);

        editor->view->header()->resizeSection(0, 250);

	connect(editor->view, SIGNAL(itemSelectionChanged()), SLOT(clipSelectionChanged()));
	connect(editor->view, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(clicked(QTreeWidgetItem*, int)));

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(editor->start, SIGNAL(valueChanged(const Pos&)), SLOT(startChanged(const Pos&)));
	connect(editor->len, SIGNAL(valueChanged(const Pos&)), SLOT(lenChanged(const Pos&)));

	updateList();
}

ClipListEdit::~ClipListEdit()
{

}

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void ClipListEdit::updateList()
{
	editor->view->clear();
	for (iSndFile f = SndFile::sndFiles.begin(); f != SndFile::sndFiles.end(); ++f)
	{
                ClipItem* item = new ClipItem(editor->view, *f);
                item->setText(0, item->text(0));
                item->setText(1, item->text(1));
                item->setText(2, item->text(2));
                item->setText(3, item->text(3));
                item->setText(4, item->text(4));
        }
	clipSelectionChanged();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void ClipListEdit::closeEvent(QCloseEvent* e)
{
	emit deleted((unsigned long) this);
	e->accept();
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ClipListEdit::songChanged(int type)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (type == SC_MIDI_CONTROLLER)
		return;

	updateList();
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ClipListEdit::readStatus(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		if (token == Xml::Error || token == Xml::End)
			break;
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "topwin")
					TopWin::readStatus(xml);
				else
					xml.unknown("CliplistEdit");
				break;
			case Xml::TagEnd:
				if (tag == "cliplist")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ClipListEdit::writeStatus(int level, Xml& xml) const
{
	xml.tag(level++, "cliplist");
	TopWin::writeStatus(level, xml);
	xml.etag(level, "cliplist");
}

//---------------------------------------------------------
//   startChanged
//---------------------------------------------------------

void ClipListEdit::startChanged(const Pos& /*pos*/)//prevent compiler warning: unsused parameter
{
	//      editor->view->triggerUpdate();
}

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void ClipListEdit::lenChanged(const Pos& /*pos*/) //prevent compiler warning: unsused parameter
{
	//      curClip.setLenFrame(pos.frame());
	//      editor->view->triggerUpdate();
}

//---------------------------------------------------------
//   clipSelectionChanged
//---------------------------------------------------------

void ClipListEdit::clipSelectionChanged()
{
	//      ClipItem* item = (ClipItem*)(editor->view->selectedItem());

	//      if (item == 0) {
	editor->start->setEnabled(false);
	editor->len->setEnabled(false);
	return;
#if 0
}
editor->start->setEnabled(true);
editor->len->setEnabled(true);
Pos pos, len;
pos.setType(Pos::FRAMES);
len.setType(Pos::FRAMES);
pos.setFrame(curClip.spos());
len.setFrame(curClip.lenFrame());
editor->start->setValue(pos);
editor->len->setValue(len);
#endif
}

//---------------------------------------------------------
//   clicked
//---------------------------------------------------------

void ClipListEdit::clicked(QTreeWidgetItem*, int)
{
	//      printf("clicked\n");
}

