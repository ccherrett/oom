//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _FEEDBACKTOOLS_H_
#define _FEEDBACKTOOLS_H_
#include <QFrame>
#include <QList>

class QHBoxLayout;
class QToolButton;
class QAction;

class FeedbackTools : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	QToolButton* m_btnShowMixer;
	QToolButton* m_btnLoadProgramChange;
	QToolButton* m_btnMidiFeedback;

private slots:
	void songChanged(int);
public slots:
public:
	FeedbackTools(QWidget* parent = 0);
	virtual ~FeedbackTools(){}
};

#endif
