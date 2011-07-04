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
		QColor(175,235,255), // OOM Part Color 
		QColor(239,225,214), // Burgundy Berry
		QColor(219,189,135), // Pumpkin Spice
		QColor(233,226,248), // Lavender Linen
		QColor(211,225,210), // Mint
		QColor(245,233,234), // -----
		QColor(201,212,242), // Evening Blue
		QColor(228,221,202), // Sea and Sand
		QColor(231,206,213), // Cranberry Frosting
		QColor(220,207,232), // Grape Frosting
		QColor(207,207,232), // Blueberry Frosting
		QColor(205,230,225), // Matcha Frosting
		QColor(206,231,209), // Green Apple Frosting
		QColor(230,216,205), // Mochaccino Frosting
		QColor(145,255,115), // Key Lime Splash
		QColor(252,253,154), // Lemon Splash
		QColor(145,255,249), // Saltwater Splash
		QColor(255,198,250)  // Wendy Cherrett
	},
	{
		QColor(1,53,70),  // OOM Wave Color
		QColor(66,16,15), // Burgundy Berry
		QColor(130,65,27),// Pumpkin Spice
		QColor(78,69,96), // Lavender Linen
		QColor(36,46,35), // Mint
		QColor(90,99,79), // -----
		QColor(16,26,64), // Evening Blue
		QColor(67,81,88), // Sea and Sand
		QColor(87,48,59), // Cranberry Frosting 
		QColor(68,48,87), // Grape Frosting
		QColor(48,47,86), // Blueberry Frosting
		QColor(47,86,78), // Matcha Frosting
		QColor(47,86,53), // Green Apple Frosting
		QColor(87,65,48), // Mochaccino Frosting
		QColor(86,87,86), // Key Lime Splash
		QColor(86,87,86), // Lemon Splash
		QColor(86,87,86), // Saltwater Splash
		QColor(86,87,86)  // Wendy Cherrett
	},
	{
		QString("OOM"), // Default part color names
		QString("Burgundy Berry"),
		QString("Pumpkin Spice"),
		QString("Lavender Linen"),
		QString("Mint"),
		QString("-----"),
		QString("Evening Blue"),
		QString("Sea and Sand"),
		QString("Cranberry Frosting"),
		QString("Grape Frosting"),
		QString("Blueberry Frosting"),
		QString("Matcha Frosting"),
		QString("Green Apple Frosting"),
		QString("Mochaccino Frosting"),
		QString("Key Lime Splash"),
		QString("Lemon Splash"),
		QString("Saltwater Splash"),
		QString("Wendy Cherrett")
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
	false, // TransportVisible;
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
	false,	// useProjectSaveDialog
	8888, //LSCP client port
	QString("localhost"), //LSCP default hostname
	5, //LSCP client retries
	1,  //LSCP client timeout
	0, //Default audio raster index
	1 //Default midi raster index
};

