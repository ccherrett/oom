//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: gconfig.cpp,v 1.15.2.13 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "gconfig.h"

GlobalConfigValues config = {
	190, // globalAlphaBlend
	{
		QColor(0xff, 0xff, 0xff), // palette
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff)
	},
	{
		QColor(43, 49, 51), // part colors partColor0
		QColor(55, 61, 55), // partColor1
		QColor(61, 55, 55), // partColor2
		QColor(61, 55, 61), // partColor3
		QColor(61, 61, 55), // partColor4
		QColor(25, 53, 59), // partColor5
		QColor(34, 59, 25), // partColor6
		QColor(58, 59, 25), // partColor7
		QColor(25, 59, 51), // partColor8
		QColor(47, 25, 59), // partColor9
		QColor(59, 25, 25), // partColor10
		QColor(17, 74, 88), // partColor11
		QColor(88, 17, 17), // partColor12
		QColor(17, 88, 19), // partColor13
		QColor(88, 17, 87), // partColor14
		QColor(88, 84, 17), // partColor15
		QColor(2, 128, 121) // partColor16
	},
	{
		QString("Default"), // Default part color names
		QString("Color 2"),
		QString("Color 3"),
		QString("Color 4"),
		QString("Color 5"),
		QString("Color 6"),
		QString("Color 7"),
		QString("Color 8"),
		QString("Color 9"),
		QString("Color 10"),
		QString("Color 11"),
		QString("Color 12"),
		QString("Color 13"),
		QString("Color 14"),
		QString("Color 15"),
		QString("Color 16"),
		QString("Color 17")
	}
,
	QColor(16, 24, 25), // transportHandleColor;
	QColor(66, 202, 230), // bigTimeForegroundColor;
	QColor(16, 24, 25), // bigTimeBackgroundColor;
	QColor(23, 23, 23), // waveEditBackgroundColor;
	{
		QFont(QString("arial"), 10, QFont::Normal),
		QFont(QString("arial"), 8, QFont::Normal),
		QFont(QString("arial"), 10, QFont::Normal),
		QFont(QString("arial"), 10, QFont::Bold),
		QFont(QString("arial"), 8, QFont::Bold), // timescale numbers
		QFont(QString("Lucidatypewriter"), 14, QFont::Bold),
		QFont(QString("arial"), 8, QFont::Bold, true) // Mixer strip labels. Looks and fits better with bold + italic than bold alone,
		//  at the price of only few more pixels than Normal mode.
	}
,
	QColor(30, 30, 30), // trackBg;
	QColor(18, 25, 28), // selected track Bg;
	QColor(71, 202, 225), // selected track Fg;

	QColor(0, 160, 255), // midiTrackLabelBg;   // Med blue
	QColor(0, 160, 255), // drumTrackLabelBg;   // Med blue
	Qt::magenta, // waveTrackLabelBg;
	Qt::green, // outputTrackLabelBg;
	Qt::red, // inputTrackLabelBg;
	Qt::yellow, // groupTrackLabelBg;
	QColor(120, 255, 255), // auxTrackLabelBg;    // Light blue
	QColor(255, 130, 0), // synthTrackLabelBg;  // Med orange

	QColor(55, 45, 61), // midiTrackBg;
	QColor(77, 65, 83), // drumTrackBg;
	QColor(40, 62, 68), // waveTrackBg;
	QColor(133, 36, 36), // outputTrackBg;
	QColor(75, 112, 75), // inputTrackBg;
	QColor(58, 64, 65), // groupTrackBg;
	QColor(83, 87, 137), // auxTrackBg;
	QColor(125, 73, 32), // synthTrackBg;

	QColor(90, 90, 90), // part canvas bg
	QColor(255, 170, 0), // ctrlGraphFg;    Medium orange
	QColor(0, 0, 0), // mixerBg;

	384, // division;
	1024, // rtcTicks
	-60, // int minMeter;
	-60.0, // double minSlider;
	false, // use Jack freewheel
	20, // int guiRefresh;
	QString(""), // userInstrumentsDir
	//QString(""),                  // helpBrowser; // Obsolete
	true, // extendedMidi
	384, // division for smf export
	QString(""), // copyright string for smf export
	1, // smf export file format
	false, // midi export file 2 byte timesigs instead of 4
	true, // optimize midi export file note offs
	true, // Split imported tracks into multiple parts.
	1, // startMode
	QString(""), // start song path
	384, // gui division
	QRect(0, 0, 400, 300), // GeometryMain;
	QRect(0, 0, 200, 100), // GeometryTransport;
	QRect(0, 0, 600, 200), // GeometryBigTime;
	QRect(0, 0, 400, 300), // GeometryPianoroll;
	QRect(0, 0, 400, 300), // GeometryDrumedit;
	//QRect(0, 0, 300, 500),        // GeometryMixer;  // Obsolete
	{
		QString("Mixer A"),
		QRect(0, 0, 300, 500), // Mixer1
		true, true, true, true,
		true, true, true, true
	},
	{
		QString("Mixer B"),
		QRect(200, 200, 300, 500), // Mixer2
		true, true, true, true,
		true, true, true, true
	},
	true, // TransportVisible;
	false, // BigTimeVisible;
	false, // mixer1Visible;
	false, // mixer2Visible;

	false, // markerVisible;
	true, // showSplashScreen
	1, // canvasShowPartType 1 - names, 2 events
	5, // canvasShowPartEvent
	true, // canvasShowGrid;
	QString(""), // canvasBgPixmap;
	QStringList(), // canvasCustomBgList
	QString(":/style.qss"), // default styleSheetFile
	QString(""), // style
	QString("sweep"), // externalWavEditor
	false, // useOldStyleStopShortCut
	true, // moveArmedCheckBox
	true, // useDenormalBias
	false, // useOutputLimiter
	true, // showDidYouKnow
	false, // vstInPlace  Enable VST in-place processing
	44100, // Dummy audio preferred sample rate
	512, // Dummy audio buffer size
	QString("./"), // projectBaseFolder
	true,	 // projectStoreInFolder
	true	// useProjectSaveDialog
};

