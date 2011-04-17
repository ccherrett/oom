//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _KEYMAPMENU_
#define _KEYMAPMENU_

#include <QMenu>
#include <QWidgetAction>
#include <QObject>

class InstrumentTree;
class MidiTrack;
class QString;
class QTextEdit;
class QLineEdit;
class KeyMap;

class KeyMapMenu : public QWidgetAction
{
	Q_OBJECT

	private:
	InstrumentTree *m_tree;
	QLineEdit *m_patch;
	QTextEdit *m_comment;
	KeyMap* m_keymap;
	MidiTrack *m_track;

	public:
		KeyMapMenu(QMenu* parent, MidiTrack *track, KeyMap* map);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void doClose();
		void clearPatch();
		void updatePatch(int, QString);
		void updateComment();
	
	signals:
		void triggered();
};

#endif
