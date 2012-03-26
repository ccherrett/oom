//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef __OOM_DOUBLESLIDER_H
#define __OOM_DOUBLESLIDER_H

#include <QFrame>
#include <QPoint>
#include <QSplitter>

class QLabel;
class QMouseEvent;

class SliderSplitter : public QSplitter
{
	Q_OBJECT
	
public slots:
	void moveSlider(int pos, int index);

public:
	SliderSplitter(Qt::Orientation, QWidget* parent = 0);
};

class SliderMiddle : public QFrame
{
	Q_OBJECT

	bool m_moving;
	bool m_startMoving;
	QPoint m_startPos;
	int m_lastX;

protected:
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
signals:
	void moveLeft(int);
	void moveRight(int);
public:
	SliderMiddle(QWidget *parent = 0);
};

class DoubleSlider : public QFrame
{
	Q_OBJECT
	
private:
	QFrame		 *_before;
	SliderMiddle *_selection;
	QFrame      *_after;
	QFrame      *_box;
	SliderSplitter    *_splitter;
	QLabel       *_minLabel;
	QLabel       *_maxLabel;
	
	double		_currentMin;
	double		_currentMax;
	double		_valueMin;
	double		_valueMax;
	int 		m_handleWidth;
	
public:
	DoubleSlider(bool showLabels, QWidget *parent = 0);
	~DoubleSlider();
	
	QSize sizeHint() const;
	
	double currentMin();
	double currentMax();
	double valueMin();
	double valueMax();
	
	void setCurrentMin(double currentMin);
	void setCurrentMax(double currentMax);
	void setRange(double, double);
	void setValueMin(double valueMin);
	void setValueMax(double valueMax);
	
protected:
	void updateMinPos();
	void updateMaxPos();
	void updateCurrentMin(double currentMin);
	void updateCurrentMax(double currentMax);
	
protected slots:
	void updateValues(int pos, int index);	
	void updateLabels();

private slots:
	void moveLeft(int);
	void moveRight(int);
		
signals:
	void minChanged(double);
	void maxChanged(double);
};

#endif


