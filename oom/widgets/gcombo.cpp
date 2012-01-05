//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  Inspired by the work in lcombo.cpp
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "gcombo.h"

#include <QWheelEvent>

//---------------------------------------------------------
//   GridCombo 
//---------------------------------------------------------

GridCombo::GridCombo(QWidget* parent, const char* name) : QComboBox(parent)
{
	setObjectName(name);
}

void GridCombo::setCurrentIndex(int i)
{
	int rc = model()->rowCount();
	if (rc == 0)
		return;
	int r = i % rc;
	int c = i / rc;
	if (c >= model()->columnCount())
		return;
	if (modelColumn() != c)
		setModelColumn(c);
	if (currentIndex() != r)
		QComboBox::setCurrentIndex(r);
}

void GridCombo::wheelEvent(QWheelEvent* e)
{
    e->accept();
}
