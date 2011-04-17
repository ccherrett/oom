//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_INS_COMBO_
#define _OOM_INS_COMBO_

#include <QComboBox>
#include <QMouseEvent>
#include <QEvent>
#include <QObject>

class MidiTrack;
class QString;
class InstrumentTree;

class InstrumentCombo : public QComboBox
{
	Q_OBJECT
	InstrumentTree *tree;
	int m_program;
	QString m_name;
    bool eventFilter(QObject *obj, QEvent *event);
public:
	InstrumentCombo(QWidget *parent = 0, MidiTrack *track = 0, int prog = 0, QString pname = "Select Patch");
	void setTrack(MidiTrack* track) { m_track = track; }
	MidiTrack* track() { return m_track; }

	int getProgram() { return m_program; }
	void setProgram(int prog);
	void setProgramName(QString pname) { m_name = pname; }
	QString getProgramName() { return m_name; }

private:
	MidiTrack* m_track;
	
protected:
	virtual void mousePressEvent(QMouseEvent*);
private slots:
	void updateValue(int, QString);
signals:
	void stopEditing();
	void patchSelected(int, QString);
};
#endif
