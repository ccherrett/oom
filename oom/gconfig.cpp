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
		QColor(175,235,255), // OOM Part Color //{{{
		QColor(255,198,250), // Wendy Cherrett
		QColor(231,206,213), // Cranberry Frosting
		QColor(220,207,232), // Grape Frosting
		QColor(207,207,232), // Blueberry Frosting
		QColor(205,230,225), // Matcha Frosting
		QColor(206,231,209), // Green Apple Frosting
		QColor(230,216,205), // Mochaccino Frosting
		QColor(145,255,115), // Key Lime Splash
		QColor(252,253,154), // Lemon Splash
		QColor(145,255,249), // Saltwater Splash
		QColor(228,221,202), // Day Off: Sea & Sand 
		QColor(255,220,252), // Day Off: Table Cloth
		QColor(200,211,240), // Day Off: His Golf Shirt 
		QColor(255,222,252), // Day Off: Her Golf Shirt
		QColor(215,255,191), // Day Off: Baby's Golf Shirt
		QColor(243,232,233), // Day Off: Book & Blanket
		QColor(230,221,196), // Day Off: Almond Roca
		QColor(223,244,116), // Day Off: Kiwi Cabana
		QColor(239,239,239), // Day Off: Cherry on the Side
		QColor(255,208,200), // Day Off: Redwood Grill
		QColor(195,193,169), // Day Off: Appetizer
		QColor(249,241,237), // Drinks: Raspberry Milkshake
		QColor(250,243,240), // Drinks: Marshmallow Hot Chocolate
		QColor(229,251,223), // Drinks: Irish Hot Chocolate
		QColor(242,232,229), // Drinks: Mochaccino
		QColor(242,232,229), // Drinks: Cream Soda Float
		QColor(219,189,135), // Drinks: Pumpkin Spice Latte
		QColor(251,226,246), // Drinks: Strawberry Apple Smoothie
		QColor(224,188,221), // Drinks: Very Berry Smoothie
		QColor(253,249,123), // Drinks: Lemonade
		QColor(229,255,210), // Drinks: Good Morning Glass
		QColor(211,225,210), // Drinks: Wintermint Steamer
		QColor(255,255,255), // Designer: Tux
		QColor(192,192,192), // Designer: Black Tie
		QColor(188,187,186), // Designer: Dad's Suit
		QColor(208,202,193), // Designer: Khaki
		QColor(201,201,201), // Designer: Midnight Stars
		QColor(255,255,255), // Designer: Friday Valet
		QColor(228,231,205), // Designer: Veranda Olivia
		QColor(255,255,255), // Designer: Gentlemen
		QColor(253,197,249)  // Designer: Wendy's Dress
		//}}}
	},
	{
		QColor(1,53,70),   // OOM Wave Color //{{{
		QColor(86,87,86),  // Wendy Cherrett 
		QColor(87,48,59),  // Cranberry Frosting 
		QColor(68,48,87),  // Grape Frosting
		QColor(48,47,86),  // Blueberry Frosting
		QColor(47,86,78),  // Matcha Frosting
		QColor(47,86,53),  // Green Apple Frosting
		QColor(87,65,48),  // Mochaccino Frosting
		QColor(86,87,86),  // Key Lime Splash
		QColor(86,87,86),  // Lemon Splash
		QColor(86,87,86),  // Saltwater Splash
		QColor(67,81,88),  // Day Off: Sea & Sand 
		QColor(110,82,108),// Day Off: Table Cloth
		QColor(16,28,64),  // Day Off: His Golf Shirt 
		QColor(16,28,64),  // Day Off: Her Golf Shirt
		QColor(16,28,64),  // Day Off: Baby's Golf Shirt
		QColor(90,99,79),  // Day Off: Book & Blanket
		QColor(52,41,39),  // Day Off: Almond Roca
		QColor(32,36,16),  // Day Off: Kiwi Cabana
		QColor(102,30,36), // Day Off: Cherry on the Side
		QColor(101,29,35), // Day Off: Redwood Grill
		QColor(71,70,13),  // Day Off: Appetizer
		QColor(154,43,82), // Drinks: Raspberry Milkshake
		QColor(65,32,17),  // Drinks: Marshmallow Hot Chocolate
		QColor(65,32,17),  // Drinks: Irish Hot Chocolate
		QColor(109,81,68), // Drinks: Mochaccino
		QColor(221,28,107),// Drinks: Cream Soda Float
		QColor(130,65,27), // Drinks: Pumpkin Spice Latte
		QColor(48,124,30), // Drinks: Strawberry Apple Smoothie
		QColor(81,83,112), // Drinks: Very Berry Smoothie
		QColor(51,51,51),  // Drinks: Lemonade
		QColor(133,36,223),// Drinks: Good Morning Glass
		QColor(36,46,35),  // Drinks: Wintermint Steamer
		QColor(24,24,24),  // Designer: Tux
		QColor(0,0,0),     // Designer: Black Tie
		QColor(40,2,1),    // Designer: Dad's Suit
		QColor(0,0,0),     // Designer: Khaki
		QColor(0,6,55),    // Designer: Midnight Stars
		QColor(96,1,0),    // Designer: Friday Valet
		QColor(38,36,0),   // Designer: Veranda Olivia
		QColor(0,0,0),     // Designer: Gentlemen
		QColor(24,24,24)   // Designer: Wendy's Dress
		//}}}
	},
	{
		QColor(84,102,108),  // OOM Part Color Automation//{{{
		QColor(109,91,108),  // Wendy Cherrett
		QColor(100,94,95),   // Cranberry Frosting
		QColor(97,94,100),   // Grape Frosting
		QColor(94,94,100),   // Blueberry Frosting
		QColor(94,100,99),   // Matcha Frosting
		QColor(94,100,95),   // Green Apple Frosting
		QColor(100,96,94),   // Mochaccino Frosting
		QColor(78,101,71),   // Key Lime Splash
		QColor(104,104,80),  // Lemon Splash
		QColor(78,104,103),  // Saltwater Splash
		QColor(100,99,94),   // Day Off: Sea & Sand 
		QColor(110,98,109),  // Day Off: Table Cloth
		QColor(94,96,104),   // Day Off: His Golf Shirt 
		QColor(110,100,109), // Day Off: Her Golf Shirt
		QColor(97,108,90),   // Day Off: Baby's Golf Shirt
		QColor(107,103,104), // Day Off: Book & Blanket
		QColor(99,97,91),    // Day Off: Almond Roca
		QColor(93,98,72),    // Day Off: Kiwi Cabana
		QColor(105,105,105), // Day Off: Cherry on the Side
		QColor(109,95,93),   // Day Off: Redwood Grill
		QColor(88,87,82),    // Day Off: Appetizer
		QColor(108,106,104), // Drinks: Raspberry Milkshake
		QColor(106,103,102), // Drinks: Marshmallow Hot Chocolate
		QColor(101,109,99),  // Drinks: Irish Hot Chocolate
		QColor(106,103,102), // Drinks: Mochaccino
		QColor(106,103,102), // Drinks: Cream Soda Float
		QColor(91,85,75),    // Drinks: Pumpkin Spice Latte
		QColor(110,100,108), // Drinks: Strawberry Apple Smoothie
		QColor(98,90,97),    // Drinks: Very Berry Smoothie
		QColor(101,100,73),  // Drinks: Lemonade
		QColor(102,110,96),  // Drinks: Good Morning Glass
		QColor(95,99,95),    // Drinks: Wintermint Steamer
		QColor(110,110,110), // Designer: Tux
		QColor(88,88,88),    // Designer: Black Tie
		QColor(86,86,86),    // Designer: Dad's Suit
		QColor(92,91,90),    // Designer: Khaki
		QColor(91,91,91),    // Designer: Midnight Stars
		QColor(90,90,90),    // Designer: Friday Valet
		QColor(101,101,85),  // Designer: Veranda Olivia
		QColor(111,111,111), // Designer: Gentlemen
		QColor(109,91,107)   // Designer: Wendy's Dress
		//}}}
	},
	{
		QColor(10,25,30), // OOM Wave Color Automation//{{{
		QColor(51,51,51), // Wendy Cherrett
		QColor(44,34,37), // Cranberry Frosting 
		QColor(39,34,44), // Grape Frosting
		QColor(33,33,43), // Blueberry Frosting
		QColor(33,43,41), // Matcha Frosting
		QColor(33,43,35), // Green Apple Frosting
		QColor(44,38,34), // Mochaccino Frosting
		QColor(50,50,50), // Key Lime Splash
		QColor(50,50,50), // Lemon Splash
		QColor(50,50,50), // Saltwater Splash
		QColor(43,47,49), // Day Off: Sea & Sand 
		QColor(60,52,59), // Day Off: Table Cloth
		QColor(17,20,29), // Day Off: His Golf Shirt 
		QColor(17,20,29), // Day Off: Her Golf Shirt
		QColor(17,20,29), // Day Off: Baby's Golf Shirt
		QColor(52,55,49), // Day Off: Book & Blanket
		QColor(29,26,25), // Day Off: Almond Roca
		QColor(17,18,12), // Day Off: Kiwi Cabana
		QColor(48,30,31), // Day Off: Cherry on the Side
		QColor(47,29,30), // Day Off: Redwood Grill
		QColor(31,31,17), // Day Off: Appetizer
		QColor(73,43,54), // Drinks: Raspberry Milkshake
		QColor(30,22,18), // Drinks: Marshmallow Hot Chocolate
		QColor(30,22,18), // Drinks: Irish Hot Chocolate
		QColor(57,50,47), // Drinks: Mochaccino
		QColor(100,48,69),// Drinks: Cream Soda Float
		QColor(60,43,32), // Drinks: Pumpkin Spice Latte
		QColor(37,57,33), // Drinks: Strawberry Apple Smoothie
		QColor(53,54,61), // Drinks: Very Berry Smoothie
		QColor(30,30,30), // Drinks: Lemonade
		QColor(77,51,101),// Drinks: Good Morning Glass
		QColor(23,25,23), // Drinks: Wintermint Steamer
		QColor(13,13,13), // Designer: Tux
		QColor(0,0,0),    // Designer: Black Tie
		QColor(16,6,6),   // Designer: Dad's Suit
		QColor(0,0,0),    // Designer: Khaki
		QColor(8,9,22),   // Designer: Midnight Stars
		QColor(40,14,14), // Designer: Friday Valet
		QColor(15,14,5),  // Designer: Veranda Olivia
		QColor(0,0,0),    // Designer: Gentlemen
		QColor(13,13,13)  // Designer: Wendy's Dress
		//}}}
	},
	{
		QString("OOM"), // Default part color names
		QString("Wendy Cherrett"),
		QString("Cranberry Frosting"),
		QString("Grape Frosting"),
		QString("Blueberry Frosting"),
		QString("Matcha Frosting"),
		QString("Green Apple Frosting"),
		QString("Cappuccino Frosting"),
		QString("Key Lime Splash"),
		QString("Lemon Splash"),
		QString("Saltwater Splash"),
		QString("Day Off: Sea and Sand"),
		QString("Day Off: Tablecloth"),
		QString("Day Off: His Golf Shirt"),
		QString("Day Off: Her Golf Shirt"),
		QString("Day Off: Baby's Golf Shirt"),
		QString("Day Off: Book and Blanket"),
		QString("Day Off: Almond Roca"),
		QString("Day Off: Kiwi Cabana"),
		QString("Day Off: Cherry on the Side"),
		QString("Day Off: Redwood Grill"),
		QString("Day Off: Appetizer"),
		QString("Drinks: Raspberry Milkshake"),
		QString("Drinks: Marshmallow Hot Chocolate"),
		QString("Drinks: Irish Hot Chocolate"),
		QString("Drinks: Mochaccino"),
		QString("Drinks: Cream Soda Float"),
		QString("Drinks: Pumpkin Spice Latte"),
		QString("Drinks: Strawberry Apple Smoothie"),
		QString("Drinks: Very Berry Smoothie"),
		QString("Drinks: Lemonade"),
		QString("Drinks: Good Morning Glass"),
		QString("Drinks: Wintermint Steamer"),
		QString("Designer: Tux"),
		QString("Designer: Black Tie"),
		QString("Designer: Dad's Suit"),
		QString("Designer: Khaki"),
		QString("Designer: Midnight Stars"),
		QString("Designer: Friday Valet"),
		QString("Designer: Veranda Olivia"),
		QString("Designer: Gentlemen"),
		QString("Designer: Wendy's Dress")
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

