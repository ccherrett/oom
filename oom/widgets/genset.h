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

class QStandardItemModel;

enum {CONF_APP_TAB = 0, CONF_AUDIO_TAB, CONF_MIDI_TAB, CONF_GUI_TAB};

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, public Ui::GlobalSettingsDialogBase
{
    Q_OBJECT

	QStandardItemModel* m_inputsModel;

private slots:
    void updateSettings();
    void apply();
    void ok();
    void cancel();
	void startLSClientNow();
	void resetLSNow();
	void startStopSampler();
	void selectedSamplerPath();
    void selectInstrumentsPath();
    void defaultInstrumentsPath();

protected:
    void showEvent(QShowEvent*);
    QButtonGroup *startSongGroup;

public slots:
	void setCurrentTab(int);
	void populateInputs();

public:
    GlobalSettingsConfig(QWidget* parent = 0);
};

#endif
