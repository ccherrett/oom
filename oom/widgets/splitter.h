//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: splitter.h,v 1.1.1.1 2003/10/27 18:54:51 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SPLITTER_H__
#define __SPLITTER_H__

#include <QSplitter>

class QShowEvent;
class Xml;

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

class Splitter : public QSplitter {
    Q_OBJECT
	bool m_configured;

public:
    Splitter(Qt::Orientation o, QWidget* parent, const char* name);
	Splitter(QWidget* parent);
	~Splitter();
    void writeStatus(int level, Xml&);
    void readStatus(Xml&);
protected:
	virtual void showEvent(QShowEvent*);
	virtual QSize sizeHint() const;
private slots:
	void saveStateInfo();
};

#endif

