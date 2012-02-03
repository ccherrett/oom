//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#ifndef _INSTRUMENTMENU_
#define _INSTRUMENTMENU_

#include <QMenu>
#include <QWidgetAction>
#include <QObject>

class InstrumentTree;
class MidiInstrument;
class QString;
class QTextEdit;
class QLineEdit;
class KeyMap;

class InstrumentMenu : public QWidgetAction
{
	Q_OBJECT

private:
	InstrumentTree *m_tree;
	MidiInstrument *m_instrument;
	int m_program;
	QString m_name;

public:
	InstrumentMenu(QMenu* parent, MidiInstrument *instr);
	virtual QWidget* createWidget(QWidget* parent = 0);

	int getProgram() { return m_program; }
	void setProgram(int prog) { m_program = prog; };
	void setProgramName(QString pname) { m_name = pname; }
	QString getProgramName() { return m_name; }


private slots:
	void doClose();
	void clearPatch();
public slots:
	void updatePatch(int, QString);
	
signals:
	void triggered();
	void patchSelected(int, QString);
};

#endif
