//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GENSET_H__
#define __GENSET_H__

#include "ui_gensetbase.h"

#include <QShowEvent>

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, public Ui::GlobalSettingsDialogBase
{
    Q_OBJECT

private slots:
    void updateSettings();
    void apply();
    void ok();
    void cancel();
    void mixerCurrent();
    void mixer2Current();
    void bigtimeCurrent();
    void arrangerCurrent();
    void transportCurrent();
    void selectInstrumentsPath();
    void defaultInstrumentsPath();

protected:
    void showEvent(QShowEvent*);
    QButtonGroup *startSongGroup;

public:
    GlobalSettingsConfig(QWidget* parent = 0);
};

#endif