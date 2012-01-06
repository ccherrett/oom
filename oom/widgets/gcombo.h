//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  Inspired by the work in lcombo.h
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GRIDCOMBO_H__
#define __GRIDCOMBO_H__

#include <QAbstractItemView>
#include <QComboBox>

class QString;
class QWheelEvent;


//---------------------------------------------------------
//   GridCombo
//---------------------------------------------------------

class GridCombo : public QComboBox
{
    Q_OBJECT

public slots:

    void setCurrentIndex(int i);

public:
    GridCombo(QWidget* parent, const char* name = 0);

    void setView(QAbstractItemView* v)
    {
		QComboBox::setModel(v->model());
		QComboBox::setView(v);
    }

protected:
    void wheelEvent(QWheelEvent* e);
};

#endif
