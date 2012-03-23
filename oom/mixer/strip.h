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
class QTabWidget;
class QPixmap;
class QColor;
class TrackEffects;

static const int STRIP_WIDTH = 65;

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame
{
    Q_OBJECT

	void layoutUi();
	QPixmap topRack;
	QPixmap topRackLarge;
	QPixmap bottomRack;
	QPixmap bottomRackLarge;
    QWidget *m_buttonBase;
    QWidget *m_mixerBase;
    QWidget *m_vuBase;
    QWidget *m_panBase;
    QFrame *m_auxBase;

protected:
    Track* track;
	int m_type;

    QVBoxLayout *m_mainVBoxLayout;
    QLabel *label;
    QLabel *toprack;
	QLabel *brack;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *m_mixerBox;
    QHBoxLayout *horizontalLayout_2;
    QHBoxLayout *m_vuBox;
    QVBoxLayout *m_buttonBox;

    QToolButton *m_btnAux;
    QToolButton *m_btnStereo;
    QToolButton *m_btnIRoute;
    QToolButton *m_btnORoute;
    QSpacerItem *verticalSpacer;
    TransparentToolButton *m_btnPower;
    TransparentToolButton *m_btnRecord;
    QToolButton *m_btnMute;
    QToolButton *m_btnSolo;
    QVBoxLayout *m_panBox;
    QVBoxLayout *m_autoBox;
    //QScrollArea *m_auxScroll;
    //QVBoxLayout *m_auxBox;
	//QTabWidget* m_tabWidget;
	//QWidget *m_auxTab;
	//QVBoxLayout *m_auxTabLayout;
	//QWidget *m_fxTab;
    //QVBoxLayout *rackBox;
	TrackEffects* m_effectsRack;

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
	QColor m_sliderBg;
    void setLabelText();
	virtual void trackChanged() = 0;

private slots:
    void recordToggled(bool);
    void soloToggled(bool);
    void muteToggled(bool);
	//void tabChanged(int);

protected slots:
    virtual void heartBeat();
    void setAutomationType(int t, int);

public slots:
    void resetPeaks();
	virtual void toggleAuxPanel(bool);
    virtual void songChanged(int) = 0;

public:
    Strip(QWidget* parent, Track* t);
    virtual ~Strip();
    void setRecordFlag(bool flag);

    Track* getTrack() const
    {
        return track;
    }

	bool setTrack(Track* t);
    void setLabelFont();
	int type(){return m_type;};
};

#endif

