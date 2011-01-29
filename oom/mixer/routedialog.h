//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: routedialog.h,v 1.2 2004/01/31 17:31:49 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ROUTEDIALOG_H__
#define __ROUTEDIALOG_H__

#include "ui_routedialogbase.h"

class QCloseEvent;
class QDialog;
class AudioTrack;

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

class RouteDialog : public QDialog, public Ui::RouteDialogBase
{
    Q_OBJECT
	AudioTrack* _selected;

    virtual void closeEvent(QCloseEvent*);
    void routingChanged();

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
    RouteDialog(QWidget* parent = 0);
	void setSourceSelection(QString);
	void setDestSelection(QString);
	void setSelected(AudioTrack* s)
	{
		_selected = s;
	}
	AudioTrack* selected()
	{
		return _selected;
	}
};


#endif

