//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: routedialog.h,v 1.2 2004/01/31 17:31:49 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUDIOPORTCONFIG_H__
#define __AUDIOPORTCONFIG_H__

#include "ui_apconfigbase.h"

class QCloseEvent;
class QDialog;
class AudioTrack;

//---------------------------------------------------------
//   AudioPortConfig
//---------------------------------------------------------

class AudioPortConfig : public QFrame, public Ui::APConfigBase
{
    Q_OBJECT
	AudioTrack* _selected;
	int selectedIndex;

    void routingChanged();
	void insertInputs();
	void insertOutputs();
        void updateRoutingHeaderWidths();

private slots:
    void routeSelectionChanged();
    void removeRoute();
    void addRoute();
    void addOutRoute();
    void srcSelectionChanged();
    void dstSelectionChanged();
	void trackSelectionChanged();
    void songChanged(int);

signals:
    void closed();

public:
    AudioPortConfig(QWidget* parent = 0);
	~AudioPortConfig();
	void setSourceSelection(QString);
	void setDestSelection(QString);
	void setSelected(AudioTrack* s);
	void setSelected(QString);
	AudioTrack* selected()
	{
		return _selected;
	}

protected:
    virtual void closeEvent(QCloseEvent*);
    virtual void resizeEvent(QResizeEvent *);
    virtual void showEvent(QShowEvent *);
    void reject();
};


#endif

