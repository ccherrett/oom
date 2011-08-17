//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "FadeCurve.h"
#include "part.h"

FadeCurve::FadeCurve(CurveType type, WavePart* p, QObject* parent)
: QObject(parent)
{
	m_type = type;
	m_part = p;
	m_active = false;
	m_range = 1.0;
}

FadeCurve::~FadeCurve()
{
}

bool FadeCurve::active()
{
	return m_active;
}

void FadeCurve::setActive(bool act)
{
	if(m_active != act)
	{
		m_active = act;
		emit stateChanged();
	}
}

double FadeCurve::range()
{
	return m_range;
}

void FadeCurve::setRange(double range)
{
	if(range >= 0.0 && range <= 1.0)
		m_range = range;
	//TODO: recalculate points
}

