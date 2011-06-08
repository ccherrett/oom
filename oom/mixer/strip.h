//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: strip.h,v 1.3.2.2 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __STRIP_H__
#define __STRIP_H__

#include <QFrame>
#include <QIcon>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>

#include "globaldefs.h"
//#include "route.h"

class Track;
class QLabel;
class QVBoxLayout;
class Meter;
class QToolButton;
class QGridLayout;
class ComboBox;
class QWidget;
class QHeaderView;
class QHBoxLayout;
class TransparentToolButton;
class QScrollArea;
class QSPacerItem;

static const int STRIP_WIDTH = 65;

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame
{
    Q_OBJECT

	void layoutUi();

protected:
    Track* track;

    QVBoxLayout *m_mainVBoxLayout;
    QLabel *label;
    QLabel *toprack;
	QLabel *brack;
    QWidget *m_rackBox;
    QVBoxLayout *rackBox;
    QHBoxLayout *horizontalLayout;
    QWidget *m_mixerBox;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_2;
    QWidget *m_vuContainer;
    QHBoxLayout *m_vuBox;
    QWidget *m_buttonStrip;
    QVBoxLayout *verticalLayout_6;
    QToolButton *m_btnAux;
    QToolButton *m_btnStereo;
    QToolButton *m_btnIRoute;
    QToolButton *m_btnORoute;
    QSpacerItem *verticalSpacer;
    TransparentToolButton *m_btnPower;
    TransparentToolButton *m_btnRecord;
    QToolButton *m_btnMute;
    QToolButton *m_btnSolo;
    QWidget *m_panContainer;
    QVBoxLayout *m_panBox;
    QVBoxLayout *m_autoBox;
    QScrollArea *m_auxScroll;
    QFrame *m_auxContainer;
    QVBoxLayout *m_auxBox;

    int _curGridRow;
    Meter* meter[MAX_CHANNELS];
    bool useSoloIconSet2;

	bool hasRecord;
	bool hasAux;
	bool hasStereo;
	bool hasIRoute;
	bool hasORoute;
	bool m_collapsed;

    QGridLayout* sliderGrid;
    ComboBox* autoType;
    void setLabelText();

private slots:
    void recordToggled(bool);
    void soloToggled(bool);
    void muteToggled(bool);

protected slots:
    virtual void heartBeat();
	virtual void toggleAuxPanel(bool);
    void setAutomationType(int t, int);

public slots:
    void resetPeaks();
    virtual void songChanged(int) = 0;

public:
    Strip(QWidget* parent, Track* t);
    virtual ~Strip();
    void setRecordFlag(bool flag);

    Track* getTrack() const
    {
        return track;
    }
    void setLabelFont();
};

#endif

