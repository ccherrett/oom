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
	enum CurveMode {
		Linear = 0,
		Quad,
		Cubic,
	};
	FadeCurve(CurveType t, CurveMode, WavePart *part, QObject* parent = 0);
	~FadeCurve();
	CurveType type()
	{
		return m_type;
	}
	CurveMode mode()
	{
		return m_mode;
	}
	void setMode(CurveMode m)
	{
		m_mode = m;
	}
	bool active();
	long width();
	void setWidth(long);
	WavePart* part()
	{
		return m_part;
	}
	void setPart(WavePart *part)
	{
		m_part = part;
	}
	void setFrame(unsigned frame)
	{
		m_frame = frame;
	}
	unsigned getFrame()
	{
		return m_frame;
	}

public slots:
	void setActive(bool);

private:
	CurveType m_type;
	CurveMode m_mode;
	WavePart* m_part;
	bool m_active;
	long m_width;
	unsigned m_frame;	// start position (frame) relative to wave part start position (frame)

signals:
	void stateChanged();
};

#endif
