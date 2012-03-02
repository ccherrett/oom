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
#include <QStandardItem>
#include <QStandardItemModel>

#include "genset.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "network/LSThread.h"

#include "mididev.h"
#include "audio.h"
#include "driver/jackaudio.h"
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
	
	m_inputsModel = new QStandardItemModel(this);
	inputView->setModel(m_inputsModel);
	populateInputs();
	btnBrowseLS->setIcon(QIcon(*browseIconSet3));
	btnRefreshInput->setIcon(QIcon(*refreshIconSet3));
	selectInstrumentsDirButton->setIcon(QIcon(*browseIconSet3));
	defaultInstrumentsDirButton->setIcon(QIcon(*refreshIconSet3));

	userInstrumentsPath->setText(config.userInstrumentsDir);
	//selectInstrumentsDirButton->setIcon(*openIcon);
	//defaultInstrumentsDirButton->setIcon(*undoIcon);
	connect(selectInstrumentsDirButton, SIGNAL(clicked()), SLOT(selectInstrumentsPath()));
	connect(defaultInstrumentsDirButton, SIGNAL(clicked()), SLOT(defaultInstrumentsPath()));

	guiRefreshSelect->setValue(config.guiRefresh);
	minSliderSelect->setValue(int(config.minSlider));
	minMeterSelect->setValue(config.minMeter);
	freewheelCheckBox->setChecked(config.freewheelMode);
	denormalCheckBox->setChecked(config.useDenormalBias);
	outputLimiterCheckBox->setChecked(config.useOutputLimiter);
	dummyAudioRate->setValue(config.dummyAudioSampleRate);

	dummyAudioRealRate->setText(QString().setNum(sampleRate));

	startSongEntry->setText(config.startSong);
	startSongGroup->button(config.startMode)->setChecked(true);

	oldStyleStopCheckBox->setChecked(config.useOldStyleStopShortCut);
	moveArmedCheckBox->setChecked(config.moveArmedCheckBox);
	projectSaveCheckBox->setChecked(config.useProjectSaveDialog);
	
	m_chkAutofade->setChecked(config.useAutoCrossFades);
	chkStartLSClient->setChecked(config.lsClientAutoStart);
	btnStartLSClient->setText(lsClientStarted ? tr("Stop Now") : tr("Start Now"));
	btnStartStopLS->setText(gSamplerStarted ? tr("Stop Now") : tr("Start Now"));
	btnResetLSNow->setEnabled(lsClientStarted);
	chkResetLSOnStartup->setChecked(config.lsClientResetOnStart);
	chkResetLSOnSongLoad->setChecked(config.lsClientResetOnSongStart);
	chkStartLS->setChecked(config.lsClientStartLS);
	txtLSPath->setText(config.lsClientLSPath);

	connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
	connect(okButton, SIGNAL(clicked()), SLOT(ok()));
	connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
	connect(btnStartLSClient, SIGNAL(clicked()), SLOT(startLSClientNow()));
	connect(btnResetLSNow, SIGNAL(clicked()), SLOT(resetLSNow()));
	connect(btnRefreshInput, SIGNAL(clicked()), this, SLOT(populateInputs()));
	connect(btnStartStopLS, SIGNAL(clicked()), SLOT(startStopSampler()));
	connect(btnBrowseLS, SIGNAL(clicked()), SLOT(selectedSamplerPath()));
}

void GlobalSettingsConfig::startStopSampler()
{
}

void GlobalSettingsConfig::selectedSamplerPath()
{
	QString ls = QFileDialog::getOpenFileName(this, tr("Locate LinuxSampler binary"));
	if(!ls.isNull())
	{
		txtLSPath->setText(ls);
	}
}

