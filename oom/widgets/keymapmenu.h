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
class Patch;

class KeyMapMenu : public QWidgetAction
{
	Q_OBJECT

	private:
	InstrumentTree *m_tree;
	QLineEdit *m_kpatch;
	QLineEdit *m_comment;
	QLineEdit *m_patchcomment;
	KeyMap* m_keymap;
	MidiTrack *m_track;
	Patch* m_patch;
	int m_datachanged;

	public:
		KeyMapMenu(QMenu* parent, MidiTrack *track, KeyMap* map, Patch* patch = 0);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void doClose();
		void clearPatch();
		void updatePatch(int, QString);
		void updateComment();
		void updatePatchComment();
	
	signals:
		void triggered();
};

#endif
