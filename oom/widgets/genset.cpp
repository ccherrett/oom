//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include <QFileDialog>
#include <QRect>
#include <QShowEvent>
#include <QMessageBox>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"

static int rtcResolutions[] = {
	1024, 2048, 4096, 8192, 16384, 32768
};
static int divisions[] = {
	48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
};
static int dummyAudioBufSizes[] = {
	16, 32, 64, 128, 256, 512, 1024, 2048
};

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

GlobalSettingsConfig::GlobalSettingsConfig(QWidget* parent)
: QDialog(parent)
{
	setupUi(this);
	groupBox13->hide();
	vstInPlaceTextLabel->hide();
	vstInPlaceCheckBox->hide();
	showDidYouKnow->hide();
	showSplash->hide();
	//startUpBox->hide();
	startSongGroup = new QButtonGroup(this);
	startSongGroup->addButton(startLastButton, 0);
	startSongGroup->addButton(startEmptyButton, 1);
	startSongGroup->addButton(startSongButton, 2);
	for (unsigned i = 0; i < sizeof (rtcResolutions) / sizeof (*rtcResolutions); ++i)
	{
		if (rtcResolutions[i] == config.rtcTicks)
		{
			rtcResolutionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (divisions) / sizeof (*divisions); ++i)
	{
		if (divisions[i] == config.division)
		{
			midiDivisionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (divisions) / sizeof (*divisions); ++i)
	{
		if (divisions[i] == config.guiDivision)
		{
			guiDivisionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (dummyAudioBufSizes) / sizeof (*dummyAudioBufSizes); ++i)
	{
		if (dummyAudioBufSizes[i] == config.dummyAudioBufSize)
		{
			dummyAudioSize->setCurrentIndex(i);
			break;
		}
	}

	userInstrumentsPath->setText(config.userInstrumentsDir);
	selectInstrumentsDirButton->setIcon(*openIcon);
	defaultInstrumentsDirButton->setIcon(*undoIcon);
	connect(selectInstrumentsDirButton, SIGNAL(clicked()), SLOT(selectInstrumentsPath()));
	connect(defaultInstrumentsDirButton, SIGNAL(clicked()), SLOT(defaultInstrumentsPath()));

	guiRefreshSelect->setValue(config.guiRefresh);
	minSliderSelect->setValue(int(config.minSlider));
	minMeterSelect->setValue(config.minMeter);
	freewheelCheckBox->setChecked(config.freewheelMode);
	denormalCheckBox->setChecked(config.useDenormalBias);
	outputLimiterCheckBox->setChecked(config.useOutputLimiter);
	vstInPlaceCheckBox->setChecked(config.vstInPlace);
	dummyAudioRate->setValue(config.dummyAudioSampleRate);

	//DummyAudioDevice* dad = dynamic_cast<DummyAudioDevice*>(audioDevice);
	//dummyAudioRealRate->setText(dad ? QString().setNum(sampleRate) : "---");
	dummyAudioRealRate->setText(QString().setNum(sampleRate));

	startSongEntry->setText(config.startSong);
	startSongGroup->button(config.startMode)->setChecked(true);

	showSplash->setChecked(config.showSplashScreen);
	showDidYouKnow->setChecked(config.showDidYouKnow);
	externalWavEditorSelect->setText(config.externalWavEditor);
	oldStyleStopCheckBox->setChecked(config.useOldStyleStopShortCut);
	moveArmedCheckBox->setChecked(config.moveArmedCheckBox);
	projectSaveCheckBox->setChecked(config.useProjectSaveDialog);
	
	m_chkAutofade->setChecked(config.useAutoCrossFades);
	chkStartLSClient->setChecked(config.lsClientAutoStart);
	btnStartLSClient->setEnabled(!lsClientStarted);
	btnResetLSNow->setEnabled(lsClientStarted);
	chkResetLSOnStartup->setChecked(config.lsClientResetOnStart);
	chkResetLSOnSongLoad->setChecked(config.lsClientResetOnSongStart);

	connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
	connect(okButton, SIGNAL(clicked()), SLOT(ok()));
	connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
	connect(btnStartLSClient, SIGNAL(clicked()), SLOT(startLSClientNow()));
	connect(btnResetLSNow, SIGNAL(clicked()), SLOT(resetLSNow()));
	//connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
	//connect(setComposerCurrent, SIGNAL(clicked()), SLOT(composerCurrent()));
	//connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));
}

//---------------------------------------------------------
//   updateSettings
//---------------------------------------------------------

void GlobalSettingsConfig::updateSettings()
{
	for (unsigned i = 0; i < sizeof (rtcResolutions) / sizeof (*rtcResolutions); ++i)
	{
		if (rtcResolutions[i] == config.rtcTicks)
		{
			rtcResolutionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (divisions) / sizeof (*divisions); ++i)
	{
		if (divisions[i] == config.division)
		{
			midiDivisionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (divisions) / sizeof (*divisions); ++i)
	{
		if (divisions[i] == config.guiDivision)
		{
			guiDivisionSelect->setCurrentIndex(i);
			break;
		}
	}
	for (unsigned i = 0; i < sizeof (dummyAudioBufSizes) / sizeof (*dummyAudioBufSizes); ++i)
	{
		if (dummyAudioBufSizes[i] == config.dummyAudioBufSize)
		{
			dummyAudioSize->setCurrentIndex(i);
			break;
		}
	}

	guiRefreshSelect->setValue(config.guiRefresh);
	minSliderSelect->setValue(int(config.minSlider));
	minMeterSelect->setValue(config.minMeter);
	freewheelCheckBox->setChecked(config.freewheelMode);
	denormalCheckBox->setChecked(config.useDenormalBias);
	outputLimiterCheckBox->setChecked(config.useOutputLimiter);
	vstInPlaceCheckBox->setChecked(config.vstInPlace);
	dummyAudioRate->setValue(config.dummyAudioSampleRate);

	dummyAudioRealRate->setText(QString().setNum(sampleRate));

	startSongEntry->setText(config.startSong);
	startSongGroup->button(config.startMode)->setChecked(true);

	showSplash->setChecked(config.showSplashScreen);
	showDidYouKnow->setChecked(config.showDidYouKnow);
	externalWavEditorSelect->setText(config.externalWavEditor);
	oldStyleStopCheckBox->setChecked(config.useOldStyleStopShortCut);
	moveArmedCheckBox->setChecked(config.moveArmedCheckBox);
	projectSaveCheckBox->setChecked(config.useProjectSaveDialog);
	chkStartLSClient->setChecked(config.lsClientAutoStart);
	btnStartLSClient->setEnabled(!lsClientStarted);
	btnResetLSNow->setEnabled(lsClientStarted);
	chkResetLSOnStartup->setChecked(config.lsClientResetOnStart);
	chkResetLSOnSongLoad->setChecked(config.lsClientResetOnSongStart);
	//TODO: Set icon for status of lsClient
}

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void GlobalSettingsConfig::showEvent(QShowEvent* e)
{
	QDialog::showEvent(e);
	//updateSettings();     // TESTING
}

void GlobalSettingsConfig::startLSClientNow()
{
	if(!lsClient)
	{
		lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
		lsClientStarted = lsClient->startClient();
		if(config.lsClientResetOnStart && lsClientStarted)
		{
			lsClient->resetSampler();
		}
	}
	else
	{
		lsClientStarted = lsClient->startClient();
		if(config.lsClientResetOnStart && lsClientStarted)
		{
			lsClient->resetSampler();
		}
	}
	btnStartLSClient->setEnabled(!lsClientStarted);
	btnResetLSNow->setEnabled(lsClientStarted);
}

void GlobalSettingsConfig::resetLSNow()
{
	if(QMessageBox::critical(this,
			tr("Reset LinuxSampler?"),
			tr("This action will cause LinuxSampler to reset."
				"Deleting all MIDI Mappings, Devices and any Audio channels created in JACK"
			),
			QMessageBox::Ok|QMessageBox::Cancel,
			QMessageBox::Cancel) == QMessageBox::Ok)
	{
		if(!lsClient)
		{
			lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
			lsClientStarted = lsClient->startClient();
			if(config.lsClientResetOnStart && lsClientStarted)
			{
				lsClient->resetSampler();
			}
			//Update the start button
			btnStartLSClient->setEnabled(!lsClientStarted);
		}
		if(lsClientStarted)
		{
			lsClient->resetSampler();
			btnResetLSNow->setEnabled(lsClientStarted);
		}
	}
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void GlobalSettingsConfig::apply()
{
	int rtcticks = rtcResolutionSelect->currentIndex();
	config.guiRefresh = guiRefreshSelect->value();
	config.minSlider = minSliderSelect->value();
	config.minMeter = minMeterSelect->value();
	config.freewheelMode = freewheelCheckBox->isChecked();
	config.useDenormalBias = denormalCheckBox->isChecked();
	config.useOutputLimiter = outputLimiterCheckBox->isChecked();
	config.vstInPlace = vstInPlaceCheckBox->isChecked();
	config.rtcTicks = rtcResolutions[rtcticks];
	config.userInstrumentsDir = userInstrumentsPath->text();
	config.startSong = startSongEntry->text();
	config.startMode = startSongGroup->checkedId();
	int das = dummyAudioSize->currentIndex();
	config.dummyAudioBufSize = dummyAudioBufSizes[das];
	config.dummyAudioSampleRate = dummyAudioRate->value();

	int div = midiDivisionSelect->currentIndex();
	config.division = divisions[div];
	div = guiDivisionSelect->currentIndex();
	config.guiDivision = divisions[div];

	config.showSplashScreen = showSplash->isChecked();
	config.showDidYouKnow = showDidYouKnow->isChecked();
	config.externalWavEditor = externalWavEditorSelect->text();
	config.useOldStyleStopShortCut = oldStyleStopCheckBox->isChecked();
	config.moveArmedCheckBox = moveArmedCheckBox->isChecked();
	config.useProjectSaveDialog = projectSaveCheckBox->isChecked();
	config.useAutoCrossFades = m_chkAutofade->isChecked();
	config.lsClientAutoStart = chkStartLSClient->isChecked();

	oomUserInstruments = config.userInstrumentsDir;
	config.lsClientResetOnStart = chkResetLSOnStartup->isChecked();
	config.lsClientResetOnSongStart = chkResetLSOnSongLoad->isChecked();

	oom->setHeartBeat(); // set guiRefresh
	midiSeq->msgSetRtc(); // set midi tick rate
	oom->changeConfig(true); // save settings
}

//---------------------------------------------------------
//   ok
//---------------------------------------------------------

void GlobalSettingsConfig::ok()
{
	apply();
	close();
}

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void GlobalSettingsConfig::cancel()
{
	close();
}

void GlobalSettingsConfig::selectInstrumentsPath()
{
	QString dir = QFileDialog::getExistingDirectory(this,
			tr("Selects instruments directory"),
			config.userInstrumentsDir);
	userInstrumentsPath->setText(dir);
}

void GlobalSettingsConfig::defaultInstrumentsPath()
{
	QString dir = configPath + "/instruments";
	userInstrumentsPath->setText(dir);
}