void GlobalSettingsConfig::populateInputs()/*{{{*/
{
	m_inputsModel->clear();
	QStringList alsaList;
	QStringList jackList;
	if(gInputList.size())
	{
		//Select the rows
		for(int i = 0; i < gInputList.size(); ++i)
		{
			QPair<int, QString> input = gInputList.at(i);
			if(input.first == MidiDevice::JACK_MIDI)
				jackList.append(input.second);
			else
				alsaList.append(input.second);
		}
	}
	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		if ((*i)->deviceType() == MidiDevice::ALSA_MIDI)
		{
			QStandardItem* item = new QStandardItem(QString((*i)->name()).append(" (ALSA)"));
			item->setData((*i)->name(), Qt::UserRole+1);
			item->setData(MidiDevice::ALSA_MIDI, Qt::UserRole+2);
			item->setEditable(false);
			item->setCheckable(true);
			if(alsaList.contains((*i)->name()))
				item->setCheckState(Qt::Checked);
			m_inputsModel->appendRow(item);
		}
	}
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->outputPorts(true, false);//No aliases
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		QStandardItem* item = new QStandardItem(QString(*ip).append(" (JACK)"));
		item->setData(*ip, Qt::UserRole+1);
		item->setData(MidiDevice::JACK_MIDI, Qt::UserRole+2);
		item->setEditable(false);
		item->setCheckable(true);
		if(jackList.contains(*ip))
			item->setCheckState(Qt::Checked);
		m_inputsModel->appendRow(item);
	}
}/*}}}*/

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
	dummyAudioRate->setValue(config.dummyAudioSampleRate);

	dummyAudioRealRate->setText(QString().setNum(sampleRate));

	startSongEntry->setText(config.startSong);
	startSongGroup->button(config.startMode)->setChecked(true);

	oldStyleStopCheckBox->setChecked(config.useOldStyleStopShortCut);
	moveArmedCheckBox->setChecked(config.moveArmedCheckBox);
	projectSaveCheckBox->setChecked(config.useProjectSaveDialog);
	chkStartLSClient->setChecked(config.lsClientAutoStart);
	btnStartLSClient->setText(lsClientStarted ? tr("Stop Now") : tr("Start Now"));
	btnStartStopLS->setText(gSamplerStarted ? tr("Stop Now") : tr("Start Now"));
	btnResetLSNow->setEnabled(lsClientStarted);
	chkResetLSOnStartup->setChecked(config.lsClientResetOnStart);
	chkResetLSOnSongLoad->setChecked(config.lsClientResetOnSongStart);
	chkStartLS->setChecked(config.lsClientStartLS);
	txtLSPath->setText(config.lsClientLSPath);
	populateInputs();
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

//---------------------------------------------------------
//   startLSClientNow Start and restart the LS Client
//---------------------------------------------------------

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

		if(!lsClientStarted)
		{
			lsClient = 0;
		}
	}
	else
	{
		lsClientStarted = false;
		lsClient->stopClient();
		lsClient = 0;
	}
	btnStartLSClient->setText(lsClientStarted ? tr("Stop Now") : tr("Start Now"));
	btnResetLSNow->setEnabled(lsClientStarted);
}

//---------------------------------------------------------
//   resetLSNow Resets LinuxSampler configuration
//---------------------------------------------------------

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
		if(lsClient && lsClientStarted)
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

	config.useOldStyleStopShortCut = oldStyleStopCheckBox->isChecked();
	config.moveArmedCheckBox = moveArmedCheckBox->isChecked();
	config.useProjectSaveDialog = projectSaveCheckBox->isChecked();
	config.useAutoCrossFades = m_chkAutofade->isChecked();
	config.lsClientAutoStart = chkStartLSClient->isChecked();

	oomUserInstruments = config.userInstrumentsDir;
	config.lsClientResetOnStart = chkResetLSOnStartup->isChecked();
	config.lsClientResetOnSongStart = chkResetLSOnSongLoad->isChecked();
	config.lsClientStartLS = chkStartLS->isChecked();
	config.lsClientLSPath = txtLSPath->text();

	gInputList.clear();
	for(int i = 0; i < m_inputsModel->rowCount(); ++i)
	{
		QStandardItem* item = m_inputsModel->item(i);
		if(item && item->checkState() == Qt::Checked)
		{
			gInputList.append(qMakePair(item->data(Qt::UserRole+2).toInt(), item->data(Qt::UserRole+1).toString()));
		}
	}
//FIXME: Initialize the ports right now that were added and unconfigurethe ones that were removed

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

void GlobalSettingsConfig::setCurrentTab(int tab)
{
	TabWidget2->setCurrentIndex(tab);
}
