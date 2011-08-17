//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _FADECURVE_H_
#define _FADECURVE_H_

#include <QObject>
#include <QList>
#include <QPointF>

class WavePart;

class FadeCurve :public QObject
{
	Q_OBJECT

public:
	enum CurveType {
		FadeIn,
		FadeOut
	};
	FadeCurve(CurveType t, WavePart *part, QObject* parent = 0);
	~FadeCurve();
	CurveType type();
	bool active();
	double range();
	void setRange(double);

public slots:
	void setActive(bool);

private:
	CurveType m_type;
	WavePart* m_part;
	bool m_active;
	QList<QPointF> m_controlPoints;
	double m_range;

signals:
	void stateChanged();
};

#endif
