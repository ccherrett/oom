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

class MidiInstrument;
class QString;
class InstrumentMenu;

class InstrumentCombo : public QComboBox
{
	Q_OBJECT
	InstrumentMenu *tree;
	int m_program;
	QString m_name;
    bool eventFilter(QObject *obj, QEvent *event);
public:
	InstrumentCombo(QWidget *parent = 0, MidiInstrument *instr = 0, int prog = 0, QString pname = "Select Patch");
	void setInstrument(MidiInstrument* instrument) { m_instrument = instrument; }
	MidiInstrument* instrument() { return m_instrument; }

	int getProgram() { return m_program; }
	void setProgram(int prog);
	void setProgramName(QString pname);// { m_name = pname; }
	QString getProgramName() { return m_name; }

private:
	MidiInstrument* m_instrument;
	
protected:
	virtual void mousePressEvent(QMouseEvent*);
public slots:
	void updateValue(int, QString);
signals:
	void patchSelected(int, QString);
};
#endif
